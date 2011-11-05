/*
 *  multipart.cc
 *  mod_ndb
 *
 *  Created by John David Duncan on 1/8/09.
 *
 */

/* Copyright (C) 2009 Sun Microsystems
 
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


/* http://www.w3.org/TR/html401/interact/forms.html#h-17.13.4.2 */

/*
*   multipart-body := preamble 1*encapsulation close-delimiter epilogue
*   encapsulation := delimiter body CRLF
*   delimiter := "--" boundary CRLF
*   close-delimiter := "--" boudary "--"
*   preamble := <ignore>
*   epilogue := <ignore>
*   body := header-part CRLF body-part
*   header-part := 1*header CRLF
*   header := header-name ":" header-value
*   header-name := <printable ascii characters except ":">
*   header-value := <any ascii characters except CR & LF>
*   body-data := <arbitrary data>
*/

bool get_attribute(ap_pool *pool, const char *header, 
                   const char *attrib, size_t attr_len, char **value) {
  char *c = header ;
  
  /*    Content-Type: multipart/form-data; boundary=AaB03x  */

  get_semicolon:
  for ( *c != 0 && *c != ';' ; c++);
  while(ap_isspace(*c)) c++;
  if(*c == 0) return 0;
  
  /* attribute, e.g. "boundary" */
  if(strncasecmp(c, attrib, attr_len)) goto get_semicolon;

  /* equals */
  for ( *c != 0 && *c != '=' ; c++);
  if(*c == 0) return 0;
  
  /* ap_get_token() handles quotes and spaces */
  *value = ap_get_token(pool, c, 1);
  
  if(*value) return 1;
  return 0;
}


bool 


#ifdef TEST_HARNESS
main() {


}
#endif