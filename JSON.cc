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

void JSON::put_value(result_buffer &rbuf,const NdbRecAttr &rec,request_rec *r){

  const NdbDictionary::Column* col;

  if (rec.isNULL())
     return rbuf.out("null");

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
        return rbuf.out("\"%s\"", MySQL::result(r->pool,rec));

    default:      
        return rbuf.out(MySQL::result(r->pool,rec));
  }
  rbuf.out("\"unknown\"");
}

/* Note: JSON::member() is an inline function defined in JSON.h 
*/
