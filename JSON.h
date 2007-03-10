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




class JSON {
  public:
    inline static void new_array(result_buffer &rbuf)  { rbuf.out(2,"[\n"); }
    inline static void end_array(result_buffer &rbuf)  { rbuf.out(2,"\n]"); }
    inline static void new_object(result_buffer &rbuf) { rbuf.out(3," { "); }
    inline static void end_object(result_buffer &rbuf) { rbuf.out(2," }") ; }
    inline static void delimiter(result_buffer &rbuf)  { rbuf.out(3," , "); }

    static void put_value(result_buffer &, const NdbRecAttr &);

    inline static void put_member(result_buffer &rbuf, const NdbRecAttr &rec) 
    {
      rbuf.out("\"%s\":", rec.getColumn()->getName());
      JSON::put_value(rbuf, rec);
    }
};


class XML {
  public:
    inline static void new_array(result_buffer &rbuf)  
      { rbuf.out(10,"<NDBScan>\n");   }
    inline static void end_array(result_buffer &rbuf)  
      { rbuf.out(11,"\n</NDBScan>"); }
    inline static void new_object(result_buffer &rbuf)
      { rbuf.out(12," <NDBTuple> "); }
    inline static void end_object(result_buffer &rbuf)
      { rbuf.out(12," </NDBTuple>"); }
    inline static void delimiter(result_buffer &rbuf) 
      { rbuf.out(3,"\n  "); }
    
    static void put_member(result_buffer &rbuf, const NdbRecAttr &rec);
};
