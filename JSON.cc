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

  /* JSON.cc
     
     Return a valid JSON formatted value for an NDB record.
  
     This is based on code in NdbRecAttr.cpp,  where the ability to represent 
     each type of value is implemented in the C++ I/O streams library by 
     overloading the "<<" operator.  
  */

const char * JSON::new_array    = "[\n";
const char * JSON::end_array    = "\n]";
const char * JSON::new_object   = " { ";
const char * JSON::end_object   = " }";
const char * JSON::delimiter    = " , ";
const char * JSON::is           = " : ";

char * JSON::value(const NdbRecAttr &rec, request_rec *r) {

  const NdbDictionary::Column* col;
  char *value;

  if (rec.isNULL())
     return "null";

  col = rec.getColumn();

  switch(rec.getType()) {
    
    /* Things that must be quoted in JSON: */
    
    case NdbDictionary::Column::Varchar:
    case NdbDictionary::Column::Char:
    case NdbDictionary::Column::Longvarchar:
    case NdbDictionary::Column::Date:
    case NdbDictionary::Column::Time:
    case NdbDictionary::Column::Datetime:
    case NdbDictionary::Column::Text:
    case NdbDictionary::Column::Binary:
    case NdbDictionary::Column::Blob:
      return ap_pstrcat(r->pool,
                        "\"",  MySQL::result(r->pool,rec), "\""
                        ,NULL);
    
    default:
      value = MySQL::result(r->pool,rec);
      if(value) return value;
  }
  return "\"unknown\"";
}


/* JSON::member() is an inline function defined in mod_ndb.h like this:
    inline static char *member(const NdbRecAttr &rec, request_rec *r) {
      return ap_pstrcat(r->pool, 
                        rec.getColumn()->getName(), 
                        JSON::is,
                        JSON::value(rec,r),
                        NULL);    
    }
*/