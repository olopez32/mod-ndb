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

/* ================================================================== 
   MOD_NDB_DEBUG is defined by "./configure --debug" and generates
   apache log messages at log level debug.
   CONFIG_DEBUG generates extra debugging messages (some to STDOUT) 
   related to configuration handling and parsing.
*/
// #define MOD_NDB_DEBUG 1
// #define CONFIG_DEBUG 1

/* Other Defaults */
#define DEFAULT_MAX_READ_OPERATIONS 20
#define DEFAULT_MAX_RETRY_MS        50
#define DEFAULT_FORCE_RESTART       0

#define MAX_ENDPOINTS 500

/* Release Number */
#define REVISION "1.1-beta"
