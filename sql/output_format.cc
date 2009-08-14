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

#include "mod_ndb.h"


/* Globals */
const char *escape_leaning_toothpicks[256];
const char *escape_xml_entities[256];
const char *escape_xml_plus_json[256];

apr_table_t *global_format_names = 0;
apache_array<struct output_format *> *global_output_formats = 0;

extern Node the_null_node;  /* from format_compiler.cc */


void initialize_output_formats(ap_pool *p) {
  global_format_names = ap_make_table(p, 6);
  global_output_formats = new(p,6) apache_array<output_format *>;
  assert(global_format_names);
  assert(global_output_formats);
  return;
}


void initialize_escapes(ap_pool *p) {
  int i;
  
  for(i = 0 ; i < 256 ; i++) {
    escape_leaning_toothpicks[i] = 0;     /* JSON escapes */
    escape_xml_entities[i] = 0;           /* XML escapes */
   }

  /* Escape sequences are stored with a leading length byte  */
  /* RFC 4627: control characters 00 - 1f must be escaped */
  for(i = 0 ; i < 32 ; i++)
    escape_leaning_toothpicks[i] = ap_psprintf(p, "\x06" "\\u%04x", i);
  
  /* www.json.org : these characters are also escaped */
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


  /* The combined JSON-on-XML encoding uses the four XML escapes, 
     plus all of the JSON escapes except the quote.  It's sufficient
     to say it's a combined set, with the XMLs taking precedence. 
  */
  
  for(i = 0 ; i < 256 ; i++) {
    escape_xml_plus_json[i] = escape_xml_entities[i] ? 
      escape_xml_entities[i] : escape_leaning_toothpicks[i] ;
  }

}


const char **get_escapes(re_esc esc) {
  if(esc == esc_xml) return escape_xml_entities;
  else if(esc == esc_json) return escape_leaning_toothpicks;
  else if(esc == esc_xmljson) return escape_xml_plus_json;
  return 0;  
}


output_format *get_format_by_name(const char *name) {
  const char *format_index = ap_table_get(global_format_names, name);
  if(format_index) {
    int idx = atoi(format_index);
    return global_output_formats->item(idx);
  }
  return 0;
}


char *register_format(ap_pool *pool, output_format *format) {  
  char idx_string[32];
  output_format *existing = get_format_by_name((char *) format->name);
  if(existing && ! existing->flag.can_override) {
    return ap_psprintf(pool,"Output format \"%s\" already exists %s"
                       "and cannot be overriden.", format->name,
                       existing->flag.is_internal ? 
                       "as an internal format " : "");
  }
  
  /* Get the index value for the new format */
  int nformats = global_output_formats->size();
  sprintf(idx_string,"%d",nformats);
  
  /* Store the pointer to the new format */
  * global_output_formats->new_item() = format;
  
  /* Store the index in the table of format names */
  /* If the entry exists, its value is replaced. */
  ap_table_set(global_format_names, format->name, idx_string);
  
  return 0;
}


void register_built_in_formatters(ap_pool *p) {
  output_format *json_format = new(p) output_format("JSON");
  output_format *raw_format  = new(p) output_format("raw");
  output_format *xml_format  = new(p) output_format("XML");
  const char *err;
  
  /* Define the raw format */
  raw_format->flag.is_internal = 1;
  raw_format->flag.is_raw = 1;  
  
  /* Define the internal JSON format */
  json_format->flag.is_internal  = 1;
  json_format->flag.can_override = 1;
  json_format->flag.is_JSON      = 1;
  
  MainLoop *Main = new(p) MainLoop("$scan$\n");
  json_format->symbol("_main", p, Main);
  json_format->symbol("scan", p, new(p) ScanLoop("[\n $row$,\n ... \n]"));
  json_format->symbol("row",  p, new(p) RowLoop(" { $item$ , ... }"));
  json_format->symbol("item", p, new(p) RecAttr(
                                       "$name/Q$:$value/qj$","$name/Q$:null"));
  json_format->top_node = Main;
  err = json_format->compile(p);
  if(err) {
    fprintf(stderr,err);
    exit(1);
  }
  
  /* Define the internal XML format */
  xml_format->flag.is_internal  = 1;
  xml_format->flag.can_override = 1; 

  Main = new (p) MainLoop("$scan$\n");
  xml_format->symbol("_main", p, Main);
  xml_format->symbol("scan", p, new(p) ScanLoop("<NDBScan>\n$row$\n...\n</NDBScan>"));
  xml_format->symbol("row",  p, new(p) RowLoop(" <NDBTuple> $attr$ \n  ...  </NDBTuple>"));
  xml_format->symbol("attr", p, new(p) RecAttr(
                                    "<Attr name=$name/Q$ value=$value/Qx$ />",
                                    "<Attr name=$name/Q$ isNull=\"1\" />"));
  xml_format->top_node = Main;
  err = xml_format->compile(p);
  if(err) {
    fprintf(stderr,err);
    exit(1);
  }  
  
  register_format(p, raw_format);
  register_format(p, json_format);
  register_format(p, xml_format);
}


/* Results_raw() simply uses the blob's internal buffer as the output buffer.
*/
int Results_raw(request_rec *r, data_operation *data, 
                result_buffer &res) {
  const MySQL::result *result = data->result_cols[0];  

  if(result && result->contents) 
    res.overlay(result->contents);  
  else {
    log_err(r->server, "Cannot use raw output format at %s", r->uri);
    return 500;
  }
  return OK;
}


int build_results(request_rec *r, data_operation *data, result_buffer &res) {
  output_format *fmt = data->fmt;
  int result_code;
  
  if(fmt->flag.is_raw) return Results_raw(r, data, res);
  res.init(r, 8192);
  for(Node *N = fmt->top_node; N != 0 ; N=N->next_node) {
    result_code = N->Run(data, res);
    if(result_code != OK) return result_code;
  }
  return OK;
}
 

void Cell::out(struct data_operation *data, result_buffer &res) {
  if(elem_type == const_string) 
    return this->out(res);
  if(i > data->n_result_cols) return; //error?
  this->out(data, i-1, res);
  return;
}


void Cell::out(data_operation *data, unsigned int n, result_buffer &res) {
  if(elem_type == const_string) {
    res.out(len, string);
    return;
  }

  MySQL::result *result = data->result_cols[n];  
  const char *col_name = data->flag.select_star ? 
    result->getColumn()->getName() : data->aliases[n] ;
    
  NdbDictionary::Column::Type col_type = result->getColumn()->getType();
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
        result->out(res, escapes);
        res.out(1,"\"");            
      }
      else result->out(res, escapes);  /* No Quotes */
      break;
    default:
      assert(0);      
  } /* end of switch statement */      
}


void RecAttr::out(data_operation *data, unsigned int n, result_buffer &res) {
  Cell *c = data->result_cols[n]->isNull() ?  null_fmt : fmt;
  for( ; c != 0 ; c=c->next) 
    c->out(data, n, res);
}


int MainLoop::Run(data_operation *data, result_buffer &res) {
  int status = OK;
  if(begin) begin->out(res);
  if(core)  status = core->Run(data, res);
  if(end)   end->out(res);
  return status;
}


int ScanLoop::Run(data_operation *data, result_buffer &res) {
  register int nrows = 0;
  
  if(data->scanop) {
    /* nextResult(true) fetches rows from NDB into cache ;
       nextResult(false) uses rows already cached */ 
    while((data->scanop->nextResult(true)) == 0) {
      do {
        if(nrows++) res.out(*sep);    /* comma */
          else begin->chain_out(res); /* open bracket */
        core->Run(data, res);
      } while((data->scanop->nextResult(false)) == 0);      
    }
    if(nrows) end->chain_out(res);   /* close bracket */
    return (nrows ? OK : 404);
  }
  else {  /* not a scan, just a single result row */
    return core->Run(data, res);
  }
}


int RowLoop::Run(data_operation *data, result_buffer &res) {
  begin->chain_out(data, res);
  for(unsigned int n = 0; n < data->n_result_cols ; n++) {
    if(n) res.out(*sep);
    core->out(data, n, res);
  }
  end->chain_out(data, res);
  return OK;
}
