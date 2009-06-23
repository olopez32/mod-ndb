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

enum token { 
  tok_no_more ,
  tok_plaintext , tok_ellipses , 
  tok_fieldname , tok_fieldval , tok_fieldnum ,
  tok_node
};

enum { pars_optional = 0, pars_required = 1 };

class ParserError { 
  public:
  const char *message; 
  ParserError(const char *m) : message(m) {} ;
};

class Parser {
public:
  const char *token_start;
  const char *token_end;
  const char *token_next;
  const char *node_symbol;
  const char *old_start;
  token current_token;
  token old_token;
  ap_pool *pool;
  
  Parser() { pool = 0; }
  token scan(const char *);
  Cell *build_cell();
  const char *copy_node_text();
  len_string *get_string(bool, const char *c=0);
  bool get_ellipses(bool, const char *c=0);
  Cell *get_cell(bool, const char *c=0);
  Cell *get_cell_chain(bool, const char *c=0);
  Node *get_node(bool, output_format *, const char *c=0);
  bool the_end(bool, const char *c=0);
  void rollback();
  void expected(const char *);
  const char *get_error();
};

