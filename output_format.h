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
};


class Cell : public len_string { 
  public:
  re_type elem_type ;
  re_quot elem_quote ;
  const char **escapes;
  int i;  
  Cell *next;
  
  Cell(re_type, re_esc, re_quot, int i=0);
  Cell(char *txt) : len_string(txt) {
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
};


class Node {
  public:
  const char *name;
  const char *unresolved;
  Cell *cell;
  Node *next_node;
  
  Node(const char *c1, const char *c2 ) : name (c1) , unresolved (c2) {}
  virtual ~Node() {}
  virtual void Run(struct data_operation *data, result_buffer &res) {
    if(cell) cell->out(data, res); 
  }
  virtual void out(const NdbRecAttr &rec, result_buffer &res) {
    if(cell) cell->out(rec, res);
  }
  virtual const char *compile(output_format *);

  void * operator new(size_t sz, ap_pool *p) {
    return ap_pcalloc(p, sz);
  };
};

class RecAttr : public Node {
  const char *unresolved2;
  Cell *fmt;
  Cell *null_fmt;

  public:
  RecAttr(const char *nm, const char *str1, const char *str2) : 
    Node(nm,str1) , unresolved2 (str2) {}
  void out(const NdbRecAttr &rec, result_buffer &res);
  const char *compile(output_format *);
};

class Loop : public Node {
  protected:
  Cell *begin;
  Node *core;
  len_string *sep;
  Cell *end;
 
  public:
  Loop(const char *c1, const char *c2) : Node(c1,c2) {}
  const char *compile(output_format *);
};


class RowLoop : public Loop {
public: 
  RowLoop(const char *c1, const char *c2) : Loop(c1,c2) {}
  const char *compile(output_format *);
  void Run(struct data_operation *, result_buffer &);
  void out(const NdbRecAttr &, result_buffer &); 
};


class ScanLoop : public Loop {  
public:
  ScanLoop(const char *c1, const char *c2) : Loop(c1,c2) {}
  const char *compile(output_format *);
  void Run(struct data_operation *, result_buffer &);
  void out(const NdbRecAttr &, result_buffer &); 
};
