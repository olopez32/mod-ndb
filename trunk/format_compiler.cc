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
#include "format_compiler.h"


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
    node->name = name;
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
  const char *s;
  /* For rollback: */
  old_token = current_token;
  old_start = token_start;

  /* if start == 0 , then start at token_next */
  if(start) token_start = start;
  else token_start = token_next;
  
  s = token_start;
  if(*token_start == 0) return tok_no_more;
  if(*token_start == '.' || *token_start == '$') {
    /* possible start of a token */
    if(*token_start == '.') {
      if(*(s+1) == '.' && *(s+2) == '.') { 
        token_end = s+2;
        token_next = s+3;
        return tok_ellipses;        
      }
    }
    if(*token_start == '$') {
      for( s++; *s != '$' && *s != 0 ; s++);
      if(*s == '$') {
        token_end = s;
        token_next = s+1;
        if( (!strncmp(token_start,"$name$",6)) || (!strncmp(token_start,"$name/",6)))
          return tok_fieldname;
        if( (!strncmp(token_start,"$value$",6)) || (!strncmp(token_start,"$value/",6)))
          return tok_fieldval;
        if(isdigit(*s+1))
          return tok_fieldnum;
        /* Anything else is a node */
        char *sym = (char *) ap_pcalloc(pool, (token_end - token_start));
        node_symbol = sym;
        for(const char *c = token_start + 1 ; c < token_end; )
          *sym++ = *c++;
        *sym = 0;        
        return tok_node;
      }
      /* No terminating dollar sign */
      error_message = ap_psprintf(pool, "Syntax error; no terminating '$' at \'%s\'\n",
                                start);
      return tok_error;
    } 
  } 
  
  /* This is plaintext; scan until the possible start of something else.*/
  for( ; *s != 0 ; s++) {
    if( (*s == '.' && *(s+1) == '.') || (*s == '$' && *(s-1) != '\\'))
      break;
  }
  token_next = s;
  token_end = s-1;
  return tok_plaintext;
}

const char *Parser::copy_node_text() {
  assert(! (token_end < token_start));
  size_t size = (token_end - token_start) + 2;
  char *copy = (char *) ap_pcalloc(pool, size);
  ap_cpystrn(copy, token_start, size);
  return copy;
}

Cell * Parser::build_cell() {
  re_esc  escape = no_esc;
  re_quot quote  = no_quot;
  int i = 0;
  token t = current_token;
    
  if(t == tok_plaintext) 
    return new(pool) Cell(copy_node_text());
    
  if(t == tok_fieldname || t == tok_fieldval || t == tok_fieldnum) {
    const char *p;
    for(p = token_start ; *p != '/' && p < token_end ; p++);
    if(*p == '/') {
      for( ; *p != '$' && p < token_end ; p++) {
        if(*p == 'q') quote = quote_char;
        else if(*p == 'Q') quote = quote_all;
        else if(*p == 'x') escape = esc_xml;
        else if(*p == 'j') escape = esc_json;
      }
    }
    if(t == tok_fieldnum) {
      char *end_num = (char *) (token_end - 1);
      i = strtol(token_start+1, &end_num, 10);
      return new(pool) Cell(item_value, escape, quote, i);
    }
    if(t == tok_fieldname) return new(pool) Cell(item_name, escape, quote);
    if(t == tok_fieldval) return new(pool) Cell(item_value, escape, quote);
    assert(0);
  }
  assert(0);
}

inline void Parser::expected(const char *what) {
  error_message = ap_psprintf(pool,"Parser error: %s expected at '%s'\n",
                              what, token_start);
}

const char * Parser::get_error() {
  const char *m = error_message;
  error_message = 0; 
  return m;
}

void Parser::rollback() {
  token_next = token_start;
  token_end = token_start - 1;
  token_start = old_start;
  current_token = old_token;
  error_message = 0;
}

len_string *Parser::get_string(pars_req req, const char *code) {  
  current_token = this->scan(code);
  if(current_token == tok_plaintext) {
    const char *copy = this->copy_node_text();
    return new(pool) len_string(copy);
  }
  if(req == required) expected("constant string");
  return 0;
}

bool Parser::get_ellipses(pars_req req, const char *code) {
  current_token = this->scan(code);
  if(current_token == tok_ellipses) 
    return true;
  if(req == required) expected("ellipses");
  return false;
}

Cell *Parser::get_cell(pars_req req, const char *code) {
  current_token = this->scan(code);
  if( (current_token == tok_plaintext) || (current_token == tok_fieldname) 
   || (current_token == tok_fieldval)  || (current_token == tok_fieldnum)) 
    return build_cell();
  if(req == required) expected("terminal");
  return 0;
}

Cell *Parser::get_cell_chain(const char *code) {
  Cell *c0 = 0, *c1 = 0, *c2 = 0;

  c0=get_cell(required, code);
  if(c0) c1 = c0;
  else return 0;
  while((c2 = get_cell(optional)) != 0) {
    c1->next = c2;
    c1 = c2;
  }
  /* Now we've scanned past the last cell in the chain. */
  if(current_token != tok_no_more) Parser::rollback();
  return c0;
}

Node *Parser::get_node(pars_req req, output_format *f, const char *code) {
  const char *node_name;
  Node *N;
  
  current_token = this->scan(code);
  if(current_token == tok_node) {
    N = f->symbol(node_symbol);
    if(N) return N;
    error_message = ap_psprintf(pool, "Undefined symbol '%s'\n",node_name);
  }
  else if(req == required) expected("node");
  return 0;
}


/* A plain node compiles simply by parsing its cell 
*/
const char * Node::compile(output_format *o) {
  cell = parser.get_cell(required, unresolved);
  return parser.get_error();
}


/* A RecAttr compiles two cell chains 
*/
const char * RecAttr::compile(output_format *o) {
  fmt = parser.get_cell_chain(unresolved);
  if(unresolved2) null_fmt = parser.get_cell_chain(unresolved2);
  else null_fmt = fmt;
  return parser.get_error();
}


/* "Loop" compiles its open, close, core, and sep.
    Loop loop 'begin $Rec$ sep ... end' 
*/
const char * Loop::compile(output_format *o) {
  const char *error = 0;
  /* First get a cell chain */
  begin = parser.get_cell_chain(unresolved);
  if(! begin) return parser.get_error();
  
  /* Then get a node reference */
  core = parser.get_node(required, o);
  error = parser.get_error();
  if(error) return error;

  /* Then get a separator */
  sep = parser.get_string(optional);  
  error = parser.get_error();
  if(error) return error;  
  
  /* Then get the ellipses */
  if(! parser.get_ellipses(required)) 
    return parser.get_error();
  
  /* Then get the final cell chain */
  end = parser.get_cell_chain();
  error = parser.get_error();
  if(error) return error;
  
  /* There should be nothing left */
  if(parser.scan(0) != tok_no_more) return "Unexpected text at end of format.";
  
  return 0;
}
