#include "mod_ndb.h"



/* util_read() and some other code in this file is originally from mod_hello.cc, 
   which is available from http://www.modperl.com/book/source/ without any 
   attached copyright notice or licensing restrictions.
   Originally by Doug MacEachern.
*/


typedef int BODY_READER(request_rec *, table **, const char *);

int read_urlencoded(request_rec *r, table **tab, const char *data) {
  const char *key, *val;
  
  while(*data && (val = ap_getword(r->pool, &data, '&'))) {
    key = ap_getword(r->pool, &val, '=');
    
    ap_unescape_url((char*)key);
    ap_unescape_url((char*)val);
    
    ap_table_merge(*tab, key, val);
  }
  
  return OK;
}


int read_jsonrequest(request_rec *r, table **tab, const char *data) {
   return OK;
}



int util_read(request_rec *r, const char **rbuf)
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
  }
  
  return rc;
}


int read_request_body(request_rec *r, table **tab) {
  int rc = OK;
  const char *data;
  const char *type = ap_table_get(r->headers_in, "Content-Type");
  BODY_READER *reader = 0;  
  
  // To do: support PUT  
  if(r->method_number != M_POST) 
    return OK;

  // To do:  support multipart/form-data
  if(strcasecmp(type, "application/x-www-form-urlencoded") == 0) 
    reader = read_urlencoded;
  else if(strcasecmp(type, "application/jsonrequest") == 0)
    reader = read_jsonrequest;
  else
    return DECLINED;   
  
  if((rc = util_read(r, &data)) != OK)
    return rc;

  if(*tab)
    ap_clear_table(*tab);
  else
    *tab = ap_make_table(r->pool, 8);
  
  return reader(r, tab, data);
}

