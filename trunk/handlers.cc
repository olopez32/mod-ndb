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
    register int response;
    
    // Apache 2 Handler name check
    CheckHandler(r,"ndb-cluster");

    // Fetch configuration  
    dir = (config::dir *) ap_get_module_config(r->per_dir_config, &ndb_module);
    if(! dir->database) {
      log_note2(r->server,"Returning NOT_IMPLEMENTED because no db is defined at %s",
                r->uri);
      return NOT_IMPLEMENTED;
    }
    if(! dir->table) {
      log_note2(r->server,"Returning NOT_IMPLEMENTED because no table is defined at %s",
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
    
    response = Query(r,dir,i);
    switch(response) {
      case OK:
        i->row_found++;
        break;
      case NOT_FOUND:
        i->row_not_found++;
        break;
      case DECLINED:
        i->declined++;
        break;
    }
    return response;
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
    ap_rprintf(r,"Result format: %s\n",(char *[4]){"[None]","JSON","Raw","XML"}
               [(int) dir->results]);
    
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
    ap_rprintf(r,"Row found:     %u\n", i->row_found);
    ap_rprintf(r,"Row not found: %u\n", i->row_not_found);
    ap_rprintf(r,"Declined:      %u\n", i->declined);
    ap_rprintf(r,"Errors:        %u\n", i->errors);
    
    return OK;
  }
} /* extern "C" */


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


