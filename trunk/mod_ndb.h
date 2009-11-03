/* mod_ndb, an Apache module to access NDB Cluster */

/* Copyright (C) 2006 - 2009 Sun Microsystems
 All rights reserved. Use is subject to license terms.
 
 
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


/* System headers */
#include <assert.h>
#include <strings.h>

/* Apache headers */
#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"
#include "http_main.h"
#include "util_script.h"
#include "ap_config.h"
#include "http_log.h"

/* Some mod_ndb headers */
#include "defaults.h"
#include "mod_ndb_compat.h"
#include "mod_ndb_debug.h"

/* NDB headers */
#include "NdbApi.hpp"

/* MySQL Headers */
#include "mysql_version.h"

/* Other mod_ndb headers are included later in this file */

#ifdef CONFIG_DEBUG
#define log_conf_debug(x,y, ...) log_debug(x,y, __VA_ARGS__)
#else
#define log_conf_debug(x,y, ...)
#endif

extern "C" module AP_MODULE_DECLARE_DATA ndb_module;

namespace log {
//debug, info, notice, warn, error, crit, alert, emerg.
  enum {
    err = APLOG_NOERRNO|APLOG_NOTICE ,
    warn = APLOG_NOERRNO|APLOG_WARNING,
    debug = APLOG_NOERRNO|APLOG_DEBUG
  };
};


namespace config {
  class key_col;
  class index;
};
 

template <class T>
class apache_array: public array_header {
  public:
    int size()    { return this->nelts; }
    T **handle()  { return (T**) &(this->elts); }
    T *items()    { return (T*) this->elts; }
    T &item(int n){ return ((T*) this->elts)[n]; }
    T *new_item() { return (T*) ap_push_array(this); }
    void * operator new(size_t, ap_pool *p, int n) {
      return ap_make_array(p, n, sizeof(T));
    };
};


enum AccessPlan {  /* Ways of executing an NDB query */
  NoPlan = 0,             // (also a bitmap)
  UseIndex = 1,
  PrimaryKey = 2,         // Lookup 
  UniqueIndexAccess = 3,  // Lookup & UseIndex 
  Scan = 4,               // Scan  
  OrderedIndexScan = 5    // Scan & UseIndex 
};


/* Other mod_ndb headers */
#include "result_buffer.h"
#include "output_format.h"
#include "MySQL_value.h"
#include "MySQL_result.h"
#include "mod_ndb_config.h"

/* The basic architecture of this module:
     A single mod_ndb_process structure for each httpd process
    plus one (or a few) ndb_connections (linked in a list)
    plus one ndb_instance per-connection, per-thread
*/


/* An operation */
struct data_operation {
  NdbOperation *op;
  NdbIndexScanOperation *scanop;
  unsigned int n_result_cols;
  MySQL::result **result_cols;
  char **aliases;
  output_format *fmt;
  struct {
    unsigned int select_star : 1;
  } flag;
};


/* An "NDB Instance" is a private per-thread data structure
   that manages an Ndb object, a transaction, an array of 
   operations, and some statistics.
 */
class ndb_instance {
  public:
  struct mod_ndb_connection *conn;  
  Ndb *db;
  NdbTransaction *tx;
  int n_read_ops;
  config::srv *server_config;
  struct data_operation *data;
  struct {
    unsigned int aborted     : 1 ;
    unsigned int use_etag    : 1 ;
    unsigned int jsonrequest : 1 ;
  } flag;
  unsigned int requests;
  unsigned int errors;
  void cleanup() {
    for(unsigned n = 0 ; n < data->n_result_cols ; n++) 
      delete data->result_cols[n];
    bzero(data, n_read_ops * sizeof(struct data_operation));
    n_read_ops    =    0;
    flag.aborted  =    0;
    flag.use_etag =    0;
    flag.jsonrequest = 0;
  }
};
// typedef struct mod_ndb_instance ndb_instance;


/* A cluster connection contains an Ndb_cluster_connection object
   and points to an array of NDB instances, one per thread.
   If several vhosts connect to different clusters, we could have 
   a linked list of connections.
*/
struct mod_ndb_connection {
    unsigned int connected;
    int ndb_force_send;
    Ndb_cluster_connection *connection;
    ndb_instance **instances;
    struct mod_ndb_connection *next;
};
typedef struct mod_ndb_connection ndb_connection;


/* One mod_ndb_process per apache process.  The process includes  
   the first mod_ndb_connection structure.
*/
struct mod_ndb_process {
    int n_connections;
    int n_threads;
    int thread_limit;
    struct mod_ndb_connection conn;  // not a pointer 
};    

ndb_instance *my_instance(request_rec *r);
void connect_to_cluster(ndb_connection *, server_rec *, config::srv *, 
                        ap_pool *, bool);
Ndb * init_instance(ndb_connection *, ndb_instance *, server_rec *, 
                    config::srv *, ap_pool *);
int print_all_params(void *v, const char *key, const char *val);
apr_table_t *http_param_table(request_rec *r, const char *c);
int ExecuteAll(request_rec *, ndb_instance *);
int read_request_body(request_rec *, apr_table_t **, const char *);
void initialize_output_formats(ap_pool *);
char *register_format(ap_pool *, output_format *);
output_format *get_format_by_name(const char *);
void register_built_in_formatters(ap_pool *);
int build_results(request_rec *, data_operation *, result_buffer &);
int ndb_handle_error(request_rec *, int, const NdbError *, const char *);
const char * allowed_methods(request_rec *, config::dir *);
void module_must_restart(void);

extern "C" int cmp_swap_int(int *, int, int);
extern "C" int cmp_swap_ptr(void *, void *, void *);
