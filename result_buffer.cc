/* Copyright (C) 2006 MySQL AB

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

char * result_buffer::init(request_rec *r, size_t size) {
  parent_pool = r->pool;
  alloc_sz = size;
  sz = 0;
  pool = ap_make_sub_pool(parent_pool);
  buff = (char *) ap_palloc(pool,alloc_sz);
  if(!buff) log_err(r->server, "mod_ndb result_buffer::init() out of memory");
  return buff;
}


void result_buffer::out(const char *fmt, ... ) {
  va_list args;
  size_t old_size = sz;
  char * old_buff;
  ap_pool *new_pool;
  bool try_again;

  do {    
    try_again = false;
    va_start(args,fmt);
    sz += vsnprintf((buff + sz), alloc_sz - sz, fmt, args);
    va_end(args);
    
    if(sz >= alloc_sz) {
      try_again = true;
      // The write was truncated.  Get a bigger buffer and do it again
      alloc_sz *= 4;
      old_buff = buff;
      sz = old_size;
      new_pool = ap_make_sub_pool(parent_pool);
      buff = (char *) ap_palloc(new_pool, alloc_sz);
      if(! buff) {
        ap_destroy_pool(new_pool);
        return;
      }      
      memcpy(buff, old_buff, sz); 
      ap_destroy_pool(pool);  /* Free the space used by the old buffer */
      pool = new_pool;
    }
  } while(try_again);
}
