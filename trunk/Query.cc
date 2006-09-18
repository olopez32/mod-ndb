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
#include "util_md5.h"

/* There are many varieties of query: 
   read, insert, update, and delete (e.g. HTTP GET, POST, and DELETE);
   single-row lookups and multi-row scans;
   access via primary key, unique hash index, ordered index, or scan filter;
   queries that use blobs;
   and final results returned in several different formats.
   Each variant possibility is represented as a "module" in this file, 
   and most (but not all) of those modules are functions that conform
   to the "PlanMethod" typedef.
*/
typedef int PlanMethod(request_rec *, config::dir *, struct QueryItems *);


/* A runtime column is the runtime complement to the config::key_col structure
*/
class runtime_col {
  public:
    char *value;
    int ndb_col_id;
    const config::key_col *conf;
};

/* The main Query() function has a single instance of the QueryItems structure,
   which is used to pass essential data among the modules  
*/
struct QueryItems {
  const NdbDictionary::Table *tab;
  const NdbDictionary::Index *idx;
  NdbTransaction::ExecType ExecType;
  NdbOperation *op;
  NdbIndexScanOperation *scanop;
  NdbBlob *blob;
  NdbRecAttr **result_cols;
  runtime_col *keys;
  short active_index;
  short key_columns_used;
  int n_filters;
  short *filter_list;
  AccessPlan plan;
  table *form_data;
  PlanMethod *op_setup;
  PlanMethod *op_action;
  PlanMethod *send_results;
  result_buffer *results;
  bool results_are_visible;
};  

/* Most modules are represented as PlanMethods
*/
namespace Plan {
 PlanMethod SetupRead; PlanMethod SetupWrite; PlanMethod SetupDelete; // setups
 PlanMethod Read;      PlanMethod Write;      PlanMethod Delete;      // actions
};  

/*  Result formatters:
*/
 PlanMethod Results_JSON;    PlanMethod Results_ap_note;
 PlanMethod Results_raw;     PlanMethod Results_XML;
 

/* This array corresponds to the four items in enum result_format_type:
*/
PlanMethod *result_formatter[5] = { 
  0 , Results_JSON , Results_raw, Results_XML, Results_ap_note
};


/* Utility function declarations
*/
short key_col_bin_search(char *, config::dir *);


/* Some very simple modules are fully defined here:
*/
int Plan::SetupRead(request_rec *r, config::dir *dir, struct QueryItems *q) { 
  return q->plan == OrderedIndexScan ? 
    q->scanop->readTuples(NdbOperation::LM_Read) :
    q->op->readTuple(NdbOperation::LM_Read);
}

int Plan::SetupWrite(request_rec *r, config::dir *dir, struct QueryItems *q) { 
  return q->op->writeTuple(); 
}

int Plan::SetupDelete(request_rec *r, config::dir *dir, struct QueryItems *q) { 
  return q->op->deleteTuple(); 
}


/* Inlined code to test the usability of an mvalue
*/
inline bool mval_is_usable(request_rec *r, mvalue &mval) {
  if(mval.use_value == can_not_use) {
    ap_log_error(APLOG_MARK, log::err, r->server,
                 "Cannot use MySQL column %s in query -- data type "
                 "not supported by mod_ndb",mval.u.err_col->getName());
    return 0;
  }
  else return 1;
}


/* Inlined code to set a bound in an IndexScan 
*/
inline int mval_set_bound(struct QueryItems *q, runtime_col &c, mvalue &mval) {
  return q->scanop->setBound(c.conf->name, c.conf->filter_op, &mval.u.val_char);
}


/* Inlined code to allocate memory for the filter list as needed. 
*/
inline void init_filters(request_rec *r, config::dir *dir, struct QueryItems *q) {
  if(! q->filter_list) 
    q->filter_list = (short *) 
      ap_pcalloc(r->pool, (dir->key_columns->size() * sizeof(short)));
}


/* Inlined code (called while processing both pathinfo and request params)
   which sets the items in the Q.keys array and determines the access plan.
*/
inline void set_key(request_rec *r, short &n, char *value, config::dir *dir, 
                    struct QueryItems *q) 
{
  const NdbDictionary::Column *column;
  config::key_col &keycol = dir->key_columns->item(n);

  if(keycol.is.alias) {  
    /* "Filter real_col < col_alias" (Could be ScanFilter or Bounds) */
    n = keycol.filter_col;  // repoint n from col_alias to real_col
    keycol = dir->key_columns->item(n);
    if(keycol.implied_plan != OrderedIndexScan) {  /* ScanFilter */
      init_filters(r, dir, q);   
      q->filter_list[q->n_filters++] = n;
    }
  }
  else if(keycol.is.filter && keycol.index_id == -1) {  
    /* "Filter real_col" (ScanFilter only) */
    init_filters(r, dir, q);
    q->filter_list[q->n_filters++] = n;
  }

  q->keys[n].value = value; 
  q->keys[n].conf = &keycol;
  column = q->tab->getColumn(keycol.name);
  if(column) {
    q->keys[n].ndb_col_id = column->getColumnNo();
    log_debug3(r->server, "Config column %s = NDB AttrID %d", 
               keycol.name, q->keys[n].ndb_col_id);
  }
  else return; 
  q->key_columns_used++;
 
  if(keycol.implied_plan > q->plan) {
    q->plan = keycol.implied_plan;
    q->active_index = keycol.index_id;
  }
}

// =============================================================


/* Query():
   Process an HTTP request,
   formulate and run an NDB execution plan,
   and create a results page.
*/
int Query(request_rec *r, config::dir *dir, ndb_instance *i) 
{
  const NdbDictionary::Dictionary *dict;
  result_buffer my_results;
  struct QueryItems Q = 
    { 0, 0, NdbTransaction::NoCommit, 0,0,0,0,0,0,0,0,
      0, NoPlan, 0, Plan::SetupRead, Plan::Read, 0, &my_results, true };
  struct QueryItems *q = &Q;
  const NdbDictionary::Column *ndb_Column;
  bool keep_tx_open = false;
  int response_code;
  mvalue mval;
  short col;

  /* Initialize the data dictionary 
  */
  i->db->setDatabaseName(dir->database);
  dict = i->db->getDictionary();
  Q.tab = dict->getTable(dir->table);
  if(Q.tab == 0) { 
    ap_log_error(APLOG_MARK, log::err, r->server,
                 "mod_ndb could not find table %s (in database %s) "
                 "in NDB Data Dictionary: %s",dir->table,dir->database,
                 dict->getNdbError().message);
    i->errors++;
    return NOT_FOUND; 
  }  
  
  /* Initialize Q.keys, the runtime array of key columns which is used
     in parallel with the configure-time array dir->key_columns.   */
  Q.keys = (runtime_col *) ap_pcalloc(r->pool, 
           (dir->key_columns->size() * sizeof(runtime_col)));

  /* Many elements of the query plan depend on the HTTP operation --
     GET, POST, or DELETE.  Set these up here.
  */
  switch(r->method_number) {
    case M_GET:
      ap_discard_request_body(r);
      // Allocate an array of NdbRecAttrs for all desired columns 
      Q.result_cols = (NdbRecAttr **) 
        ap_pcalloc(r->pool, dir->visible->size() * sizeof(NdbRecAttr *));
      Q.send_results = result_formatter[dir->results];
      break;
    case M_POST:
      Q.op_setup = Plan::SetupWrite;
      Q.op_action = Plan::Write;
      Q.ExecType = NdbTransaction::Commit;
      Q.form_data = 0;
      Q.results_are_visible = false;
      /* Fetch the update request from the client */
      response_code = read_http_post(r, & Q.form_data);
      if(response_code != OK) return response_code;
      break;
    case M_DELETE:
      if(! dir->allow_delete) return DECLINED;
      Q.op_setup = Plan::SetupDelete;
      Q.op_action = Plan::Delete;
      Q.ExecType = NdbTransaction::Commit;
      Q.results_are_visible = false;
      break;
    default:
      return DECLINED;
  }

  /* Is this request an Apache sub-request? 
  */
  if(r->main) {
    const char *note = ap_table_get(r->main->notes,"ndb_keep_tx_open");
    if(note && *note == '1') {
      keep_tx_open = true;
      ap_table_unset(r->main->notes,"ndb_keep_tx_open");
    }
  }


  /* ===============================================================
     Process arguments, and then pathinfo.  Determine an access plan.
     The detailed work is done within the inlined function set_key().
  */

  /* Arguments */
  if(r->args) {
    register const char *c = r->args;
    char *key, *val;
    short n;
    
    while(*c && (val = ap_getword(r->pool, &c, '&'))) {
      key = ap_getword(r->pool, (const char **) &val, '=');
      ap_unescape_url(key);
      ap_unescape_url(val);
      n = key_col_bin_search(key, dir);
      if(n >= 0) { 
        log_debug3(r->server,"key %s [%d]",key,n);
        set_key(r, n, val, dir, &Q);
      }
      else
        log_debug(r->server,"Unidentified key %s",key);
    }
  }   
  
  /* Pathinfo.  If args were insufficient to define a query plan (or pathinfo
     has the "always" flag), process r->path_info from right to left, 
  */
  if(dir->pathinfo_size && 
     ((Q.plan == NoPlan) || dir->flag.pathinfo_always)) {
    size_t item_len = 0;
    short element = dir->pathinfo_size - 1;
    register const char *s;
    // Set s to the end of the string, then work backwards.
    for(s = r->path_info ; *s; ++s);
    if(* (s-1) == '/') s -=2;   /* ignore a trailing slash */
    for(; s >= r->path_info && element >= 0; --s) {
      if(*s == '/') {
        set_key(r, dir->pathinfo[element--], 
                ap_pstrndup(r->pool, s+1, item_len), 
                dir, &Q);
        item_len = 0;
      }
      else item_len++;
    }
  }
  
  /* ===============================================================*/
  if(! Q.key_columns_used) return NOT_FOUND;    /* no query */

  /* Open a transaction.
     This creates an obligation to close it later, using tx->close().
     If a non-null i->tx already exists, then the transaction is open
     because of keep_tx_open in a previous request.
  */    
  if(i->tx == 0) {
    if(!(i->tx = i->db->startTransaction())) {
      log_err2(r->server,"db->startTransaction failed: %s",
                i->db->getNdbError().message);
      return NOT_FOUND;
    }
  }

  /* Now set the Query Items that depend on the access plan.
  */
  if(Q.plan == PrimaryKey) {
    Q.op = i->tx->getNdbOperation(Q.tab);
    log_debug(r->server,"Using primary key lookup; key %d",Q.active_index);
  }
  else {                                        /* Indexed Access */
    register const char * idxname = dir->indexes->item(Q.active_index).name;
    Q.idx = dict->getIndex(idxname, dir->table);
    if(! Q.idx)
    {
      ap_log_error(APLOG_MARK, log::err, r->server, "mod_ndb: index %s "
                   "does not exist (db: %s, table: %s)", idxname, 
                   dir->database, dir->table);
      goto abort;
    }    
    if(Q.plan == UniqueIndexAccess) {
      log_debug3(r->server,"Using UniqueIndexAccess; key # %d - %s",
                 Q.active_index, Q.idx->getName());
      Q.op = i->tx->getNdbIndexOperation(Q.idx);
    }
    else if(Q.plan == OrderedIndexScan) {
      log_debug3(r->server,"Using OrderedIndexScan; key # %d - %s",
                 Q.active_index, Q.idx->getName());
      Q.op = Q.scanop = i->tx->getNdbIndexScanOperation(Q.idx);
    }
    else
      log_debug(r->server," --SHOULD NOT HAVE REACHED THIS POINT-- %d",Q.plan);
  }
  if(Q.active_index < 0) goto abort;


  // Query setup, e.g. Plan::SetupRead calls op->readTuple() 
  if(Q.op_setup(r, dir, & Q)) { // returns 0 on success
    log_debug(r->server,"Returning 404 because Q.op_setup() failed: %s",
              Q.op->getNdbError().message);
    goto abort;
  }


  // Traverse the index parts and build the query
  col = dir->indexes->item(Q.active_index).first_col;
  ndb_Column = Q.tab->getColumn(Q.keys[col].ndb_col_id);

  while (col >= 0 && Q.key_columns_used-- > 0) {
    log_debug3(r->server," ** Query key: %s -- value: %s", 
               dir->key_columns->item(col).name, Q.keys[col].value);
    
    mval = MySQL::value(r->pool, ndb_Column, Q.keys[col].value);
    if(mval_is_usable(r, mval)) {
      if(q->scanop) 
      {
        if(mval_set_bound(q, Q.keys[col], mval)) 
          goto abort;
      }
      else {   /* what is going on here !?!?! */
        if(Q.plan == PrimaryKey) 
        { 
          if(q->op->equal(Q.keys[col].ndb_col_id, (const char *) (&mval.u.val_char)))
          {
            log_debug(r->server," op->equal failed, column %s", ndb_Column->getName());
            goto abort;
          }
        }
        else { 
          if(q->op->equal(ndb_Column->getName(), (const char *) (&mval.u.val_char)))
          {
            log_debug(r->server," op->equal failed, column %s", ndb_Column->getName());
            goto abort; 
          }
        }
      }
    }
    col = dir->key_columns->item(col).next_in_key;
  }

  // Set filters
  if(Q.plan == OrderedIndexScan && Q.n_filters) {
    NdbScanFilter filter(Q.op);
    filter.begin(NdbScanFilter::AND);
    for(int n = 0 ; n < Q.n_filters ; n++) {
      runtime_col * filter_col = & Q.keys[Q.filter_list[n]];
      ndb_Column = q->tab->getColumn(filter_col->ndb_col_id);
      log_debug3(r->server," ** Filter : %s -- value: %s", 
                 filter_col->conf->name, filter_col->value);
      mval = MySQL::value(r->pool, ndb_Column, filter_col->value);
      filter.cmp( (NdbScanFilter::BinaryCondition) filter_col->conf->filter_op, 
                 filter_col->ndb_col_id, (&mval.u.val_char) );
    }                    
    filter.end();
  }

  // Perform the action; i.e. get the value of each column
  response_code = Q.op_action(r, dir, &Q);
  
  if(response_code == OK) {
    /* Execute the transaction.  tx->execute() returns 0 on success. */
    if(i->tx->execute(Q.ExecType, NdbTransaction::AbortOnError, 
                      i->conn->ndb_force_send))
    {        
      log_debug(r->server,"Returning 410 because tx->execute failed: %s",
                i->tx->getNdbError().message);
      response_code = HTTP_GONE;
    }
    else 
      if(Q.results_are_visible && Q.send_results)
        response_code = Q.send_results(r, dir, q);
  }
  
  if(response_code == OK && q->results->buff && q->results_are_visible) {
    /* Set the content length and ETag headers 
    */
    ap_set_content_length(r, q->results->sz);
    if(dir->use_etags) {
      char *etag = ap_md5_binary(r->pool, (const unsigned char *) 
                                 q->results->buff, q->results->sz);
      ap_table_setn(r->headers_out, "ETag",  etag);
      // If the ETag matches the client's cache the page should not be returned 
      response_code = ap_meets_conditions(r);
    }
    
    if(response_code == OK) {   /* Send the page. */
      ap_send_http_header(r); 
      ap_rwrite(q->results->buff, q->results->sz, r);
    }
  };
  
  if(! keep_tx_open) {
    i->tx->close();
    i->tx = 0;
  }
  return response_code;
  
  abort:
  i->tx->close();
  i->tx = 0;
  return NOT_FOUND;
}



/******** Result Page formatters *************/


inline void JSON_send_result_row(request_rec *r, config::dir *dir, 
                                 struct QueryItems *q) {
  q->results->out(JSON::new_object);
  for(int n = 0; n < dir->visible->size() ; n++) {
    if(n) q->results->out(JSON::delimiter);
    JSON::put_member(*q->results, *q->result_cols[n], r);
  }
  q->results->out(JSON::end_object);
}
  

int Results_JSON(request_rec *r, config::dir *dir, struct QueryItems *q) {
  int nrows = 0;
  q->results->init(r, 8192);

  if(q->scanop) {
    /* Multi-row result set */   
    while((q->scanop->nextResult(true)) == 0) {
      do {
        if(nrows++) q->results->out(",\n");
        else q->results->out(JSON::new_array,r);
        JSON_send_result_row(r, dir, q);
      } while((q->scanop->nextResult(false)) == 0);
    }
    if(nrows) q->results->out(JSON::end_array);
    else return HTTP_GONE;
  }
  else {
    /* Single row result set */
    JSON_send_result_row(r, dir, q);
  }
  
  q->results->out("\n");
  return OK;
}


int Results_raw(request_rec *r, config::dir *dir, struct QueryItems *q) {
  unsigned long long size64 = 0;
  unsigned int size;
  
  if(q->blob) {
    q->blob->getLength(size64);  //passed by reference
    size = (unsigned int) size64;
    q->results->init(r, size);
    if(q->blob->readData(q->results->buff, size)) 
      log_debug(r->server,"Error reading blob data: %s",
                q->blob->getNdbError().message);
  }
  q->results->sz = size;
  return OK;
}

int Results_XML(request_rec *r, config::dir *dir, struct QueryItems *q) {
  log_debug(r->server,"In Results formatter %s", "Results_XML");

  return NOT_FOUND;
}

int Results_ap_note(request_rec *r, config::dir *dir, struct QueryItems *q) {
  log_debug(r->server,"In Results formatter %s", "Results_ap_note");
  // If this is a subrequest, set the note for the parent:
  request_rec *rr = r->main ? r->main : r;
  
  if(dir->sub_results != no_results && dir->sub_results != ap_note) {
    /* Generate results in the sub_result format (e.g. JSON or raw) 
       and save them as a note named "NDB_results" 
    */
    result_formatter[dir->sub_results](r,dir,q); 
    ap_table_set(rr->notes,"NDB_results",q->results->buff);
    q->results_are_visible = false;
  }
  else {
    for(int n = 0; n < dir->visible->size() ; n++) {
      register NdbRecAttr *rec =  q->result_cols[n];
      if(! rec->isNULL()) {
        register const NdbDictionary::Column* col = rec->getColumn();
        ap_table_set(rr->notes, col->getName(), MySQL::result(r->pool, *rec));
      }
    }
  }
  return OK;
}


int Plan::Read(request_rec *r, config::dir *dir, struct QueryItems *q) {  
  char **column_list;

  // Call op->getValue() for each desired result column
  column_list = dir->visible->items();
  for(int n = 0; n < dir->visible->size() ; n++) {
    q->result_cols[n] = q->op->getValue(column_list[n], 0);

    /* If the result format is "raw", check for BLOBs */
    if(dir->results == raw) {
      int isz = q->tab->getColumn(column_list[n])->getInlineSize();
      if(isz) {   /* then the column is a blob... */
        log_debug(r->server,"Treating column %s as a blob",column_list[n])
        q->blob = q->op->getBlobHandle(column_list[n]);
      }
    }
  }
  return OK;
}


int Plan::Write(request_rec *r, config::dir *dir, struct QueryItems *q) {
  char **column_list;
  const NdbDictionary::Column *col;
  mvalue mval;
  const char *key, *val;
  
  column_list = dir->updatable->items();
  // iterate over the updatable columns
  for(int n = 0; n < dir->updatable->size() ; n++) {
    key = column_list[n];
    // finding them in the posted form data
    val = ap_table_get(q->form_data, key);
    if(val) {   
      log_debug(r->server,"Updating column %s",key);
      col = q->tab->getColumn(key);
      if(col) {
        // Encode the HTTP ASCII data into proper MySQL data types
        mval = MySQL::value(r->pool, col, val);
        // And call op->setValue
        if(mval_is_usable(r, mval))
           q->op->setValue(col->getName(), (const char *) (&mval.u.val_char));
      }
      else log_err2(r->server,"AllowUpdate list includes invalid column name %s", key);
    }
  }
  return OK;
}


int Plan::Delete(request_rec *r, config::dir *dir, struct QueryItems *q) {
  log_debug(r->server,"Deleting Row %s","")
  return OK;
}


/* Based on Kernighan's C binsearch from TPOP pg. 31
*/
short key_col_bin_search(char *name, config::dir *dir) {
  int low = 0;
  int high = dir->key_columns->size() - 1;
  int mid;
  register int cmp;
  
  while ( low <= high ) {
    mid = (low + high) / 2;
    cmp = strcmp(name, dir->key_columns->item(mid).name);
    if(cmp < 0) 
      high = mid - 1;
    else if (cmp > 0)
      low = mid + 1;
    else
      return mid;
  }
  return -1;
}

