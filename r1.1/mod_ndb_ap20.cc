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
#include "apr_hash.h"

/* Multi-threaded Apache 2 version: */
struct mod_ndb_process process;
int apache_is_threaded = 0;
int ndb_force_send = 0;
apr_status_t mod_ndb_child_exit(void *);

int will_restart = 0;
apr_thread_mutex_t *restart_lock;


//
// INITIALIZATION & CLEAN-UP FUNCTIONS
//


/* The post_config hook runs only in the Apache parent (monitor) process.  
   Attempt to connect to the cluster; 
   if the attempt fails, Apache should die.  
*/
int mod_ndb_post_config(apr_pool_t* conf_pool, apr_pool_t* log_pool, 
                        apr_pool_t* temp_pool, server_rec* s) {

  ap_add_version_component(conf_pool, "NDB/" MYSQL_SERVER_VERSION) ;
    
  config::srv *srv = (config::srv *) 
                     ap_get_module_config(s->module_config, &ndb_module);

  ndb_init();    
  connect_to_cluster(& process.conn, s, srv, temp_pool, true);

  if( process.conn.connected) {    
    log_err(s, "Connnection test OK: succesfully connected to NDB Cluster."); 
    delete process.conn.connection;
    return OK ;
  }

  /* else: */
  log_err(s, "Connection test failed.  Cannot connect to NDB Cluster.  "
          "Apache will exit.");
  return 500;
}  


void mod_ndb_child_init(ap_pool *p, server_rec *s) {
  int n_ok = 0;
  int n_fail = 0;

  /* Initialize the NDB API */
  ndb_init();
  
  /* Build arrays of escape sequences, for encoding output */
  initialize_escapes(p);
  
  /* Get server configuration */
  config::srv *srv = (config::srv *)
    ap_get_module_config(s->module_config, &ndb_module);
  
  /* Multi-threaded? */
  ap_mpm_query(AP_MPMQ_IS_THREADED, &apache_is_threaded);
  
  if(apache_is_threaded) {
    ap_mpm_query(AP_MPMQ_MAX_THREADS, & process.n_threads);
    ap_mpm_query(AP_MPMQ_HARD_LIMIT_THREADS, & process.thread_limit);
  }
  else {
    process.n_threads = 1;
    process.thread_limit = 1;
  }
  
  /* Connections (distinct cluster connect strings) */
  process.n_connections = 1;
  connect_to_cluster(& process.conn, s, srv, p, false);
    
  /* Allocate the global instance table */
  process.conn.instances = (ndb_instance **) ap_pcalloc(p, 
              (process.n_threads * sizeof(ndb_instance *)));
  
  for(int i = 0; i < process.n_threads ; i++) {
    /* Create an instance */
    process.conn.instances[i] = 
      (ndb_instance *) ap_pcalloc(p, sizeof(ndb_instance));
    Ndb *ndbp = init_instance(& process.conn,process.conn.instances[i],s,srv,p);
    if(ndbp) n_ok++;
    else n_fail++;
  }

  if(process.conn.connected) { 
    log_err(s, "Node %d initialized %d "
               "NDB thread instance%s (%d success%s, %d failure%s).", 
               process.conn.connection->node_id(),
               process.n_threads, (process.n_threads == 1 ? "" :  "s"),
               n_ok, (n_ok == 1 ? "": "es") ,
               n_fail, (n_fail == 1 ? "" :  "s")); 
  }
  else
    log_err(s, "mod_ndb cannot connect to cluster.");
  
  
  /* Register the exit handler */
  apr_pool_cleanup_register(p, (const void *) s, 
                            mod_ndb_child_exit, mod_ndb_child_exit);
                            

  /* We have one mutex, used when forcing the module to restart. */
  apr_thread_mutex_create(&restart_lock, APR_THREAD_MUTEX_UNNESTED, p);

  return /* APR_SUCCESS */;
}


//
// child_exit
//
apr_status_t mod_ndb_child_exit(void *v) {
  server_rec *s = (server_rec *) v;
  ndb_connection *c = & process.conn;
  int id = 0;
  int n_destroyed = 0;
  
  if(c->connection != 0) {
    id = c->connection->node_id();
      
    /* These were allocated by the C++ runtime, so let C++ free them,
        e.g. during "apachectl graceful"
    */    
    for(int n = 0; n < process.n_threads ; n++) {
      ndb_instance *i = process.conn.instances[n];
      if(i && i->db) {
        delete i->db;
        n_destroyed++;
      }
    }
    delete c->connection;
    log_err(s, "Node %d disconnected from cluster; destroyed %d Ndb instances.", 
             id, n_destroyed);
  }

  /* Shut down the NDB API */
  ndb_end(0);  
  
  return APR_SUCCESS;
}


/* connect_to_cluster is always called from child_init(),
   but it might also be called from my_instance(), if different vhosts
   connect to different clusters, or if initialization failed the first time.
*/
void connect_to_cluster(ndb_connection *c, server_rec *s, 
                        config::srv *srv, ap_pool *p, bool in_parent) {
  int conn_retries = 0;
  
  /* The C++ runtime allocates an Ndb_cluster_connection here */
  c->connection = new Ndb_cluster_connection(srv->connect_string);
  
  /* Set name that appears in the cluster log file */
  c->connection->set_name(ap_psprintf(p, "Apache mod_ndb %s/%d",
                          s->server_hostname, getpid()));

  while(1) {
    conn_retries++;
    int r = c->connection->connect(2,1,0);
    if(r == 0)          // success 
      break;
    else if(r == -1)   // unrecoverable error
      return;
    else if (r == 1) { // recoverable error
      log_err(s,"Cannot connect to NDB Cluster (connectstring: \"%s\") %d/5",
              srv->connect_string, conn_retries) ;
      if(conn_retries == 5)
        return;
      else 
        sleep(1);
    }
  }

  /* In the Apache parent process, don't wait for the cluster to become ready. 
     Just report a succesful connection test */     
  if(in_parent) {
    c->connected = 1;
    return;
  }

  int ready_nodes = c->connection->wait_until_ready(5, 5);
  if(ready_nodes < 0) {
    log_err(s, "Timeout waiting for cluster to become ready (%d).", 
            ready_nodes);
    return;
  }
  
  /* Succesfully connected */
  c->connected=1;
  log_err(s, "PID %d : mod_ndb (%s) connected to NDB Cluster as node %d "
          "(%d thread%s; hard limit: %d)", 
           getpid(), REVISION, c->connection->node_id(),
           process.n_threads, 
           process.n_threads == 1 ? "" : "s",
           process.thread_limit);
  log_debug(s,"*--  %s --*","DEBUGGING ENABLED");

  /* Some day this might be configurable */
  c->ndb_force_send = ndb_force_send;

  return;
}


Ndb *init_instance(ndb_connection *c, ndb_instance *i, server_rec *s,
                   config::srv *srv, ap_pool *p) {
  
  /* The C++ runtime allocates an Ndb object here */
  i->db = new Ndb(c->connection);

  if(i->db) {
    /* init(n) where n is max no. of active transactions; default is 4 */
    if(i->db->init(2) == -1) {  // Error 
      ap_log_error(APLOG_MARK, log::err, 0, s, "Ndb::init() failed: %d %s", 
      i->db->getNdbError().code, i->db->getNdbError().message);

      i->db = 0; 
    }
  }

  /* i->conn is a pointer back to the parent connection */
  i->conn = c;

  /* i->n_ops is a counter of operations in the current transaction */
  i->n_read_ops = 0;
 
  /* Pointer to the server configuration */
  i->server_config = srv;
    
  /* i->data is an array of operations */
  i->data = (struct data_operation *) 
    ap_pcalloc(p, srv->max_read_operations * sizeof(struct data_operation));
    
  return i->db;
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
      connect_to_cluster(c, r->server, srv, r->pool, false);
      if(! c->connected) return (ndb_instance *) 0;
    }
    return c->instances[thread_id];
  }
  
  /* To do: implement the case with multiple connect strings */
  my_ap_log_error(log::warn, r->server,
                  "Unwritten code in mod_ndb_ap20.cc at line %d!", __LINE__);
  
  return (ndb_instance *) 0;
}


void module_must_restart() {
  apr_thread_mutex_lock(restart_lock);  
  if(! will_restart++)  
    kill(getppid(), SIGUSR1);  /* Tells parent apache to restart gracefully */
  apr_thread_mutex_unlock(restart_lock);
}


extern "C" {

  extern command_rec configuration_commands[];
  extern int ndb_handler(request_rec *);
  extern int ndb_exec_batch_handler(request_rec *);
  extern int ndb_dump_format_handler(request_rec *);
  extern int ndb_status_handler(request_rec *);
  
 /************************
  *       Hooks          *
  ************************/
  
  void mod_ndb_register_hooks(ap_pool *p) {
    ap_hook_post_config(mod_ndb_post_config, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_child_init(mod_ndb_child_init, NULL, NULL, APR_HOOK_MIDDLE);

    ap_hook_handler(ndb_handler, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_handler(ndb_exec_batch_handler, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_handler(ndb_dump_format_handler, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_handler(ndb_status_handler, NULL, NULL, APR_HOOK_MIDDLE);
  }
    
  /************************
   * Global Dispatch List *
   ************************/

  module AP_MODULE_DECLARE_DATA ndb_module = {
    STANDARD20_MODULE_STUFF, 
    config::init_dir,           /* create per-dir    config structures */
    config::merge_dir,          /* merge  per-dir    config structures */
    config::init_srv,           /* create per-server config structures */
    config::merge_srv,          /* merge  per-server config structures */
    configuration_commands,     /* table of config file commands       */
    mod_ndb_register_hooks,     /* register_hooks                      */
  };

};

