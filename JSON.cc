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

const char *escape_leaning_toothpicks[128];
const char *escape_xml_entities[128];

void initialize_escapes() {
  for(int i = 0 ; i < 128 ; i++) {
    escape_leaning_toothpicks[i] = 0;
    escape_xml_entities[i] = 0;
  }
  
  escape_leaning_toothpicks[static_cast<int>('\\')] = "\x02" "\\" "\\";
  escape_leaning_toothpicks[static_cast<int>('\"')] = "\x02" "\\" "\"";
  
  escape_xml_entities[static_cast<int>('<')] = "\x04" "&lt;";
  escape_xml_entities[static_cast<int>('>')] = "\x04" "&gt;";
  escape_xml_entities[static_cast<int>('&')] = "\x05" "&amp;";
  escape_xml_entities[static_cast<int>('\"')]= "\x06" "&quot;";
}

 
void JSON::put_value(result_buffer &rbuf,const NdbRecAttr &rec){

  if (rec.isNULL())
     return rbuf.out(4,"null");

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
        rbuf.out(1,"\"");
        MySQL::result(rbuf, rec, escape_leaning_toothpicks);
        rbuf.out(1,"\"");
        return;

    default:      
        return MySQL::result(rbuf, rec, escape_leaning_toothpicks);
  }
  rbuf.out(9,"\"unknown\"");
}


void XML::put_member(result_buffer &rbuf, const NdbRecAttr &rec)
{
  rbuf.out("<Attr name=\"%s\" ", rec.getColumn()->getName());
  if(rec.isNULL()) rbuf.out(10,"isNull=\"1\"");
  else {
    rbuf.out("value=\"");
    MySQL::result(rbuf, rec, escape_xml_entities); 
    rbuf.out(1,"\""); 
  }
  rbuf.out(3," />");
}

