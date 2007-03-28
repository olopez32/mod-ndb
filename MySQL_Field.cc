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
   
   This file is based largely on code from sql/field.cc.
   It has a different set of includes than other mod_ndb source files:
   it uses my_global.h to get MySQL's typedefs and macros like "sint3korr",
   but it does not include mod_ndb.h (because you can get trouble if you try 
   to combine mysql headers and apache headers in a single source file).
*/

#include "mysql_version.h"
#include "my_global.h"
#include "mysql.h"
#include "NdbApi.hpp"
#include "httpd.h"
#include "http_config.h"
#include "mod_ndb_compat.h"
#include "output_format.h"
#include "result_buffer.h"
#include "MySQL_Field.h"

// Apache disabled this
#undef strtoul

// The NdbRecAttr interface changed between MySQL 5.0 and 5.1
#if MYSQL_VERSION_ID < 50100
#define Attr_Size(r) r.arraySize()
#else 
#define Attr_Size(r) r.get_size_in_bytes()
#endif

// From apache_time.cc:
extern void parse_http_date(time_struct *tm, const char *string);

namespace MySQL {
  /* Prototypes of private functions implemented here: */
  void field_to_tm(time_struct *tm, const NdbRecAttr &rec);
  void Decimal(result_buffer &rbuf, const NdbRecAttr &rec);
  void String(result_buffer &rbuf, const NdbRecAttr &rec, 
              enum ndb_string_packing packing, const char **escapes); 
}

inline void factor_HHMMSS(time_struct *tm, int int_time) {
  tm->tm_hour = int_time/10000;
  tm->tm_min  = int_time/100 % 100;
  tm->tm_sec  = int_time % 100;  
}

inline void factor_YYYYMMDD(time_struct *tm, int int_date) {
  tm->tm_year = int_date/10000 % 10000;
  tm->tm_mon  = int_date/100 % 100;
  tm->tm_mday = int_date % 100;  
}

void MySQL::field_to_tm(time_struct *tm, const NdbRecAttr &rec) {
  int int_date = -1, int_time = -1;
  unsigned long long datetime;

  bzero (tm, sizeof(time_struct));
  switch(rec.getType()) {
    case NdbDictionary::Column::Datetime :
      datetime = rec.u_64_value();
      int_date = datetime / 1000000;
      int_time = datetime - (unsigned long long) int_date * 1000000;
      break;
    case NdbDictionary::Column::Time :
      int_time = sint3korr(rec.aRef());
      break;
    case NdbDictionary::Column::Date :
      int_date = uint3korr(rec.aRef());
      tm->tm_mday = (int_date & 31);      // five bits
      tm->tm_mon  = (int_date >> 5 & 15); // four bits
      tm->tm_year = (int_date >> 9);
      return;
    default:
      assert(0);
  }
  if(int_time != -1) factor_HHMMSS(tm, int_time);
  if(int_date != -1) factor_YYYYMMDD(tm, int_date);
}


void MySQL::Decimal(result_buffer &rbuf, const NdbRecAttr &rec) {
  return;
}

void MySQL::result(result_buffer &rbuf, const NdbRecAttr &rec,
                   const char **escapes) {
  time_struct tm;

  switch(rec.getType()) {
    
    case NdbDictionary::Column::Int:
      return rbuf.out("%d", (int)  rec.int32_value()); 
      
    case NdbDictionary::Column::Unsigned:
    case NdbDictionary::Column::Timestamp:
      return rbuf.out("%u", (unsigned int) rec.u_32_value());
      
    case NdbDictionary::Column::Varchar:
    case NdbDictionary::Column::Varbinary:
      return MySQL::String(rbuf, rec, char_var, escapes);
    
    case NdbDictionary::Column::Char:
    case NdbDictionary::Column::Binary:
      return MySQL::String(rbuf, rec, char_fixed, escapes);
      
    case NdbDictionary::Column::Longvarchar:
      return MySQL::String(rbuf, rec, char_longvar, escapes);
      
    case NdbDictionary::Column::Float:
      return rbuf.out("%G", (double) rec.float_value());
      
    case NdbDictionary::Column::Double:
      return rbuf.out("%G", (double) rec.double_value());
      
    case NdbDictionary::Column::Date:
      MySQL::field_to_tm(&tm, rec);
      return rbuf.out("%04d-%02d-%02d",tm.tm_year, tm.tm_mon, tm.tm_mday);
      
    case NdbDictionary::Column::Time:
      MySQL::field_to_tm(&tm, rec);
      return rbuf.out("%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
      
    case NdbDictionary::Column::Bigunsigned:
      return rbuf.out("%llu", rec.u_64_value()); 
      
    case NdbDictionary::Column::Smallunsigned:
      return rbuf.out("%hu", (short) rec.u_short_value());
      
    case NdbDictionary::Column::Tinyunsigned:
      return rbuf.out("%u", (int) rec.u_char_value());
      
    case NdbDictionary::Column::Bigint:
      return rbuf.out("%ll", rec.int64_value());
      
    case NdbDictionary::Column::Smallint:
      return rbuf.out("%hd", (short) rec.short_value());
      
    case NdbDictionary::Column::Tinyint:
      return rbuf.out("%d", (int) rec.char_value());

    case NdbDictionary::Column::Mediumint:
      return rbuf.out("%d", sint3korr(rec.aRef()));

    case NdbDictionary::Column::Mediumunsigned:
      return rbuf.out("%d", uint3korr(rec.aRef()));
    
    case NdbDictionary::Column::Year:
      return rbuf.out("%04d", 1900 + rec.u_char_value());
      
    case NdbDictionary::Column::Datetime:
      MySQL::field_to_tm(&tm, rec);
      return rbuf.datetime(&tm);
      
    case NdbDictionary::Column::Decimal:
    case NdbDictionary::Column::Decimalunsigned:
      return MySQL::Decimal(rbuf,rec);

    case NdbDictionary::Column::Bit:
    case NdbDictionary::Column::Text:
    case NdbDictionary::Column::Blob:
    case NdbDictionary::Column::Olddecimal:
    case NdbDictionary::Column::Olddecimalunsigned:
    default:
      return;
  
  }
}


// MySQL::String
// Derived from ndbrecattr_print_string in NdbRecAttr.cpp
// Correct behavior here depends on MySQL version 
// and on Column.StorageType 

void MySQL::String(result_buffer &rbuf, const NdbRecAttr &rec, 
                     enum ndb_string_packing packing,
                     const char **escapes) {
  unsigned sz = 0;
  char *ref = 0;

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
    default:
      assert(0);
   }
  
  /* If the string is null-padded at the end, don't count those in the length*/
  for(int i=sz-1; i >= 0; i--) {
    if (ref[i] == 0) sz--;
    else break;
  }
  
  if(escapes) {
    unsigned escaped_size = 0;

    /* How long will the string be when it is escaped? */
    for(unsigned int i = 0; i < sz ; i++) {
      const char *esc = escapes[ref[i]];
      if(esc) escaped_size += esc[0];
      else escaped_size++;
    }
    
    /* Prepare the buffer.  This returns false only after a malloc error. */
    if(!rbuf.prepare(escaped_size)) return;

    /* Now copy the string from NDB into the result buffer,
       encoded appropriately according to the escapes 
     */
    for(unsigned int i = 0; i < sz ; i++) {
      const unsigned char c = ref[i];
      if(c < 128) {
        const char *esc = escapes[c];
        if(esc) {
          for(char j = 1 ; j <= esc[0]; j++) 
            rbuf.putc(esc[j]);
          continue;
        }
      }
      rbuf.putc(c);
    }
  }
  else                /* alternate code path -- no escapes */   
    rbuf.out(sz, ref);
}


/* MySQL::value:
   take an ASCII value "val", and encode it properly for NDB so that it can be 
   stored in (or compared against) column "col"
*/
void MySQL::value(mvalue &m, ap_pool *p, 
                  const NdbDictionary::Column *col, const char *val) 
{
  const unsigned short s_lo = 255;
  const unsigned short s_hi = 65535 ^ 255; 
  unsigned char len;
  unsigned short s_len;
  unsigned int l_len;
  char *s, *q;
  bool is_char_col = 
    ( (col->getType() == NdbDictionary::Column::Varchar) ||
      (col->getType() == NdbDictionary::Column::Longvarchar) ||
      (col->getType() == NdbDictionary::Column::Char));        
  int aux_int;

  m.ndb_column = col;
  
  /* String columns */
  if(is_char_col) {
  
    if(! val) { /* null pointer */
      m.use_value = use_null;
      m.u.val_64 = 0;
      return;
    }
  
    switch(col->getType()) {
      /* "If the attribute is of variable size, its value must start with
      1 or 2 little-endian length bytes"   [ i.e. LSB first ]*/
      
      case NdbDictionary::Column::Varchar:      
        m.len = len = (unsigned char) strlen(val);
        if(len > col->getLength()) len = (unsigned char) col->getLength();
          m.u.val_char = (char *) ap_palloc(p, len + 2);
        * m.u.val_char = len;
        ap_cpystrn(m.u.val_char+1, val, len+1);
        m.use_value = use_char; 
        return;
        
      case NdbDictionary::Column::Longvarchar:
        m.len = s_len = strlen(val);
        if(s_len > col->getLength()) s_len = col->getLength();
          m.u.val_char = (char *) ap_palloc(p, s_len + 3);
        * m.u.val_char     = (char) (s_len & s_lo);
        * (m.u.val_char+1) = (char) (s_len & s_hi);
        ap_cpystrn(m.u.val_char+2, val, s_len+1);
        m.use_value = use_char; 
        return;
        
      case NdbDictionary::Column::Char:
        // Copy the value into the buffer, then right-pad with spaces
        m.len = l_len = strlen(val);
        if(l_len > (unsigned) col->getLength()) l_len = col->getLength();
          m.u.val_char = (char *) ap_palloc(p,col->getLength() + 1);
        strcpy(m.u.val_char, val);
        s = m.u.val_char + l_len;
        q = m.u.val_char + col->getLength();
        while (s < q) *s++ = ' ';
          *q = 0;      
        m.use_value = use_char;
        return;
        
      default:
        assert(0);
    }
  }

  /* Date columns */
  if ( (col->getType() == NdbDictionary::Column::Time) ||
       (col->getType() == NdbDictionary::Column::Date) ||
       (col->getType() == NdbDictionary::Column::Datetime)) {
    time_struct tm;

    if(! val) { /* null pointer */
      m.use_value = use_null;
      m.u.val_64 = 0;
      return;
    }
    bzero(&tm, sizeof(time_struct));

    if(col->getType() == NdbDictionary::Column::Datetime) {
      parse_http_date(&tm, val);
      m.use_value = use_unsigned_64;
      m.u.val_unsigned_64 = 
        tm.tm_sec + (tm.tm_min * 100) + (tm.tm_hour * 10000) +
        (tm.tm_mday * 1000000) + (tm.tm_mon * 100000000LL) + 
        (tm.tm_year * 10000000000LL);
      return;
    }
    else {
      char strbuf[40];
      char *buf = strbuf;
      const char *c = val;
      if(*c == '-' || *c == '+') *buf++ = *c++;
      for(register int i = 0 ; i < 38 && *c != 0 ; c++, i++ ) 
        if(! (*c == ':' || *c == '-' || *c == '/' || *c == ' '))
          *buf++ = *c; 
      *buf = 0;
      aux_int = strtol(buf, 0, 10);

      if(col->getType() == NdbDictionary::Column::Time) {
        m.use_value = use_signed;
        m.u.val_signed = aux_int;
      }
      else {  /* NdbDictionary::Column::Date */
        factor_YYYYMMDD(&tm, aux_int);
        aux_int = (tm.tm_year << 9) | (tm.tm_year << 5) | tm.tm_mday;
        m.use_value = use_signed;
        store24(m.u.val_signed, aux_int);
      }
      return;
    }
  }

  /* Numeric columns */  
  if(! val) {  // You can't do anything with a null pointer
    m.use_value = can_not_use;
    return;
  }

  /* Dynamic values @++. @--. @null, @time, @autoinc */
  if(*val == '@') {
    if(!strcmp(val,"@null")) {
      m.use_value = use_null;
      m.u.val_64 = 0;
      return;
    }
    if(!strcmp(val,"@++")) {
      m.use_value = use_interpreted;
      m.interpreted = is_increment;
      return;
    }
    if(!strcmp(val,"@--")) {
      m.use_value = use_interpreted;
      m.interpreted = is_decrement;
      return;
    }
    if(!strcmp(val,"@time")) {
      m.use_value = use_unsigned;
      time(& m.u.val_time);
      return;
    }
    if(!strcmp(val,"@autoinc")) {
      m.use_value = use_autoinc;
      if(col->getType() == NdbDictionary::Column::Bigint 
       || col->getType() == NdbDictionary::Column::Bigunsigned)
        m.len = 8;
      else m.len = 4;
      return;
    }
  }
  
  switch(col->getType()) {
    case NdbDictionary::Column::Int:
      m.use_value = use_signed;
      m.u.val_signed = atoi(val);
      return;
      
    case NdbDictionary::Column::Unsigned:
    case NdbDictionary::Column::Bit:
    case NdbDictionary::Column::Timestamp:
      m.use_value = use_unsigned;
      m.u.val_unsigned = strtoul(val,0,0);
      return;
      
    case NdbDictionary::Column::Float:
      m.use_value = use_float;
      m.u.val_float = atof(val);
      return;
      
    case NdbDictionary::Column::Double:
      m.use_value = use_double;
      m.u.val_double = strtod(val,0);
      return;
      
    case NdbDictionary::Column::Bigint:
      m.use_value = use_64;
      m.u.val_64 = strtoll(val,0,0);
      return;
      
    case NdbDictionary::Column::Bigunsigned:
      m.use_value = use_unsigned_64;
      m.u.val_unsigned_64 = strtoull(val,0,0);
      return;

    /* Tiny, small, and medium types -- be like mysql: 
        on overflow, put in the highest allowed value */
    case NdbDictionary::Column::Tinyint:
      m.use_value = use_signed;
      aux_int = strtol(val,0,0);
      if(aux_int < -128) aux_int = -128 , m.over = 1;
      else if( aux_int > 127) aux_int = 127, m.over = 1;
      m.u.val_8 = (char) aux_int;
      return;

    case NdbDictionary::Column::Tinyunsigned:
      m.use_value = use_unsigned;
      aux_int = strtol(val,0,0);
      if(aux_int > 255) aux_int = 255 , m.over = 1;
      else if (aux_int < 0) aux_int = 0 , m.over = 1;
      m.u.val_unsigned_8 = (unsigned char) aux_int;
      return;

    case NdbDictionary::Column::Smallint:
      m.use_value = use_signed;
      aux_int = strtol(val,0,0);
      if(aux_int < -32768) aux_int = -32768 , m.over = 1;
      else if(aux_int > 32767)  aux_int = 32767 , m.over = 1;
      m.u.val_16 = (int16_t) aux_int;
      return;

    case NdbDictionary::Column::Smallunsigned:
      m.use_value = use_unsigned;
      aux_int = strtol(val,0,0);
      if(aux_int > 65535) aux_int = 65535 , m.over = 1;
      else if (aux_int < 0) aux_int = 0 , m.over = 1;
      m.u.val_unsigned_16 = (u_int16_t) aux_int;
      return;

    case NdbDictionary::Column::Mediumint:
      m.use_value = use_signed;
      aux_int = strtol(val,0,0);
      if(aux_int > 8388607) aux_int = 8388607 , m.over = 1;
      else if(aux_int < -8388608) aux_int = -8388608 , m.over = 1;
      store24(m.u.val_signed, aux_int);
      return;

    case NdbDictionary::Column::Mediumunsigned:
      m.use_value = use_unsigned;
      aux_int = strtol(val,0,0);
      if(aux_int > 16777215) aux_int = 16777215 , m.over = 1;
      else if (aux_int < 0) aux_int = 0 , m.over = 1;
      store24(m.u.val_unsigned, aux_int);
      return;

    case NdbDictionary::Column::Year:
      m.use_value = use_unsigned;
      aux_int = strtol(val,0,0) - 1900;      
      m.u.val_unsigned_8 = (unsigned char) aux_int;
      return;
            
    /* not implemented */

    case NdbDictionary::Column::Text:
    case NdbDictionary::Column::Blob:
    case NdbDictionary::Column::Varbinary:
    case NdbDictionary::Column::Binary:
    case NdbDictionary::Column::Olddecimal:
    case NdbDictionary::Column::Olddecimalunsigned:
      /* Olddecimal types are just strings.  But you cannot create old decimal
         columns with MySQL 5, so this is difficult to test. */
    case NdbDictionary::Column::Decimal:
    case NdbDictionary::Column::Decimalunsigned:
      /* Use Column::getPrecision() and getScale() to find the column's 
         dimensions, then use string2decimal() and decimal2bin().  
      */
    default:
      m.use_value = can_not_use;
      return;
  }
}
