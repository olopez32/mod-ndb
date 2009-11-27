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

/* MySQL_value.cc : encode values according to MySQL data types 
   for storing them in the database.
*/

#define DECIMAL_BUFF 9 


/** store24() is the macro for storing a 3-byte value (e.g. a time or date). 
 *  In ha_ndbcluster.cc, low_byte_first() is FALSE on WORDS_BIGENDIAN machines
 *  and TRUE on other (e.g. intel) machines.  This is a key to understanding
 *  how NDB actually stores values from sql/field.cc Field_xxx::store()
 */
#ifdef __i386__
#define store24(A,V) A = V
#else
#define store24(A,V) int3store(& A, V)
#endif

/** ndbapi_bit_flip is used for NdbDictionary::Column::Bit ,
 *  which the NDB API presents as a "little endian" array of two 32-bit ints
 */
#ifdef __i386__
#define ndbapi_bit_flip(x) x
#else
uint64_t flip64(uint64_t);
#define ndbapi_bit_flip(x) flip64(x)
#endif


enum mvalue_use {
  err_bad_user_value, err_bad_data_type, err_bad_column, 
  must_use_binary,
  mvalue_is_good,  /* everything greater than this is OK */
  use_char,
  use_signed, use_unsigned, 
  use_64, use_unsigned_64,
  use_float, use_double,
  use_interpreted, use_null,
  use_autoinc, use_blob
}; 

enum mvalue_interpreted {
  not_interpreted = 0,
  is_increment, is_decrement
};

struct mvalue {
  const NdbDictionary::Column *ndb_column;
  union {
    const char *        val_const_char;
    char *              val_char;
    int                 val_signed;
    unsigned int        val_unsigned;
    time_t              val_time;
    long long           val_64;
    unsigned long long  val_unsigned_64;
    float               val_float;
    double              val_double;
    char                val_8;
    unsigned char       val_unsigned_8;
    int16_t             val_16;
    Uint16              val_unsigned_16;
    NdbBlob *           blob_handle;
  } u;
  size_t len;
  len_string *binary_info;
  Uint32 col_len;
  mvalue_use use_value;
  mvalue_interpreted interpreted;
  bool over;        /* overflow indicator */
};
typedef struct mvalue mvalue;

namespace MySQL
{ 
  void value(mvalue &, ap_pool *, const NdbDictionary::Column *, const char *);
  void binary_value(mvalue &, ap_pool *, const NdbDictionary::Column *, len_string *);
};



/* --------------------------------------------------------------------
   == DECIMAL support that is not in MySQL's public include files ==
 This is only needed in MySQL_Field.cc and only if the "real" decimal.h
 has not been included.  "#ifdef _mysql_h" is here because mysql.h is required 
 by these prorotypes (for e.g. my_bool).  MySQL_Field.cc is the 
 only source file that includes mysql.h, but this file (MySQL_value.h) is 
 included by mod_ndb.h and thus by every source file.
*/

#ifdef _mysql_h
#ifndef _decimal_h
typedef int32 decimal_digit_t;

typedef struct st_decimal_t {
  int intg, frac, len;
  my_bool sign;
  decimal_digit_t *buf;
} decimal_t;

#define string2decimal(A,B,C) internal_str2dec((A), (B), (C), 0)
#define decimal_string_size(dec) (((dec)->intg ? (dec)->intg : 1) + \
				  (dec)->frac + ((dec)->frac > 0) + 2)
extern "C" {
  int decimal_size(int precision, int scale);
  int decimal_bin_size(int precision, int scale);                                  
  int decimal2bin(decimal_t *from, char *to, int precision, int scale);
  int bin2decimal(char *from, decimal_t *to, int precision, int scale);
  int decimal2string(decimal_t *from, char *to, int *to_len,
                     int fixed_precision, int fixed_decimals,
                     char filler);
  int internal_str2dec(const char *from, decimal_t *to, char **end,
                       my_bool fixed);
}
#endif
#endif
