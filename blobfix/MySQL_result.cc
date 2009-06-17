/* Copyright (C) 2006 - 2008 MySQL AB
 2008 - 2009 Sun Microsystems
 
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

/* Class MySQL::result 
   Fetch values from NDB, which are encoded there as MySQL datatypes.
   Plus helper functions for MySQL datatypes (string, date/time, and decimal).
   Plus "BlobHook" callback function
*/

#include <strings.h>
#include "my_global.h"
#include "mysql.h"
#include "mysql_time.h"
#include "NdbApi.hpp"
#include "httpd.h"
#include "http_config.h"
#include "mod_ndb_compat.h"
#include "result_buffer.h"
#include "output_format.h"

#include "MySQL_value.h"
#include "MySQL_result.h"

/*  BlobHook() is a callback function; we register it with the NDB API 
    in the MySQL::result constructor.  It is called from the API when 
    a prepared blob becomes active.
 */
int BlobHook(NdbBlob *blob, void *v) {
  MySQL::result *result = (MySQL::result *) v;
  return result->activateBlob();
}


namespace {  // i.e. "static"
  inline void factor_HHMMSS(MYSQL_TIME *tm, int int_time) {
    if(int_time < 0) {
      tm->neg = true; int_time = - int_time;
    }
    tm->hour = int_time/10000;
    tm->minute  = int_time/100 % 100;
    tm->second  = int_time % 100;  
  }

  inline void factor_YYYYMMDD(MYSQL_TIME *tm, int int_date) {
    tm->year = int_date/10000 % 10000;
    tm->month  = int_date/100 % 100;
    tm->day = int_date % 100;  
  }
}

/* result_buffer::out(decimal_t *) is defined here
   because this file includes the full set of MySQL headers
*/

void result_buffer::out(decimal_t *decimal) {
  int to_len = decimal_string_size(decimal);
  this->prepare(to_len);
  decimal2string(decimal, buff + sz, &to_len, 0, 0, 0);
  sz += to_len; // to_len has been reset to the length actually written 
}


namespace MySQL {

  /* These private functions are defined here so that 
     the rest of the source does not need to include a 
     full set of MySQL header files
  */
  void field_to_tm(MYSQL_TIME *, const NdbRecAttr &);
  void Decimal(result_buffer &, const NdbRecAttr &);
  void String(result_buffer &, const NdbRecAttr &,
              enum ndb_string_packing, const char **);   
  void escape_string(char *, unsigned, result_buffer &, const char **);
  
  
  /* Implementation of class MySQL::result */
  
  result::result(NdbOperation *op, const NdbDictionary::Column *col) : 
        contents(0) , blob(0) , _RecAttr(0) , _col(col)  
  {    
    type = col->getType();
    
    if((type == NdbDictionary::Column::Blob) || 
       (type == NdbDictionary::Column::Text)) { 
      
      blob = op->getBlobHandle(col->getColumnNo()); 
      blob->setActiveHook(BlobHook, (void *) this);
      contents = new result_buffer();
      contents->init(0, 8192);
    }
    else
      _RecAttr = op->getValue(col, 0);
  }
  
  result::~result() {
    if(contents) delete contents;
  }
  
  
  int result::activateBlob() {    
    if(isNull()) return 0;
    contents->read_blob(blob);  
    return 0;
  }
  
  
  bool result::BLOBisNull() {
    int is_null = 0;
    blob->getNull(is_null);
    assert(is_null != -1);
    return is_null ? true : false;
  }
  
  
  void result::out(result_buffer &rbuf, const char **escapes) {
    MYSQL_TIME tm;
    NdbRecAttr &rec = *_RecAttr;
    
    switch(type) {
        
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
        field_to_tm(&tm, rec);
        return rbuf.out("%04d-%02d-%02d",tm.year, tm.month, tm.day);
        
      case NdbDictionary::Column::Time:
        field_to_tm(&tm, rec);
        return rbuf.out("%s%02d:%02d:%02d", tm.neg ? "-" : "" ,
                        tm.hour, tm.minute, tm.second);
        
      case NdbDictionary::Column::Bigunsigned:
        return rbuf.out("%llu", rec.u_64_value()); 
        
      case NdbDictionary::Column::Smallunsigned:
        return rbuf.out("%hu", (short) rec.u_short_value());
        
      case NdbDictionary::Column::Tinyunsigned:
        return rbuf.out("%u", (int) rec.u_char_value());
        
      case NdbDictionary::Column::Bigint:
        return rbuf.out("%lld", rec.int64_value());
        
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
        field_to_tm(&tm, rec);
        return rbuf.out("%04d-%02d-%02d %02d:%02d:%02d", tm.year, tm.month, 
                        tm.day, tm.hour, tm.minute, tm.second);
        
      case NdbDictionary::Column::Decimal:
      case NdbDictionary::Column::Decimalunsigned:
        return MySQL::Decimal(rbuf,rec);
        
      case NdbDictionary::Column::Text:
        if(escapes) 
          escape_string(contents->buff, contents->sz, rbuf, escapes);
        else
          rbuf.out(contents->sz, contents->buff);
        return;
        
      case NdbDictionary::Column::Blob:
        if(escapes) rbuf.out("++ CANNOT ESCAPE BLOB ++");
        else rbuf.out(contents->sz, contents->buff);
        return;
        
      case NdbDictionary::Column::Bit:
      case NdbDictionary::Column::Olddecimal:
      case NdbDictionary::Column::Olddecimalunsigned:
      default:
        return;        
    }
  }
    
  /* MySQL:: data type helper funtions */
  
  void escape_string(char *ref, unsigned sz, result_buffer &rbuf, 
                             const char **escapes) {
    size_t escaped_size = 0;
    
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
      const char *esc = escapes[c];
      if(esc) {
        for(char j = 1 ; j <= esc[0]; j++) 
          rbuf.putc(esc[j]);
      }
      else rbuf.putc(c);
    }
  }

    
  void String(result_buffer &rbuf, const NdbRecAttr &rec,
              enum ndb_string_packing packing, const char **escapes) {
    unsigned sz = 0;
    char *ref = 0;
    
    switch(packing) {
      case char_fixed:
        sz = rec.get_size_in_bytes();
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
    
    if(escapes) 
      escape_string(ref, sz, rbuf, escapes);
    else 
      rbuf.out(sz, ref);
  }


  void field_to_tm(MYSQL_TIME *tm, const NdbRecAttr &rec) {
    int int_date = -1, int_time = -99;
    unsigned long long datetime;
    
    bzero (tm, sizeof(MYSQL_TIME));
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
        tm->day = (int_date & 31);      // five bits
        tm->month  = (int_date >> 5 & 15); // four bits
        tm->year = (int_date >> 9);
        return;
      default:
        assert(0);
    }
    if(int_time != -99)factor_HHMMSS(tm, int_time);
    if(int_date != -1) factor_YYYYMMDD(tm, int_date);
  }


  void Decimal(result_buffer &rbuf, const NdbRecAttr &rec) {
    decimal_digit_t digits[DECIMAL_BUFF]; // (an array of ints, not base-10 digits)
    decimal_t dec = { 0, 0, DECIMAL_BUFF, 0, digits };
    
    int prec  = rec.getColumn()->getPrecision();
    int scale = rec.getColumn()->getScale();  
    bin2decimal(rec.aRef(), &dec, prec, scale);
    rbuf.out(&dec);
    return;
  }  

} // Namespace

