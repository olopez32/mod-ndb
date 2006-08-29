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
typedef char * NextKeyColumn(struct QueryItems *, int);


enum AccessPlan {         // How to fetch the data:
  NoPlan = 0,             // (also a bitmap)
  UseIndex = 1,
  PrimaryKey = 2,         // Lookup 
  UniqueIndexAccess = 3,  // Lookup & UseIndex 
  Scan = 4,               // Scan  
  OrderedIndexScan = 5,   // Scan & UseIndex 
};

/* A runtime column is the "other half" of the config::key_col structure
*/
struct runtime_col {
  char *value;

};

/* The main Query() function has a single instance of the QueryItems structure,
   which is used to pass essential data among the modules  
*/
struct QueryItems {
  AccessPlan plan;
  table *form_data;
  NdbTransaction::ExecType ExecType;
  NdbOperation *op;
  const NdbDictionary::Table *tab;
  const NdbDictionary::Index *idx;
  int n_key_parts;
  NextKeyColumn *NextPart;
  PlanMethod *op_setup;
  PlanMethod *run_plan;
  PlanMethod *op_action;
  PlanMethod *send_results;
  NdbBlob *blob;
  NdbRecAttr **result_cols;
};  

/* Most modules are represented as PlanMethods
*/
namespace Plan {
 PlanMethod Lookup;    PlanMethod Scan;                    // data access plans
 PlanMethod SetupRead; PlanMethod SetupWrite; PlanMethod SetupDelete; // setups
 PlanMethod Read;      PlanMethod Write;      PlanMethod Delete;     // actions
};  

//  Result formatters:
 PlanMethod Results_none;    PlanMethod Results_JSON;
 PlanMethod Results_raw;     PlanMethod Results_XML;
 PlanMethod Results_ap_note;

/* This array corresponds to the four items in enum result_format:
*/
PlanMethod *result_formatter[5] = { 
  Results_none , Results_JSON , Results_raw, Results_XML, Results_ap_note
};


/* Some very simple modules are fully defined here:
*/
int Plan::SetupRead(request_rec *r, config::dir *dir, struct QueryItems *q) { 
  return q->op->readTuple(NdbOperation::LM_Read);
}
int Plan::SetupWrite(request_rec *r, config::dir *dir, struct QueryItems *q) { 
  return q->op->writeTuple(); 
}
int Plan::SetupDelete(request_rec *r, config::dir *dir, struct QueryItems *q) { 
  return q->op->deleteTuple(); 
}
char * next_pk_part(struct QueryItems *q, int n) {
  return (char *) q->tab->getPrimaryKey(n); 
}
char * next_index_part(struct QueryItems *q, int n) {
  return (char *) q->idx->getColumn(n)->getName(); 
}


// =============================================================


/* Query():
   Process an HTTP request,
   formulate and run an NDB execution plan,
   and create a results page.
*/
int Query(request_rec *r, config::dir *dir, ndb_instance *i) 
{
  struct QueryItems Q ;
  const NdbDictionary::Dictionary *dict;
  char *key1; 
  const char *idxentry;
  int response_code;

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
  

  /* Many elements of the query plan depend on the HTTP operation --
     GET, POST, or DELETE.  Set these up here.
  */
  switch(r->method_number) {
    case M_GET:
      Q.op_setup = Plan::SetupRead;   
      Q.op_action = Plan::Read;
      Q.ExecType = NdbTransaction::NoCommit;
      // Allocate an array of NdbRecAttrs for all desired columns 
      Q.result_cols = (NdbRecAttr **) 
        ap_pcalloc(r->pool, dir->visible->size() * sizeof(NdbRecAttr *));
      Q.send_results = result_formatter[dir->results];
      break;
    case M_POST:
      Q.op_setup = Plan::SetupWrite;
      Q.op_action = Plan::Write;
      Q.ExecType = NdbTransaction::Commit;
      Q.send_results = Results_none;
      Q.form_data = 0;
      /* Fetch the update request from the client */
      response_code = read_http_post(r, & Q.form_data);
      if(response_code != OK) return response_code;
      break;
    case M_DELETE:
      if(! dir->allow_delete) return DECLINED;
      Q.op_setup = Plan::SetupDelete;
      Q.op_action = Plan::Delete;
      Q.ExecType = NdbTransaction::Commit;
      Q.send_results = Results_none;
      break;
    default:
      return DECLINED;
  }

  /* Set up arguments and pathinfo */
  if(!r->args) return HTTP_OK;
  // Q.param_tab = http_param_table(r, r->args);


  /* ===============================================================*/
  /* Determine an access plan.
  
     To do this, look at the first key in the parameter table. 
     It names a column.  If this column has been configured in
     httpd.conf as part of a UniqueIndex, then use that index.
     If it is part of an ordered index, use an ordered index scan.
     Otherwise use primary key access.
  */

  // The default plan is primary key access.
  Q.plan = PrimaryKey;

  // Get the first paramater from the query string.
  key1 = ((table_entry *) (ap_table_elts(Q.param_tab)->elts))->key;

  // Was this parameter configured to use an index?
  idxentry = ap_table_get((const table *) dir->indexes, key1);
  if(idxentry) log_debug(r->server,"Found index %s",idxentry);

  // If so, idxentry is the name of the NDB index, along with
  // a prefix U* or O* denoting whether it is unique or ordered
  if(idxentry && (strlen(idxentry) > 2)) {
    char *idxname = (char *)(idxentry+2);
    Q.idx = dict->getIndex(idxname, dir->table);
    if(Q.idx) { 
      // Set the access plan:
      if(*idxentry == 'U') Q.plan = UniqueIndexAccess;
      else if(*idxentry == 'O') Q.plan = OrderedIndexScan;
      else log_debug(r->server,"mod_ndb: strange prefix in idxentry %s",idxentry);
    }
    else ap_log_error(APLOG_MARK, log::err, r->server,
         "mod_ndb: index %s does not exist (db: %s, table: %s)",
         (char *)idxentry+2, dir->database, dir->table);
  }
  /* ===============================================================*/


  /* Open a transaction.
     This creates an obligation to close it later, 
     using tx->close().
  */    
  if(!(i->tx = i->db->startTransaction())) {
    log_err2(r->server,"db->startTransaction failed: %s",
              i->db->getNdbError().message);
    return NOT_FOUND;
  }
    



  /* Now set the Query Items that depend on the  access plan.
  */
  if(Q.plan == PrimaryKey) {
    Q.op = i->tx->getNdbOperation(Q.tab);
    Q.n_key_parts = Q.tab->getNoOfPrimaryKeys();
    Q.NextPart = next_pk_part;
    log_debug(r->server,"Using primary key lookup; key size %d",Q.n_key_parts);
  }
  else {                                        /* Indexed Access */
    Q.n_key_parts = Q.idx->getNoOfColumns();
    Q.NextPart = next_index_part;
    if(Q.plan == UniqueIndexAccess) {
      log_debug(r->server,"Using UniqueIndexAccess; key size %d",Q.n_key_parts);
      Q.op = i->tx->getNdbIndexOperation(Q.idx);
    }
    else if(Q.plan == OrderedIndexScan) {
      log_debug(r->server,"Using OrderedIndexScan; key size %d",Q.n_key_parts);
      Q.op = i->tx->getNdbIndexScanOperation(Q.idx);
    }
    log_debug(r->server," --SHOULD NOT HAVE REACHED THIS POINT-- %d",Q.plan);
  }


  /* The parameter count must be equal to
     the number of key parts + the number of filters.
     (Filters are not yet implemented).
  */
  if(ap_table_elts(Q.param_tab)->nelts != Q.n_key_parts) {
    log_debug(r->server,"Returning 404 because query param count "
              "does not match key length %d", Q.n_key_parts);
    i->tx->close();
    return NOT_FOUND;
}

  // Query setup, e.g. Plan::SetupRead calls op->readTuple() 
  if(Q.op_setup(r, dir, & Q)) { // returns 0 on success
    log_debug(r->server,"Returning 404 because Q.setup() failed: %s",
              Q.op->getNdbError().message);
    i->tx->close();
    return NOT_FOUND;
  }

  /* Run the plan */
  // TO DO: This could be a scan
  response_code = Plan::Lookup(r, dir, & Q);

  if(response_code != OK) {
    i->tx->close();
    return response_code;
  }
 
  /* Execute the transaction */
  if(i->tx->execute(Q.ExecType,       // returns 0 on succes.
                   NdbTransaction::AbortOnError, 
                   i->conn->ndb_force_send)) {        
    log_debug(r->server,"Returning 404 because tx->execute failed: %s",
              i->tx->getNdbError().message);
    i->tx->close();
    return NOT_FOUND;
  }
     
  /* Deliver the result page */
  Q.send_results(r, dir, & Q);
  
  i->tx->close();
  return OK;
}


/******** Result Page formatters *************/

int Results_none(request_rec *r, config::dir *dir, struct QueryItems *q) {
  log_debug(r->server,"In Results formatter %s", "Results_none");
  return 0;
}

int Results_JSON(request_rec *r, config::dir *dir, struct QueryItems *q) {

  log_debug(r->server,"In Results formatter %s", "Results_JSON");
  ap_send_http_header(r); 
  ap_rputs(JSON::new_object,r);
  for(int n = 0; n < dir->visible->size() ; n++) {
    if(n) ap_rputs(JSON::delimiter,r);
    ap_rputs(JSON::member(*q->result_cols[n],r),r);
  }
  ap_rputs(JSON::end_object,r);
  return 0;
}

int Results_raw(request_rec *r, config::dir *dir, struct QueryItems *q) {
  unsigned long long size64 = 0;
  unsigned int size = 0;
  void *buffer;
  
  if(q->blob) {
    q->blob->getLength(size64);  //passed by reference
    size = (unsigned int) size64;
    buffer = ap_palloc(r->pool, size);
    if(q->blob->readData(buffer,size)) { // 0 on success
      log_debug(r->server,"Error reading blob data: %s",
                q->blob->getNdbError().message);
    }
    ap_set_content_length(r, size);
    ap_send_http_header(r); 
    ap_rwrite(buffer, size, r);
  }
  return 0;
}

int Results_XML(request_rec *r, config::dir *dir, struct QueryItems *q) {
  log_debug(r->server,"In Results formatter %s", "Results_XML");

  return 0;
}

int Results_ap_note(request_rec *r, config::dir *dir, struct QueryItems *q) {
  
  log_debug(r->server,"In Results formatter %s", "Results_note");
  for(int n = 0; n < dir->visible->size() ; n++) {
    register NdbRecAttr *rec =  q->result_cols[n];
    if(! rec->isNULL()) {
      register const NdbDictionary::Column* col = rec->getColumn();
      ap_table_set(r->notes, col->getName(), MySQL::result(r->pool, *rec));
    }
  }
  return 0;
}

/* ===============================================================*/



/* mval_operation() is a wrapper around 
   NdbOperation->equal() and ->setValue(),
   which takes mvalues (from MySQL_Field.h),
   logs some errors and debug messages, 
   and returns an HTTP status code.
*/
enum ValueOp {  equal,    setValue  };

int mval_operation(request_rec *r, struct QueryItems *q, 
                   const char *key, ValueOp op, mvalue mval) {
                   
  if(mval.use_value == can_not_use) {
    ap_log_error(APLOG_MARK, log::err, r->server,
                 "Cannot use MySQL column %s in query -- data type "
                 "not supported by mod_ndb",mval.u.err_col->getName());
    return NOT_FOUND;
  }

  if(mval.use_value == use_char)
    log_debug(r->server,"mval.u.val_char: %s", mval.u.val_char);
  
  switch(op) {
    case equal:
      if(!(q->op->equal(key, (const char *) &(mval.u.val_char)))) // 0 on succes
        return OK;
      else { 
        log_debug(r->server,"op->equal failed: %s",q->op->getNdbError().message);
        return NOT_FOUND;
      }
    case setValue:
      if(!(q->op->setValue(key, (const char *) &(mval.u.val_char)))) // 0 on success
        return OK;
      else {
        log_debug(r->server,"op->setValue failed: %s",q->op->getNdbError().message);
        return NOT_FOUND;
      }
    default:
      log_debug(r->server," --SHOULD NOT HAVE REACHED THIS POINT-- %d",op);
      return NOT_FOUND;
  }
}
      

int Plan::Lookup(request_rec *r, config::dir *dir, struct QueryItems *q) {

  const NdbDictionary::Column *col;
  mvalue mval;
  const char *key, *val;
  
  // Call op->equal(column,value) for each part of the key
  for(int n = 0 ; n < q->n_key_parts ; n++) {
    key = q->NextPart(q,n); 
    val = (char *) ap_table_get(q->param_tab, key);
    if(val) {
      log_debug(r->server,"Lookup key: %s",key);
      log_debug(r->server,"Lookup value: %s",val);
      col = q->tab->getColumn(key);
      mval = MySQL::value(r->pool, col, val);
      if(mval_operation(r, q, key, equal, mval) != OK)
        return NOT_FOUND;
    }
    else {
      log_debug(r->server,"Returning 404 because key %s was not in index.",key);
      return NOT_FOUND;
    }
  }

  return q->op_action(r, dir, q);
}

int Plan::Scan(request_rec *r, config::dir *dir, struct QueryItems *q) {
  log_debug(r->server,"In unimplemented function %s","Plan::Scan")
  return NOT_FOUND;
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
        // Encode the HTTP ASCII data in proper MySQL data types
        mval = MySQL::value(r->pool, col, val);
        // And call op->setValue
        mval_operation(r, q, key, setValue, mval);
      }
      else log_err2(r->server,"AllowUpdate includes invalid column name %s", key);
    }
  }
  return OK;
}

int Plan::Delete(request_rec *r, config::dir *dir, struct QueryItems *q) {
  log_debug(r->server,"Deleting Row %s","")
  return OK;
}
