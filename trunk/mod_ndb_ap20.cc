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


/* Multi-threaded Apache 2 version: */
struct mod_ndb_process process;
int apache_is_threaded = 0;
int ndb_force_send = 0;
apr_status_t mod_ndb_child_exit(void *);


//
// INITIALIZATION & CLEAN-UP FUNCTIONS:
//
apr_status_t mod_ndb_child_init(ap_pool *p, ap_pool *plog, 
                                ap_pool *ptmp, server_rec *s) {
  /* Initialize the NDB API */
  ndb_init();

  /* Get server configuration */
  config::srv *srv = (config::srv *)
    ap_get_module_config(s->module_config, &ndb_module);

  /* Connections (distinct cluster connect strings) */
  process.n_connections = 1;
  connect_to_cluster(& process.conn, s, srv);

  /* Multi-threaded? */
  ap_mpm_query(AP_MPMQ_IS_THREADED, &apache_is_threaded);
  if(apache_is_threaded) {
    process.n_threads = 1;
    process.thread_limit = 1;
  }
  else {
    ap_mpm_query(AP_MPMQ_MAX_THREADS, & process.n_threads);
    ap_mpm_query(AP_MPMQ_HARD_LIMIT_THREADS, & process.thread_limit);
  }

  /* Allocate the global instance table */
  process.conn.instances = (ndb_instance **) ap_pcalloc(p, 
              (process.n_threads * sizeof(ndb_instance *)));
  
  for(int i = 0; i < process.n_threads ; i++) {
    /* Create an instance */
    process.conn.instances[i] = 
      (ndb_instance *) ap_pcalloc(p, sizeof(ndb_instance));
    init_instance(& process.conn , process.conn.instances[i]);
 } 
  
  /* Register the exit handler */
  apr_pool_cleanup_register(p, (const void *) s, 
                            mod_ndb_child_exit, mod_ndb_child_exit);
                            
  return APR_SUCCESS;
}


//
// child_exit
//
apr_status_t mod_ndb_child_exit(void *v) {
  server_rec *s = (server_rec *) v;
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

      log_err2(s, "Node %d disconnected from cluster.", id);
    }
  }

  /* Shut down the NDB API */
  ndb_end(0);  
  
  return APR_SUCCESS;
}


/* connect_to_cluster is always called from child_init(),
   but it might also be called from my_instance(), if different vhosts
   connect to different clusters, or if initialization failed the first time.
*/
void connect_to_cluster(ndb_connection *c, server_rec *s, config::srv *srv) {
  
  /* The C++ runtime allocates an Ndb_cluster_connection here */
  c->connection = new Ndb_cluster_connection((srv->connect_string));

  // To do: arguments to connnect() ???
  if (c->connection->connect()) {
    /* If the cluster is down, there could be a flood of these,
    so write it as a warning to maybe prevent filling up a log file. */
    log_err(s,"Cannot connect to NDB Cluster.");
    return;
  }

  if((c->connection->wait_until_ready(20, 0)) < 0) {
    log_err(s,"Timeout waiting for cluster to become ready.");
    return;
  }
  
  /* Succesfully connected */
  c->connected=1;
  ap_log_error(APLOG_MARK, log::err, 0, s,
               "Process %d connected to NDB Cluster as node %d", 
               getpid(), c->connection->node_id());

  /* Some day this might be configurable */
  c->ndb_force_send = ndb_force_send;

  return;
}


void init_instance(ndb_connection *c, ndb_instance *i) {
  
  /* The C++ runtime allocates an Ndb object here */
  i->db = new Ndb(c->connection);
  
  /* init(n) where n is max no. of active transactions; default is 4 */
  i->db->init();
  
  /* i->conn is a pointer back to the parent connection */
  i->conn = c;

  return;
}


ndb_instance *my_instance(request_rec *r) {
  ndb_connection *c = & process.conn;
  int thread_id = 0;
  
  config::srv *srv = (config::srv *)
  ap_get_module_config(r->server->module_config, &ndb_module);
  
  if(apache_is_threaded) 
    thread_id = r->connection->id % process.thread_limit;
  
  /* This is the common case: */
  if(process.n_connections == 1) {
    if(c->connected == 0) {
      connect_to_cluster(c, r->server, srv);
      if(! c->connected) return (ndb_instance *) 0;
    }
    return c->instances[thread_id];
  }
  
  /* To do: implement the case with multiple connect strings */
  my_ap_log_error(log::warn, r->server,
                  "Unwritten code in mod_ndb.cc at line %d!", __LINE__);
  
  return (ndb_instance *) 0;
}


//
// Configuration
//

namespace config {

  command_rec commands[] =
  {
      {   /* Per-server: ndb-connectstring */
        "ndb-connectstring",          
        (CMD_HAND_TYPE) ap_set_string_slot,
        (void *)XtOffsetOf(config::srv, connect_string),
        RSRC_CONF,      TAKE1, 
        "NDB Connection String" 
      },
      {
        "Database",         // inheritable
        (CMD_HAND_TYPE) ap_set_string_slot,
        (void *)XtOffsetOf(config::dir, database),
        ACCESS_CONF,    TAKE1, 
        "MySQL database schema" 
      },
      {
        "Table",            // inheritable
        (CMD_HAND_TYPE) ap_set_string_slot,
        (void *)XtOffsetOf(config::dir, table),
        ACCESS_CONF,    TAKE1, 
        "NDB Table"
      },            
      {
        "Deletes",          // NOT inheritable, defaults to 0
        (CMD_HAND_TYPE) ap_set_flag_slot,
        (void *)XtOffsetOf(config::dir, allow_delete),
        ACCESS_CONF,     FLAG,
        "Allow DELETE over HTTP"
      },
      {
        "ETags",          // Inheritable, defaults to 1
        (CMD_HAND_TYPE) ap_set_flag_slot,
        (void *)XtOffsetOf(config::dir, use_etags),
        ACCESS_CONF,     FLAG,
        "Compute and set ETag header in response"
      },    
      {
        "Format",           // inheritable
        (CMD_HAND_TYPE) config::result_format,
        NULL,
        ACCESS_CONF,    TAKE123, 
        "Result Set Format"
      },     
      {
        "Columns",          // inheritable
        (CMD_HAND_TYPE) config::non_key_column,
        (void *) "R",
        ACCESS_CONF,    ITERATE,
        "List of attributes to include in result set"
      },
      {
        "AllowUpdate",      // NOT inheritable
        (CMD_HAND_TYPE) config::non_key_column,
        (void *) "W",
        ACCESS_CONF,    ITERATE,
        "List of attributes that can be updated using HTTP"
      },
      {
        "PrimaryKey",      // NOT inheritable
        (CMD_HAND_TYPE) config::primary_key,
        (void *) "P",  
        ACCESS_CONF,    ITERATE,
        "Allow Primary Key lookups"
      },
      {
        "UniqueIndex",      // NOT inheritable
        (CMD_HAND_TYPE) config::named_index,
        (void *) "U",  
        ACCESS_CONF,    ITERATE2,
        "Allow unique key lookups, given index name and columns"
      },
      {
        "OrderedIndex",      // NOT inheritable
        (CMD_HAND_TYPE) config::named_index,
        (void *) "O",  
        ACCESS_CONF,    ITERATE2,
        "Allow ordered index scan, given index name and columns"
      },
      {                      // inheritable
        "PathInfo", // e.g. "PathInfo id" or "PathInfo keypt1/keypt2"
        (CMD_HAND_TYPE) config::pathinfo,  
        NULL,
        ACCESS_CONF,    TAKE12, 
        "Schema for interpreting the rightmost URL path components"
      },
      {                     // NOT inheritable
        "Filter",  // Filter col op keyword, e.g. "Filter id < min_id"
        (CMD_HAND_TYPE) config::filter, 
        NULL,
        ACCESS_CONF,    TAKE13, 
        "NDB Table"
      }, 
    
     {NULL, NULL, NULL, 0, cmd_how(0), NULL}
  };
}



extern int ndb_handler(request_rec *);

extern "C" {

 /************************
  *       Hooks          *
  ************************/

  void mod_ndb_register_hooks(ap_pool *p) {
    ap_hook_handler(ndb_handler, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_post_config(mod_ndb_child_init, NULL, NULL, APR_HOOK_MIDDLE);
  }
    
  /************************
   * Global Dispatch List *
   ************************/

  module AP_MODULE_DECLARE_DATA ndb_module = {
    STANDARD20_MODULE_STUFF, 
    config::init_dir,           /* create per-dir    config structures */
    config::merge_dir,          /* merge  per-dir    config structures */
    config::init_srv,           /* create per-server config structures */
    NULL,                       /* merge  per-server config structures */
    config::commands,           /* table of config file commands       */
    mod_ndb_register_hooks,     /* register_hooks                      */
  };

};

