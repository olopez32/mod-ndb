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

#include "JSON/Scanner.h"
#include "mod_ndb.h"


char JSON_unescape_table[128] = 
{ 0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,
  
  0,   0, '"',   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0, '/',
  0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,
  
  0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,'\\',   0,   0,   0,
  
  0,   0,'\b',   0,   0,   0,'\f',   0,
  0,   0,   0,   0,   0,   0,'\n',   0,
  0,   0,'\r',   0,'\t', 'u',   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0
};


int xval(char c) {
  if(c >= '0' && c <= '9') 
    return c - '0';
  switch(c) {
    case 'a':   case 'A':     return 10;
    case 'b':   case 'B':     return 11;
    case 'c':   case 'C':     return 12;
    case 'd':   case 'D':     return 13;
    case 'e':   case 'E':     return 14;
    case 'f':   case 'F':     return 15;
  }
  assert(0);   // If we get here, there's a coding error in the JSON parser.
  return 0;
}


/* JSON_unescape(str) 
   Unescape string in place
   Return length of unescaped string.
*/
int JSON_unescape(char *str) {
  register char c;
  char *r = str;
  char *w = 0;   // w is overloaded: both a write-flag and a write-position
  int more = 1;
  int pair1 = 0;  // first half of a surrogate pair
  
  while(more) {
    c = *r;
    if(c == 0) more = 0;
    else if(c == '\\') {
      if(w == 0) w = r;  /* start rewriting */ 
      
      /* Look up the escape code in the table */
      c = JSON_unescape_table[(int) (*++r)]; 
      
      if(c == 'u') {  /* \uXXXX */
         /* Convert the character code from hex to UTF-8 */
         char c4 = *++r;     char c3 = *++r;
         char c2 = *++r;   char c1 = *++r;  
         r++;
         unsigned int char_code = 
          ((16 * 16 * 16) * xval(c4)) + ((16 * 16) * xval(c3)) +
          (16 * xval(c2)) + xval(c1); 
          
         if(char_code < 0x80) {        // 1-byte UTF-8
           *w++ = (char) char_code;
           continue;
         }
         else if(char_code < 0x800) {  // 2-byte UTF-8
           *w++ = (char) ( 0xC0 | char_code >> 6       );
           *w++ = (char) ( 0x80 | char_code      & 0x3F);
           continue;
         }
         else if(char_code > 0xD7FF && char_code < 0xE000) {
           /* Mapped UTF-16 Surrogate pair */
           if(pair1 == 0) pair1 = char_code;
           else {
             char_code = 0x10000 + ((pair1 & 0x3FF) << 10) + (char_code & 0x3FF);
             *w++ = (char) ( 0xF0 | char_code >> 18 & 0x07);
             *w++ = (char) ( 0x80 | char_code >> 12 & 0x3F);
             *w++ = (char) ( 0x80 | char_code >> 6  & 0x3F);
             *w++ = (char) ( 0x80 | char_code       & 0x3F);
             pair1 = 0;
           }
           continue;
         }
         else {                        // 3-byte UTF-8 
           *w++ = (char) ( 0xE0 | char_code >> 12       );
           *w++ = (char) ( 0x80 | char_code >> 6  & 0x3F);
           *w++ = (char) ( 0x80 | char_code       & 0x3F);
           continue;
         }
      }
    }
    r++;
    if(w) *w++ = c;
  }
  return w ? w - str - 1 : r - str - 1;
}


/* JSON_string() 
   Take a token from Coco and an Apache pool.
   Strip the quotes, if there are any. 
*/
char *JSON_string(ap_pool *my_pool, JSON::Token *tok) {;
  int len = tok->len;
  wchar_t *start = tok->val;
  if(*start == L'"') start++, len -= 2;
  char *res = (char *) ap_palloc(my_pool, len + 1);
  for (int i = 0; i < len; ++i)
    res[i] = (char) start[i]; 
  res[len] = 0;
  
  len = JSON_unescape(res);
  return res;
}
