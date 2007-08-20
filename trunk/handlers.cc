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

#ifdef THIS_IS_APACHE2
#define CheckHandler(r,h) if(strcmp(r->handler,h)) return DECLINED;
#else
#define CheckHandler(r,h) ;
#endif

// Globals:
extern struct mod_ndb_process process;      /* from mod_ndb.cc */

// 
// Content handlers
//

extern "C" {
  int ndb_handler(request_rec *r) {
    config::dir *dir;
    ndb_instance *i;
    
    // Apache 2 Handler name check
    CheckHandler(r,"ndb-cluster");

    // Fetch configuration  
    dir = (config::dir *) ap_get_module_config(r->per_dir_config, &ndb_module);
    if(! dir->database) {
      log_note(r->server,"Returning NOT_IMPLEMENTED because no db is defined at %s",
                r->uri);
      return NOT_IMPLEMENTED;
    }
    if(! dir->table) {
      log_note(r->server,"Returning NOT_IMPLEMENTED because no table is defined at %s",
                r->uri);
      return NOT_IMPLEMENTED;      
    }
    
    // Get Ndb 
    i = my_instance(r);
    if(i == 0) {
      log_note(r->server,"Returning UNAVAILABLE because ndb_instance *i is null");
      return HTTP_SERVICE_UNAVAILABLE;
    }
    
    i->requests++;
    
    return Query(r,dir,i);
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
    if(!fmt) {
      res.out("Unknown format \"%s\".\n",name);
    }
    else {
      fmt->dump(r->pool, res);
      etag = ap_md5_binary(r->pool, (const unsigned char *) res.buff, res.sz);
      ap_table_setn(r->headers_out, "ETag",  etag);
    }
    ap_set_content_length(r, res.sz);
    r->content_type = "text/plain";
    ap_send_http_header(r);
    ap_rwrite(res.buff, res.sz, r);
    
    return OK;
  }


  int ndb_config_check_handler(request_rec *r) {
    table *param_tab;
    config::key_col *columns;
    
    // Apache 2 Handler name check
    CheckHandler(r,"ndb-config-check");

    ndb_instance *i = my_instance(r);
    
    config::dir *dir = (config::dir *)
      ap_get_module_config(r->per_dir_config, &ndb_module);
    config::srv *srv = (config::srv *)
      ap_get_module_config(r->server->module_config, &ndb_module);
    
    r->content_type = "text/plain";
    ap_send_http_header(r);
    
    if(! (dir->database && dir->table)) {
      ap_rprintf(r,"No database or table configured at %s\n",r->uri);
      return OK;
    }
    
    ap_rprintf(r,"Process ID: %d\n",getpid());
    ap_rprintf(r,"Connect string: %s\n",srv->connect_string);
    ap_rprintf(r,"Database: %s\n",dir->database);
    ap_rprintf(r,"Table: %s\n",dir->table);
    ap_rprintf(r,"Size of configuration structures:");
    /* gcc may give a warning here where glibc printf uses "%z" for size_t */
    ap_rprintf(r,"   dir: %lu.  index: %lu.  key_col: %lu.\n", sizeof(config::dir),
               sizeof(config::index), sizeof(config::key_col));
    if(i == (ndb_instance *) 0) {
      ap_rprintf(r, "Cannot access NDB instance. ");
      ap_rprintf(r, process.conn.connected ? "\n" : "Not connected to cluster.\n");
    }
    else {
      i->db->setDatabaseName(dir->database);
      NdbDictionary::Dictionary *dict = i->db->getDictionary();
      const NdbDictionary::Table *tab = dict->getTable(dir->table);
      ap_rprintf(r,"Node Id: %d\n",i->conn->connection->node_id());
      ap_rprintf(r,"\n");
      if(tab) {
        ap_rprintf(r,"Primary key according to NDB Dictionary:");
        for(int n = 0; n < tab->getNoOfPrimaryKeys() ; n++) 
          ap_rprintf(r,"%s %s", (n ? "," : " ") , tab->getPrimaryKey(n));
        ap_rprintf(r,"\n");
      }
      else
        if(i) ap_rprintf(r," ** Table does not exist in data dictionary.\n");
    }
    if(dir->visible) {
      ap_rprintf(r,"%d visible column%s:  ", dir->visible->size(),
                 dir->visible->size() == 1 ? "" : "s");
      ap_rprintf(r,"%s \n",ap_array_pstrcat(r->pool,dir->visible,',')); 
    } 
    if(dir->updatable) {
      ap_rprintf(r,"%d updatable column%s:  ", dir->updatable->size(),
                 dir->visible->size() == 1 ? "" : "s");
      ap_rprintf(r,"%s \n",ap_array_pstrcat(r->pool,dir->updatable,','));  
    }
    ap_rprintf(r,"Result format: %s\n", dir->fmt->name);
    
    columns = dir->key_columns->items();
    int n_indexes = dir->indexes->size();
    int n_kcols = dir->key_columns->size();
    ap_rprintf(r,"\n%d key column%s:    ",n_kcols, n_kcols == 1 ? "" : "s");
    for(int n = 0 ; n < n_kcols ; n++)
      ap_rprintf(r,"%s ",columns[n].name);
    ap_rprintf(r,"\n%d index%s\n", n_indexes, n_indexes == 1 ? "" : "es");
    for(int n = 0 ; n < n_indexes ; n++) {
      config::index &idx = dir->indexes->item(n);
      ap_rprintf(r,"Type: %c | %-30s\n",idx.type, idx.name);
      int t = idx.first_col;
      ap_rprintf(r," %d key column alias%s:", idx.n_columns,
                 idx.n_columns == 1 ? "" : "es");
      while(t != -1) {
        ap_rprintf(r," %s", columns[t].name);
        t = columns[t].next_in_key;
      }
      ap_rprintf(r,"\n");
    }
    ap_rprintf(r,"Pathinfo: ");
    for(int n = 0 ; n < dir->pathinfo_size ; n++)
      ap_rprintf(r,"%s ", columns[dir->pathinfo[n]].name);
    
    ap_rprintf(r,"\n\n");
    ap_rprintf(r,"args: %s\n",r->args);
    if(r->args) {
      param_tab = http_param_table(r, r->args);
      ap_table_do(print_all_params,r,param_tab,NULL);
    }
    return OK;
  }


  int ndb_status_handler(request_rec *r) {
    
    // Apache 2 Handler name check
    CheckHandler(r,"ndb-status");
    
    config::srv *srv = 
    (config::srv *) ap_get_module_config(r->server->module_config, &ndb_module);
    
    r->content_type = "text/plain";
    ap_send_http_header(r);
    
    ap_rprintf(r,"Process ID: %d\n",getpid());
    ap_rprintf(r,"Connect string: %s\n",srv->connect_string);
    ap_rprintf(r,"NDB Cluster Connections: %d\n",process.n_connections);
    ap_rprintf(r,"Apache Threads: %d\n",process.n_threads);
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
    
    return OK;
  }
} /* extern "C" */


/* This strategy, taken from mod_dav, is to write our own error document,
   set verbose-errors-to=*, and return DONE. 
*/
int ndb_handle_error(request_rec *r, int status, data_operation *data, 
                     const char *message) {
  ap_table_setn(r->notes, "verbose-error-to", "*");
  r->status = status;
  r->content_type = "text/plain";
  ap_send_http_header(r);
  
  switch(status) {
    case 404:
      ap_rprintf(r,"No data could be found.\n");
      break;
    default:
      ap_rprintf(r,"HTTP return code %d.\n", status);
  }
  
  return DONE; 
}


table *http_param_table(request_rec *r, const char *c) {
  table *t = ap_make_table(r->pool, 4);
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


