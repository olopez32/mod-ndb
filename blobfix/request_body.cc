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


/* util_read() and some other code in this file is originally from mod_hello.cc, 
   which is available from http://www.modperl.com/book/source/ without any 
   attached copyright notice or licensing restrictions.
   Originally by Doug MacEachern.
*/

#include "JSON/Parser.h"

#ifdef MOD_NDB_DEBUG
int walk_tab(void *rec, const char *k, const char *v) {
  request_rec *r = (request_rec *) rec;
  log_debug(r->server,"%s => %s",k, v);  
  return 1;
}
#define DEBUG_LOG_TABLE(A, B) ap_table_do(walk_tab, A, B, NULL);
#else
#define DEBUG_LOG_TABLE(A, B)
#endif


typedef int BODY_READER(request_rec *, apr_table_t **, const char *, int);

int read_urlencoded(request_rec *r, apr_table_t **tab, const char *data, int) {
  const char *key, *val;
  
  while(*data && (val = ap_getword(r->pool, &data, '&'))) {
    key = ap_getword(r->pool, &val, '=');
    
    ap_unescape_url((char*)key);
    ap_unescape_url((char*)val);

    /* ap_getword() already made a copy, so use ap_table_mergen() */
    ap_table_mergen(*tab, key, val);
  }
  
  return OK;
}


int read_jsonrequest(request_rec *r, apr_table_t **tab, const char *data, int length){
  JSON::Scanner scanner((const unsigned char *)data, length);
  JSON::Parser parser(&scanner);
  
  parser.pool = r->pool;
  parser.tabl = *tab;
  
  parser.Parse();
  
  if(parser.errors->count) {
    log_debug(r->server,"JSON parser: %d errors.  Returning 400.", parser.errors->count);  
    return 400;
  }
  return OK;
}



int util_read(request_rec *r, const char **rbuf, int *len)
{
  int rc = OK;
  
  if ((rc = ap_setup_client_block(r, REQUEST_CHUNKED_ERROR))) {
    return rc;    // To do: accept chunked transfers?
  }
  
  if (ap_should_client_block(r)) {
    char argsbuffer[HUGE_STRING_LEN];
    int rsize, len_read, rpos=0;
    long length = r->remaining;
    *rbuf = (const char *) ap_pcalloc(r->pool, length + 1); 
    
    ap_hard_timeout("util_read", r);
    
    while ((len_read =
            ap_get_client_block(r, argsbuffer, sizeof(argsbuffer))) > 0) {
      ap_reset_timeout(r);
      if ((rpos + len_read) > length) {
        rsize = length - rpos;
      }
      else {
        rsize = len_read;
      }
      memcpy((char*)*rbuf + rpos, argsbuffer, rsize);
      rpos += rsize;
    }
    
    ap_kill_timeout(r);
    *len = rpos;
  }
  
  return rc;
}


int read_request_body(request_rec *r, apr_table_t **tab, const char *type) {
  int rc = OK;
  const char *data;
  BODY_READER *reader = 0;  
  int buf_size = 0;
  
  // To do: support PUT  
  if(r->method_number != M_POST) 
    return OK;

  // To do: support multipart/form-data
  if(!type) {
    /* This is a POST or PUT with an entity body but no content-type.
       As far as I can tell, HTTP allows this -- neither sec. 4-3 (Message Body)
       nor sec. 9-5 (POST) requires the client to send a content-type.
       No real client sends such a request, but the httperf benchmark tool does.
    */
    reader = read_urlencoded;
  }
  else if(strcasecmp(type, "application/x-www-form-urlencoded") == 0) 
    reader = read_urlencoded;
  else if(strcasecmp(type, "application/jsonrequest") == 0) 
    reader = read_jsonrequest;
  else {
    log_debug(r->server, "Unsupported request body: %s", type);
    return DECLINED;   
  }
  
  if((rc = util_read(r, &data, &buf_size)) != OK)
    return rc;

  if(*tab)
    ap_clear_table(*tab);
  else
    *tab = ap_make_table(r->pool, 8);
  
  rc = reader(r, tab, data, buf_size);
  
  DEBUG_LOG_TABLE(r, *tab);
  
  return rc;
}


