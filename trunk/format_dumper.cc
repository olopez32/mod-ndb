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

extern const char *escape_leaning_toothpicks[256];
extern const char *escape_xml_entities[256];


inline char *make_inset(ap_pool *pool, int size) {
  char *inset = (char *) ap_pcalloc(pool, size + 2);
  inset[0] = '\n'; 
  for(int i = 1 ; i <= size ; i++) 
    inset[i] = ' ';
  return inset;
}


void output_format::dump(ap_pool *pool, result_buffer &res, int indent) {
  int n = 0;
  char *inset = make_inset(pool, indent);
  char *out = ap_pstrcat(pool, 
                         inset, "{ \"", name, "\":",
                         inset, "  { ", 
                         "is_internal:", (flag.is_internal ? "1" : "0"), 
                         ", can_override:", (flag.can_override ? "1" : "0"),
                         ", is_raw:", (flag.is_raw ? "1" : "0"),", nodes:", 
                         inset, "    [", 0);
  res.out(strlen(out), out);
  
  for(Node *N = top_node ; N != 0 ; N = N->next_node) {
    if(n++) res.out(1, ",");
    N->dump(pool, res, indent+6);
  }
  res.out("%s    ]%s  }%s}", inset, inset, inset);
}


void Node::dump(ap_pool *p, result_buffer &res, int indent) {
  char *inset = make_inset(p, indent);
  res.out(inset);
  res.out("{ \"cell\":");
  cell->dump(p, res);
  res.out(" }");
}


void Loop::dump(ap_pool *p, result_buffer &res, int indent) {
  char *inset = make_inset(p, indent);
  char *out;
  out = ap_pstrcat(p, "{ \"", name , "\":", 
                      inset, "  {",
                      inset, "    begin: ");
  res.out(out); begin->dump(p, res);
  res.out(" ,%s    core:  ", inset); 
  core->dump(p, res, indent + 4);
  res.out(" ,%s    sep:   \"%s\" ,",inset, json_str(p, *sep));
  res.out("%s    end:   ",inset);
  end->dump(p, res), 
  res.out("%s  }%s}", inset, inset);
}


void RecAttr::dump(ap_pool *p, result_buffer &res, int indent) {
  char *inset = make_inset(p, indent);
  res.out("%s{%s  fmt :     ",inset, inset);
  fmt->dump(p, res);
  res.out(" ,%s  null_fmt: ",inset);
  null_fmt->dump(p, res), 
  res.out("%s}", inset);
}


void Cell::dump(ap_pool *p, result_buffer &res) {
  int n = 0;
  char *out;
  const char *val;
  res.out("[");

  for(Cell *c = this ; c != 0 ; c = c->next) {
    if(n++) res.out(" , ");
    switch(c->elem_type) {
      case const_string: 
        val = json_str(p, *c);
        res.out("\"%s\"", val);
        break;
      case item_name :
        if(c->elem_quote == quote_char) val = "/q";
        else if (c->elem_quote == quote_all) val ="/Q";
        else val="";
        res.out("\"$name%s$\"", val);
        break;
      case item_value:
      {
        char flags[4] = { 0, 0, 0, 0 };
        int f = 1;
        char *item;
        if(c->escapes || ( c->elem_quote != no_quot)) {
          flags[0] = '/';
          if(c->elem_quote == quote_char)             flags[f++] = 'q';
          else if(c->elem_quote == quote_all)         flags[f++] = 'Q';
          if(c->escapes == escape_leaning_toothpicks) flags[f++] = 'j';
          else if(c->escapes == escape_xml_entities)  flags[f++] = 'x';
        }
        if(c->i > 0) item = ap_psprintf(p, "$%d", c->i);
        else item = "$value";
        res.out("\"%s%s$\"", item, flags);
      }
        break;
      default:
        res.out(" \"*HOW_DO_I_DUMP_THIS_KIND_OF_CELL*\" ");
    }
  }
  res.out("]");
}

const char *escape_string(ap_pool *pool, const char **escapes, len_string &str) {  
  size_t escaped_size = 0;
  register const char *esc;
  
  /* How long will the string be when it is escaped? */
  for(unsigned int i = 0; i < str.len ; i++) {
    const unsigned char c = str.string[i];
    esc = escapes[c];
    if(esc) escaped_size += esc[0];
    else escaped_size++;
  }
  if(escaped_size == str.len) return str.string;
  
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


inline const char *json_str(ap_pool *pool, len_string &str) {
  return escape_string(pool, escape_leaning_toothpicks, str);
}
