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


/* This header describes the Apache 1.3 "proof of concept" version of mod_ndb.
   It is not engineered for performance.  The preforking server model of Apache
   1.3 does not map well with NDB, because each Apache worker proccess needs 
   a node ID, and the total number of nodes in a cluster is very limited.
*/

/* Apache headers */
#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"
#include "http_main.h"
#include "util_script.h"
#include "ap_config.h"
#include "http_log.h"

/* NDB headers */
#include "NdbApi.hpp"

/* mod_ndb headers */
#include "MySQL_Field.h"

#define MOD_NDB_DEBUG 1

#define log_err(s,txt) ap_log_error(APLOG_MARK, log::err, s, txt);
#define log_err2(s,txt,arg) ap_log_error(APLOG_MARK, log::err, s, txt,arg);
#define log_note(s,txt) ap_log_error(APLOG_MARK, log::warn, s, txt);
#ifdef MOD_NDB_DEBUG 
#define log_debug(s,txt,arg) ap_log_error(APLOG_MARK, log::debug, s, txt, arg);
#else
#define log_debug(s,txt,arg) 
#endif

typedef const char *(*CMD_HAND_TYPE) ();

extern "C" module MODULE_VAR_EXPORT ndb_module;

enum result_format { json = 1, raw, xml, ap_note };

class JSON {
  public:
    static const char * new_array;
    static const char * end_array;
    static const char * new_object;
    static const char * end_object;
    static const char * delimiter ;
    static const char * is        ;
    static char *value(const NdbRecAttr &rec, request_rec *r);
    inline static char *member(const NdbRecAttr &rec, request_rec *r) {
      return ap_pstrcat(r->pool, 
                        rec.getColumn()->getName(), 
                        JSON::is,
                        JSON::value(rec,r),
                        NULL);    
    }
};

namespace log {
//debug, info, notice, warn, error, crit, alert, emerg.
  enum {
    err = APLOG_NOERRNO|APLOG_NOTICE ,
    warn = APLOG_NOERRNO|APLOG_WARNING,
    debug = APLOG_NOERRNO|APLOG_DEBUG
  };
}


namespace config {

  /* Apache per-directory configuration
  */
  struct dir {
    char *database;
    char *table;
    int allow_delete;
    result_format results;
    array_header *columns;
    array_header *updatable;
    void *indexes;
  };
  
  /* Apache per-server configuration 
  */
  struct srv {
    char *connect_string;
  };

  void * init_dir(pool *, char *);
  void * init_srv(pool *, server_rec *);
  void * merge_dir(pool *, void *, void *);
  const char * build_column_list(cmd_parms *, void *, char *);
  const char * build_index_list(cmd_parms *, void *, char *, char *);
  const char * set_result_format(cmd_parms *, void *, char *);
}


/* The basic architecture of this module:
    A single mod_ndb_process structure for each httpd process
    plus one (or a few) ndb_connections (linked in a list)
    plus one ndb_instance per-connection, per-thread
*/

/* An "NDB Instance" is a private per-thread data structure
   that contains an Ndb object plus some statistics
 */
struct mod_ndb_instance {
    Ndb *db;
    struct mod_ndb_connection *conn;  
    unsigned int requests;
    unsigned int errors;
    unsigned int declined;
    unsigned int row_not_found;
    unsigned int row_found;
};
typedef struct mod_ndb_instance ndb_instance;


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

/* Each process maintains a count of connections (which should usually be equal
   to 1).
*/

struct mod_ndb_process {
    unsigned short n_connections;
    unsigned short n_threads;
    struct mod_ndb_connection conn;  // not a pointer 
};    

ndb_instance *my_instance(request_rec *r);
void connect_to_cluster(ndb_connection *c, server_rec *s, config::srv *srv);
void init_instance(ndb_connection *c, ndb_instance *i);
int print_all_params(void *v, const char *key, const char *val);
table *http_param_table(request_rec *r, const char *c);
int Query(request_rec *, config::dir *, ndb_instance *);
int read_http_post(request_rec *r, table **tab);
