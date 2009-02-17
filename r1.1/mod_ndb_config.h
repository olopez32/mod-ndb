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

#define SQL_BUFFER_LEN  MAX_STRING_LEN*2

typedef const char *(*CMD_HAND_TYPE) ();

class output_format;  // Forward declaration

namespace NSQL {
  enum { Relation = 1 };               // expression types
  enum { Param = 1, Const, Column };   // value types
  enum { Asc = 1, Desc };              // sort orders
  class Expr;   // forward declaration
};


class base_expr {
 public:
  char *name;
  char *base_col_name;
  int rel_op;
  AccessPlan implied_plan;
  short index_id;
};


/* Coulmn used in a query */
class config::key_col : public base_expr {
 public:
  short serial_no;
  short idx_map_bucket;
  short next_in_key_serial;
  short next_in_key; 
  struct {
    unsigned int in_pk       : 1;
    unsigned int filter      : 1;
    unsigned int in_ord_idx  : 1;
    unsigned int in_hash_idx : 1;
    unsigned int in_pathinfo : 1;
  } is;
};


class NSQL::Expr : public base_expr {
 public:
  short type;
  short vtype;
  char *value;
  apache_array<NSQL::Expr> *args;
  NSQL::Expr *next;
  
  void * operator new(size_t sz, ap_pool *p) {
    return ap_pcalloc(p, sz);
  };
};  


namespace config {
  
  /* Apache per-server configuration  */
  struct srv {
    char *connect_string;
    int max_read_operations;
    unsigned int max_retry_ms;
    unsigned int force_restart;
    unsigned int magic_number;
  };
    
  /* Apache per-directory configuration */
  struct dir {
    char *path;
    char *database;
    char *table;
    int pathinfo_size;
    short *pathinfo;
    output_format *fmt;
    int incr_prefetch;
    short default_key;
    struct {
      unsigned pathinfo_always  : 1;
      unsigned has_filters      : 1;
      unsigned table_scan       : 1;
      unsigned use_etags        : 1;
      unsigned allow_delete     : 1;
      unsigned select_star      : 1;
      unsigned is_resolved      : 1; // basic optimizer has run 
    } flag;
    struct index *index_scan;
    apache_array<char*> *visible;
    apache_array<char*> *updatable;
    apache_array<char*> *aliases;
    apache_array<config::index> *indexes;
    apache_array<config::key_col> *key_columns;
    unsigned int magic_number;
  };
  
  /* NDB Index */
  struct index {
    char *name;
    unsigned short n_columns;
    short first_col_serial;
    short first_col;
    char type;
    struct {
      unsigned sorted     : 1;
      unsigned descending : 1;
    } flag;
    NSQL::Expr *constants;
  };
  
  void * init_dir(ap_pool *, char *);
  void * init_srv(ap_pool *, server_rec *);
  void * merge_dir(ap_pool *, void *, void *);
  void * merge_srv(ap_pool *, void *, void *);
  void   sort_scan(config::dir *, int, const char *, int);
  const char * non_key_column(cmd_parms *, void *, char *);
  const char * named_index(cmd_parms *, void *, const char *, char *);
  const char * named_idx(cmd_parms *, config::dir *, const char *, NSQL::Expr *);
  const char * result_format(cmd_parms *, void *, char *);
  const char * pathinfo(cmd_parms *, void *, char *, char *);
  const char * table(cmd_parms *, void *, char *, char *, char *);
  const char * filter(cmd_parms *, void *, char *, char *, char *);
  const char * primary_key(cmd_parms *, void *, char *);
  const char * connectstring(cmd_parms *, void *, char *);
  const char * maxreadsubrequests(cmd_parms *, void *, char *);
  const char * result_fmt_container(cmd_parms *, void *, char *);
  const char * sql_container(cmd_parms *, void *, char *);
  const char * index_constant(cmd_parms*,config::dir*, char *, NSQL::Expr *);
  short get_index_by_name(config::dir *, const char *);
  short build_index_record(cmd_parms*,config::dir*, char *, const char*);
}
