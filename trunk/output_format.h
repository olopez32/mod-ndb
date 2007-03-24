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
