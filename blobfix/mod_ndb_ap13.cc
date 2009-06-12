/* Copyright (C) 2006 MySQL AB

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
*/


#include "mod_ndb.h"
#include <signal.h>

     /* Single-threaded (Apache 1.3) version: */
ndb_instance *instance1;
struct mod_ndb_process process;
int ndb_force_send = 1;
int will_restart = 0;

//
// INITIALIZATION & CLEAN-UP FUNCTIONS:
// child_init, child_exit, and init_instance
//
void mod_ndb_child_init(server_rec *s, pool *p) {

  /* Initialize the NDB API */
  ndb_init();
  
  /* Build arrays of escape sequences, for encoding output */
  initialize_escapes(p);

  /* Get server configuration */
  config::srv *srv = (config::srv *)
    ap_get_module_config(s->module_config, &ndb_module);
  
  /* Connect to the cluster */
  connect_to_cluster(& process.conn, s, srv, p, false);

  /* Initialize the process structure */
  process.n_connections = 1;
  process.n_threads = 1;
  process.conn.instances = & instance1;

  /* Create an ndb instance */
  instance1 = (ndb_instance *) ap_pcalloc(p, sizeof(ndb_instance));
  init_instance(& process.conn, instance1, s, srv, p);
}


//
// child_exit
//
void mod_ndb_child_exit(server_rec *s, pool *p) {
  ndb_connection *c;
  ndb_instance *i;
  int n = 0, id = 0;
  
  for(c = & process.conn; c != (ndb_connection *) 0 ; c=c->next) {
    if(c->connection != 0) {
      id = c->connection->node_id();
      
      /* These were allocated by the C++ runtime, so let C++ free them,
         e.g. during "apachectl graceful"
      */    
      for(i = c->instances[n] ; n < process.n_threads ; n++)
          delete i->db;
      delete c->connection;

      if(c->connected)
        ap_log_error(APLOG_MARK, log::err, s, 
                     "Node %d disconnected from cluster.", id);
    }
  }

  /* Shut down the NDB API */
  ndb_end(0);  
}


/* connect_to_cluster is always called from child_init(),
   but it might also be called from my_instance(), if different vhosts
   connect to different clusters, or if initialization failed the first time.
*/
void connect_to_cluster(ndb_connection *c, server_rec *s, 
                        config::srv *srv, ap_pool *p, bool in_parent) {
  
  /* The C++ runtime allocates an Ndb_cluster_connection here */
  c->connection = new Ndb_cluster_connection((srv->connect_string));

  /* Set name that appears in the cluster log file */
  c->connection->set_name(ap_psprintf(p, "Apache mod_ndb %s/%d",
                                      s->server_hostname, (int) getpid()));
    
  // To do: arguments to connect() ???
  if (c->connection->connect()) {
    /* If the cluster is down, there could be a flood of these,
    so write it as a warning to maybe prevent filling up a log file. */
    log_err(s,"Cannot connect to NDB Cluster. (MySQL/NDB %s)",
             MYSQL_SERVER_VERSION);
    return;
  }

  /* In the Apache parent process, don't wait for the cluster to become ready. 
   Just report a succesful connection test */     
  if(in_parent) {
    c->connected = 1;
    return;
  }
  
  if((c->connection->wait_until_ready(20, 0)) < 0) {
    log_err(s,"Timeout waiting for cluster to become ready.");
    return;
  }
  
  /* Succesfully connected */
  c->connected=1;
  ap_log_error(APLOG_MARK, log::err, s, 
               "PID %d: mod_ndb (%s) connected to NDB Cluster as node %d", 
               (int) getpid(), REVISION, c->connection->node_id());
  log_debug(s,"*--  %s --*","DEBUGGING ENABLED");

  /* In multi-threaded apache 2 this might be configurable */
  c->ndb_force_send = ndb_force_send;

  return;
}


Ndb *init_instance(ndb_connection *c, ndb_instance *i, server_rec *s,
                   config::srv *srv_config, ap_pool *p) {
  
  /* The C++ runtime allocates an Ndb object here */
  i->db = new Ndb(c->connection);
  
  if(i->db) {
    /* init(n) where n is max no. of active transactions; default is 4 */
    i->db->init();
  }
  
  /* i->conn is a pointer back to the parent connection */
  i->conn = c;
  
  /* i->n_read_ops is a counter of read operations in the current transaction */
  i->n_read_ops = 0;
  
  /* Pointer to the configuration
  */
  i->server_config = srv_config;
  
  /* i->data is an array of data_operations */
  i->data = (struct data_operation *) 
    ap_pcalloc(p, srv_config->max_read_operations * sizeof(struct data_operation));
  
  return i->db;
}


ndb_instance *my_instance(request_rec *r) {
  ndb_connection *c = & process.conn;
  
  config::srv *srv = (config::srv *)
  ap_get_module_config(r->server->module_config, &ndb_module);
  
  
  /* This is the common case: */
  if(process.n_connections == 1) {
    if(c->connected == 0) {
      connect_to_cluster(c, r->server, srv, r->pool, false);
      if(! c->connected) return (ndb_instance *) 0;
    }
    return c->instances[0];
  }
  
  /* To do: implement the case with multiple connect strings */
  ap_log_error(APLOG_MARK, log::warn, r->server,
               "Unwritten code in %s at line %d!",  __FILE__, __LINE__);
  
  return (ndb_instance *) 0;
}


void module_must_restart() {
  if(! will_restart++)  /* this is a semaphore */
    kill(getppid(), SIGUSR1);  /* Tells parent apache to restart gracefully */
}


extern "C" {

  void mod_ndb_initializer(server_rec* s, ap_pool * p) {
    ap_add_version_component("NDB/" MYSQL_SERVER_VERSION) ;
  }  
  
  extern command_rec configuration_commands[];

  /* HANDLER LIST */
  
  /* The handlers are in handlers.cc: */
  extern int ndb_handler(request_rec *);
  extern int ndb_status_handler(request_rec *);
  extern int ndb_exec_batch_handler(request_rec *);
  extern int ndb_dump_format_handler(request_rec *);

  static const handler_rec mod_ndb_handlers[] = { 
      { "ndb-cluster", ndb_handler },
      { "ndb-status", ndb_status_handler },
      { "ndb-exec-batch", ndb_exec_batch_handler },
      { "ndb-dump-format", ndb_dump_format_handler },
      { NULL, NULL }
  };

  /************************
   * Global Dispatch List *
   ************************/

  module MODULE_VAR_EXPORT ndb_module = {
    STANDARD_MODULE_STUFF, 
    mod_ndb_initializer,        /* module initializer                  */
    config::init_dir,           /* create per-dir    config structures */
    config::merge_dir,          /* merge  per-dir    config structures */
    config::init_srv,           /* create per-server config structures */
    config::merge_srv,          /* merge  per-server config structures */
    configuration_commands,     /* table of config file commands       */
    mod_ndb_handlers,           /* [#8] MIME-typed-dispatched handlers */
    NULL,                       /* [#1] URI to filename translation    */
    NULL,                       /* [#4] validate user id from request  */
    NULL,                       /* [#5] check if the user is ok _here_ */
    NULL,                       /* [#3] check access by host address   */
    NULL,                       /* [#6] determine MIME type            */
    NULL,                       /* [#7] pre-run fixups                 */
    NULL,                       /* [#9] log a transaction              */
    NULL,                       /* [#2] header parser                  */
    mod_ndb_child_init,         /* child_init                          */
    mod_ndb_child_exit,         /* child_exit                          */
    NULL                        /* [#0] post read-request              */
  #ifdef EAPI
    ,NULL,                      /* EAPI: add_module                    */
    NULL,                       /* EAPI: remove_module                 */
    NULL,                       /* EAPI: rewrite_command               */
    NULL,                       /* EAPI: new_connection                */
    NULL                        /* EAPI: close_connection              */
  #endif
  };
};
