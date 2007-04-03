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

enum token { 
  tok_error , tok_no_more ,
  tok_plaintext , tok_ellipses , 
  tok_fieldname , tok_fieldval , tok_fieldnum ,
  tok_node 
};

enum pars_req { optional, required };


class Parser {
public:
  const char *token_start;
  const char *token_end;
  const char *token_next;
  const char *node_symbol;
  const char *old_start;
  char *error_message;
  token current_token;
  token old_token;
  ap_pool *pool;
  
  Parser() { pool = 0 ; error_message = 0; }
  token scan(const char *);
  Cell *build_cell();
  const char *copy_node_text();
  len_string *get_string(pars_req, const char *c=0);
  bool get_ellipses(pars_req, const char *c=0);
  Cell *get_cell(pars_req, const char *c=0);
  Cell *get_cell_chain(const char *c=0);
  Node *get_node(pars_req, output_format *, const char *c=0);
  void rollback();
  void expected(const char *);
  const char *get_error();
};

