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
#include "util_md5.h"
#include "query_source.h"

#ifdef THIS_IS_APACHE2
#define CheckHandler(r,h) if(strcmp(r->handler,h)) return DECLINED;
#else
#define CheckHandler(r,h) ;
#endif

// Globals:
extern struct mod_ndb_process process;      /* from mod_ndb.cc */
extern config::dir *all_endpoints[MAX_ENDPOINTS];
extern int n_endp;

extern int Query(request_rec *, config::dir *, ndb_instance *, query_source &);

// 
// Content handlers
//

extern "C" {
  int ndb_handler(request_rec *r) {
    config::dir *dir;
    ndb_instance *i;
    query_source *qsource;
    
    // Apache 2 Handler name check
    log_debug(r->server, "r->handler is \"%s\"", r->handler);
    CheckHandler(r,"ndb-cluster");

    // Fetch configuration  
    dir = (config::dir *) ap_get_module_config(r->per_dir_config, &ndb_module);
    if(! dir->database) {
      log_note(r->server,"No database defined at %s.", r->uri);
      return ndb_handle_error(r, 500, NULL, "Configuration error.");
    }
    if(! dir->table) {
      log_note(r->server,"No table is defined at %s.", r->uri);
      return ndb_handle_error(r, 500, NULL, "Configuration error.");  
    }
    
    // Get Ndb 
    i = my_instance(r);
    if(i == 0) {
      log_note(r->server,"Returning UNAVAILABLE because ndb_instance *i is null");
      return HTTP_SERVICE_UNAVAILABLE;
    }
    
    i->requests++;
    
    if(r->main) 
      qsource = new(r->pool) Apache_subrequest_query_source(r);
    else 
      qsource = new(r->pool) HTTP_query_source(r);
    
    return Query(r, dir, i, *qsource);
  }

  int ndb_exec_batch_handler(request_rec *r) {
    // Apache 2 Handler name check
    CheckHandler(r,"ndb-exec-batch");

    // Get Ndb 
    ndb_instance *i = my_instance(r);;
    if(i == 0) {
      log_note(r->server,"Cannot execute batch: ndb_instance *i is null");
      return HTTP_SERVICE_UNAVAILABLE;
    }
    i->requests++;
    
    return ExecuteAll(r,i);
  }
  
  
  int ndb_dump_format_handler(request_rec *r) {
    // Apache 2 Handler name check    
    CheckHandler(r, "ndb-dump-format");

    char *etag;
    result_buffer res;
    res.init(r, 8192);
    
    const char *name = r->args;
    output_format *fmt = get_format_by_name(name);
    if(!fmt)
      return ndb_handle_error(r, 404, 0, "Unknown format.\n");
 
    /* If the request path ends in "/source", dump source */
    if((r->path_info) && (! ap_fnmatch("*/source",r->path_info,0)))
        fmt->dump_source(r->pool, res);
    else /* dump parse tree */
      fmt->dump(r->pool, res);  
 
    etag = ap_md5_binary(r->pool, (const unsigned char *) res.buff, res.sz);
    ap_table_setn(r->headers_out, "ETag",  etag);
    ap_set_content_length(r, res.sz);
    r->content_type = "text/plain";
    ap_send_http_header(r);
    ap_rwrite(res.buff, res.sz, r);
    
    return OK;
  }


  int ndb_status_handler(request_rec *r) {
    
    // Apache 2 Handler name check
    CheckHandler(r,"ndb-status");
    
    config::srv *srv = 
    (config::srv *) ap_get_module_config(r->server->module_config, &ndb_module);
    
    r->content_type = "text/plain";
    ap_send_http_header(r);
    
    ap_rprintf(r, "Process ID: %d\n", (int) getpid());
    ap_rprintf(r, "Connect string: %s\n", srv->connect_string);
    ap_rprintf(r, "NDB Cluster Connections: %d\n", process.n_connections);
    ap_rprintf(r, "Apache Threads: %d\n", process.n_threads);
    ap_rprintf(r, "Force restart on stale dictionary: %s\n",  
               srv->force_restart ? "Yes" : "No");
    ap_rprintf(r, "Max retry time on temporary errors: %d ms\n", srv->max_retry_ms);

    ndb_instance *i = my_instance(r);
    if(i == (ndb_instance *) 0) {
      ap_rprintf(r,"\n -- RUNTIME ERROR: Cannot retrieve an NDB instance.\n");
      if(! process.conn.connected) ap_rprintf(r, "Not connected to NDB cluster.\n");
      return OK;
    }
    ap_rprintf(r,"Node Id: %d\n",i->conn->connection->node_id());
    ap_rprintf(r,"\n");
    ap_rprintf(r,"Requests in:   %u\n", i->requests);
    ap_rprintf(r,"Errors:        %u\n", i->errors);
    ap_rprintf(r,"\n");
    ap_rprintf(r,"Endpoints:     %d\n", n_endp);
    
    for(int i = 0 ; i < n_endp ; i ++) {
      config::dir *dir = all_endpoints[i];
      ap_rprintf(r,"  .. DB: %s , Table: %s , Path: %s\n",
                 dir->database, dir->table, dir->path);
    }
    
    return OK;
  }
} /* extern "C" */


/* This strategy, taken from mod_dav, is to write our own error document,
   set verbose-errors-to=*, and return DONE. 
*/
int ndb_handle_error(request_rec *r, int status, 
                     const NdbError *error, const char *msg) {
  result_buffer page;
  page.init(r, 4096);

  if(error) log_debug(r->server, "Error %d %s",error->code, error->message);
  
  ap_table_setn(r->notes, "verbose-error-to", "*");
  r->status = status;
  r->content_type = "text/plain";
  
  if(status == 405 && msg) ap_table_setn(r->headers_out, "Allow", msg);
  if(status == 503 && msg) ap_table_setn(r->headers_out, "Retry-After", msg);

  switch(status) {
    case 404:
      page.out(msg ? msg : "No data could be found.\n");
      break;
    case 405:
    case 406: 
      break;  // no message
    case 409:
      page.out("%s.\n", error->message);
      break;
    case 500:
      if(msg) page.out(msg);
      break;
    case 503:
      break;
    default:
      page.out("HTTP return code %d.\n", status);
  }

  ap_set_content_length(r, page.sz);
  ap_send_http_header(r);
  ap_rwrite(page.buff, page.sz, r);

  return DONE; 
}


apr_table_t *http_param_table(request_rec *r, const char *c) {
  apr_table_t *t = ap_make_table(r->pool, 4);
  char *key, *val;
  if(c == 0) return 0;
  
  while(*c && (val = ap_getword(r->pool, &c, '&'))) {
    key = ap_getword(r->pool, (const char **) &val, '=');
    ap_unescape_url(key);
    ap_unescape_url(val);
    ap_table_set(t,key,val);
  }
  return t;
}


int print_all_params(void *v, const char *key, const char *val) {
  request_rec *r = (request_rec *) v;
  ap_rprintf(r," [ \'%s\'=\'%s\' ]\n",key,val);
  return 1;
}


const char * allowed_methods(request_rec *r, config::dir *dir) {
  apache_array<char *> *methods = new(r->pool, 4) apache_array<char *>;

  if(dir->visible->size())    *methods->new_item() = "GET";
  if(dir->flag.use_etags)     *methods->new_item() = "HEAD";
  if(dir->updatable->size())  *methods->new_item() = "POST";
  if(dir->flag.allow_delete)  *methods->new_item() = "DELETE";
  
  return ap_array_pstrcat(r->pool, methods, ',');
}
