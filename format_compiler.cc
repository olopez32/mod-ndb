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
#include <ctype.h>


/* In Apache (regardless of MPM), configuration processing is single-threaded.
   So, compiling of output formats is single-threaded, and this file uses a
   single "static" parser.
*/
Parser parser;
    
Cell::Cell(re_type type, re_esc esc, re_quot quote, int i) :
elem_type (type) , elem_quote (quote) , i (i) {   
  
  escapes = get_escapes(esc);  
  next = 0;
};

/* Given a string from "start" to "end" that has been parsed out to token 
type "t", build a Cell to represent the string
*/

Cell *build_cell(ap_pool *pool, token t, const char *start, const char *end) {
  re_esc  escape = no_esc;
  re_quot quote  = no_quot;
  int i = 0;
  
  if(t == tok_fieldname || t == tok_fieldval || t == tok_fieldnum) {
    const char *p;
    for(p = start ; *p != '/' && p < end ; p++);
    if(*p == '/') {
      for( ; *p != '$' && p < end ; p++) {
        if(*p == 'q') quote = quote_char;
        else if(*p == 'Q') quote = quote_all;
        else if(*p == 'x') escape = esc_xml;
        else if(*p == 'j') escape = esc_json;
      }
    }
    if(t == tok_fieldnum) {
      char *end_num = (char *) (end - 1);
      i = strtol(start+1, &end_num, 10);
      return new(pool) Cell(item_value, escape, quote, i);
    }
    if(t == tok_fieldname) return new(pool) Cell(item_name, escape, quote);
    if(t == tok_fieldval) return new(pool) Cell(item_value, escape, quote);
    assert(0);
  }
  if(t == tok_plaintext) {
    size_t size = end - start + 1;
    char *copy = (char *) ap_pcalloc(pool, size);
    ap_cpystrn(copy, start, size);
    return new(pool) Cell(size, copy);
  }
  assert(0);
}


/*  Look up "name" in a format's symbol table. 
    If ap_pool *p is non-null and the name cannot be found, add it to the table.
*/
Node * output_format::symbol(const char *name, ap_pool *p=0, Node *node=0) {
  struct symbol *sym;
  
  unsigned int h=0;  // hash function:
  for(const char *s = name; *s != 0; s++) h = 37 * h + *s;
  h = h % SYM_TAB_SZ;
  for (sym = symbol_table[h] ; sym != 0 ; sym = sym->next_sym) 
    if(!strcmp(name, sym->node->name)) 
      return sym->node;
  if(p) {
    sym = (struct symbol *) ap_pcalloc(p, sizeof(struct symbol)); 
    sym->node = node; 
    sym->next_sym = symbol_table[h]; 
    symbol_table[h] = sym;
  }
  return sym->node;
}


/*  Find each node in the symbol table and compile it */
const char * output_format::compile(ap_pool *pool)  {
  const char *err;
  parser.pool = pool;
  parser.error_message = 0;
  for(unsigned int h = 0 ; h < SYM_TAB_SZ ; h++)
    for(struct symbol *sym = symbol_table[h]; sym != 0 ; sym = sym->next_sym)
      if( (err = sym->node->compile(this)) != 0)
        return err;
  return 0;
}


token Parser::scan(const char *start) {  
  const char *s = start;
  token_start = s;
  
  if(*s == 0) return tok_no_more;
  if(*s == '.' || *s == '$') {
    /* possible start of a token */
    if(*s == '.') {
      if(*s+1 == '.' && *s+2 == '.') { 
        token_end = s+2;
        return tok_elipses;        
      }
    }
    if(*s == '$') {
      const char *p;
      for(p = s; *p != '$' && *p != 0 ; p++);
      if(*p == '$') {
        token_start = s;
        token_end = p;
        if( (!strncmp(s,"$name$",6)) || (!strncmp(s,"$name:",6)))
          return tok_fieldname;
        if( (!strncmp(s,"$value$",6)) || (!strncmp(s,"$value:",6)))
          return tok_fieldval;
        if(isdigit(*s+1))
          return tok_fieldnum;
        /* fall through to here */
        return tok_node;
      }
      /* No terminating dollar sign */
      error_message = ap_psprintf(pool, "Syntax error; no terminating '$' at \'%s\'\n",
                                start);
      return tok_error;
    } 
  } 
  
  /* This is plaintext; scan until the possible start of something else.*/
  s++;
  for( ; *s != 0 ; s++) {
    if( (*s == '.' && *s+1 == '.') || (*s == '$' && *s-1 != '\\'))
      break;
  }
  token_end = s;
  return tok_plaintext;
}

const char *Parser::copy_node_text(const char *start, const char *end) {
  size_t size = (end - start) + 1;
  char *copy = (char *) ap_pcalloc(pool, size);
  ap_cpystrn(copy, start, size);
  return copy;
}

len_string *Parser::get_string(const char *code, const char **end) {  
  current_token = this->scan(code);
  if(current_token == tok_plaintext) {
    const char *copy = this->copy_node_text(token_start, token_end);
    return new(pool) len_string(copy);
  }
  return 0;
}

Cell *Parser::get_cell(const char *code, const char **end) {
  const char *s = code;
  Cell *c0 = 0, *c1 = 0, *c2 = 0;

  while(1) {
    current_token = this->scan(s);
    if(current_token == tok_no_more || current_token == tok_node) 
      return c0;

    if(current_token == tok_error)
      return 0;

    /* Build a cell: */
    c2 = build_cell(pool, current_token, token_start, token_end);
    if(c0 == 0) c0 = c1 = c2;
    else {
      c1->next = c2;
      c1 = c2;
    }
    s = token_end;
  }
  return c0;
}

Node *Parser::get_node(output_format *f, const char *code, const char **end) {
  const char *node_name;
  Node *N;
  
  current_token = this->scan(code);
  if(current_token == tok_node) {
    node_name = this->copy_node_text(token_start + 1, token_end -1);
    N = f->symbol(node_name);
    if(N) return N;
    error_message = ap_psprintf(pool, "Undefined symbol '%s'\n",node_name);
  }
  return 0;
}


/* A plain node compiles simply by parsing its cell 
*/
const char * Node::compile(output_format *o) {
  const char *endp;
  cell = parser.get_cell(unresolved, &endp);
  return parser.get_error();
}


/* A RecAttr compiles two cells 
*/
const char * RecAttr::compile(output_format *o) {
  const char *endp;
  fmt = parser.get_cell(unresolved, &endp);
  if(unresolved2) null_fmt = parser.get_cell(unresolved2, &endp);
  else null_fmt = fmt;
  return parser.get_error();
}


/* "Loop" compiles its open, close, and separator and leaves a pointer to an
    unresolved node name in "core_node" 
    Loop loop 'begin $Rec$ sep ... end' 
*/
const char * Loop::compile(output_format *o) {
  const char *endp, *pos, *error;
  /* First get a cell */
  begin = parser.get_cell(unresolved, &endp);
  error = parser.get_error();
  if(error) return error;
  if(! begin) return "Expected string";
  
  /* Then get a node reference */
  pos = endp;
  core = parser.get_node(o, pos, &endp);
  error = parser.get_error();
  if(error) return error;
  if(! core) return "Expected node reference";

  /* Then get a separator */
  pos = endp;
  sep = parser.get_string(pos, &endp);
  error = parser.get_error();
  if(error) return error;  
  if(! sep) return "Expected separator string";
  
  /* The get the final cell */
  pos = endp;
  end = parser.get_cell(pos, &endp);
  error = parser.get_error();
  if(error) return error;
  if(! end) return "Expected string";
  
  /* There should be nothing left */
  if(parser.scan(endp) != tok_no_more)
    return "Unexpected text at end of format.";
  
  return 0;
}

void RowLoop::Run(data_operation *data, result_buffer &res) {
  begin->chain_out(data, res);
  for(unsigned int n = 0; n < data->n_result_cols ; n++) {
    if(n) res.out(*sep);
    const NdbRecAttr &rec = *data->result_cols[n];
    core->out(rec, res);
  }
  end->chain_out(data, res);
};


void RecAttr::out(const NdbRecAttr &rec, result_buffer &res) {
  for( Cell *c = rec.isNULL() ? null_fmt : fmt; c != 0 ; c=c->next) 
    c->out(rec, res);
}

void RowLoop::out(const NdbRecAttr &rec, result_buffer &res) {
  assert(0);
}

void ScanLoop::out(const NdbRecAttr &rec, result_buffer &res) {
  assert(0);
}

