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

namespace config {
  
  /* init_dir():
     initialize the per-directory configuration structure
  */
  void *init_dir(ap_pool *p, char *path) {
    // Initialize everything to zero with ap_pcalloc()
    config::dir *dir = (config::dir *) ap_pcalloc(p, sizeof(config::dir));

    dir->visible     = new(p, 4) apache_array<char *>;
    dir->updatable   = new(p, 4) apache_array<char *>;
    dir->indexes     = new(p, 2) apache_array<config::index>;
    dir->key_columns = new(p, 3) apache_array<config::key_col>;
    dir->results = json;
    dir->use_etags = 1;
    
    return (void *) dir;
  }
  
  
  /* init_srv() : simply ask apache for some zeroed memory 
  */
  void *init_srv(ap_pool *p, server_rec *s) {
    return ap_pcalloc(p, sizeof(config::srv));
  }
  
  
  /* Merge directory configuration:
     d2 is the current directory config exactly as specified in the conf file;
     d1 is a parent directory config that it might inherit something from.
  */
  void *merge_dir(ap_pool *p, void *v1, void *v2) {
    config::dir *dir = (config::dir *) ap_palloc(p, sizeof(config::dir));
    config::dir *d1 = (config::dir *) v1;
    config::dir *d2 = (config::dir *) v2;

    // Start with a copy of d2
    memcpy(dir,d2,sizeof(config::dir));

    // These parts can be inherited from the parent.
    if(! d2->database)  dir->database  = d1->database;
    if(! d2->table)     dir->table     = d1->table;
    if(! d2->visible)   dir->visible   = d1->visible;
    if(! d2->updatable) dir->updatable = d1->updatable;
    if(! d2->results)   dir->results   = d1->results;
    if(! d2->sub_results) dir->sub_results = d1->sub_results;
    if(! d2->format_param[0]) dir->format_param[0] = d1->format_param[0];
    if(! d2->format_param[1]) dir->format_param[1] = d1->format_param[1];
 
    return (void *) dir;
  }
    

  /*  Process Format directives, e.g.   
      "Format JSON" 
  */
  result_format_type fmt_from_str(char *str) {

    ap_str_tolower(str);   // Do all comparisons in lowercase 
    if(!strcmp(str,"json")) return json;
    else if(!strcmp(str,"raw")) return raw;
    else if(!strcmp(str,"xml")) return xml;
    else if(!strcmp(str,"apachenote")) return ap_note;
    else return no_results;
  }

  const char *result_format(cmd_parms *cmd, void *m, 
                            char *fmt, char *arg0, char *arg1)
  {    
    config::dir *dir = (config::dir *) m;
    
    /* Some formatters can take 1 or 2 extra parameters */
    dir->format_param[0] = ap_pstrdup(cmd->pool, arg0);
    dir->format_param[1] = ap_pstrdup(cmd->pool, arg1);

    dir->results = fmt_from_str(fmt);
    if(dir->results == no_results) {
      dir->results = json;
      log_note3(cmd->server,
                "Invalid result format %s at %s. Using default JSON results.\n",
                fmt, cmd->path);
    }
    if(arg0 && dir->results == ap_note) 
      dir->sub_results = fmt_from_str(arg0);    
    else dir->sub_results = no_results;
    
    return 0;
  }

  
  /*  add_key_column():
      While building the dir->key_columns array, we want to keep
      it sorted on the key name.
  
      Add a new element to the end of the array, then shift itmes
      toward the end if necessary to make a hole for the new items
      in the appropriate place.  Fix every chain of column id links,
      then return the array index of the hole.
      
      If the new key already exists, set "exists" to 1, and return
      the index of the column.
      
      add_key_column() allocates permanent space for the column name.
  */
  short add_key_column(cmd_parms *cmd, config::dir *dir, 
                       char *keyname, bool &exists) {
    exists = 0;
    int list_size = dir->key_columns->size();
    config::key_col *keys = dir->key_columns->items();
    register int n = 0, c;
    short insertion_point = 0;
    
    // this could be a binary search instead of a linear search?
    for(n = 0; n < list_size; n++) {
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
    size_t len = (sizeof(config::key_col) * (list_size - insertion_point));
    void *src = (void *) &keys[insertion_point];
    if(len > 0) {
      void *dst = (void *) &keys[insertion_point + 1];
      memmove(dst, src, len);
    }
    // Clear out the previous contents and initialize the column.
    bzero(src, sizeof(config::key_col));
    
    // Set the immutable column attributes: name and serial_no
    keys[insertion_point].name = ap_pstrdup(cmd->pool, keyname);
    keys[insertion_point].serial_no = list_size;

    /* ==================================== fix all columns  */
    if(len > 0) { 
      /* The columns were reordered, so fix up all links */ 
      short serial, this_col, next_col;
      int n_columns = dir->key_columns->size();
      int n_indexes = dir->indexes->size();
      config::index *indexes = dir->indexes->items();

      /* Build a map from serial_no to column id.
         (This is what the idx_map_bucket is for).
      */
      for(n = 0 ; n < n_columns ; n++) 
        keys[keys[n].serial_no].idx_map_bucket = n;
      
      /* For each index record, fix the first_col link
         and the subsequent chain of next_in_key links.
      */
      for(n = 0 ; n < n_indexes ; n++) {
        serial = indexes[n].first_col_serial;
        if(serial != -1) {
        // Fix the head of the chain
          this_col = keys[serial].idx_map_bucket;
          indexes[n].first_col = this_col;
          serial = keys[this_col].next_in_key_serial;
        // Follow the chain, fixing every item
          while(serial != -1) {
          // Fix the column
            next_col = keys[serial].idx_map_bucket;
            keys[this_col].next_in_key = next_col;
          // Advance the pointer
            serial = keys[next_col].next_in_key_serial;
            this_col = next_col;
          } /* End of chain */
        }
      } /* End of indexes */  
      
      // Fix the filter_col pointer in each filter alias column
      for(n = 0 ; n < n_columns ; n++)
        if(keys[n].is.filter)
          keys[n].filter_col =
            keys[(keys[n].filter_col_serial)].idx_map_bucket;
      
      // Fix the pathinfo chain
      for(n = 0 ; n < dir->pathinfo_size ; n++) 
        dir->pathinfo[n] =
          keys[dir->pathinfo[n+(dir->pathinfo_size)]].idx_map_bucket;      
    } /* ================================= end of fixing links */
    
    return insertion_point;
  }


  /* "Pathinfo col1/col2" 
  */
  const char *pathinfo(cmd_parms *cmd, void *m, char *arg1, char *arg2) {
    
    /* The Representation of Pathinfo:
       Pathinfo specifies how to interpret the rightmost components of a 
       pathname. dir->pathinfo_size is n, the number of these components.
       dir->pathinfo points to an array of 2n short integers. 
       The first n array elements hold the column ids of the pathinfo 
       columns; the second n elements hold the corresponding serial numbers.
    */
    config::dir *dir = (config::dir *) m;
    int pos_size=1, real_size=0;
    char *c, *word, **items;
    const char *path = arg1;
    short col_id;
    bool col_exists;
    
    // Count the number of '/' characters
    for(c = arg1 ; *c ; c++) if(*c == '/') pos_size++;
    
    // Allocate space for an array of strings
    items = (char **) ap_pcalloc(cmd->temp_pool, pos_size * sizeof(char *));
    
    // Parse the spec into its components
    while(*path && (word = ap_getword(cmd->temp_pool, &path, '/')))
      if(strlen(word)) 
        items[real_size++] = word;
    
    // Initialize the dir->pathinfo array.
    dir->pathinfo_size = real_size;
    dir->pathinfo = (short *) 
      ap_pcalloc(cmd->pool, 2 * real_size * sizeof(short));
    
    // Fetch the column IDs and serial numbers
    for(int n = 0 ; n < real_size ; n++) {
      col_id = add_key_column(cmd, dir, items[n], col_exists);
      dir->pathinfo[n] = col_id;
      dir->pathinfo[real_size+n] = dir->key_columns->item(col_id).serial_no;
    }

    // Flags
    if(arg2) {
      ap_str_tolower(arg2);
      if(! strcmp(arg2,"always"))
        dir->flag.pathinfo_always = 1;
    }
    
    return 0;
  }
  
  
  /* Process a "Columns" or "AllowUpdates" directive
  */
  const char *non_key_column(cmd_parms *cmd, void *m, char *arg) {
    char *which = (char *) cmd->cmd->cmd_data;
    config::dir *dir = (config::dir *) m;
    
    switch(*which) {
      case 'R':
        *dir->visible->new_item() = ap_pstrdup(cmd->pool, arg);
        break;
      case 'W':
        *dir->updatable->new_item() = ap_pstrdup(cmd->pool, arg);
    }
    return 0;
  }    
  
  
  short add_column_to_index(cmd_parms *cmd, config::dir *dir, char *col_name, 
                            short index_id, bool & col_exists)
  {
    config::index *indexes = dir->indexes->items();
    config::key_col *cols;   // do not initialize until after add_key_column()
    short id;
    
    log_conf_debug(cmd->server,"Adding column %s to index",col_name);
    id = add_key_column(cmd, dir, col_name, col_exists);
    cols = dir->key_columns->items();

    if(col_exists) {
      if((cols[id].index_id != -1) && (index_id != -1))
        log_err3(cmd->server, "Reassociating column %s with index %s", 
            col_name, indexes[index_id].name);
    }    
    cols[id].index_id = index_id;
    if(index_id >= 0) {
      if(indexes[index_id].type == 'P') {
        cols[id].is.in_pk = 1;
        cols[id].implied_plan = PrimaryKey;
      }
      else if(indexes[index_id].type == 'U') {
        cols[id].is.in_hash_idx = 1;
        cols[id].implied_plan = UniqueIndexAccess;
      }
      else if(indexes[index_id].type == 'O') {
        cols[id].is.in_ord_idx = 1;
        cols[id].implied_plan = OrderedIndexScan;
        cols[id].filter_op = NdbIndexScanOperation::BoundEQ;
      }
    }
    cols[id].next_in_key_serial = -1;  
    cols[id].next_in_key = -1; 
 
    return  id;
  }
  
  
  const char *primary_key(cmd_parms *cmd, void *m, char *col) {
    return named_index(cmd, m, "*Primary$Key*", col);
  }
  
  
  /* filter(), syntax "Filter column [operator pseudocolumn]"
     as in (a) "Filter year" or (b) "Filter x > min_x".
  */
  const char *filter(cmd_parms *cmd, void *m, char *base_col_name,
                     char *filter_op, char *alias_col_name) {
    bool base_col_exists, alias_col_exists;
    short filter_col, base_col_id = -1, alias_col_id = -1;
    config::dir *dir = (config::dir *) m;

    // This must correspond to items 0-5 in NdbScanFilter::BinaryCondition 
    char *valid_filter_ops[] = { "<=" , "<" , ">=" , ">" , "=" , "!=" , 0 };
            
    // Create the columns 
    if(base_col_name)
      base_col_id = add_key_column(cmd, dir, base_col_name, base_col_exists);    
    if(alias_col_name) 
      alias_col_id = add_key_column(cmd, dir, alias_col_name, alias_col_exists);

    config::key_col *columns = dir->key_columns->items();

    if(! base_col_exists)
      columns[base_col_id].index_id = -1;  
 
    if(alias_col_name) {
      // Three-argument syntax, e.g. "Filter x >= min_x".  This could be 
      // either an NdbScanFilter or a boundary condition on an OrderedIndex.
      if((alias_col_exists) && (columns[alias_col_id].index_id >= 0))
        return ap_psprintf(cmd->pool,"Alias column %s must not be a real column.",
                           alias_col_name);

      // Use the alias column as the filter 
      filter_col = alias_col_id;
      columns[alias_col_id].is.alias = 1;

      /* Is the base column part of an ordered index?  (Because of this test, we
         require the index to be defined before the filter in httpd.conf)
      */
      if(columns[base_col_id].implied_plan != OrderedIndexScan) {
        dir->flag.has_filters = 1;
        /* ?? columns[base_col_id].is.filter = 1; ?? */
      }
      // Parse the operator
      bool found_match = 0;
      for(int n = 0 ; valid_filter_ops[n] ; n++) {
        if(!strcmp(filter_op,valid_filter_ops[n])) {
          columns[alias_col_id].filter_op = n;
          found_match = 1;
        }
      }
      if(!found_match)
        return ap_psprintf(cmd->pool,"Error: %s is not a valid filter operator",
                          filter_op);
    }
    else {
      // One-argument syntax, e.g. "Filter year."   This is an NdbScanFilter.
      if(columns[base_col_id].index_id >= 0) {
        columns[base_col_id].implied_plan = NoPlan;
        log_debug3(cmd->server,"Column %s is a filter, so including it in a req"
                   "uest will NOT cause that request to use index %s", base_col_name, 
                   dir->indexes->item(columns[base_col_id].index_id).name);
      }
      dir->flag.has_filters = 1;
      filter_col = base_col_id;   // Use the base col as the filter 
      columns[base_col_id].filter_op = NdbScanFilter::COND_EQ; 
      log_conf_debug(cmd->server,"Creating new filter %s",base_col_name);
    }

    columns[filter_col].is.filter = 1;
    columns[filter_col].filter_col = base_col_id;
    columns[filter_col].filter_col_serial = columns[base_col_id].serial_no;
    
    return 0;
  }


  short get_index_by_name(config::dir *dir, char *idx) {
    config::index *indexes = dir->indexes->items();
    for(short n = 0 ; n < dir->indexes->size() ; n++) 
      if(!strcmp(idx, indexes[n].name))
        return n;
    return -1;
  }
    
    
  /* named_index():  process Index directives.
     UniqueIndex index-name column [column ... ]
     OrderedIndex index-name column [column ... ]

     Create an index record for the index, and a key_column record 
     for each column    
  */
  const char *named_index(cmd_parms *cmd, void *m, char *idx, char *col) 
  {
    short index_id, col_id;
    config::index *index_rec;
    config::key_col *cols;
    short i;

    config::dir *dir = (config::dir *) m;
    char *which = (char *) cmd->cmd->cmd_data;
    index_id = get_index_by_name(dir,idx);

    if(index_id == -1) {
      /* Build the index record */
      log_conf_debug(cmd->server,"Creating new index record %s",idx);
      index_rec = dir->indexes->new_item();
      bzero(index_rec, dir->indexes->elt_size);
      index_id = dir->indexes->size() - 1;
      index_rec->name = ap_pstrdup(cmd->pool, idx);
      index_rec->type = *which;                       
      index_rec->first_col_serial = -1;
      index_rec->first_col = -1;
    }
    else 
      index_rec = & dir->indexes->item(index_id);

    /* Sometimes a column name is not actually a column, but a flag */
    if(index_rec->type == 'O' && *col == '[') {
      if(!strcmp(col,"[ASC]")) {
        index_rec->flag = NdbScanOperation::SF_OrderBy;
        return 0;
      }
      else if(!strcmp(col,"[DESC]")) {
        index_rec->flag = NdbScanOperation::SF_Descending;
        return 0;
      }
    }
    
    /* Create a column record */
    bool col_exists = 0;
    col_id = add_column_to_index(cmd, dir, col, index_id, col_exists);
    cols = dir->key_columns->items();
    index_rec->n_columns++;
    
    /* Manage the chain of links from an index to its member columns */
    short head_col, tail_col;
    // 1: Find the end of the chain
    head_col = index_rec->first_col;
    if(head_col == -1) {
      // The chain ends at the index record;
      // push the new column on to the chain.
      index_rec->first_col_serial = cols[col_id].serial_no;
      index_rec->first_col = col_id;
    }
    else {
      // Follow the chain to the end
      i = head_col;
      while(i != -1) {
        tail_col = i;
        i = cols[i].next_in_key;
      }
      // 2: Push the new column onto the chain
      cols[tail_col].next_in_key_serial = cols[col_id].serial_no; 
      cols[tail_col].next_in_key = col_id;
    }
    return 0;
  }
} 
