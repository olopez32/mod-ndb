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

#include "mod_ndb.h"
#include "mod_ndb_compat.h"
#include "query_source.h"


class BLOB : public apache_object {
public:
  const char *name;
  len_string info;
  BLOB *next;
};


inline unsigned int do_hash(const char *name) {
  unsigned int h = 0;
  for(const char *s = name; *s != 0; s++) h = 37 * h + *s;
  return h % FORM_TABLE_SIZE;
}


void query_source::set_item(const char *name, const char *pos, size_t sz) {
  BLOB * blob = new(r->pool) BLOB;
  unsigned int h = do_hash(name); 
   
  blob->name = name;
  blob->info.len = sz;
  blob->info.string = pos;
  blob->next = form_table[h];
  
  form_table[h] = blob;
}


len_string * query_source::get_item(const char *name) {
  BLOB *b;
  unsigned int h = do_hash(name);  
  
  for (b = form_table[h] ; b != 0 ; b = b->next) 
    if(!strcmp(name, b->name)) 
      return & b->info;

  return 0;  
}


Apache_subrequest_query_source::Apache_subrequest_query_source(request_rec *req)
{
  r = req;
  keep_tx_open = true;
  const char *note = ap_table_get(r->main->notes,"ndb_request_method");
  if(note)  {
    if(!strcmp(note,"POST")) req_method = M_POST;
    else if(!strcmp(note,"DELETE")) req_method = M_DELETE;
    ap_table_unset(r->main->notes,"ndb_request_method");
  }
}


int Apache_subrequest_query_source::get_form_data() {
  const char *subrequest_data = ap_table_get(r->main->notes,"ndb_request_data");
  register const char *c = subrequest_data;
  char *key, *val;
  while(*c && (val = ap_getword(r->pool, &c, '&'))) {
    key = ap_getword(r->pool, (const char **) &val, '=');
    ap_unescape_url(key);
    ap_unescape_url(val);
    set_item(key, val);
  }
  ap_table_unset(r->main->notes,"ndb_request_data");
  
  return OK;
}


/*  int HTTP_query_source::get_form_data()  is implemented in 
    request_body.cc 
*/
