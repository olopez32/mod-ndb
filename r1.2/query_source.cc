#include "mod_ndb.h"
#include "mod_ndb_compat.h"
#include "query_source.h"


Apache_subrequest_query_source::Apache_subrequest_query_source(request_rec *req)
{
  r = req;
  keep_tx_open = true;
  const char *note = ap_table_get(r->main->notes,"ndb_request_method");
  if(note)  {
    if(!strcmp(note,"POST")) req_method = M_POST;
    else if(!strcmp(note,"DELETE")) req_method = M_DELETE;
    ap_table_unset(r->main->notes,"ndb_request_method");
  }
  form_data = ap_make_table(r->pool, 6);
}


int Apache_subrequest_query_source::get_form_data() {
  const char *subrequest_data = ap_table_get(r->main->notes,"ndb_request_data");
  register const char *c = subrequest_data;
  char *key, *val;
  while(*c && (val = ap_getword(r->pool, &c, '&'))) {
    key = ap_getword(r->pool, (const char **) &val, '=');
    ap_unescape_url(key);
    ap_unescape_url(val);
    ap_table_merge(form_data, key, val);
  }
  ap_table_unset(r->main->notes,"ndb_request_data");
  
  return OK;
}
