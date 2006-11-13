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
#include "mod_ndb_compat.h"

/* NDB headers */
#include "NdbApi.hpp"

/* MySQL Headers */
#include "mysql_version.h"

#ifdef CONFIG_DEBUG
#define log_conf_debug(x,y,z) log_debug(x,y,z)
#else
#define log_conf_debug(x,y,z)
#endif

extern "C" module AP_MODULE_DECLARE_DATA ndb_module;

enum result_format_type { no_results = 0, json, raw, xml, ap_note };

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


class result_buffer {
  private:
    size_t alloc_sz;
  
  public:
    char *buff;
    size_t sz; 
    char *init(request_rec *r, size_t size);
    void out(const char *fmt, ...);
    ~result_buffer();
};


enum AccessPlan {  /* Ways of executing an NDB query */
  NoPlan = 0,             // (also a bitmap)
  UseIndex = 1,
  PrimaryKey = 2,         // Lookup 
  UniqueIndexAccess = 3,  // Lookup & UseIndex 
  Scan = 4,               // Scan  
  OrderedIndexScan = 5,   // Scan & UseIndex 
};


/* Other mod_ndb headers */
#include "MySQL_Field.h"
#include "mod_ndb_config.h"
#include "JSON.h"
#include "defaults.h"



/* The basic architecture of this module:
    A single mod_ndb_process structure for each httpd process
    plus one (or a few) ndb_connections (linked in a list)
    plus one ndb_instance per-connection, per-thread
*/

/* An "NDB Instance" is a private per-thread data structure
   that contains an Ndb object plus some statistics
 */
struct mod_ndb_instance {
  struct mod_ndb_connection *conn;  
  Ndb *db;
  NdbTransaction *tx;
  struct operation **ops;
  int n_ops;
  unsigned int requests;
  unsigned int errors;
  unsigned int declined;
};
typedef struct mod_ndb_instance ndb_instance;

/* An operation */

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
Ndb * init_instance(ndb_connection *, ndb_instance *);
int print_all_params(void *v, const char *key, const char *val);
table *http_param_table(request_rec *r, const char *c);
int Query(request_rec *, config::dir *, ndb_instance *);
int read_http_post(request_rec *r, table **tab);
