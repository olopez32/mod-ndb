/* mod_ndb, an Apache module to access NDB Cluster */

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


/* System headers */
#include <assert.h>

/* Apache headers */
#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"
#include "http_main.h"
#include "util_script.h"
#include "ap_config.h"
#include "http_log.h"

/* Apache 1.3 / Apache 2 compatibility */
#include "defaults.h"
#include "mod_ndb_compat.h"

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
  OrderedIndexScan = 5,   // Scan & UseIndex 
};

class len_string {
public:
  size_t len;
  const char *string;
  
  len_string() {};
  len_string(const char *str) : string (str) {
    len = strlen(str);
  };
  
  void * operator new(size_t sz, ap_pool *p) {
    return ap_pcalloc(p, sz);
  };
};

/* Other mod_ndb headers */
#include "output_format.h"
#include "result_buffer.h"
#include "MySQL_Field.h"
#include "mod_ndb_config.h"


/* The basic architecture of this module:
     A single mod_ndb_process structure for each httpd process
    plus one (or a few) ndb_connections (linked in a list)
    plus one ndb_instance per-connection, per-thread
*/


/* An "NDB Instance" is a private per-thread data structure
   that manages an Ndb object, a transaction, an array of 
   operations, and some statistics.
 */
struct mod_ndb_instance {
  struct mod_ndb_connection *conn;  
  Ndb *db;
  NdbTransaction *tx;
  int n_read_ops;
  int max_read_ops;
  struct data_operation *data;
  struct {
    unsigned int has_blob : 1 ;
    unsigned int aborted  : 1 ;
    unsigned int use_etag : 1 ;
  } flag;
  unsigned int requests;
  unsigned int errors;
};
typedef struct mod_ndb_instance ndb_instance;


/* An operation */
struct data_operation {
  NdbOperation *op;
  NdbIndexScanOperation *scanop;
  NdbBlob *blob;
  unsigned int n_result_cols;
  const NdbRecAttr **result_cols;
  output_format *fmt;
};


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
void connect_to_cluster(ndb_connection *, server_rec *, config::srv *, ap_pool *);
Ndb * init_instance(ndb_connection *, ndb_instance *, uint, ap_pool *);
int print_all_params(void *v, const char *key, const char *val);
table *http_param_table(request_rec *r, const char *c);
int Query(request_rec *, config::dir *, ndb_instance *);
int ExecuteAll(request_rec *, ndb_instance *);
int read_http_post(request_rec *r, table **tab);
void initialize_output_formats(ap_pool *);
char *register_format(char *, output_format *);
output_format *get_format_by_name(char *);
void register_built_in_formatters(ap_pool *);
int build_results(request_rec *, data_operation *, result_buffer &);
