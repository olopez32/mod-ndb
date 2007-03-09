/* Copyright (C) 2006, 2007 MySQL AB

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
#include "ndb_api_compat.h"

/* There are many varieties of query: 
   read, insert, update, and delete (e.g. HTTP GET, POST, and DELETE);
   single-row lookups and multi-row scans;
   access via primary key, unique hash index, ordered index, or scan filter;
   queries that use blobs.
   Each variant possibility is represented as a module in this file, 
   and those modules are functions that conform to the "PlanMethod" typedef.
*/
typedef int PlanMethod(request_rec *, config::dir *, struct QueryItems *);

/* The modules: */
namespace Plan {
  PlanMethod SetupRead; PlanMethod SetupWrite; PlanMethod SetupDelete; // setups
  PlanMethod SetupInsert;
  PlanMethod Read;      PlanMethod Write;      PlanMethod Delete;     // actions
};  


/* A "runtime column" is the runtime complement to the config::key_col structure
*/
class runtime_col {
  public:
    char *value;
};

class index_object;   // forward declaration

/* The main Query() function has a single instance of the QueryItems structure,
   which is used to pass essential data among the modules  
*/
struct QueryItems {
  ndb_instance *i;
  const NdbDictionary::Table *tab;
  const NdbDictionary::Index *idx;
  runtime_col *keys;
  short active_index;
  index_object *idxobj;
  short key_columns_used;
  int n_filters;
  short *filter_list;
  AccessPlan plan;
  PlanMethod *op_setup;
  PlanMethod *op_action;
  table *form_data;
  mvalue *set_vals;
  data_operation *data;
};  

#include "index_object.h"


/* Utility function declarations
*/
int set_up_write(request_rec *, config::dir *, struct QueryItems *, bool);
short key_col_bin_search(char *, config::dir *);


/* Some very simple modules are fully defined here:
*/
int Plan::SetupRead(request_rec *r, config::dir *dir, struct QueryItems *q) {
  switch(q->plan) {
    case OrderedIndexScan:
      return q->data->scanop->readTuples(NdbOperation::LM_CommittedRead, 
                                         dir->indexes->item(q->active_index).flag);
    case Scan:
      return q->data->scanop->readTuples();
    default:
      return q->data->op->readTuple(NdbOperation::LM_CommittedRead);
  }
}

int Plan::SetupInsert(request_rec *r, config::dir *dir, struct QueryItems *q) { 
  return set_up_write(r, dir, q, true);
}

int Plan::SetupWrite(request_rec *r, config::dir *dir, struct QueryItems *q) { 
  return set_up_write(r, dir, q, false);
}

int Plan::SetupDelete(request_rec *r, config::dir *dir, struct QueryItems *q) { 
  return q->data->op->deleteTuple(); 
}


/* Inlined code to test the usability of an mvalue
*/
inline bool mval_is_usable(request_rec *r, mvalue &mval) {
  if(mval.use_value == can_not_use) {
    log_err2(r->server, "Cannot use MySQL column %s in query -- data type "
             "not supported by mod_ndb",mval.u.err_col->getName());
    return 0;
  }
  else return 1;
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
  log_debug3(r->server, "set value for key column %d [%s]",n,keycol.name);
  q->key_columns_used++;
 
  if(keycol.implied_plan > q->plan) {
    q->plan = keycol.implied_plan;
    q->active_index = keycol.index_id;
  }
}

// =============================================================

/* Query():
   Process an HTTP request, then formulate and run an NDB execution plan.
*/
int Query(request_rec *r, config::dir *dir, ndb_instance *i) 
{
  const NdbDictionary::Dictionary *dict;
  data_operation local_data_op = { 0, 0, 0, 0, 0, no_results };
  struct QueryItems Q = 
    { i, 0, 0,            // ndb_instance, tab, idx
      0, -1, 0, 0,        // keys, active_index, idxobj, key_columns_used 
      0, 0,               // n_filters, filter_list,
      NoPlan,             // execution plan
      Plan::SetupRead,    // setup module
      Plan::Read,         // action module
      0, 0,               // form_data, set_vals
      &local_data_op      // data
    };
  struct QueryItems *q = &Q;
  const NdbDictionary::Column *ndb_Column;
  bool keep_tx_open = false;
  int response_code = 0;
  mvalue mval;
  short col;
  int req_method = r->method_number;
  const char *subrequest_data = 0;
  
  // Initialize all four of these, but only one will be needed: 
  PK_index_object       PK_idxobj(q,r);
  Unique_index_object   UI_idxobj(q,r);
  Ordered_index_object  OI_idxobj(q,r);
  Table_Scan_object     TS_idxobj(q,r);

  // Initialize the data dictionary 
  i->db->setDatabaseName(dir->database);
  dict = i->db->getDictionary();
  q->tab = dict->getTable(dir->table);
  if(q->tab == 0) { 
    log_err4(r->server, "mod_ndb could not find table %s (in database %s) "
             "in NDB Data Dictionary: %s",dir->table,dir->database,
             dict->getNdbError().message);
    i->errors++;
    return NOT_FOUND; 
  }  
  
  /* Initialize q->keys, the runtime array of key columns which is used
     in parallel with the configure-time array dir->key_columns.   */
  q->keys = (runtime_col *) ap_pcalloc(r->pool, 
           (dir->key_columns->size() * sizeof(runtime_col)));

  /* Is this request an Apache sub-request? 
  */
  if(r->main) {
    keep_tx_open = true;
    const char *note = ap_table_get(r->main->notes,"ndb_request_method");
    if(note) {
      if(!strcmp(note,"POST")) req_method = M_POST;
      else if(!strcmp(note,"DELETE")) req_method = M_DELETE;
      ap_table_unset(r->main->notes,"ndb_request_method");
    }
    subrequest_data = ap_table_get(r->main->notes,"ndb_request_data");
  }
  
  /* Many elements of the query plan depend on the HTTP operation --
     GET, POST, or DELETE.  Set these up here.
  */
  switch(req_method) {
    case M_GET:
      /* Write requests use the local data_operations structure (which is 
         discarded after the request), but read requests use one that is 
         stored in the ndb_instance, so that we can be access it after the
         batch of transactions is executed and fetch the results.
      */
      if(i->n_read_ops < i->max_read_ops) {
        q->data = i->data + i->n_read_ops++;
        if(dir->use_etags) i->flag.use_etag = 1;
        q->data->result_format = dir->results;
        q->data->n_result_cols = dir->visible->size();
      }
      else {
        log_err(r->server,"Too many read operations in one transaction.");
        return DECLINED;
      }
      
      ap_discard_request_body(r);
      // Allocate an array of NdbRecAttrs for all desired columns.
      // Like anything that will be stored in the ndb_instance, allocate
      // from r->connection->pool, not r->pool
      q->data->result_cols =  (const NdbRecAttr**)
        ap_pcalloc(r->connection->pool, 
                   dir->visible->size() * sizeof(NdbRecAttr *));
      break;
    case M_POST:
      Q.op_setup = Plan::SetupWrite;
      Q.op_action = Plan::Write;
      Q.form_data = 0;
      Q.set_vals = (mvalue *) ap_pcalloc(r->pool, dir->updatable->size() * sizeof (mvalue));
      if(r->main && subrequest_data) {
        /* The POST data is in the "ndb_request_data" note. */
        Q.form_data = ap_make_table(r->pool, 4);
        register const char *c = subrequest_data;
        char *key, *val;
        while(*c && (val = ap_getword(r->pool, &c, '&'))) {
          key = ap_getword(r->pool, (const char **) &val, '=');
          ap_unescape_url(key);
          ap_unescape_url(val);
          ap_table_merge(Q.form_data, key, val);
        }
        ap_table_unset(r->main->notes,"ndb_request_data");
      }
      else {
        /* Fetch the update request from the client */
        response_code = read_http_post(r, & Q.form_data);
        if(response_code != OK) return response_code;
      }
      /* An INSERT has a primary key plan: */
      if(! (r->args || dir->pathinfo_size)) {
        Q.plan = PrimaryKey;
        Q.op_setup = Plan::SetupInsert;
      }
      break;
    case M_DELETE:
      if(! dir->allow_delete) return DECLINED;
      Q.op_setup = Plan::SetupDelete;
      Q.op_action = Plan::Delete;
      break;
    default:
      return DECLINED;
  }


  /* ===============================================================
     Process arguments, and then pathinfo.  Determine an access plan.
     The detailed work is done within the inlined function set_key().
  */

  if(dir->flag.table_scan) 
    Q.plan = Scan;
  else if(r->args) {  /* Arguments */
    register const char *c = r->args;
    char *key, *val;
    short n;
    
    while(*c && (val = ap_getword(r->pool, &c, '&'))) {
      key = ap_getword(r->pool, (const char **) &val, '=');
      ap_unescape_url(key);
      ap_unescape_url(val);
      n = key_col_bin_search(key, dir);
      if(n >= 0) 
        set_key(r, n, val, dir, &Q);
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
  if(r->method_number == M_GET && 
     ! ( dir->flag.table_scan || Q.key_columns_used)) {
    log_debug(r->server,"No key column aliases found in request %s", 
              r->unparsed_uri);
    return NOT_FOUND;    /* no query */
  }
  
  /* Open a transaction, if one is not already open.
     This creates an obligation to close it later, using tx->close().
  */    
  if(i->tx == 0) {
    if(i->flag.aborted) {
      /* Transaction was already aborted */
      return 500;
    }
    if(!(i->tx = i->db->startTransaction())) {  // To do: supply a hint
      log_err2(r->server,"db->startTransaction failed: %s",
                i->db->getNdbError().message);
      return 500;
    }
  }

  
  /* Now set the Query Items that depend on the access plan and index type.
  */    
  if(Q.plan == Scan) 
    q->idxobj = & TS_idxobj;
  else {
    if(Q.active_index < 0) {
      response_code = 500;
      goto abort;
    }
    if(Q.plan == PrimaryKey)
      q->idxobj = & PK_idxobj;    
    else {
      /* Lookup the active index in the data dictionary to set q->idx */
      register const char * idxname = dir->indexes->item(Q.active_index).name;
      if((q->idx = dict->getIndex(idxname, dir->table)) == 0) {
        log_err4(r->server, "mod_ndb: index %s does not exist (db: %s, table: %s)",
                 idxname, dir->database, dir->table);
        response_code = 500;
        goto abort;
      }
      if(Q.plan == UniqueIndexAccess) 
        q->idxobj = & UI_idxobj;
      else if (Q.plan == OrderedIndexScan) 
        q->idxobj = & OI_idxobj;
    }
  }
  
  // Get an NdbOperation
  q->data->op = q->idxobj->get_ndb_operation(i->tx);

  // Query setup, e.g. Plan::SetupRead calls op->readTuple() 
  if(Q.op_setup(r, dir, & Q)) { // returns 0 on success
    log_debug(r->server,"Returning 404 because Q.op_setup() failed: %s",
              q->data->scanop->getNdbError().message);
    response_code = 404;
    goto abort;
  }

  // Traverse the index parts and build the query
  if(Q.plan != Scan) {
    col = dir->indexes->item(Q.active_index).first_col;

    while (col >= 0 && Q.key_columns_used-- > 0) {
      config::key_col &keycol = dir->key_columns->item(col);
      ndb_Column = q->idxobj->get_column();

      log_debug3(r->server," ** Request column_alias: %s -- value: %s", 
                 keycol.name, Q.keys[col].value);
      
      MySQL::value(mval, r->pool, ndb_Column, Q.keys[col].value);
      if(mval_is_usable(r, mval)) {
        if(q->idxobj->set_key_part(keycol, mval)) {
          log_debug(r->server," op->equal failed, column %s", ndb_Column->getName());
          response_code = 404;
          goto abort;
        }                
      }     
      col = dir->key_columns->item(col).next_in_key;
      if(! q->idxobj->next_key_part()) break;
    }
  }
  
  // Set filters
  if(Q.plan >= Scan && Q.n_filters) {
    NdbScanFilter filter(q->data->op);
    filter.begin(NdbScanFilter::AND);
    for(int n = 0 ; n < Q.n_filters ; n++) {
      int m = Q.filter_list[n];
      runtime_col *filter_col = & Q.keys[m];
      config::key_col &keycol = dir->key_columns->item(m);
      
      ndb_Column = q->tab->getColumn(keycol.name);
      log_debug3(r->server," ** Filter : %s -- value: %s", 
                 keycol.name, filter_col->value);
      MySQL::value(mval, r->pool, ndb_Column, filter_col->value);
      filter.cmp( (NdbScanFilter::BinaryCondition) keycol.filter_op,  
                 ndb_Column->getColumnNo(), (&mval.u.val_char) ); 
    }                    
    filter.end();
  }

  // Perform the action; i.e. get the value of each column
  if(Q.op_action(r, dir, &Q)) {
    response_code = 404;
    goto abort;
  }
  if(keep_tx_open) 
    return OK;
  else
    return ExecuteAll(r, i);

  abort:
  // Look at this later.  A failure of any operation causes the whole transaction
  // to be aborted?  
  log_debug(r->server,"Aborting open transaction at '%s'",r->unparsed_uri);
  if(! response_code) response_code = 500;
  i->tx->close();
  i->tx = 0;
  i->flag.aborted = 1;
  return response_code;
}


int Plan::Read(request_rec *r, config::dir *dir, struct QueryItems *q) {  
  char **column_list;

  // Call op->getValue() for each desired result column
  column_list = dir->visible->items();
  for(int n = 0; n < dir->visible->size() ; n++) {
    q->data->result_cols[n] = q->data->op->getValue(column_list[n], 0);

    /* If the result format is "raw", check for BLOBs */
    if(dir->results == raw) {
      int isz = q->tab->getColumn(column_list[n])->getInlineSize();
      if(isz) {   /* then the column is a blob... */
        log_debug(r->server,"Treating column %s as a blob",column_list[n])
        q->data->blob = q->data->op->getBlobHandle(column_list[n]);
        q->i->flag.has_blob = 1;
      }
    }
  }
  return 0;
}


int set_up_write(request_rec *r, config::dir *dir, 
                 struct QueryItems *q, bool is_insert) 
{ 
  const NdbDictionary::Column *col;
  bool is_interpreted = 0;
  char **column_list = dir->updatable->items();
  const char *key, *val;

  // iterate over the updatable columns and set up mvalues for them
  for(int n = 0; n < dir->updatable->size() ; n++) {
    key = column_list[n];
    val = ap_table_get(q->form_data, key);
    if(val) {   
      col = q->tab->getColumn(key);
      if(col) {
        mvalue &mval = q->set_vals[n];
        MySQL::value(mval, r->pool, col, val);
        if(mval.use_value == use_interpreted) {
          is_interpreted = 1;
          log_debug3(r->server,"Interpreted update; column %s = [%s]", key,val);
        }
        else log_debug3(r->server,"Updating column %s = %s", key,val);
      }
      else log_err2(r->server,"AllowUpdate list includes invalid column name %s", key);
    }
  }
  if(is_insert) return q->data->op->insertTuple();
  return is_interpreted ? q->data->op->interpretedUpdateTuple() : 
                          q->data->op->writeTuple();
}


int Plan::Write(request_rec *r, config::dir *dir, struct QueryItems *q) {
  const NdbDictionary::Column *col;
  int eqr = 1;  
  
  // iterate over the mvalues that were set up in Plan::SetupWrite
  for(int n = 0; n < dir->updatable->size() ; n++) {
    mvalue &mval = q->set_vals[n];
    col = mval.ndb_column;
    if(col) {
      Uint64 next_value;
      if(mval_is_usable(r, mval)) {
        switch(mval.use_value) {
          case use_char:
            eqr = q->data->op->setValue(col->getColumnNo(), mval.u.val_const_char );
            break;
          case use_autoinc:
            /* to do: tunable prefetch */
            eqr = get_auto_inc_value(q->i->db, q->tab, next_value, 10);
            if(!eqr) 
              eqr = (mval.len == 8 ?
                 q->data->op->setValue(col->getColumnNo(), next_value) :
                 q->data->op->setValue(col->getColumnNo(), (Uint32) next_value));
            /* to do: else make some note of error */
            break;
          case use_null:
            eqr = q->data->op->setValue(col->getColumnNo(), 0);
            break;
          case use_interpreted: 
            if(mval.interpreted == is_increment) 
              eqr = q->data->op->incValue(col->getColumnNo(), (Uint32) 1);
            else if(mval.interpreted == is_decrement) 
              eqr = q->data->op->subValue(col->getColumnNo(), (Uint32) 1);
            else assert(0);
            break;
          default:
            eqr = q->data->op->setValue(col->getColumnNo(), (const char *) (&mval.u.val_char));
        }
      } /* if(mval_is_usable) */
      else eqr = -1;
      if(eqr) log_debug(r->server,"setValue failed: %s", q->data->op->getNdbError().message);
    }
  } // for()
  return eqr;
}


int Plan::Delete(request_rec *r, config::dir *dir, struct QueryItems *q) {
  log_debug(r->server,"Deleting Row %s","")
  return 0;
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

