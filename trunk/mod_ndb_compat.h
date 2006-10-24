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


/*  Apache Compatibility */

#ifdef STANDARD20_MODULE_STUFF
          /* Apache 2: */

#include <unistd.h>
#include "ap_compat.h"          
#include "ap_mpm.h"
#include "apr_strings.h"

typedef apr_table_t table;
typedef apr_pool_t  ap_pool;
typedef apr_array_header_t array_header;

#define ap_pcalloc apr_pcalloc
#define ap_make_array apr_array_make
#define XtOffsetOf APR_OFFSETOF

#define NOT_FOUND HTTP_NOT_FOUND
#define NOT_IMPLEMENTED HTTP_NOT_IMPLEMENTED

#define ap_table_get apr_table_get
#define ap_table_set apr_table_set
#define ap_table_unset apr_table_unset
#define ap_make_table apr_table_make
#define ap_table_merge apr_table_merge
#define ap_table_setn apr_table_setn
#define ap_clear_table apr_table_clear
#define ap_table_do apr_table_do

#define ap_push_array apr_array_push
#define ap_array_pstrcat apr_array_pstrcat

#define ap_pstrndup apr_pstrndup
#define ap_pstrdup apr_pstrdup
#define ap_psprintf apr_psprintf
#define ap_cpystrn apr_cpystrn
#define ap_palloc apr_palloc

#define ap_destroy_pool apr_pool_destroy

/* The timeout functions are gone, because of a new I/O model */
#define ap_hard_timeout(a,b) ;
#define ap_reset_timeout(a) ;
#define ap_kill_timeout(a) ;

/* Apache 2 logging defines */
#define my_ap_log_error(l,s,fmt,arg) ap_log_error(APLOG_MARK,l,0,s,fmt,arg);
#define log_err(s,txt) ap_log_error(APLOG_MARK,log::err,0,s,txt);
#define log_err2(s,txt,arg) ap_log_error(APLOG_MARK,log::err,0,s,txt,arg);
#define log_err3(s,fmt,a1,a2) ap_log_error(APLOG_MARK,log::err,0,s,fmt,a1,a2);
#define log_err4(s,fmt,a1,a2,a3) ap_log_error(APLOG_MARK,log::err,0,s,fmt,a1,a2,a3);
#define log_note(s,txt) ap_log_error(APLOG_MARK,log::warn,0,s,txt);
#define log_note2(s,txt,arg) ap_log_error(APLOG_MARK,log::warn,0,s,txt,arg);
#define log_note3(s,fmt,a1,a2) ap_log_error(APLOG_MARK,log::warn,0,s,fmt,a1,a2);

#ifdef MOD_NDB_DEBUG 
#define log_debug(s,txt,arg) ap_log_error(APLOG_MARK,log::debug,0,s,txt,arg);
#define log_debug3(s,txt,arg1,arg2) ap_log_error(APLOG_MARK,log::debug,0,s,txt,arg1,arg2);
#else
#define log_debug(s,txt,arg) 
#define log_debug3(s,txt,arg1,arg2)
#endif
/* end of logging defines */

#else 
          /* Apache 1.3: */

#define AP_MODULE_DECLARE_DATA MODULE_VAR_EXPORT

/* Apache 1.3 logging defines */
#define my_ap_log_error(l,s,fmt,arg) ap_log_error(APLOG_MARK,l,s,fmt,arg);
#define log_err(s,txt) ap_log_error(APLOG_MARK, log::err, s, txt);
#define log_err2(s,txt,arg) ap_log_error(APLOG_MARK, log::err, s,txt,arg);
#define log_err3(s,fmt,a1,a2) ap_log_error(APLOG_MARK,log::err,s,fmt,a1,a2);
#define log_err4(s,fmt,a1,a2,a3) ap_log_error(APLOG_MARK,log::err,s,fmt,a1,a2,a3);
#define log_note(s,txt) ap_log_error(APLOG_MARK, log::warn, s, txt);
#define log_note2(s,txt,arg) ap_log_error(APLOG_MARK, log::warn, s,txt,arg);
#define log_note3(s,fmt,a1,a2) ap_log_error(APLOG_MARK,log::warn,s,fmt,a1,a2);

#ifdef MOD_NDB_DEBUG 
#define log_debug(s,txt,arg) ap_log_error(APLOG_MARK, log::debug, s, txt, arg);
#define log_debug3(s,txt,arg1,arg2) ap_log_error(APLOG_MARK,log::debug,s,txt,arg1,arg2);
#else
#define log_debug(s,txt,arg) 
#define log_debug3(s,txt,arg1,arg2)
#endif
/* end of logging defines */

#endif
