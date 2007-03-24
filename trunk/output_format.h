/* Copyright (C) 2007 MySQL AB

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

enum re_type { const_string, item_name, item_value };
enum re_esc  { no_esc, esc_xml, esc_json };
enum re_quot { no_quot, quote_char, quote_all };

class row_element {
  public:
  re_type elem_type ;
  re_quot elem_quote ;
  char *string;
  const char **escapes;
  size_t len;
  row_element *next;

  row_element(re_type, re_esc, re_quot);

  row_element(char *txt) {
    elem_type = const_string;
    string = txt;
    len = strlen(txt);
    next = 0;
  };

  void * operator new(size_t sz, ap_pool *p) {
    return ap_pcalloc(p, sz);
  };
};


class len_string {
public:
  size_t len;
  const char *string;
  
  len_string() {};

  len_string(const char *str) {
    string = str;
    len = strlen(str);
  };
    
  void * operator new(size_t sz, ap_pool *p) {
    return ap_pcalloc(p, sz);
  };
};

class output_format {
public:  
  const char *name;
  len_string begin_page;
  len_string begin_scan;
  len_string mid_scan;
  len_string end_scan;
  len_string begin_row;
  len_string mid_row;
  len_string end_row;
  len_string end_page;  
  struct row_element *row_elements;
  struct row_element *null_elements;
  struct {
    unsigned int is_internal  : 1;
    unsigned int can_override : 1;
    unsigned int is_raw       : 1;
  } flag;

  void * operator new(size_t sz, ap_pool *p) {
    return ap_pcalloc(p, sz);
  };
  
  void set_scan_parts(char *a, char *b, char *c) {
    begin_scan = len_string(a);
    mid_scan   = len_string(b);
    end_scan   = len_string(c);
  };
    
  void set_row_parts(char *a, char *b, char *c) {
    begin_row = len_string(a);
    mid_row   = len_string(b);
    end_row   = len_string(c);
  };  
};

typedef struct output_format output_format;



/* 
Notes:

There needs to be a global table of output formats,
 when we create the table, we initialize the "built in" formats (like "raw").
 In apache 1.3, this should happen in the module initializer.
 In apache 2, it happens in the pre_config hook.
 
 In an apache 1.3 table, the key and value are both strings. 
 In apache 2, you can use an APR hash table to point directly to a structure.
 
 Because of this, we'll put the function that maps a format name to an
 output_format struct inside the version-specific source file.  The global
 format table pointer itself will be declared in that file.  In all, we'll 
 have the following functions there:
 
 void initialize_output_formats(ap_pool *p);
 char *register_format(char *name, output_format &format);
 output_format *get_format_by_name(char *name);
 
 get_format_by_name() is only ever called in processing configs -- not at
 runtime.
 
 
 Here is an example pre_config hook from mod_headers:
 556 static int header_pre_config(apr_pool_t *p, apr_pool_t *plog, apr_pool_t *ptemp)
 557 {
 558     format_tag_hash = apr_hash_make(p);
 559     register_format_tag_handler(p, "D", (void*) header_request_duration, 0);
 560     register_format_tag_handler(p, "t", (void*) header_request_time, 0);
 561     register_format_tag_handler(p, "e", (void*) header_request_env_var, 0);
 562 
 563     return OK;
 564 } 
 
*/
