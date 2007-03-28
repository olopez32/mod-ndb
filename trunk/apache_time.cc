/* Copyright (C) 2007 MySQL AB

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

#ifdef THIS_IS_APACHE2
typedef apr_time_exp_t time_struct;
void parse_http_date(time_struct *tm, const char *string) {
  apr_time_t timestamp = apr_date_parse_rfc(string);
  apr_time_exp_gmt(tm, timestamp);
}

void result_buffer::datetime(const time_struct *tm) {
  apr_time_t timestamp;
  apr_time_exp_get(&timestamp, (time_struct *) tm);
  apr_rfc822_date((buff + sz), timestamp);
  sz += (APR_RFC822_DATE_LEN - 1);
}

/* Apache 1.3 */
#else
typedef struct tm time_struct;
void parse_http_date(time_struct *tm, const char *string) {
  time_t timestamp = ap_parseHTTPdate(string);
  gmtime_r(&timestamp, tm);
}

void result_buffer::datetime(const time_struct *tm) {
  if(! this->prepare(32)) return;
  sz += strftime((buff + sz), 32, "%a %d %b %Y %T %Z", tm);
}
#endif
