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


typedef struct {
  unsigned int len;
  char *string;
} len_string;

struct row_element {
  enum { const_string , item_name , item_value } row_elem_type ;
  enum { esc_none, esc_xml , esc_json } row_elem_escape ;
  enum { quote_none , quote_char , quote_all } row_elem_quote ;
  len_string text;
  struct row_element *next;
};

struct {
  len_string begin_page;
  len_string begin_scan;
  len_string mid_scan;
  len_string end_scan;
  len_string end_page;  
  struct row_element *row_elements;
} output_format;

typedef struct output_format result_format;
