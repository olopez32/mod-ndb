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

/* There are two versions of Ndb::getAutoIncrementValue().
   If this version of mysql uses the old API, wrap the new one
   around the call. 
*/

#if ( MYSQL_VERSION_ID > 50000 && MYSQL_VERSION_ID < 50024 ) \
 || ( MYSQL_VERSION_ID > 50100 && MYSQL_VERSION_ID < 50112 )
#define AUTOINC_V1
#else
#define AUTOINC_V2
#endif


#ifdef AUTOINC_V1
inline int get_auto_inc_value(Ndb *ndb,
                              const NdbDictionary::Table *tab, 
                              Uint64 &next_value, Uint32 prefetch) {
  next_value = ndb->getAutoIncrementValue(tab, prefetch);
  return next_value ? 0 : -1 ;
}
#else 
inline int get_auto_inc_value(Ndb *ndb,
                              const NdbDictionary::Table *tab, 
                              Uint64 &next_value, Uint32 prefetch) {
  return ndb->getAutoIncrementValue(tab, next_value, prefetch);
}
#endif
