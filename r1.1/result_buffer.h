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

#ifndef _RESULT_BUFFER_H
#define _RESULT_BUFFER_H


/* 
 *  This file, result_buffer.h is the first other file included in mod_ndb.h
 *  and is also included in MySQL_Field.h.  It contains the prototypes for 
 *  project-wide classes such as apache_object and len_string.
 */


// If putc() is a macro it can prevent this file from compiling.
#ifdef putc
#undef putc
#endif

void initialize_escapes(ap_pool *);


class apache_object {
public:
  void * operator new(size_t sz, ap_pool *p) {
    return ap_pcalloc(p, sz);
  }
};


class len_string : public apache_object {
public:
  size_t len;
  const char *string;
  
  len_string() {};
  len_string(size_t l, const char *str) : len (l) , string (str) {}
  len_string(const char *str) : string (str) {
    len = strlen(str);
  }  
};


class result_buffer {
private:
  size_t alloc_sz;
  
public:
  char *buff;
  size_t sz; 
  result_buffer() : buff(0) , sz(0) {};
  char *init(request_rec *r, size_t );
  bool prepare(size_t);
  inline void putc(char c) { *(buff + sz++) = c; };
  void out(const char *fmt, ...);
  void out(size_t, const char *);
  void out(struct st_decimal_t *);   /* in MySQL_Field.cc */
  inline void out(len_string &ls) { out(ls.len, ls.string); }
  void read_blob(NdbBlob *blob);
  void overlay(result_buffer *);
  ~result_buffer();
};

#endif  /* _RESULT_BUFFER_H */
