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

#include "mod_ndb.h"

void build_result_row(output_format *, data_operation *, result_buffer &);

/* Globals */
const char *escape_leaning_toothpicks[128];
const char *escape_xml_entities[128];
table *global_format_names = 0;
apache_array<struct output_format *> *global_output_formats = 0;


row_element::row_element(re_type type, re_esc esc, re_quot quote) {
  elem_type   = type;
  elem_quote  = quote;
  if(esc == esc_xml) 
    escapes = escape_xml_entities;
  else if(esc == esc_json) 
    escapes = escape_leaning_toothpicks;
  else
    escapes = 0;
  next = 0;
};


void initialize_output_formats(ap_pool *p) {
  global_format_names = ap_make_table(p, 6);
  global_output_formats = new(p,6) apache_array<output_format *>;
  assert(global_format_names);
  assert(global_output_formats);
  return;
}


output_format *get_format_by_name(char *name) {
  const char *format_index = ap_table_get(global_format_names, name);
  if(format_index) {
    int idx = atoi(format_index);
    return global_output_formats->item(idx);
  }
  return 0;
}


char *register_format(char *name, output_format *format) {  
  char idx_string[32];
  output_format *existing = get_format_by_name(name);
  if(existing && ! existing->flag.can_override)
    return "Cannot redefine existing format";      
  
  /* Get the index value for the new format */
  int nformats = global_output_formats->size();
  sprintf(idx_string,"%d",nformats);
  
  /* Store the pointer to the new format */
  * global_output_formats->new_item() = format;
  
  /* Store the index in the table of format names */
  ap_table_set(global_format_names, name, idx_string);
  
  format->name = name;
  return 0;
}


void initialize_escapes() {
  for(int i = 0 ; i < 128 ; i++) {
    escape_leaning_toothpicks[i] = 0;
    escape_xml_entities[i] = 0;
  }
  
  escape_leaning_toothpicks[static_cast<int>('\\')] = "\x02" "\\" "\\";
  escape_leaning_toothpicks[static_cast<int>('\"')] = "\x02" "\\" "\"";
  escape_leaning_toothpicks[static_cast<int>('\b')] = "\x02" "\\" "\b";
  escape_leaning_toothpicks[static_cast<int>('\f')] = "\x02" "\\" "\f";
  escape_leaning_toothpicks[static_cast<int>('\n')] = "\x02" "\\" "\n";
  escape_leaning_toothpicks[static_cast<int>('\r')] = "\x02" "\\" "\r";
  escape_leaning_toothpicks[static_cast<int>('\t')] = "\x02" "\\" "\t";

  escape_xml_entities[static_cast<int>('<')] = "\x04" "&lt;";
  escape_xml_entities[static_cast<int>('>')] = "\x04" "&gt;";
  escape_xml_entities[static_cast<int>('&')] = "\x05" "&amp;";
  escape_xml_entities[static_cast<int>('\"')]= "\x06" "&quot;";
}


void register_built_in_formatters(ap_pool *p) {
  struct row_element *my_item[13];
  struct output_format *json_format = new(p) output_format;
  struct output_format *raw_format  = new(p) output_format;
  struct output_format *xml_format  = new(p) output_format;
  
  /* Define the raw format (simply by setting some flags) */
  raw_format->flag.is_internal = 1;
  raw_format->flag.is_raw = 1;
  
  /* Define the internal JSON format */
  json_format->flag.is_internal  = 1;
  json_format->flag.can_override = 1;

  json_format->set_scan_parts("[\n", ",\n", "\n]\n");  // JSON array

  json_format->set_row_parts(" { ", " , ", " }");    // JSON object
  my_item[0] = new(p) row_element(item_name, no_esc, quote_all);
  my_item[1] = new(p) row_element(":");
  my_item[2] = new(p) row_element(item_value, esc_json, quote_char);
  json_format->row_elements = my_item[0];
  my_item[0]->next = my_item[1];
  my_item[1]->next = my_item[2];
  
  my_item[3] = new(p) row_element(item_name, no_esc, quote_all);
  my_item[4] = new(p) row_element(":null");
  json_format->null_elements = my_item[3];
  my_item[3]->next = my_item[4];
  
  /* Define the internal XML format */
  xml_format->flag.is_internal  = 1;
  xml_format->flag.can_override = 1; 
   
  xml_format->set_scan_parts("<NDBScan>\n", "\n", "\n</NDBScan>\n");
  xml_format->set_row_parts(" <NDBTuple> ", "\n  ", " </NDBTuple>");  
  my_item[5] = new(p) row_element("<Attr name=");
  my_item[6] = new(p) row_element(item_name, no_esc, quote_all);
  my_item[7] = new(p) row_element(" value=");
  my_item[8] = new(p) row_element(item_value, no_esc, quote_all);
  my_item[9] = new(p) row_element(" />");
  xml_format->row_elements = my_item[5];
  for(int i = 5; i < 9 ; i++) my_item[i]->next = my_item[i+1];

  my_item[10] = new(p) row_element("<Attr name=");
  my_item[11] = new(p) row_element(item_name, no_esc, quote_all);
  my_item[12] = new(p) row_element("isNull=\"1\" />");
  xml_format->null_elements = my_item[10];
  my_item[10]->next = my_item[11];
  my_item[11]->next = my_item[12];
  
  register_format("raw",  raw_format);
  register_format("JSON", json_format);
  register_format("XML",  xml_format);
}


int Results_raw(request_rec *r, data_operation *data, 
                result_buffer &res) {
  unsigned long long size64 = 0;
  unsigned int size;
  
  if(data->blob) {
    data->blob->getLength(size64);  //passed by reference
    size = (unsigned int) size64;
    res.init(r, size);
    if(data->blob->readData(res.buff, size)) 
      log_debug(r->server,"Error reading blob data: %s",
                data->blob->getNdbError().message);
    res.sz = size;
  }
  return OK;
}


int build_results(request_rec *r, data_operation *data, result_buffer &res) {
  int nrows = 0;
  output_format *fmt = data->fmt;
  
  if(fmt->flag.is_raw) return Results_raw(r, data, res);
  
  res.init(r, 8192);
  if(data->scanop) {
    while((data->scanop->nextResult(true)) == 0) {
      do {
        res.out(nrows++ ? fmt->mid_scan : fmt->begin_scan);
        if(nrows++) res.out(fmt->begin_scan);
        build_result_row(fmt, data, res);
      } while((data->scanop->nextResult(false)) == 0);
      if(nrows) res.out(fmt->end_scan);
      else return HTTP_GONE; // ??
    }
  }
  else {  /* not a scan, just a single result row */
    build_result_row(fmt, data, res);
  }
  return OK;
}


void build_result_row(output_format *fmt, data_operation *data, result_buffer &res) {
  res.out(fmt->begin_row);
  for(unsigned int n = 0; n < data->n_result_cols ; n++) {
    const NdbRecAttr &rec = *data->result_cols[n];

    const char *col_name = rec.getColumn()->getName();
    NdbDictionary::Column::Type col_type = rec.getColumn()->getType();
    row_element *item = rec.isNULL() ? fmt->null_elements : fmt->row_elements;
    
    if(n) res.out(fmt->mid_row);
    
    for( ; item != 0 ; item=item->next) {
      switch(item->elem_type) {
        case const_string :
          res.out(item->len, item->string);        
          break;
        case item_name:
          if(item->elem_quote == no_quot) 
            res.out(strlen(col_name), col_name);
          else
            res.out("\"%s\"",col_name);
          break;
        case item_value:          
          if((item->elem_quote == quote_all) ||
             ((item->elem_quote == quote_char)
               && (col_type == NdbDictionary::Column::Char        ||
                   col_type == NdbDictionary::Column::Varchar     ||
                   col_type == NdbDictionary::Column::Longvarchar ||
                   col_type == NdbDictionary::Column::Date        ||
                   col_type == NdbDictionary::Column::Time        ||
                   col_type == NdbDictionary::Column::Datetime    ||
                   col_type == NdbDictionary::Column::Text 
          ))) {
            /* Quoted Value */
              res.out(1,"\"");
              MySQL::result(res, rec, item->escapes);
              res.out(1,"\"");            
          }
          else MySQL::result(res, rec, item->escapes);  /* No Quotes */
          break;
        default:
          assert(0);      
      } /* end of switch statement */      
    } /* loop over list of row_elements in the output format */
  } /* loop over columns in the row */
  
  res.out(fmt->end_row);
}

