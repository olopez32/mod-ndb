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
#include <stdlib.h>   


namespace config {
  
  /* init_dir():
  initialize the structure and its array of columns
  */
  void *init_dir(pool *p, char *path) {
    config::dir *dir;
    
    dir = (config::dir *) ap_pcalloc(p, sizeof(config::dir));
    dir->columns   = ap_make_array(p, 1, sizeof(char *));
    dir->updatable = ap_make_array(p, 5, sizeof(char *));
    dir->indexes = (void *) ap_make_table(p, 4);
    dir->results = json;
    
    return (void *) dir;
  }
  
  
  /* init_srv() : simply ask apache for some memory 
  */
  void *init_srv(pool *p, server_rec *s) {
    return ap_pcalloc(p, sizeof(config::srv));
  }
  
  
  /* It is possible to merge directory configs;
  let the child config override the parent. 
  */
  void *merge_dir(pool *p, void *v1, void *v2) {
    config::dir *dir = (config::dir *) ap_pcalloc(p, sizeof(config::dir));
    config::dir *d1 = (config::dir *) v1;
    config::dir *d2 = (config::dir *) v2;
    
    dir->database = d2->database ? d2->database : d1->database;
    dir->table =    d2->table    ? d2->table    : d1->table;  
    dir->columns =  d2->columns  ? d2->columns  : d1->columns;
    dir->indexes =  d2->indexes  ? d2->indexes  : d1->indexes;
    dir->results =  d2->results  ? d2->results  : d1->results;
    dir->updatable= d2->updatable? d2->updatable: d1->updatable;
    // dir->pathinf =  d2->pathinf  ? d2->pathinf  : d1->pathinf;

    // Deletes defaults to "Off":
    dir->allow_delete = d2->allow_delete ? 1 : 0;
    return (void *) dir;
  }
    
  /* Process a "Columns" or "AllowUpdates" directive
  by adding the name of each column onto the APR array
  */
  const char *build_column_list(cmd_parms *cmd, void *m, char *arg) {
    config::dir *dir;
    char **str;
    bool do_sort = 0;
    char *which = (char *) cmd->cmd->cmd_data;
    
    dir = (config::dir *) m;
    switch(*which) {
      case 'R':
        str = (char **) ap_push_array(dir->columns);
        break;
      case 'W':
        str = (char **) ap_push_array(dir->updatable);
        do_sort = 1;
        break;
      default:
        return "Unusual error in build_column_list";
    }
    *str = ap_pstrdup(cmd->pool, arg);
    
    return 0;
  }    
  
  /* Process Index directives.
  UniqueIndex index-name column [column ... ]
  OrederedIndex index-name column [column ... ]
  
  When a config file says "UniqueIndex col1$unique col1 col2",
  mod_ndb creates two records to map from column names to indexes, 
  "col1" => "U*col1$unique"  and "col2" => "U*col1$unique".
  "OrderedIndex IDX1 cost" creates a map "cost" => "O*IDX1"
  
  */
  const char *build_index_list(cmd_parms *cmd, void *m, char *idx, char *col) {
    config::dir *dir;
    char *prefix;
    
    dir = (config::dir *) m;
    prefix = (char *) cmd->cmd->cmd_data;
    
    ap_table_set((table *) dir->indexes, col,  /* the key */
      ap_pstrcat(cmd->pool,prefix,idx,0)); /* the value */
    
    return 0;
  }    
  
  const char *set_result_format(cmd_parms *cmd, void *m, char *arg) {
    config::dir *dir;
    
    dir = (config::dir *) m;
    ap_str_tolower(arg);
    if(!strcmp(arg,"raw")) dir->results = raw;
    else if(!strcmp(arg,"XML")) dir->results = xml;
    else if(!strcmp(arg,"ApacheNote")) dir->results = ap_note;
    else {
      dir->results = json;
      if(strcmp(arg,"JSON")) 
        ap_log_error(APLOG_MARK, log::warn, cmd->server,
                     "Invalid result format %s at %s. Using default JSON results.\n",
                     arg, cmd->path);
    }
    return 0;
  }
}
  