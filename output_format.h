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

class output_format {
public:  
  const char *name;
  struct {
    unsigned int is_internal  : 1;
    unsigned int can_override : 1;
    unsigned int is_raw       : 1;
  } flag;
  Node *top_node;
  
  void * operator new(size_t sz, ap_pool *p) {
    return ap_pcalloc(p, sz);
  };
};


class Cell : public len_string { 
  public:
  re_type elem_type ;
  re_quot elem_quote ;
  const char **escapes;
  int i;
  Cell *next;
  
  Cell(re_type, re_esc, re_quot);

  Cell(char *txt) {
    elem_type = const_string;
    string = txt;
    len = strlen(txt);
    next = 0;
  };
  void out(result_buffer &res) {
    res.out(len,string);
  }
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
  virtual ~Node() {};
  virtual void Run(struct data_operation *data, result_buffer &res) {
    if(cell) cell->out(data, res);
  }

  void * operator new(size_t sz, ap_pool *p) {
    return ap_pcalloc(p, sz);
  };
};

class RecAttr : public Node {
  Cell *fmt;
  Cell *null_fmt;

  public:
  RecAttr(const char *c1, const char *c2) : Node(c1,c2) {}
  void set_formats(Cell *c1, Cell *c2) {
    fmt = c1 ; null_fmt = c2 ;
  }
  void out(const NdbRecAttr &rec, result_buffer &res);
};

class Loop : public Node {
  protected:
  Cell *begin;
  len_string sep;
  Cell *end;
 
  public:
  Loop(const char *c1, const char *c2) : Node(c1,c2) {}
  void set_parts(Cell *a, char *b, Cell *c) {
    begin = a; sep = len_string(b); end = c;
  };
  void set_parts (ap_pool *p, char *a, char *b, char *c) {
    begin = new(p) Cell(a); 
    sep = len_string(b);
    end = new(p) Cell(c);
  }
};


class RowLoop : public Loop {
public: 
  RecAttr *record;
  RowLoop(const char *c1, const char *c2) : Loop(c1,c2) {}
  void Run(struct data_operation *, result_buffer &);
};


class ScanLoop : public Loop {  
  public:
  RowLoop *inner_loop;
  ScanLoop(const char *c1, const char *c2) : Loop(c1,c2) {}
  void Run(struct data_operation *, result_buffer &);
};
