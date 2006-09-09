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


/* 
   MySQL_Field.cc
   
   This file has a different set of includes than other mod_ndb source files:
   it uses my_global.h to get MySQL's typedefs and macros like "sint3korr",
   but it does not include mod_ndb.h (because you can get trouble if you try 
   to combine mysql headers and apache headers in a single source file).
*/
   
#include "mysql_version.h"
#include "my_global.h"
#include "NdbApi.hpp"
#include "httpd.h"
#include "MySQL_Field.h"


// Apache disabled this
#undef strtoul

// The NdbRecAttr interface changed between MySQL 5.0 and 5.1
#if MYSQL_VERSION_ID < 50100
#define Attr_Size(r) r.arraySize()
#else 
#define Attr_Size(r) r.get_size_in_bytes()
#endif


/* This is based on code from Field.cc 
   (which was also copied into NdbRecAttr.cpp)
   and documentation at 
   http://dev.mysql.com/doc/internals/en/myisam-column-attributes.html
*/

char * MySQL::Time(pool *p, const NdbRecAttr &rec) {
  long tmp=(long) sint3korr(rec.aRef());
  int hour=(uint) (tmp/10000);
  int minute=(uint) (tmp/100 % 100);
  int second=(uint) (tmp % 100);
  return ap_psprintf(p, "%02d:%02d:%02d", hour, minute, second);
}

char * MySQL::Date(pool *p, const NdbRecAttr &rec) {
  unsigned int tmp= ( unsigned int ) uint3korr(rec.aRef());
  int part;
  char *buf = (char *) ap_pcalloc(p,40);
  char *pos=(char*) buf+10;
  *pos--=0;
  part=(int) (tmp & 31);
  *pos--= (char) ('0'+part%10);
  *pos--= (char) ('0'+part/10);
  *pos--= '-';
  part=(int) (tmp >> 5 & 15);
  *pos--= (char) ('0'+part%10);
  *pos--= (char) ('0'+part/10);
  *pos--= '-';
  part=(int) (tmp >> 9);
  *pos--= (char) ('0'+part%10); part/=10;
  *pos--= (char) ('0'+part%10); part/=10;
  *pos--= (char) ('0'+part%10); part/=10;
  *pos=   (char) ('0'+part);
  return buf;
}

char * MySQL::Datetime(pool *p, const NdbRecAttr &rec) {
  unsigned long long tmp=rec.u_64_value();
  long part1,part2,part3;
  part1=(long) (tmp / (long long) (1000000));
  part2=(long) (tmp - (unsigned long long) part1 * (long long)(1000000));
  char *buf = (char *) ap_pcalloc(p,40);
  char* pos=(char*) buf+19;
  *pos--=0;
  *pos--= (char) ('0'+(char) (part2%10)); part2/=10; 
  *pos--= (char) ('0'+(char) (part2%10)); part3= (int) (part2 / 10);
  *pos--= ':';
  *pos--= (char) ('0'+(char) (part3%10)); part3/=10;
  *pos--= (char) ('0'+(char) (part3%10)); part3/=10;
  *pos--= ':';
  *pos--= (char) ('0'+(char) (part3%10)); part3/=10;
  *pos--= (char) ('0'+(char) part3);
  *pos--= ' ';
  *pos--= (char) ('0'+(char) (part1%10)); part1/=10;
  *pos--= (char) ('0'+(char) (part1%10)); part1/=10;
  *pos--= '-';
  *pos--= (char) ('0'+(char) (part1%10)); part1/=10;
  *pos--= (char) ('0'+(char) (part1%10)); part3= (int) (part1/10);
  *pos--= '-';
  *pos--= (char) ('0'+(char) (part3%10)); part3/=10;
  *pos--= (char) ('0'+(char) (part3%10)); part3/=10;
  *pos--= (char) ('0'+(char) (part3%10)); part3/=10;
  *pos=(char) ('0'+(char) part3);
  return buf;    
}


char * MySQL::result(pool *p, const NdbRecAttr &rec) {

  switch(rec.getType()) {
    
    case NdbDictionary::Column::Int:
      return ap_psprintf(p,"%d", (int)  rec.int32_value()); 
      
    case NdbDictionary::Column::Bit:
    case NdbDictionary::Column::Unsigned:
    case NdbDictionary::Column::Timestamp:
      return ap_psprintf(p,"%u", (unsigned int) rec.u_32_value());
      
    case NdbDictionary::Column::Varchar:
    case NdbDictionary::Column::Varbinary:
      return MySQL::String(p,rec,char_var);
    
    case NdbDictionary::Column::Char:
    case NdbDictionary::Column::Binary:
      return MySQL::String(p,rec,char_fixed);
      
    case NdbDictionary::Column::Longvarchar:
      return MySQL::String(p,rec,char_longvar);
      
    case NdbDictionary::Column::Float:
      return ap_psprintf(p,"%G", (double) rec.float_value());
      
    case NdbDictionary::Column::Double:
      return ap_psprintf(p,"%G", (double) rec.double_value());
      
    case NdbDictionary::Column::Date:
      return MySQL::Date(p,rec);
      
    case NdbDictionary::Column::Time:
      return MySQL::Time(p,rec);
      
    case NdbDictionary::Column::Bigunsigned:
      return ap_psprintf(p,"%llu", (unsigned long long) rec.u_64_value());
      
    case NdbDictionary::Column::Smallunsigned:
      return ap_psprintf(p,"%u", (short) rec.u_short_value());
      
    case NdbDictionary::Column::Tinyunsigned:
      return ap_psprintf(p,"%u", (int) rec.u_char_value());
      
    case NdbDictionary::Column::Bigint:
      return ap_psprintf(p,"%lli", (long long int) rec.int64_value());
      
    case NdbDictionary::Column::Smallint:
      return ap_psprintf(p,"%d", (short) rec.short_value());
      
    case NdbDictionary::Column::Tinyint:
      return ap_psprintf(p,"%d", (int) rec.char_value());
      
    case NdbDictionary::Column::Year:
      return ap_psprintf(p, "%04d", 1900 + rec.u_char_value());
      
    case NdbDictionary::Column::Datetime:
      return MySQL::Datetime(p,rec);
      
    case NdbDictionary::Column::Text:
    case NdbDictionary::Column::Blob:
    case NdbDictionary::Column::Olddecimal:
    case NdbDictionary::Column::Olddecimalunsigned:
    case NdbDictionary::Column::Decimal:
    case NdbDictionary::Column::Decimalunsigned:
    default:
      return 0;
  
  }
}


// MySQL::String
// Derived from ndbrecattr_print_string in NdbRecAttr.cpp
// BUG: ap_pstrndup() might not be safe for UTF-8 !?!?!?
// Correct behavior here depends on MySQL version 
// and on Column.StorageType 

char * MySQL::String(pool *p, const NdbRecAttr &rec, 
                     enum ndb_string_packing packing) {
  unsigned sz;
  char *ref;


  switch(packing) {
    case char_fixed:
      sz = Attr_Size(rec);
      ref =rec.aRef();
      break;
    case char_var:
      sz = *(const unsigned char*) rec.aRef();
      ref = rec.aRef() + 1;
      break;
    case char_longvar:
      sz = uint2korr(rec.aRef());
      ref = rec.aRef() + 2;
      break;
   }
  
  for (int i=sz-1; i >= 0; i--) {
    if (ref[i] == 0) sz--;
    else break;
  }
  
  return ap_pstrndup(p, ref, sz);  
}


/* MySQL::value:
   take an ASCII value "val", and encode it properly for NDB so that it can be 
   stored in (or compared against) column "col"
*/
mvalue MySQL::value(pool *p, const NdbDictionary::Column *col, const char *val) 
{

  mvalue m;
  char len_header[3] = { 0 , 0 , 0 };
  
  switch(col->getType()) {
    case NdbDictionary::Column::Int:
      m.use_value = use_signed;
      m.u.val_signed = atoi(val);
      return m;
          
    case NdbDictionary::Column::Varchar:
      len_header[0] = (char) strlen(val);
      m.u.val_char = ap_pstrcat(p, len_header, val, 0); 
      m.use_value = use_char; 
      return m;
      
    case NdbDictionary::Column::Char:
      // ap_pcalloc() fills the buffer with 0s
      m.u.val_char = (char *) ap_pcalloc(p,col->getLength());
      strcpy(m.u.val_char, val);
      m.use_value = use_char;
      return m;
      
    case NdbDictionary::Column::Unsigned:
    case NdbDictionary::Column::Bit:
    case NdbDictionary::Column::Timestamp:
      m.use_value = use_unsigned;
      m.u.val_signed = strtoul(val,0,0);
      return m;
      
    case NdbDictionary::Column::Float:
      m.use_value = use_float;
      m.u.val_float = atof(val);
      return m;
      
    case NdbDictionary::Column::Double:
      m.use_value = use_double;
      m.u.val_double = strtod(val,0);
      return m;
      
    case NdbDictionary::Column::Bigint:
      m.use_value = use_64;
      m.u.val_64 = strtoll(val,0,0);
      return m;

    case NdbDictionary::Column::Bigunsigned:
      m.use_value = use_unsigned_64;
      m.u.val_unsigned_64 = strtoull(val,0,0);
      return m;
      
    /* not implemented */

    case NdbDictionary::Column::Smallint:
    case NdbDictionary::Column::Tinyint:
    case NdbDictionary::Column::Smallunsigned:
    case NdbDictionary::Column::Tinyunsigned:
    case NdbDictionary::Column::Longvarchar:
      // is like varchar but with a two-byte length (what byte order??)
    case NdbDictionary::Column::Date:
      // can use http or rfc822 dates via apache utility functions
    case NdbDictionary::Column::Time:
    case NdbDictionary::Column::Year:
      // is a one-byte character + 1900
    case NdbDictionary::Column::Text:
    case NdbDictionary::Column::Datetime:
    case NdbDictionary::Column::Blob:
    case NdbDictionary::Column::Varbinary:
    case NdbDictionary::Column::Binary:      
    case NdbDictionary::Column::Olddecimal:
    case NdbDictionary::Column::Olddecimalunsigned:
    case NdbDictionary::Column::Decimal:
    case NdbDictionary::Column::Decimalunsigned:
    case NdbDictionary::Column::Mediumint:
    default:
      m.use_value = can_not_use;
      m.u.err_col = col;
      return m;
  }
}
