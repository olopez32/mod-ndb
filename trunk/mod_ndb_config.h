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


namespace config {
  
  /* Apache per-server configuration  */
  struct srv {
    char *connect_string;
  };
  
  
  /* Apache per-directory configuration */
  struct dir {
    char *database;
    char *table;
    int allow_delete;
    result_format results;
    char *format_param[2];
    array_header *visible;
    array_header *updatable;
    array_header *pathinfo;
    array_header *indexes;
    array_header *key_columns;      
    void *index_map;
  };
  
  /* NDB Index */
  class index {
    public:
      char *name;
      ndb_key_type type;
      unsigned short n_columns;
      short first_col_serial;
      short first_col_idx;
  };
  
  /* Coulmn used in a query */
  class key {
    public:
      char *name;
      bool is_filter;
      NdbScanFilter::BinaryCondition filter_op;
      short index_id;
      short serial_no;
      short idx_map_bucket;
      short filter_col_serial;
      short filter_col_idx;
      short next_in_key_serial;
      short next_in_key_idx;      
  };
  
  void * init_dir(pool *, char *);
  void * init_srv(pool *, server_rec *);
  void * merge_dir(pool *, void *, void *);
  const char * build_column_list(cmd_parms *, void *, char *);
  const char * build_index_list(cmd_parms *, void *, char *, char *);
  const char * set_result_format(cmd_parms *, void *, char *, char *, char *);
  const char * pathinfo(cmd_parms *, void *, char *);
  const char * filter(cmd_parms *, void *, char *, char *, char *);
}

