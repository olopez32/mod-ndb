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


class JSON {
  public:
    static const char * new_array;
    static const char * end_array;
    static const char * new_object;
    static const char * end_object;
    static const char * delimiter ;
    static const char * is        ;
    static char *value(const NdbRecAttr &rec, request_rec *r);
    inline static char *member(const NdbRecAttr &rec, request_rec *r) {
      return ap_pstrcat(r->pool, 
                        rec.getColumn()->getName(), 
                        JSON::is,
                        JSON::value(rec,r),
                        NULL);    
    }
};
