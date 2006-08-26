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
     initialize the per-directory configuration structure
  */
  void *init_dir(pool *p, char *path) {
    config::dir *dir;
    
    dir = (config::dir *) ap_pcalloc(p, sizeof(config::dir));
    // Using ap_pcalloc(), everything is initialized at 0,
    // including an important explicit default:
    // dir->allow_delete = 0; 

    dir->visible     = new(p, 4) apache_array<char *>;
    dir->updatable   = new(p, 4) apache_array<char *>;
    dir->pathinfo    = new(p, 2) apache_array<char *>;
    dir->indexes     = new(p, 2) apache_array<config::index>;
    dir->key_columns = new(p, 4) apache_array<config::key_col>;
    dir->results = json;
    
    return (void *) dir;
  }
  
  
  /* init_srv() : simply ask apache for some zeroed memory 
  */
  void *init_srv(pool *p, server_rec *s) {
    return ap_pcalloc(p, sizeof(config::srv));
  }
  
  
  /* It is possible to merge directory configs;
     If an item is "inhheritable" let the child config override the parent,
     but if the item is not inheritable, ignore the parent config.
  */
  void *merge_dir(pool *p, void *v1, void *v2) {
    config::dir *dir = (config::dir *) ap_pcalloc(p, sizeof(config::dir));
    config::dir *d1 = (config::dir *) v1;
    config::dir *d2 = (config::dir *) v2;

    // Things that cannot be inherited:
    dir->allow_delete = d2->allow_delete; 
    dir->indexes = d2->indexes;
    dir->key_columns = d2->key_columns;
    
    // Things that can be inherited:    
    dir->database = d2->database ? d2->database : d1->database;
    dir->table =    d2->table    ? d2->table    : d1->table;  
    dir->visible =  d2->visible  ? d2->visible  : d1->visible;
    dir->updatable= d2->updatable? d2->updatable: d1->updatable;
    dir->results =  d2->results  ? d2->results  : d1->results;
    dir->pathinfo=  d2->pathinfo ? d2->pathinfo : d1->pathinfo;
    dir->format_param[0] = d2->format_param[0] ?
                    d2->format_param[0] : d1->format_param[0];
    dir->format_param[1] = d2->format_param[1] ?
                    d2->format_param[1] : d1->format_param[1];    
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
        str = dir->visible->new_item();
        break;
      case 'W':
        str = dir->updatable->new_item();
        do_sort = 1;
        break;
      default:
        return "Unusual bug in build_column_list()";
    }
    *str = ap_pstrdup(cmd->pool, arg);
    
    return 0;
  }    


  /*  Process Format directives, e.g.   
      "Format JSON" 
  */

  const char *set_result_format(cmd_parms *cmd, void *m, 
                                char *fmt, char *arg0, char *arg1) {
    
    config::dir *dir = (config::dir *) m;
    
    /* Some formatters can take 1 or 2 extra parameters */
    dir->format_param[0] = ap_pstrdup(cmd->pool, arg0);
    dir->format_param[1] = ap_pstrdup(cmd->pool, arg1);
    
    ap_str_tolower(fmt);   // Do all comparisons in lowercase 
    if(!strcmp(fmt,"raw")) dir->results = raw;
    else if(!strcmp(fmt,"xml")) dir->results = xml;
    else if(!strcmp(fmt,"apachenote")) dir->results = ap_note;
    else {
      dir->results = json;
      if(strcmp(fmt,"json")) 
        ap_log_error(APLOG_MARK, log::warn, cmd->server,
                     "Invalid result format %s at %s. Using default JSON results.\n",
                     fmt, cmd->path);
    }
    return 0;
  }

  
  /* 
    The list of key columns is sorted, so, when a new column is 
    added, some columns may get shuffled around --  
    so we rebuild all of the links between columns.
  */  
  void fix_all_columns(config::dir *dir) {
    short n, serial, this_col, next_col;
    
    config::key_col *cols = dir->key_columns->items();
    int n_columns = dir->key_columns->size();
    config::index *indexes = dir->indexes->items();
    int n_indexes = dir->indexes->size();
        
    /* Build a map from serial_no to column id.
       (This is what the idx_map_bucket is for).
    */
    for(n = 0 ; n < n_columns ; n++) 
      cols[cols[n].serial_no].idx_map_bucket = n;
    
    /* For each index record, fix the first_col link
       and the subsequent chain of next_in_key links.
    */
    for(n = 0 ; n < n_indexes ; n++) {
      serial = indexes[n].first_col_serial;
      if(serial != -1) {
        // Fix the head of the chain
        this_col = cols[serial].idx_map_bucket;
        indexes[n].first_col = this_col;
        serial = cols[this_col].next_in_key_serial;
        // Follow the chain, fixing every item
        while(serial != -1) {
          // Fix the column
          next_col = cols[serial].idx_map_bucket;
          cols[this_col].next_in_key = next_col;
          // Advance the pointer
          serial = cols[next_col].next_in_key_serial;
          this_col = next_col;
        }
        // End of chain
      }
    }
    
    /* Fix the filter_col pointer in each filter column
    */
    for(n = 0 ; n < n_columns ; n++)
      if(cols[n].is_filter)
        cols[n].filter_col =
          cols[(cols[n].filter_col_serial)].idx_map_bucket;
  }

  
  /*  add_key_column():
      While building the dir->key_columns array, we want to keep
      it sorted on the key name.
  
      Add a new element to the end of the array, then shift itmes
      toward the end if necessary to make a hole for the new item 
      in the appropriate place. Return the array index of the hole.
      
      If the new key already exists, set "exists" to 1, and return
      the index of the column.
  */
  short add_key_column(cmd_parms *cmd, config::dir *dir, 
                       char *keyname, bool &exists) {
    exists = 0;
    int list_size = dir->key_columns->size();
    config::key_col *keys = dir->key_columns->items();
    register int n = 0, c;
    short insertion_point = 0;
    
    for(n = 0; n < list_size; n++) {
      ap_log_error(APLOG_MARK, log::debug, cmd->server, 
                   "Comparing %s to %s",keyname, keys[n].name); 
      c = strcmp(keyname, keys[n].name); 
      if(c < 0) break;  // the new key sorts before this one 
      if(c > 0) insertion_point = n + 1;  // the new key sorts after this one
      else {  // the keys are the same 
        exists = 1;
        return n;
      }
    }        

    // Create a new key_col at the end of the array
    dir->key_columns->new_item();
    
    // Expansion may have caused the whole array to be relocated
    keys = dir->key_columns->items();
      
    // All elements after the insertion point get shifted right one place
    size_t len = (sizeof(config::key_col)) * (list_size - insertion_point);
    void *src = (void *) &keys[insertion_point];
    if(len > 0) {
      void *dst = (void *) &keys[insertion_point + 1];
      ap_log_error(APLOG_MARK, log::debug, cmd->server, "Moving %d bytes "
                   "(%d records) at record %hd", (int) len,
                   list_size - insertion_point, insertion_point);
      memmove(dst, src, len);
    }
    // clear out the previous contents and initialize the column.
    bzero(src, sizeof(config::key_col));
    keys[insertion_point].name = ap_pstrdup(cmd->pool, keyname);
    keys[insertion_point].serial_no = list_size;

    log_debug(cmd->server,"add_key_column(): created column at position %hd",
              insertion_point);
    return insertion_point;
  }

  
  short add_column_to_index(cmd_parms *cmd, config::dir *dir, char *col_name, 
                            short index_id, bool & col_exists)
  {
    config::index *indexes = dir->indexes->items();
    config::key_col *cols;   // do not initialize until after add_key_column()
    short id;
    
    log_debug(cmd->server,"Adding column %s to index",col_name);
    id = add_key_column(cmd, dir, col_name, col_exists);
    cols = dir->key_columns->items();

    if(col_exists) {
      log_debug(cmd->server,"Column %s already existed.",col_name);
      if((cols[id].index_id != -1) && (index_id != -1)) {
        ap_log_error(APLOG_MARK, log::err, cmd->server,
                     "Configuration error at %s associating column %s "
                     "with index %s; it is already connected to index %s "
                     "and mod_ndb does not allow you to associate a key column "
                     "with multiple named indexes.", cmd->path, col_name, 
                     indexes[index_id].name,indexes[cols[id].index_id].name);
       }
    }

    cols[id].index_id = index_id; 
    cols[id].next_in_key_serial = -1;  
    cols[id].next_in_key = -1; 
 
    /* If the column's serial number is the same as its id, then it was added
       at the end of the array.  But if not, some columns were moved around, 
       and all of the links need to be fixed. */     
    if(id != cols[id].serial_no) {
      log_debug(cmd->server,"Fixing links for %d resorted column(s).",
                cols[id].serial_no - id);
      fix_all_columns(dir); 
    }
    ap_log_error(APLOG_MARK, log::debug, cmd->server,"Column %s created. "
                 "Id: %hd.  Serial: %hd.  Index:  %hd.", 
                 cols[id].name, id, cols[id].serial_no, cols[id].index_id);
    return  id;
  }
  
  
  const char *primary_key(cmd_parms *cmd, void *m, char *col) {
    bool col_exists = 0;

    log_debug(cmd->server,"Registering column %s to primary key",col);
    config::dir *dir = (config::dir *) m;    
    short col_id = add_column_to_index(cmd, dir, col, -1, col_exists); // ??
    dir->key_columns->items()[col_id].is_in_pk = 1;
    
    return 0;
  }
  
  
  short get_index_by_name(config::dir *dir, char *idx) {
    int n;
    for(n = 0 ; n < dir->indexes->size() ; n++) 
      if(!strcmp(idx, dir->indexes->items()[n].name))
        return n;
    return -1;
  }
    
    
  /* 
    Process Index directives:
    UniqueIndex index-name column [column ... ]
    OrderedIndex index-name column [column ... ]

    Create an index record for the index, and a key_column record 
    for each column    
  */
  const char *build_index_list(cmd_parms *cmd, void *m, char *idx, char *col) 
  {
    short index_id, col_id;
    config::index *index_rec;
    config::key_col *cols, *new_col;
    short i;

    config::dir *dir = (config::dir *) m;
    char *which = (char *) cmd->cmd->cmd_data;
    index_id = get_index_by_name(dir,idx);

    if(index_id == -1) {
      /* Build the index record */
      log_debug(cmd->server,"Creating new index record %s",idx);
      index_rec = dir->indexes->new_item();
      bzero(index_rec, sizeof(config::index));
      index_id = dir->indexes->size() - 1;
      index_rec->name = ap_pstrdup(cmd->pool, idx);   // Set the name ...
      index_rec->type = *which;                       
      index_rec->n_columns = 0;                       
      index_rec->first_col_serial = -1;
      index_rec->first_col = -1;
    }
    
    /* Create a column record */
    bool col_exists = 0;
    col_id = add_column_to_index(cmd, dir, col, index_id, col_exists);
    new_col = & dir->key_columns->items()[col_id];
    index_rec->n_columns++;
    
    /* Manage the chain of links from an index to its member columns */
    // 1: Find the end of the chain
    short first_key_part = index_rec->first_col;
    if(first_key_part == -1) {
      // The chain ends at the index record;
      // push the new column on to the chain.
      index_rec->first_col_serial = new_col->serial_no;
      index_rec->first_col = col_id;
    }
    else {
      // Follow the chain to the end
      cols = dir->key_columns->items();
      i = first_key_part;
      while(i != -1) 
        i = cols[i].next_in_key;

      // 2: Push the new column on to the chain
      cols[i].next_in_key_serial = new_col->serial_no; 
      cols[i].next_in_key = col_id;
    }
        
    return 0;
  }
}  