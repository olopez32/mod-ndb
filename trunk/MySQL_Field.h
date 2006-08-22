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


enum ndb_string_packing {
  char_fixed,
  char_var,
  char_longvar
};  

enum mvalue_use {
  can_not_use, use_char,
  use_signed, use_unsigned, 
  use_64, use_unsigned_64,
  use_float, use_double
}; 

struct mvalue {
  mvalue_use use_value;
  union {
    const char *        val_const_char;
    char *              val_char;
    int                 val_signed;
    unsigned int        val_unsigned;
    long long           val_64;
    unsigned long long  val_unsigned_64;
    float               val_float;
    double              val_double;
    const NdbDictionary::Column * err_col;
  } u;
};
typedef struct mvalue mvalue;

namespace MySQL {
  char *Time(pool *p, const NdbRecAttr &rec);
  char *Date(pool *p, const NdbRecAttr &rec);
  char *Datetime(pool *p, const NdbRecAttr &rec);
  char *String(pool *p, const NdbRecAttr &rec, enum ndb_string_packing packing);  
  char *result(pool *p,  const NdbRecAttr &rec);
  mvalue value(pool *p, const NdbDictionary::Column *col, const char *val);
};



