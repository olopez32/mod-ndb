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

const char **get_escapes(re_esc);
const char *json_str(ap_pool *, len_string &);

class Node;
class RecAttr;

struct symbol {
  Node *node;
  struct symbol *next_sym;
}; 


#define SYM_TAB_SZ 16
class output_format {
public:  
  const char *name;
  struct {
    unsigned int is_internal  : 1;
    unsigned int can_override : 1;
    unsigned int is_raw       : 1;
  } flag;
  Node *top_node;
  struct symbol *symbol_table[SYM_TAB_SZ];
  
  output_format(const char *n) : name(n) {}
  void * operator new(size_t sz, ap_pool *p) {
    return ap_pcalloc(p, sz);
  };
  Node * symbol(const char *, ap_pool *, Node *);
  const char *compile(ap_pool *);
  void dump(ap_pool *, result_buffer &, int);
};


class Cell : public len_string { 
  public:
  re_type elem_type ;
  re_quot elem_quote ;
  const char **escapes;
  int i;  
  Cell *next;
  
  Cell(re_type, re_esc, re_quot, int i=0);
  Cell(const char *txt) : len_string(txt) {
    elem_type = const_string;
  }
  Cell(size_t size, const char *txt) : len_string(size,txt) {
    elem_type = const_string; 
  }
  void out(result_buffer &res) { res.out(len,string); }
  void out(struct data_operation *, result_buffer &);
  void out(const NdbRecAttr &, result_buffer &); 
  void chain_out(struct data_operation *data, result_buffer &res) {
    for(Cell *c = this; c != 0 ; c = c->next) c->out(data,res);
  }
  void chain_out(result_buffer &res) {
    for(Cell *c = this; c != 0 ; c = c->next)  c->out(res);    
  }
  void dump(ap_pool *, result_buffer &);
};


class Node {
  public:
  const char *name;
  const char *unresolved;
  Cell *cell;
  Node *next_node;
  
  Node(const char *c) : unresolved (c) {}
  Node(const char *n, Cell *cell) : name(n), cell(cell), next_node(0) {}
  virtual ~Node() {}
  virtual void compile(output_format *);
  virtual void Run(struct data_operation *d, result_buffer &b) {cell->out(d,b);}
  virtual void out(const NdbRecAttr &r, result_buffer &b) {cell->out(r,b);}
  virtual void dump(ap_pool *p, result_buffer &, int);
  void * operator new(size_t sz, ap_pool *p) {
    return ap_pcalloc(p, sz);
  };
};

class RecAttr : public Node {
  const char *unresolved2;
  Cell *fmt;
  Cell *null_fmt;

  public:
  RecAttr(const char *str1, const char *str2) : Node(str1), unresolved2(str2) {}
  void out(const NdbRecAttr &rec, result_buffer &res);
  void compile(output_format *);
  void dump(ap_pool *p, result_buffer &, int);
};

class Loop : public Node {
  protected:
  Cell *begin;
  Node *core;
  len_string *sep;
  Cell *end;
 
  public:
  Loop(const char *c) : Node(c) {}
  void compile(output_format *);
  void dump(ap_pool *p, result_buffer &, int);
};


class RowLoop : public Loop {
public: 
  RowLoop(const char *c) : Loop(c) {}
  void Run(struct data_operation *, result_buffer &);
  void compile(output_format *o) { return Loop::compile(o); }
  void dump(ap_pool *p, result_buffer &r, int i) { Loop::dump(p,r,i); }
  void out(const NdbRecAttr &, result_buffer &) { assert(0); }
};


class ScanLoop : public Loop {  
public:
  ScanLoop(const char *c) : Loop(c) {}
  void Run(struct data_operation *, result_buffer &);
  void compile(output_format *o) { return Loop::compile(o); }
  void dump(ap_pool *p, result_buffer &r, int i) { Loop::dump(p,r,i); }
  void out(const NdbRecAttr &, result_buffer &) { assert(0); } 
};

