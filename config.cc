/* Copyright (C) 2006 - 2009 Sun Microsystems
 All rights reserved. Use is subject to license terms.

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

#include "N-SQL/Parser.h"

config::dir *all_endpoints[MAX_ENDPOINTS];
int n_endp = 0;

/* These operators correspond to the items in NdbScanFilter::BinaryCondition 
*/
const char *filter_ops[] = {"<=","<", ">=",">", "=","!=", "LIKE","NOTLIKE", 0};

/* NDB scan bounds are defined to "exclude" values outside the bounds, and are
   either strict or non-strict, so this list of operators maps to the sequence
   BoundLE, BoundLT, BoundGE, BoundGT in NdbIndexScanOperation::BoundType 
*/
const char *relational_ops[] = { ">=" , ">" , "<=" , "<" , 0 };


/* unescape() handles user-defined output formats that may contain "\n"
*/
const char *unescape(ap_pool *p, const char *str) {
  /* The unescaped string will never be longer than the original */
  char *res = (char *) ap_pcalloc(p, strlen(str) + 1);
  char *c = res;
  
  while(*str) {
    if(*str == '\\' && *(str+1) == 'n')
      *c++ = '\n', str+=2;
    else
      *c++ = *str++;
  }
  *c = 0;
  return (const char *) res;
}


/* unquote_qstring() handles quoted strings that come from the N-SQL parser
*/
char *unquote_qstring(cmd_parms *cmd, const char *str) {
  /* The unquoted string will never be longer than the original */
  char *res = (char *) ap_pcalloc(cmd->pool, strlen(str) + 1);
  char *c = res;

  assert(*str++ == '"');  /* opening quote */
  while(*str) {
    if(*str == '\\') {    /* the next character is a backslash or a quote */
      str++; 
      *c++ = *str++;  
    }
    else if(*str == '"') break; /* closing quote */
    else 
      *c++ = *str++;
  }
  *c = 0;
  return res;
}


namespace config {
  
  /* init_dir():
     initialize the per-directory configuration structure
  */
  void *init_dir(ap_pool *p, char *path) {
    // Initialize everything to zero with ap_pcalloc()
    config::dir *dir = (config::dir *) ap_pcalloc(p, sizeof(config::dir));

    dir->path        = ap_pstrdup(p, path);
    dir->visible     = new(p, 4) apache_array<char *>;
    dir->aliases     = new(p, 4) apache_array<char *>;
    dir->updatable   = new(p, 4) apache_array<char *>;
    dir->indexes     = new(p, 2) apache_array<config::index>;
    dir->key_columns = new(p, 3) apache_array<config::key_col>;
    dir->index_scan  = (index *) ap_pcalloc(p, sizeof(index));
    dir->fmt = get_format_by_name("JSON");
    dir->flag.use_etags = 1;
    dir->default_key = -1;
    dir->magic_number = 0xBABECAFE ;
  
    all_endpoints[n_endp++] = dir;
  
    return (void *) dir;
  }
  
  
  /* init_srv()  
  */
  void *init_srv(ap_pool *p, server_rec *s) {
    config::srv *srv = (config::srv *) ap_pcalloc(p, sizeof(config::srv));

    srv->connect_string = 0;
    srv->max_read_operations = DEFAULT_MAX_READ_OPERATIONS;
    srv->max_retry_ms = DEFAULT_MAX_RETRY_MS ;
    srv->force_restart = DEFAULT_FORCE_RESTART ;
    srv->magic_number = 0xCAFEBABE ;

    initialize_output_formats(p);
    register_built_in_formatters(p);

    return (void *) srv;
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
    if(! d2->fmt)       dir->fmt       = d1->fmt;
    if(! d2->incr_prefetch) dir->incr_prefetch = d1->incr_prefetch;
 
    return (void *) dir;
  }


  void *merge_srv(ap_pool *p, void *v1, void *v2) {
    config::srv *srv = (config::srv *) ap_pcalloc(p, sizeof(config::srv));
    config::srv *s1 = (config::srv *) v1;
    config::srv *s2 = (config::srv *) v2;
    
    // Start with a copy of s2
    memcpy(srv,s2,sizeof(config::srv));
    
    // These parts can be inherited from the parent.
    if(! s2->connect_string)    srv->connect_string = s1->connect_string ;
    if(! s2->max_read_operations)
        srv->max_read_operations = s1->max_read_operations;
    
    return (void *) srv;    
  }



/* With per-server config directives, it is not valid to 
   cast m to a config::srv structure, and you cannot use the
   ap_set_string_slot() family of functions. 
*/
  const char *connectstring(cmd_parms *cmd, void *m, char *arg) {
    config::srv *srv = (config::srv *) 
      ap_get_module_config(cmd->server->module_config, &ndb_module);
    
    assert(srv->magic_number == 0xCAFEBABE);
    srv->connect_string = ap_pstrdup(cmd->pool, arg);
    return 0;  
  }


  const char *srv_set_int(cmd_parms *cmd, void *m, char *arg) {
    config::srv *srv = (config::srv *) 
      ap_get_module_config(cmd->server->module_config, &ndb_module);
    
    assert(srv->magic_number == 0xCAFEBABE);
    if(!strcmp(cmd->cmd->name, "ndb-max-read-subrequests"))
       srv->max_read_operations = atoi(arg);
    else if(!strcmp(cmd->cmd->name, "ndb-retry-ms"))
       srv->max_retry_ms = atoi(arg);
    else assert(0);
    
    return 0;
  }


  const char *force_restart(cmd_parms *cmd, void *m, int flag) {
    config::srv *srv = (config::srv *) 
    ap_get_module_config(cmd->server->module_config, &ndb_module);
    
    assert(srv->magic_number == 0xCAFEBABE);
    srv->force_restart = flag;
    return 0;
  }

  
  const char *result_format(cmd_parms *cmd, void *m, char *format)
  {    
    config::dir *dir = (config::dir *) m;
    
    dir->fmt = get_format_by_name(format);
    if(! dir->fmt) 
      return ap_psprintf(cmd->pool,"Undefined result format \"%s\".", format);

    return 0;
  }


  const char *dir_set_flag(cmd_parms *cmd, void *m, int flag) {
    config::dir *dir = (config::dir *) m;

    if(!strcmp(cmd->cmd->name, "Deletes"))
      dir->flag.allow_delete = flag;
    else if(!strcmp(cmd->cmd->name, "ETags"))
      dir->flag.use_etags = flag;
    else assert(0);

    return 0;
  }
  
  
  /*  add_key_column():
      While building the dir->key_columns array, we want to keep
      it sorted on the key name.
  
      Add a new element to the end of the array, then shift items
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
        if(! strcmp(arg,"*")) dir->flag.select_star = 1;
        else {
          *dir->visible->new_item() = ap_pstrdup(cmd->pool, arg);
          *dir->aliases->new_item() = ap_pstrdup(cmd->pool, arg);
        }
        break;
      case 'W':
        *dir->updatable->new_item() = ap_pstrdup(cmd->pool, arg);
    }
    return 0;
  }    
  
  
  short add_column_to_index(cmd_parms *cmd, config::dir *dir, short index_id, 
                            NSQL::Expr *expr, bool &col_exists)
  {
    config::index *indexes = dir->indexes->items();
    config::key_col *cols;   // do not initialize until after add_key_column()
    char *col_name = expr->value;
    short id;
    
    id = add_key_column(cmd, dir, col_name, col_exists);
    cols = dir->key_columns->items();

    if(col_exists) {
      if((cols[id].index_id != -1) && (index_id != -1))
        log_err(cmd->server, "Reassociating column %s with index %s", 
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
        cols[id].rel_op = expr->rel_op;
        cols[id].base_col_name = expr->base_col_name;
      }
    }
    cols[id].next_in_key_serial = -1;  
    cols[id].next_in_key = -1; 
 
    return id;
  }

  
  const char *primary_key(cmd_parms *cmd, void *m, char *col) {
    return named_index(cmd, m, "*Primary$Key*", col);
  }


  const char *table(cmd_parms *cmd, void *m, char *arg1, char *arg2, char *arg3) {
    config::dir *dir = (config::dir *) m;

    dir->table = ap_pstrdup(cmd->pool, arg1);    
    if(arg2) { /* SCAN */
      if(!(ap_strcasecmp_match(arg2,"scan"))) {
        dir->flag.table_scan = 1;
        if(arg3) dir->index_scan->name = ap_pstrdup(cmd->pool, arg3);
      }
    }
    return 0;
  }
  
  
  /* "Filter column operator pseudocolumn"
     To do: How to handle "real_column IS [not] NULL" filters?
  */
  const char *filter(cmd_parms *cmd, void *m, char *base_col_name,
                     char *filter_op, char *alias_col_name) {
    bool alias_col_exists;
    short alias_col_id = -1;
    config::dir *dir = (config::dir *) m;
            
    // Create the alias column 
    alias_col_id = add_key_column(cmd, dir, alias_col_name, alias_col_exists);

    config::key_col *columns = dir->key_columns->items();

    if(alias_col_exists) return ap_psprintf(cmd->pool,
      "Filter parameter %s already defined.", alias_col_name);

    columns[alias_col_id].is.filter = 1;
    columns[alias_col_id].base_col_name = ap_pstrdup(cmd->pool, base_col_name);
    dir->flag.has_filters = 1;

    // Parse the operator
    bool found_match = 0;
    for(int n = 0 ; filter_ops[n] ; n++) {
      if(!strcmp(filter_op,filter_ops[n])) {
        columns[alias_col_id].rel_op = n;
        found_match = 1;
      }
    }
    if(!found_match)
      return ap_psprintf(cmd->pool,"invalid filter operator '%s'", filter_op);
    
    return 0;
  }


  short get_index_by_name(config::dir *dir, const char *idx) {
    config::index *indexes = dir->indexes->items();
    for(short n = 0 ; n < dir->indexes->size() ; n++) 
      if(!strcmp(idx, indexes[n].name))
        return n;
    return -1;
  }


  short build_index_record(cmd_parms *cmd, config::dir *dir, 
                           char *idxtype, const char *name) 
  {
    short index_id;
    
    config::index *index_rec = dir->indexes->new_item();
    bzero(index_rec, dir->indexes->elt_size);
    index_id = dir->indexes->size() - 1;
    index_rec->name = ap_pstrdup(cmd->pool, name);
    index_rec->type = *idxtype;                       
    index_rec->first_col_serial = -1;
    index_rec->first_col = -1;
    
    log_conf_debug(cmd->server,"Creating new index record \"%s\"", name);    
    return index_id;
  }
  
  
  /* index_constant() is called from the SQL parser. 
  */
  const char *index_constant(cmd_parms *cmd, config::dir *dir, char *idx, 
                             NSQL::Expr *in_expr) 
  {
    short index_id = get_index_by_name(dir, idx);
    assert(index_id != -1);    
    
    dir->default_key = index_id;
    config::index *index_rec = & dir->indexes->item(index_id);
    NSQL::Expr *expr = new(cmd->pool) NSQL::Expr;

    // fix me:
    if(index_rec->type =='P' || index_rec->type =='U')
      return "Sorry, you cannot compare a primary key or unique index "
             "to a constant value in mod_ndb 1.0";
    
    expr->type  = NSQL::Relation;
    expr->vtype = NSQL::Const;
    expr->base_col_name = in_expr->base_col_name;
    expr->value = (* (in_expr->value) == '"') ? 
        unquote_qstring(cmd, in_expr->value) : in_expr->value;
    expr->rel_op= in_expr->rel_op;
    
    if(index_rec->type =='P')     expr->implied_plan = PrimaryKey;
    else if(index_rec->type =='U')expr->implied_plan = UniqueIndexAccess;
    else if(index_rec->type =='O')expr->implied_plan = OrderedIndexScan;
    else assert(0);
      
    /* Linked list: */
    expr->next = index_rec->constants;
    index_rec->constants = expr;
    
    return 0;
  }
 
    
  void sort_scan(config::dir *dir, int bounded, const char *idxname, int sort_order) {
    config::index * index_rec;
    
    if(bounded) {
      short index_id = get_index_by_name(dir, idxname);
      assert(index_id != -1);
      index_rec = & dir->indexes->item(index_id);
    }
    else index_rec = dir->index_scan;

    index_rec->flag.sorted = 1;
    if(sort_order == NSQL::Desc)
      index_rec->flag.descending = 1;
  }
    
    
  /* named_index():  process Index directives.
     UniqueIndex index-name column [column ... ]
     OrderedIndex index-name column [column ... ]

     Create an index record for the index, and a key_column record 
     for each column    
  */
  const char *named_index(cmd_parms *cmd, void *m, const char *idx, char *col) {
    char *which = (char *) cmd->cmd->cmd_data;
    config::dir *dir = (config::dir *) m;
    NSQL::Expr *e = new(cmd->pool) NSQL::Expr;

    /* type-safe test: (is this void pointer really a dir?) */
    assert(dir->magic_number == 0xBABECAFE);

    if(dir->index_scan->name && ! strcmp(idx, dir->index_scan->name)) 
      return "Cannot define key columns for an ordered index scan.";

    /* Create the index */
    short index_id = get_index_by_name(dir, idx);
    if(index_id == -1)
      index_id = build_index_record(cmd, dir, which, idx);
        
    e->rel_op = NdbIndexScanOperation::BoundEQ;
    e->base_col_name = "";
    e->vtype = NSQL::Param;
    e->value = col;

    return named_idx(cmd, dir, idx, e);
  }


  const char *named_idx(cmd_parms *cmd, config::dir *dir, const char *idx, 
                        NSQL::Expr *expr) 
  {
    short index_id, col_id;
    config::index *index_rec;
    config::key_col *cols;
    short i;
    char *col = expr->value;

    index_id = get_index_by_name(dir, idx);
    assert(index_id != -1);
    index_rec = & dir->indexes->item(index_id);
    assert(index_rec);
    
    /* Sometimes a column name is not actually a column, but a flag */
    if(index_rec->type == 'O' && *col == '[') {
      if(!strcmp(col,"[ASC]")) {
        sort_scan(dir, 1, idx, NSQL::Asc);
        return 0;
      }
      else if(!strcmp(col,"[DESC]")) {
        sort_scan(dir, 1, idx, NSQL::Desc);
        return 0;
      }
      return ap_psprintf(cmd->pool,"Unrecognized sort flag: %s.", col);
    }      
    
    /* Create a column record */
    bool col_exists = 0;
    log_conf_debug(cmd->server,"Adding key column %s to index %s:%s",
                   col, dir->table, idx);
    col_id = add_column_to_index(cmd, dir, index_id, expr, col_exists);
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


  /* Compile a user-defined output format 
  */  
  const char *result_fmt_container(cmd_parms *cmd, void *m, char *args) {
    const char *pos = args;
    char line[MAX_STRING_LEN];
    output_format *fmt;
    
    const char *name = ap_getword_conf(cmd->pool, &pos);
    fmt = new(cmd->pool) output_format(name);

    /* This line must end with a closing bracket */
    if(!(ap_find_last_token(cmd->pool, pos, ">")))
       return ap_psprintf(cmd->pool, "Could not find closing '>' at top line "
                          "of result format \"%s\". (Put the format name in " 
                          "quotes?)", name);

    /* read lines */
    while(!ap_cfg_getline(line, sizeof(line), cmd->config_file)) {
      if(!strcasecmp(line,"</ResultFormat>")) break;

      pos = line;
      const char *word1 = ap_getword_conf(cmd->pool, &pos);
      const char *word2 = ap_getword_conf(cmd->pool, &pos);
      const char *word3 = ap_getword_conf(cmd->pool, &pos);
      const char *word4 = ap_getword_conf(cmd->pool, &pos);

      /* Syntax check */
      if( (! *word1) || (*word1 == '#')) continue;
      if(!strcmp(word1,"Encoding")) {
        
      }
      if((*word2 == 0) || (ap_ind(word2, '$') != -1))
        return ap_psprintf(cmd->pool,"Syntax error at \"%s\" "
                           "in result format \"%s\"", word1, name);      
      if(*word3 != '=') 
        return ap_psprintf(cmd->pool, "Expected '=' after \"%s %s\" "
                           "in result format \"%s\"", word1, word2, name);
      if(*word4 == 0) 
        return ap_psprintf(cmd->pool, "Incomplete assignment to %s %s "
                           "in result format \"%s\"", word1, word2, name);
      
      word4 = unescape(cmd->pool, word4);
      if(!strcasecmp(word1,"Format")) {
        fmt->top_node = new(cmd->pool) MainLoop(word4);
        fmt->symbol("_main", cmd->pool, fmt->top_node);
      }
      else if(!strcasecmp(word1,"Scan")) {
        fmt->symbol(word2, cmd->pool, new(cmd->pool) ScanLoop(word4));
      }
      else if(!strcasecmp(word1,"Row")) {
        fmt->symbol(word2, cmd->pool, new(cmd->pool) RowLoop(word4));
      }
      else if(!strcasecmp(word1,"Record")) {
        const char *word5 = ap_getword_conf(cmd->pool, &pos);
        const char *word6 = ap_getword_conf(cmd->pool, &pos);
        if(*word6) {
          if(strcasecmp(word5,"or")) return "Expected 'or'";
          word6 = unescape(cmd->pool, word6);
        }
        else word6 = word4;        
        fmt->symbol(word2, cmd->pool, new(cmd->pool) RecAttr(word4, word6));
      }
      else return ap_psprintf(cmd->pool,"Unknown object type \"%s\" "
                              "in result format \"%s\".", word1, name);
    }
    if(!fmt->top_node) 
      return ap_psprintf(cmd->pool,"You must define a Format object "
                         "in result format \"%s\"", name);
    
    const char *error = fmt->compile(cmd->pool);
    if(!error)
      error = register_format(cmd->pool, fmt);
    return error;
  }  


  /* As apache configuration goes, copy_sql_into_buffer() is unusual code.

     The apache 1.3 version calls ap_cfg_getc() to read a whole SQL query 
     from the actual configuration file, up to the terminating ';'.  

     The apache 2 version cannot read the file, because the file has already 
     been stored in a configuration tree, but it reads from the tree 
     starting at cmd->directive, and then resets cmd->directive->next so as
     to force apache to skip over the lines that it has already processed.
  */

  #ifdef THIS_IS_APACHE2
  const char *copy_sql_into_buffer(cmd_parms *cmd, char *args, char *&query_buff) {
    const char *buff_end = query_buff + SQL_BUFFER_LEN;
    register char *end = query_buff;
    const char *pos;
    ap_directive_t *cfline = cmd->directive;
    int more_query = 1;

    /* Copy everything from the parsed config tree into the query buffer
       up to the semicolon that terminates the query */

    while(cfline && more_query && (end < buff_end)) {
      // copy the first word on the line 
      pos = cfline->directive;
      while((*end = *pos++)) 
        if(*end++ == ';') more_query = 0;
      
      if((end < buff_end) && more_query) {        
        *end++ = ' ';         // add a space       
        // copy the rest of the line 
        pos = cfline->args;
        while((*end = *pos++)) {
          if(*end++ == ';') more_query = 0;
        }
        *end++ = '\n';
      }
      cfline = cfline->next;
    }

    *end = 0;
    if(end >= buff_end) return "N-SQL query too long (missing semicolon?).";
    
    /* Now we muck with the config tree, so that the second and subsequent lines
       of the query will not get processed again */
    cmd->directive->next = cfline;
    
    return 0;
  }
  #else  
  /* Apache 1.3 */
  const char *copy_sql_into_buffer(cmd_parms *cmd, char *args, char *&query_buff) {
    const char *buff_end = query_buff + SQL_BUFFER_LEN;
    const char *keyword = (const char *) cmd->cmd->cmd_data;
    char *pos = args;
    int end_of_query = 0;
        
    /* Copy everything from the config file into the query buffer 
       up to the semicolon that terminates the query */
       
    // First the SQL keyword 
    register char *end = ap_cpystrn(query_buff, keyword, 10);
    
    // Then the rest of the current line
    while((*end = *pos++))
      if(*end++ == ';' || end == buff_end) end_of_query = 1;
    
    // Then the remaining lines.
    if(! end_of_query) {
      register char c;
      while((c = ap_cfg_getc(cmd->config_file))) {
        *end++ = c;         
        if(c == ';' || end == buff_end) break;
      }
    }
    *end=0;
    if(end >= buff_end) return "N-SQL query too long (missing semicolon?).";
    return 0;
  }  
  #endif
  
  /* Handle an N-SQL query 
  */
  const char *sql_container(cmd_parms *cmd, void *m, char *args) {
    char *query_buff = (char *) ap_pcalloc(cmd->pool, SQL_BUFFER_LEN);
    const char *err;

    if(! cmd->path) return "N-SQL query found outside of <Location> section";
    err = copy_sql_into_buffer(cmd, args, query_buff);
    if(err) return err;
    
    log_conf_debug(cmd->server, "N-SQL query: %s", query_buff);
    
    NSQL::Scanner *scanner = new NSQL::Scanner(query_buff, SQL_BUFFER_LEN);
    NSQL::Parser  *parser  = new NSQL::Parser(scanner);
    
    parser->cmd = cmd;
    parser->dir = (config::dir *) m;
    parser->errors->http_server = cmd->server;
    assert(parser->dir->magic_number = 0xBABECAFE);

    parser->Parse();

    if(parser->errors->count) 
      return ap_psprintf(cmd->pool,"NSQL parser: %d error%s in '%s'.",
                         parser->errors->count, 
                         parser->errors->count == 1 ? "" : "s",
                         query_buff);
    /* success */
    delete parser;
    delete scanner;
    return 0;
  }

} /* end of namespace config */


extern "C" {
  command_rec configuration_commands[] =
  {
  {   /* Per-server: ndb-connectstring */
    "ndb-connectstring",          
    (CMD_HAND_TYPE) config::connectstring,
    NULL,
    RSRC_CONF,      TAKE1, 
    "NDB Connection String" 
  },
  {   // Per-server
    "ndb-max-read-subrequests",
    (CMD_HAND_TYPE) config::srv_set_int,
    NULL,
    RSRC_CONF,     TAKE1,
    "Limit to number of read subrequests in mod_ndb scripts"
  },  
  {   // Per-server
    "ndb-retry-ms",
    (CMD_HAND_TYPE) config::srv_set_int,
    NULL,
    RSRC_CONF,     TAKE1,
    "Milliseconds to spend re-trying a transaction before returning 503 error."
  },  
  {   // Per-server
    "ndb-force-restart",
    (CMD_HAND_TYPE) config::force_restart,
    NULL,
    RSRC_CONF,     FLAG,
    "Whether to force an apache graceful restart after ALTER TABLE."
  }, 
  {
    "<ResultFormat",  // Define a result format 
    (CMD_HAND_TYPE) config::result_fmt_container,
    NULL,
    RSRC_CONF | EXEC_ON_READ,      RAW_ARGS,
    "Result Format Definition"
  },
  {
    "Database",         // inheritable
    (CMD_HAND_TYPE) ap_set_string_slot,
    (void *)XtOffsetOf(config::dir, database),
    ACCESS_CONF,    TAKE1, 
    "MySQL database schema" 
  },
  {
    "Table",            // NOT inheritable
    (CMD_HAND_TYPE) config::table,
    NULL,
    ACCESS_CONF,    TAKE123, 
    "NDB Table"
  },            
  {
    "Deletes",          // NOT inheritable, defaults to 0
    (CMD_HAND_TYPE) config::dir_set_flag,
    NULL,
    ACCESS_CONF,     FLAG,
    "Allow DELETE over HTTP"
  },
  {
    "ETags",          // Inheritable, defaults to 1
    (CMD_HAND_TYPE) config::dir_set_flag,
    NULL,
    ACCESS_CONF,     FLAG,
    "Compute and set ETag header in response"
  },    
  {
    "Format",           // inheritable
    (CMD_HAND_TYPE) config::result_format,
    NULL,
    ACCESS_CONF,    TAKE1, 
    "Result Set Format"
  },     
  {
    "Columns",          // NOT inheritable
    (CMD_HAND_TYPE) config::non_key_column,
    (void *) "R",
    ACCESS_CONF,    ITERATE,
    "List of attributes to include in result set"
  },
  {
    "AllowUpdate",      // NOT inheritable
    (CMD_HAND_TYPE) config::non_key_column,
    (void *) "W",
    ACCESS_CONF,    ITERATE,
    "List of attributes that can be updated using HTTP"
  },
  {
    "PrimaryKey",      // NOT inheritable
    (CMD_HAND_TYPE) config::primary_key,
    (void *) "P",  
    ACCESS_CONF,    ITERATE,
    "Allow Primary Key lookups"
  },
  {
    "UniqueIndex",      // NOT inheritable
    (CMD_HAND_TYPE) config::named_index,
    (void *) "U",  
    ACCESS_CONF,    ITERATE2,
    "Allow unique key lookups, given index name and columns"
  },
  {
    "OrderedIndex",      // NOT inheritable
    (CMD_HAND_TYPE) config::named_index,
    (void *) "O",  
    ACCESS_CONF,    ITERATE2,
    "Allow ordered index scan, given index name and columns"
  },
  {                      // inheritable
    "PathInfo", // e.g. "PathInfo id" or "PathInfo keypt1/keypt2"
    (CMD_HAND_TYPE) config::pathinfo,  
    NULL,
    ACCESS_CONF,    TAKE12, 
    "Schema for interpreting the rightmost URL path components"
  },
  {                     // NOT inheritable
    "Filter",  // Filter col op keyword, e.g. "Filter id < min_id"
    (CMD_HAND_TYPE) config::filter, 
    NULL,
    ACCESS_CONF,    TAKE3, 
    "Result Filter"
  }, 
  { 
    "SELECT",  // N-SQL statement
    (CMD_HAND_TYPE) config::sql_container,
    (void *) "SELECT ",
    ACCESS_CONF ,      RAW_ARGS,
    "N-SQL SELECT Query"    
  },
  { 
    "DELETE",  // N-SQL statement
    (CMD_HAND_TYPE) config::sql_container,
    (void *) "DELETE ",
    ACCESS_CONF,      RAW_ARGS,
    "N-SQL DELETE Query"    
  },    
  { 
    "WHERE",  // N-SQL statement
    (CMD_HAND_TYPE) config::sql_container,
    (void *) "WHERE ",
    ACCESS_CONF,      RAW_ARGS,
    "N-SQL one-row query plan"    
  },    
  { 
    "USING",  // N-SQL statement
    (CMD_HAND_TYPE) config::sql_container,
    (void *) "USING ",
    ACCESS_CONF,      RAW_ARGS,
    "N-SQL query plan"    
  },     
  {NULL, NULL, NULL, 0, cmd_how(0), NULL}
  };
}
