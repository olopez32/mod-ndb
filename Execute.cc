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
#include "util_md5.h"


/*  Result formatters:  */

typedef int ResultBuilder(request_rec *, data_operation *, result_buffer &);

ResultBuilder Results_JSON;
ResultBuilder Results_raw;
ResultBuilder Results_XML;

/* A map from enum result_format_type to the corresponding function: */
 ResultBuilder *result_formatter[4] = 
{ 
  0 , 
  Results_JSON , 
  Results_raw, 
  Results_XML 
};


inline void set_note(request_rec *r, int num, result_buffer &res) {
  char note[32];
  sprintf(note, "ndb_result_%d",num);
  ap_table_set(r->main->notes, note, res.buff);
}


/******** Execute a batched transaction *************/

int ExecuteAll(request_rec *r, ndb_instance *i) {
 
  int response_code = OK;
  int opn;     // operation number
  result_buffer my_results;
  my_results.buff = 0;
  ResultBuilder *build_results;
  bool apache_notes = 0;
  int use_etags = 0;
  
  /* Check for an NdbTransaction */
  if(! i->tx) {
    log_err(r->server, "ExecuteAll() returning 410 because tx does not exist.");
    response_code = HTTP_GONE; 
    goto cleanup;
  } 
 
  /* Determine whether the output should be written as an Apache note
  */ 
  if(r->main) {
    /* This is a subrequest.
       But if the user has set a note called "ndb_send_result", 
       then send the result on directly to the client anyway. */
    if(! ap_table_get(r->main->notes,"ndb_send_result"))
      apache_notes = 1;
  }
 
  /* If an operation involves reading a BLOB, then some special cases apply:
     - There can only be one operation in the transaction.
     - The transaction must be executed "NoCommit" before reading the BLOB
     - BLOBs that are truly binary cannot be returned as Apache notes 
  */
  if(i->flags.has_blob) {
    /* Execute NoCommit */
    if(i->tx->execute(NdbTransaction::NoCommit, NdbTransaction::AbortOnError, 
                      i->conn->ndb_force_send))
    {        
      log_debug(r->server,
                "Execute with BLOB: code 410 because tx->execute() failed: %s",
                i->tx->getNdbError().message);
      response_code = HTTP_GONE;
    }
 
    /* Loop over operations & find BLOBs) */
    for(opn = 0 ; opn < i->n_read_ops ; opn++) {
      struct data_operation *data = i->data + opn ;
      if(data->blob && data->result_cols) {
        build_results = result_formatter[data->dir->results];
        if(build_results) {
          response_code = build_results(r, data, my_results);
          if(apache_notes) set_note(r, opn, my_results);
          else use_etags += data->dir->use_etags;
        }
      }
    }
  }
  
  /* Execute and Commit the transaction */
  if(i->tx->execute(NdbTransaction::Commit, NdbTransaction::AbortOnError, 
                    i->conn->ndb_force_send))
  {        
    log_debug(r->server,"Returning 410 because tx->execute failed: %s",
              i->tx->getNdbError().message);
    response_code = HTTP_GONE;
    goto cleanup;
  } 
  
  /* Loop over the operations and build the result page */
  /* Recognize read operations by data->result_cols being non-null */
  for(opn = 0 ; opn < i->n_read_ops ; opn++) {
    struct data_operation *data = i->data + opn ;
    if(data->result_cols && (! data->blob)) {
      build_results = result_formatter[data->dir->results];
      if(build_results) {
        response_code = build_results(r, data, my_results);
        if(apache_notes) set_note(r, opn, my_results);
        else use_etags += data->dir->use_etags;
      }
    }
  }
  
  if(response_code == OK && (! apache_notes)) {
    // Set content-length
    if(my_results.buff)
      ap_set_content_length(r, my_results.sz);
    else 
      ap_set_content_length(r, 0);
    
    // Set ETag
    if(use_etags && my_results.buff) {
      char *etag = ap_md5_binary(r->pool, (const unsigned char *) 
                                 my_results.buff, my_results.sz);
      ap_table_setn(r->headers_out, "ETag",  etag);
      // If the ETag matches the client's cache, the page should not be returned 
      response_code = ap_meets_conditions(r);
    }
    
    ap_send_http_header(r);
    
    /* Send the page  (but recheck the response code,
      after ap_meets_conditions) */   
    if(response_code == OK && my_results.buff)
      ap_rwrite(my_results.buff, my_results.sz, r);
  }

  cleanup:
  
  i->tx->close();
  i->tx = 0;
  i->n_read_ops = 0;
  i->flags.aborted  = 0;
  i->flags.has_blob = 0;
  /* Clear all used operations */
  bzero(i->data, i->n_read_ops * sizeof(struct data_operation));

  return response_code;
  
}


/******** Result Page formatters *************/


inline void JSON_send_result_row(request_rec *r, data_operation *data, 
                                 result_buffer &res) {
  res.out(JSON::new_object);
  for(int n = 0; n < data->dir->visible->size() ; n++) {
    if(n) res.out(JSON::delimiter);
    JSON::put_member(res, *data->result_cols[n], r);
  }
  res.out(JSON::end_object);
}


int Results_JSON(request_rec *r, data_operation *data, 
                 result_buffer &res) {
  int nrows = 0;
  res.init(r, 8192);
  
  if(data->scanop) {
    /* Multi-row result set */   
    while((data->scanop->nextResult(true)) == 0) {
      do {
        if(nrows++) res.out(",\n");
        else res.out(JSON::new_array,r);
        JSON_send_result_row(r, data, res);
      } while((data->scanop->nextResult(false)) == 0);
    }
    if(nrows) res.out(JSON::end_array);
    else return HTTP_GONE;
  }
  else {
    /* Single row result set */
    JSON_send_result_row(r, data, res);
  }
  
  res.out("\n");
  return OK;
}


int Results_raw(request_rec *r, data_operation *data, 
                result_buffer &res) {
  unsigned long long size64 = 0;
  unsigned int size;
  
  if(data->blob) {
    data->blob->getLength(size64);  //passed by reference
    size = (unsigned int) size64;
    res.init(r, size);
    if(data->blob->readData(res.buff, size)) 
      log_debug(r->server,"Error reading blob data: %s",
                data->blob->getNdbError().message);
    res.sz = size;
  }
  return OK;
}

int Results_XML(request_rec *r, data_operation *data, 
                result_buffer &res) {
  log_debug(r->server,"In Results formatter %s", "Results_XML");
  
  return NOT_FOUND;
}



