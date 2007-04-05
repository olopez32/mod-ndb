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


/* Globals */
const char *escape_leaning_toothpicks[256];
const char *escape_xml_entities[256];
table *global_format_names = 0;
apache_array<struct output_format *> *global_output_formats = 0;

extern Node the_null_node;  /* from format_compiler.cc */


void initialize_output_formats(ap_pool *p) {
  global_format_names = ap_make_table(p, 6);
  global_output_formats = new(p,6) apache_array<output_format *>;
  assert(global_format_names);
  assert(global_output_formats);
  return;
}


void initialize_escapes() {
  for(int i = 0 ; i < 256 ; i++) {
    escape_leaning_toothpicks[i] = 0;
    escape_xml_entities[i] = 0;
  }
  
  escape_leaning_toothpicks[static_cast<int>('\\')] = "\x02" "\\" "\\";
  escape_leaning_toothpicks[static_cast<int>('\"')] = "\x02" "\\" "\"";
  escape_leaning_toothpicks[static_cast<int>('\b')] = "\x02" "\\" "b";
  escape_leaning_toothpicks[static_cast<int>('\f')] = "\x02" "\\" "f";
  escape_leaning_toothpicks[static_cast<int>('\n')] = "\x02" "\\" "n";
  escape_leaning_toothpicks[static_cast<int>('\r')] = "\x02" "\\" "r";
  escape_leaning_toothpicks[static_cast<int>('\t')] = "\x02" "\\" "t";
  
  escape_xml_entities[static_cast<int>('<')] = "\x04" "&lt;";
  escape_xml_entities[static_cast<int>('>')] = "\x04" "&gt;";
  escape_xml_entities[static_cast<int>('&')] = "\x05" "&amp;";
  escape_xml_entities[static_cast<int>('\"')]= "\x06" "&quot;";
}


const char **get_escapes(re_esc esc) {
  if(esc == esc_xml) return escape_xml_entities;
  else if(esc == esc_json) return escape_leaning_toothpicks;
  return 0;  
}


char *escape_string(ap_pool *pool, const char **escapes, len_string &str) {  
  size_t escaped_size = 0;
  register const char *esc;
  
  /* How long will the string be when it is escaped? */
  for(unsigned int i = 0; i < str.len ; i++) {
    const unsigned char c = str.string[i];
    esc = escapes[c];
    if(esc) escaped_size += esc[0];
    else escaped_size++;
  }
  
  char *out = (char *) ap_pcalloc(pool, escaped_size);
  char *p = out;
  for(unsigned int i = 0; i < str.len ; i++) {
    const unsigned char c = str.string[i];
    esc = escapes[c];
    if(esc) 
      for(char j = 1 ; j <= esc[0]; j++) *p++ = esc[j];
    else 
      *p++ = c;
  }
  return out;
}


char *json_str(ap_pool *pool, len_string &str) {
  return escape_string(pool, escape_leaning_toothpicks, str);
}


output_format *get_format_by_name(char *name) {
  const char *format_index = ap_table_get(global_format_names, name);
  if(format_index) {
    int idx = atoi(format_index);
    return global_output_formats->item(idx);
  }
  return 0;
}


char *register_format(output_format *format) {  
  char idx_string[32];
  output_format *existing = get_format_by_name((char *) format->name);
  if(existing && ! existing->flag.can_override)
    return "Cannot redefine existing format";      
  
  /* Get the index value for the new format */
  int nformats = global_output_formats->size();
  sprintf(idx_string,"%d",nformats);
  
  /* Store the pointer to the new format */
  * global_output_formats->new_item() = format;
  
  /* Store the index in the table of format names */
  ap_table_set(global_format_names, format->name, idx_string);
  
  return 0;
}


void register_built_in_formatters(ap_pool *p) {
  output_format *json_format = new(p) output_format("JSON");
  output_format *raw_format  = new(p) output_format("raw");
  output_format *xml_format  = new(p) output_format("XML");
  
  /* Define the raw format */
  raw_format->flag.is_internal = 1;
  raw_format->flag.is_raw = 1;
  raw_format->top_node = &the_null_node;
  
  /* Define the internal JSON format */
  json_format->flag.is_internal  = 1;
  json_format->flag.can_override = 1;
  
  ScanLoop *Scan = new(p) ScanLoop("[\n $row$,\n ... \n]\n");
  json_format->symbol("scan", p, Scan);
  json_format->symbol("row",  p, new(p) RowLoop(" { $item$ , ... }"));
  json_format->symbol("item", p, new(p) RecAttr("$name/Q$:$value/qj$","$name/Q$:null"));
  json_format->top_node = Scan;
  const char *err = json_format->compile(p);
  if(err) {
    fprintf(stderr,err);
    exit(1);
  }
  
  /* Define the internal XML format */
  xml_format->flag.is_internal  = 1;
  xml_format->flag.can_override = 1; 

  Scan = new (p) ScanLoop("<NDBScan>\n$row$\n...\n</NDBScan>\n");   
  xml_format->symbol("scan", p, Scan);
  xml_format->symbol("row",  p, new(p) RowLoop(" <NDBTuple> $attr$ \n  ...  </NDBTuple>"));
  xml_format->symbol("attr", p, new(p) RecAttr("<Attr name=$name/Q$ value=$value/Qx$ />",
                                               "<Attr name=$name/Q$ isNull=\"1\" />"));
  xml_format->top_node = Scan;
  err = xml_format->compile(p);
  if(err) {
    fprintf(stderr,err);
    exit(1);
  }
  
  
  register_format(raw_format);
  register_format(json_format);
  register_format(xml_format);
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
  output_format *fmt = data->fmt;
  
  if(fmt->flag.is_raw) return Results_raw(r, data, res);
  res.init(r, 8192);
  for(Node *N = fmt->top_node; N != 0 ; N=N->next_node)
    N->Run(data, res);

  res.out("\n"); // should be a node after the scan
  return OK;
}
 

char *Cell::dump(ap_pool *p, int indent) {
  int n = 0;
  char *out = ap_pstrdup(p, "[");
  for(Cell *c = this ; c != 0 ; c = c->next) {
    if(n++) out = ap_pstrcat(p, out, " , ", 0);
    switch(c->elem_type) {
      case const_string: 
        const char *val = escape_string(p, escape_leaning_toothpicks, *c);
        out = ap_pstrcat(p, out, "\"", val, "\"", 0);
        break;
      case item_name :
        if(c->elem_quote == quote_char) out = ap_pstrcat(p, out, "\"$name/q$\"", 0);
        else if (c->elem_quote == quote_all) out = ap_pstrcat(p, out, "\"$name/Q$\"", 0);
        else out = ap_pstrcat(p, out, "\"$name$\"", 0);
        break;
      case item_value:
        {
          char flags[4] = { 0, 0, 0, 0 };
          int f = 1;
          char *item;
          if(c->escapes || ( c->elem_quote != no_quot)) {
            flags[0] = '/';
            if(c->escapes == escape_leaning_toothpicks) flags[f++] = 'j';
            else if(c->escapes == escape_xml_entities)  flags[f++] = 'x';
            if(c->elem_quote == quote_char)             flags[f++] = 'q';
            else if(c->elem_quote == quote_all)         flags[f++] = 'Q';
          }
          if(c->i > 0) item = ap_psprintf(p, "$%d", c->i);
          else item = "$value";
          out = ap_pstrcat(p, out, "\"", item, flags, "$\"", 0);
        }
        break;
      default:
        out = ap_pstrcat(p, out, "\"*HOW_DO_I_DUMP_THIS_KIND_OF_CELL*\"", 0);
    }
  }
  out = ap_pstrcat(p, out, "]", 0);
  return out;
}

void Cell::out(struct data_operation *data, result_buffer &res) {
  if(elem_type == const_string) 
    return this->out(res);
  const NdbRecAttr &rec = *data->result_cols[i]; 
  this->out(rec, res);
  return;
}

void Cell::out(const NdbRecAttr &rec, result_buffer &res) {
  if(elem_type == const_string) {
    res.out(len, string);
    return;
  }
  
  const char *col_name = rec.getColumn()->getName();
  NdbDictionary::Column::Type col_type = rec.getColumn()->getType();
  switch(elem_type) {
    case item_name:
      if(elem_quote == no_quot) 
        res.out(strlen(col_name), col_name);
      else
        res.out("\"%s\"",col_name);
      break;
    case item_value:          
      if((elem_quote == quote_all) || 
         ((elem_quote == quote_char)
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
        MySQL::result(res, rec, escapes);
        res.out(1,"\"");            
      }
      else MySQL::result(res, rec, escapes);  /* No Quotes */
      break;
    default:
      assert(0);      
  } /* end of switch statement */      
}


void RecAttr::out(const NdbRecAttr &rec, result_buffer &res) {
  for( Cell *c = rec.isNULL() ? null_fmt : fmt; c != 0 ; c=c->next) 
    c->out(rec, res);
}


void ScanLoop::Run(data_operation *data, result_buffer &res) {
  int nrows = 0;
  
  if(data->scanop) {
    while((data->scanop->nextResult(true)) == 0) {
      do {
        if(nrows++) res.out(*sep);
        else begin->chain_out(res);
        core->Run(data, res);
      } while((data->scanop->nextResult(false)) == 0);
      if(nrows) end->chain_out(res);
      else return; // ?? used to be return HTTP_GONE
    }
  }
  else {  /* not a scan, just a single result row */
    core->Run(data, res);
  }
}


void RowLoop::Run(data_operation *data, result_buffer &res) {
  begin->chain_out(data, res);
  for(unsigned int n = 0; n < data->n_result_cols ; n++) {
    if(n) res.out(*sep);
    const NdbRecAttr &rec = *data->result_cols[n];
    core->out(rec, res);
  }
  end->chain_out(data, res);
}

