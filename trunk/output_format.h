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

enum re_type { const_string, item_name, item_value };
enum re_esc  { no_esc, esc_xml, esc_json, esc_xmljson };
enum re_quot { no_quot, quote_char, quote_all };
enum node_type { top_node, loop_node, simple_node };

const char **get_escapes(re_esc);
const char *json_str(ap_pool *, len_string &);

class Node;
class RecAttr;

struct symbol {
  Node *node;
  struct symbol *next_sym;
}; 


#define SYM_TAB_SZ 16
class output_format : public apache_object {
public:  
  const char *name;
  struct {
    unsigned int is_internal  : 1;
    unsigned int can_override : 1;
    unsigned int is_raw       : 1;
    unsigned int is_JSON      : 1;
  } flag;
  Node *top_node;
  struct symbol *symbol_table[SYM_TAB_SZ];
  int charset_id;
  output_format(const char *n) : name(n) {}
  Node * symbol(const char *, ap_pool *, Node *);
  const char *compile(ap_pool *);
  void dump(ap_pool *, result_buffer &);
  void dump_source(ap_pool *, result_buffer &);
};


class Cell : public len_string { 
  public:
  re_type elem_type ;
  re_quot elem_quote ;
  const char **escapes;
  unsigned int i;  
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
  void out(struct data_operation *, unsigned int, result_buffer &); 
  void chain_out(struct data_operation *data, result_buffer &res) {
    for(Cell *c = this; c != 0 ; c = c->next) c->out(data,res);
  }
  void chain_out(result_buffer &res) {
    for(Cell *c = this; c != 0 ; c = c->next)  c->out(res);    
  }
  void dump(ap_pool *, result_buffer &);
};


class Node : public apache_object {
  public:
  const char *name;
  const char *unresolved;
  Cell *cell;
  Node *next_node;
  node_type type;
  
  Node(const char *c, node_type t) : unresolved(c) , type(t) {}
  Node(const char *n, Cell *cell) : name(n), cell(cell), next_node(0) {}
  virtual ~Node() {}
  virtual void compile(output_format *);
  virtual int  Run(struct data_operation *d, result_buffer &b) { 
    cell->out(d,b); return OK; }
  virtual void out(struct data_operation *d, unsigned int n, result_buffer &b) {
    cell->out(d, n, b); }
  virtual void dump(ap_pool *, result_buffer &, int);
  virtual void dump_source(ap_pool *, result_buffer &, const char *) {};
};

class RecAttr : public Node {
  const char *unresolved2;
  Cell *fmt;
  Cell *null_fmt;

  public:
  RecAttr(const char *str1, const char *str2) : Node(str1, simple_node), unresolved2(str2) {}
  void out(struct data_operation *, unsigned int, result_buffer &);
  void compile(output_format *);
  void dump(ap_pool *, result_buffer &, int);
  void dump_source(ap_pool *, result_buffer &, const char *);
};

class Loop : public Node {
  friend class output_format;
  protected:
  Cell *begin;
  Node *core;
  len_string *sep;
  Cell *end;
 
  public:
  Loop(const char *c, node_type t = loop_node) : Node(c, t) {}
  void compile(output_format *);
  void dump(ap_pool *, result_buffer &, int);
  void dump_source(ap_pool *, result_buffer &, int) { assert(0); };
};


class RowLoop : public Loop {
public: 
  RowLoop(const char *c) : Loop(c) {}
  int Run(struct data_operation *, result_buffer &);
  void compile(output_format *o) { return Loop::compile(o); }
  void out(const NdbRecAttr &, result_buffer &) { assert(0); }
  void dump(ap_pool *p, result_buffer &r, int i) { Loop::dump(p,r,i); }
  void dump_source(ap_pool *, result_buffer &, const char *);
};


class ScanLoop : public Loop {  
public:
  ScanLoop(const char *c) : Loop(c) {}
  int Run(struct data_operation *, result_buffer &);
  void compile(output_format *o) { return Loop::compile(o); }
  void dump(ap_pool *p, result_buffer &r, int i) { Loop::dump(p,r,i); }
  void out(const NdbRecAttr &, result_buffer &) { assert(0); } 
  void dump_source(ap_pool *, result_buffer &, const char *);
};


class MainLoop : public Loop {
  public: 
  MainLoop(const char *c) : Loop(c, top_node) {}
  int Run(struct data_operation *, result_buffer &);
  void compile(output_format *);
  void dump(ap_pool *, result_buffer &r, int i);
  void out(const NdbRecAttr &, result_buffer &) { assert(0); }
  void dump_source(ap_pool *, result_buffer &, const char *);
};  
