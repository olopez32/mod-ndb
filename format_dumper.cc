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


char * output_format::dump(ap_pool *pool, int indent) {
  int n = 0;
  char *inset = make_inset(pool, indent);

  char *out = ap_pstrcat(pool, 
                         inset, "{ \"", name, "\":",
                         inset, "  { ", 
                         "is_internal:", (flag.is_internal ? "1" : "0"), 
                         ", can_override:", (flag.can_override ? "1" : "0"),
                         ", is_raw:", (flag.is_raw ? "1" : "0"),", nodes:", 
                         inset, "    [", 0);
  for(Node *N = top_node ; N != 0 ; N = N->next_node) 
    out = ap_pstrcat(pool, out, (n++ ? "," : "") , N->dump(pool, indent+6), 0);
  out = ap_pstrcat(pool, out, inset, "    ]", inset, "  }", inset, "}", 0);
  return out;
}


char *Node::dump(ap_pool *p, int indent) {
  char *inset = make_inset(p, indent);
  return ap_pstrcat(p, inset,"{ \"cell\":", cell->dump(p), " }", 0);
}


char * Loop::dump(ap_pool *p, int indent) {
  char *inset = make_inset(p, indent);
  return ap_pstrcat(p, "{ \"", name , "\":", 
                    inset, "  {",
                    inset, "    begin: ",  begin->dump(p), " ,", 
                    inset, "    core:  ",  core->dump(p, indent + 4),  " ,", 
                    inset, "    sep:   \"",json_str(p, *sep),   "\" ,", 
                    inset, "    end:   ",  end->dump(p), 
                    inset, "  }", 
                    inset, "}", 0 );
}


char *RecAttr::dump(ap_pool *p, int indent) {
  char *inset = make_inset(p, indent);
  return ap_pstrcat(p, 
                    inset, "{",
                    inset, "  fmt :     ", fmt->dump(p), " ,",
                    inset, "  null_fmt: ", null_fmt->dump(p), 
                    inset, "}", 0);
}


char *Cell::dump(ap_pool *p) {
  int n = 0;
  char *out = ap_pstrdup(p, "[");
  const char *val;

  for(Cell *c = this ; c != 0 ; c = c->next) {
    if(n++) out = ap_pstrcat(p, out, " , ", 0);
    switch(c->elem_type) {
      case const_string: 
        val = json_str(p, *c);
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
          if(c->elem_quote == quote_char)             flags[f++] = 'q';
          else if(c->elem_quote == quote_all)         flags[f++] = 'Q';
          if(c->escapes == escape_leaning_toothpicks) flags[f++] = 'j';
          else if(c->escapes == escape_xml_entities)  flags[f++] = 'x';
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
