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

    dir->visible     = ap_make_array(p, 4, sizeof(char *));
    dir->updatable   = ap_make_array(p, 4, sizeof(char *));
    dir->pathinfo    = ap_make_array(p, 2, sizeof(char *));
    dir->indexes     = ap_make_array(p, 4, sizeof(config::index));
    dir->key_columns = ap_make_array(p, 4, sizeof(config::key_col));
    dir->index_map   = ap_make_table(p, 4);
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
        str = (char **) ap_push_array(dir->visible);
        break;
      case 'W':
        str = (char **) ap_push_array(dir->updatable);
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
  
  
  /* While building the dir->key_columns array, we want to keep
     it sorted on the key name.
     
     ap_push_array() allocates a new element at the end of the array and 
     returns a pointer to it.  add_key_column() is a wrapper that shifts
     items toward the end if necessary to make a hole for the new item 
     in the appropriate place, and then returns the array index of the hole.
     
     If the new key already exists, "exists" is set to 1, no new element is
     allocated, and the return value indicates the index of the column.
  */
  short add_key_column(array_header *a, char *keyname, bool &exists) {
    exists = 0;
    int list_size = a->nelts;
    config::key_col **keys = (config::key_col **) a->elts;
    register int n = 0, insertion_point = 0, c;
    
    for(n = 0; n < list_size; n++) {
      c = strcmp(keyname, keys[n]->name);
      if(c < 0) 
        /* keyname < item->name */
        break;
      if(c > 0) 
        /* keyname > item->name */
        insertion_point = n + 1;
      else {
        /* c == 0 */
        exists = 1;
        return n;
      }
    }    

    // ap_push_array returns a pointer, but ignore it.
    ap_push_array(a);

    // ap_push_array may have caused the whole array to get relocated */
    keys = (config::key_col **) a->elts;
    
    // All elements after the insertion point get shifted right one place
    size_t len = (sizeof(config::key_col)) * (list_size - insertion_point);
    if(len > 0) {
      void *src = (void *) keys[insertion_point];
      void *dst = (void *) keys[insertion_point + 1];
      memmove(dst, src, len);
      // clear out the previous value
      bzero(src, sizeof(config::key_col));
    }

    return insertion_point;
  }


  /* 
    Every time a new column is added, the columns get reshuffled some,
    so we have to fix all the mappings between serial numbers and 
    actual column id numbers.
    
    The configuration API in Apache never gives the module a chance to 
    "finalize" a configuration structure.  You never know when you're finished
    with a particular directory.  So, we run fix_all_columns() every time we
    create a new column, which, alas, does not scale too well.
    
    While processing the config file, the CPU time spent fixing columns grows 
    with n-squared, the square of the number of columns.  This could be improved 
    using config handling that was more complex (a container directive) or less
    user-friendly (an explicit "end" token).
    
    On the other hand, the design is optimized for handling queries at runtime, 
    where some operations (e.g. following the list of columns that belong to an 
    index) are constant, and the worst (looking up a column name in the columns 
    table) grows at log n.
    
  */
  
  void fix_all_columns(config::dir *dir) {
    short n, i, j;
  
    const int n_columns = dir->key_columns->nelts;
    const int n_indexes = dir->indexes->nelts;
  
    config::key_col **cols = (config::key_col **) dir->key_columns->elts;
    config::index **indexes = (config::index **) dir->indexes->elts;

    /* Build a map from serial_no to column id.
      (This is what the idx_map_bucket slot is for).
    */
    for(n = 0 ; n < n_columns ; n++) 
      cols[(cols[n]->serial_no)]->idx_map_bucket = n;
    
    /* Fix the first_col_idx pointer in each index record
       and the subsequent chain of next_in_key_idx pointers.
    */
    for(n = 0 ; n < n_indexes ; n++) {
      i = cols[(indexes[n]->first_col_serial)]->idx_map_bucket;
      indexes[n]->first_col_idx = i;
      while(i != -1) {
        j = cols[(cols[i]->next_in_key_serial)]->idx_map_bucket;
        cols[i]->next_in_key_idx = j;
        i = j;
      }
    } 
        
    /* Fix the filter_col_idx pointer in each filter column
    */
    for(n = 0 ; n < n_columns ; n++)
      if(cols[n]->is_filter)
        cols[n]->filter_col_idx =
          cols[(cols[n]->filter_col_serial)]->idx_map_bucket;
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
    config::index * index_rec;
    config::key_col *this_col;
    short i, j;

    config::dir *dir = (config::dir *) m;
    config::key_col **cols = (config::key_col **) dir->key_columns->elts;
    char *which = (char *) cmd->cmd->cmd_data;
    table *idxmap = (table *) dir->index_map;
    char *index_id_str = (char *) ap_table_get(idxmap,idx);

    if(index_id_str) {
      index_id = atoi(index_id_str);
      index_rec = (config::index *) dir->indexes->elts[index_id];
    }
    else {
      /* Build the index record */
      index_rec = (config::index *) ap_push_array(dir->indexes);
      index_id = dir->indexes->nelts - 1;
      index_rec->name = ap_pstrdup(cmd->pool, idx);   // Set the name
      if(*which == 'U')                               // Set the type
        index_rec->type = unique;
      else if (*which == 'O')
        index_rec->type = ordered;
      else 
        return "Unusual bug in build_index_list()";

      index_rec->n_columns = 0;                       // Set the rest
      index_rec->first_col_serial = -1;
      index_rec->first_col_idx = -1;
      
      /* Create the map entry */
      ap_table_set(idxmap, idx, ap_psprintf(cmd->pool,"%d",index_id));      
    }
    
    /* Create a column record */
    bool col_exists = 0;
    col_id = add_key_column(dir->key_columns, col, col_exists);
    this_col = cols[col_id];
    
    /* The column record already existed */
    if(col_exists) {
      if(this_col->index_id != -1) {
        index_rec = (config::index *) dir->indexes->elts[this_col->index_id];
        return ap_pstrcat(cmd->pool,"Configuration error at ", cmd->path, 
                          " -- cannot associate column ", col, " with index ", 
                          idx, "; it is already connected to index",
                          index_rec->name);
      }
      // how did we get here??
      log_debug(cmd->server,"how did we get here? %s", col);
    }
    else {
      /* Initialize the column record */
      this_col->name = col;
      this_col->index_id = index_id;
      this_col->serial_no = dir->key_columns->nelts - 1;
      this_col->next_in_key_serial = -1;
      this_col->next_in_key_idx = -1;
      fix_all_columns(dir);
    }
      
    /* Manage the chain of links from an index to its member columns */
    short first_key_part = index_rec->first_col_idx;
    if(first_key_part == -1) {
      index_rec->first_col_serial = this_col->serial_no;
      index_rec->first_col_idx = col_id;
    }
    else {
      i = first_key_part;
      while((j = cols[i]->next_in_key_idx) != -1)
        i = j;
     cols[i]->next_in_key_serial = this_col->serial_no;
     cols[i]->next_in_key_idx = col_id;
    }    
  
  }
  
}
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  