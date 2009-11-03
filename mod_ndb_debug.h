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


#ifdef THIS_IS_APACHE2
                /* Apache 2 */

#ifdef MOD_NDB_DEBUG 
#define log_debug(s, ...) ap_log_error(APLOG_MARK,log::debug,0,s, __VA_ARGS__);
#else
#define log_debug(s, ...) 
#endif

#else
                /* Apache 1.3 */
#ifdef MOD_NDB_DEBUG 
#define log_debug(s, ... ) ap_log_error(APLOG_MARK, log::debug, s, __VA_ARGS__ );
#else
#define log_debug(s, ... ) 
#endif

#endif

/* DTRACE */
#ifdef MAC_DTRACE

#include "probes.h"

#endif


/* Code Coverage */
#ifdef MAC_DTRACE
#define _STRINGOF(x) #x
#define STRINGOF(x) _STRINGOF(x)
#define COV_point(name) MODNDB_COVERAGE(__FILE__, name)
#define COV_line() MODNDB_COVERAGE(__FILE__, "Line:" STRINGOF(__LINE__))
#else
#define COV_point(name)
#define COV_line()
#endif
