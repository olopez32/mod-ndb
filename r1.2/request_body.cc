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


#include "JSON/Parser.h"

#ifdef MOD_NDB_DEBUG
int walk_tab(void *rec, const char *k, const char *v) {
  request_rec *r = (request_rec *) rec;
  log_debug(r->server,"%s => %s",k, v);  
  return TRUE;
}
#define DEBUG_LOG_TABLE(A, B) ap_table_do(walk_tab, A, B, NULL);
#else
#define DEBUG_LOG_TABLE(A, B)
#endif


typedef int BODY_READER(query_source *, apr_pool_t *, int);

/* read_urlencoded(): for application/x-www-form-urlencoded
   key1=val1&key2=val2...
 */
int read_urlencoded(query_source *qsource, apr_pool_t *pool, int) {
  const char *key, *val;  
  const char * &data = qsource->databuffer;
  
  while(*data && (val = ap_getword(pool, & data, '&'))) {
    key = ap_getword(pool, &val, '=');
    
    ap_unescape_url((char*)key);
    ap_unescape_url((char*)val);
    
    qsource->set_item(key, val);
  }  
  return OK;
}


/* read_jsonrequest(): for application/jsonrequest
   The JSON parser is generated by Coco from JSON/JSON.atg
 */
int read_jsonrequest(query_source *qsource, apr_pool_t *pool, int length) {
  JSON::Scanner scanner(qsource->databuffer, length);
  JSON::Parser parser(&scanner);
  
  parser.pool = pool;
  parser.qsource = qsource;
  
  parser.Parse();
  
  if(parser.errors->count) return 400;
  return OK;
}


/* read_multipart(): for multipart/form-data
*/
int read_multipart(query_source *qsource, apr_pool_t *pool, int length) {

}


/* util_read() is originally from mod_hello.cc,  which is available from
   http://www.modperl.com/book/source/ without any attached copyright notice 
   or licensing restrictions.  Originally by Doug MacEachern.
 */
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


int HTTP_query_source::get_form_data() {
  int rc = OK;
  BODY_READER *reader = 0;  
  int buf_size = 0;
  
  // To do: support PUT  
  if(r->method_number != M_POST) 
    return OK;

  if(!content_type) {
    /* This is a POST or PUT with an entity body but no content-type.
       As far as I can tell, HTTP allows this -- neither sec. 4-3 (Message Body)
       nor sec. 9-5 (POST) requires the client to send a content-type.
       No real client sends such a request, but the httperf benchmark tool does.
    */
    reader = read_urlencoded;
  }
  else if(strcmp(content_type, "application/x-www-form-urlencoded") == 0) 
    reader = read_urlencoded;
  else if(strcasecmp(content_type, "application/jsonrequest") == 0) 
    reader = read_jsonrequest;
  else if(strncmp(content_type, "multipart/form-data", 19) == 0) 
    reader = read_multipart;
  else {
    log_debug(r->server, "Unsupported request body: %s", content_type);
    return DECLINED;   
  }
  
  if((rc = util_read(r, &databuffer, &buf_size)) != OK)
    return rc;
  
  rc = reader(this, r->pool, buf_size);
    
  return rc;
}

