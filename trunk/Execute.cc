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
  log_debug(r->server,"Setting note %s",note);
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
  
  log_debug(r->server, "Entering ExecuteAll() with %d read operations",
            i->n_read_ops);
            
  /* Check for an NdbTransaction */
  if(! i->tx) {
    log_err(r->server, "tx does not exist.");
    response_code = HTTP_GONE;  /* Should be 400? */
    goto cleanup2;
  } 

 
  /* Determine whether the output should be written as an Apache note
  */ 
  if(r->main) {
    /* This is a subrequest.
       But if the user has set a note called "ndb_send_result", 
       then send the result on directly to the client anyway. */
    if(! ap_table_get(r->main->notes,"ndb_send_result"))  // UNTESTED!
      apache_notes = 1;
  }
 
  /* If an operation involves reading a BLOB, then some special cases apply:
     - There can only be one operation in the transaction.
     - The transaction must be executed "NoCommit" before reading the BLOB
     - BLOBs that are truly binary cannot be returned as Apache notes 
  */
  if(i->flag.has_blob) {
    /* Execute NoCommit */
    if(i->tx->execute(NdbTransaction::NoCommit, NdbTransaction::AbortOnError, 
                      i->conn->ndb_force_send))
    {        
      log_debug(r->server, "tx->execute() with BLOB failed: %s", 
                i->tx->getNdbError().message);
      response_code = HTTP_GONE;  /* Should be 400? */
    }
 
    /* Loop over operations & find BLOBs) */
    for(opn = 0 ; opn < i->n_read_ops ; opn++) {
      struct data_operation *data = i->data + opn ;
      if(data->blob && data->result_cols) {
        build_results = result_formatter[data->result_format];
        if(build_results) {
          response_code = build_results(r, data, my_results);
          if(apache_notes) set_note(r, opn, my_results);
        }
      }
    }
  }
  
  /* Execute and Commit the transaction */
  if(i->tx->execute(NdbTransaction::Commit, NdbTransaction::AbortOnError, 
                    i->conn->ndb_force_send)) 
  {        
    log_debug(r->server,"tx->execute failed: %s", i->tx->getNdbError().message);
    response_code = HTTP_GONE;  /* Should be 400? */
    goto cleanup1;
  } 
  
  /* Loop over the operations and build the result page */
  for(opn = 0 ; opn < i->n_read_ops ; opn++) {
    struct data_operation *data = i->data + opn ;
    if(data->result_cols && (! data->blob)) {
      build_results = result_formatter[data->result_format];
      if(build_results) {
        response_code = build_results(r, data, my_results);
        if(apache_notes) set_note(r, opn, my_results);
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
    if(i->flag.use_etag && my_results.buff) {
      char *etag = ap_md5_binary(r->pool, (const unsigned char *) 
                                 my_results.buff, my_results.sz);
      ap_table_setn(r->headers_out, "ETag",  etag);
      // If the ETag matches the client's cache, the page should not be returned 
      response_code = ap_meets_conditions(r);
    }
        
    /* Send the page  (but recheck the response code,
      after ap_meets_conditions) */   
    if(response_code == OK) {
      ap_send_http_header(r);
      if(my_results.buff)
        ap_rwrite(my_results.buff, my_results.sz, r);
    }
  }

  cleanup1:
  i->tx->close();
  i->tx = 0;  
  
  cleanup2:
  /* Clear all used operations */
  bzero(i->data, i->n_read_ops * sizeof(struct data_operation));
  i->n_read_ops = 0;
  i->flag.aborted  = 0;
  i->flag.has_blob = 0;
  i->flag.use_etag = 0;
  
  log_debug(r->server,"ExecuteAll() returning %d",response_code);
  return response_code;
}


/******** Result Page formatters *************/


inline void JSON_send_result_row(data_operation *data, result_buffer &res) {
  JSON::new_object(res);
  for(unsigned int n = 0; n < data->n_result_cols ; n++) {
    if(n) JSON::delimiter(res);
    JSON::put_member(res, *data->result_cols[n]);
  }
  JSON::end_object(res);
}


int Results_JSON(request_rec *r, data_operation *data, 
                 result_buffer &res) {
  int nrows = 0;
  res.init(r, 8192);
  
  if(data->scanop) {
    while((data->scanop->nextResult(true)) == 0) {
      do {
        if(nrows++) res.out(2,",\n");
        else JSON::new_array(res);
        JSON_send_result_row(data, res);
      } while((data->scanop->nextResult(false)) == 0);
    }
    if(nrows) JSON::end_array(res);
    else return HTTP_GONE; // ??
  }
  else {
    JSON_send_result_row(data, res);
  }
  
  res.out(1,"\n");
  return OK;
}


inline void XML_send_result_row(data_operation *data, result_buffer &res) {
  for(unsigned int n = 0; n < data->n_result_cols ; n++) {
    if(n) XML::delimiter(res);
    XML::put_member(res, *data->result_cols[n]);
  }
}


int Results_XML(request_rec *r, data_operation *data, 
                result_buffer &res) {
  int nrows = 0;
  res.init(r, 8192);
  
  if(data->scanop) {
    while((data->scanop->nextResult(true)) == 0) {
      do {
        if(nrows++) res.out(1,"\n"); 
        else XML::new_array(res);
        XML::new_object(res);
        XML_send_result_row(data, res);
        XML::end_object(res);
      } while((data->scanop->nextResult(false)) == 0);
    }
    if(nrows) XML::end_array(res);
    else return HTTP_GONE;  // ??
  }
  else {
    XML::new_object(res);
    XML_send_result_row(data, res);
    res.out(1,"\n");
    XML::end_object(res);    
  }
  
  res.out(1,"\n");
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




