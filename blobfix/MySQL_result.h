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
*/
 

enum ndb_string_packing {
  char_fixed,
  char_var,
  char_longvar
};  


namespace MySQL {
  class result {
  public:
    result(NdbOperation *, const NdbDictionary::Column *);
    ~result();
    const NdbDictionary::Column *getColumn()  { return _col;    };
    bool isNull() { return _RecAttr ? _RecAttr->isNULL() : BLOBisNull(); };  
    int activateBlob();
    void out(result_buffer &, const char **);

    result_buffer *contents;
    
  private:
    NdbDictionary::Column::Type type;
    NdbBlob * blob;  
    NdbRecAttr * _RecAttr;
    const NdbDictionary::Column *_col;

    bool BLOBisNull();
  };
}