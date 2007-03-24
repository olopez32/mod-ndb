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

#ifndef _RESULT_BUFFER_H
#define _RESULT_BUFFER_H

// If putc() is a macro it can prevent this file from compiling.
#ifdef putc
#undef putc
#endif

void initialize_escapes();

class result_buffer {
private:
  size_t alloc_sz;
  
public:
  char *buff;
  size_t sz; 
  char *init(request_rec *r, size_t );
  bool prepare(size_t);
  inline void putc(char c) { *(buff + sz++) = c; };
  void out(const char *fmt, ...);
  void out(size_t, const char *);
  inline void out(len_string &ls) { out(ls.len, ls.string); };
  ~result_buffer();
};

#endif  /* _RESULT_BUFFER_H */
