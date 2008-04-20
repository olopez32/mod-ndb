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
#include "ndb_api_compat.h"


inline void set_note(request_rec *r, int num, result_buffer &res) {
  char note[32];
  sprintf(note, "ndb_result_%d",num);
  log_debug(r->server,"Setting note %s",note);
  ap_table_set(r->main->notes, note, res.buff);
}


inline void milliSleep(int milliseconds){
  struct timeval sleeptime;
  
  sleeptime.tv_sec = milliseconds / 1000;
  sleeptime.tv_usec = (milliseconds - (sleeptime.tv_sec * 1000)) * 1000000;
  select(0, 0, 0, 0, &sleeptime);
}


/* handle_exec_error():
   set response code, and return 0 on pass-through or 1 on retry
*/
bool handle_exec_error(request_rec *r, int &response_code, 
                               const NdbError &error) {  
  bool stale_dictionary = 0;
  
  // client errors
  if(error.classification == NdbError::NoDataFound)
    response_code = 404;
  else if(error.classification == NdbError::ConstraintViolation)
    response_code = 409;
  // server errors
  else if(error.classification == NdbError::SchemaError) {
    switch(error.code) {
      case 241:  /* Invalid schema object version */
      case 283:  /* Table is being dropped */
      case 284:  /* Table not defined in transaction coordinator */
      case 709:  /* No such table existed */
        stale_dictionary = 1;
        response_code = 500;
        break;
      default:
        log_err(r->server,"tx execute failed: %s %s", error.message, error.details);
        response_code = 500;
    }
  }
  // misc. server error 
  else {
    response_code = 500;  
    log_err(r->server,"tx execute failed: %s %s", error.message, error.details);
  }
  return stale_dictionary;
}


/******** Execute batched transactions *************/

int ExecuteAll(request_rec *r, ndb_instance *i) {
 
  int response_code = OK;
  int opn;     // operation number
  unsigned int retries = 0, total_wait_time = 0;
  bool apache_notes = 0, must_restart = 0;
  char *error_message = 0;
  result_buffer my_results;
  my_results.buff = 0;
  
  log_debug(r->server, "Ready to execute transaction with %d read operation%s",
            i->n_read_ops, i->n_read_ops == 1 ? "" : "s");

  /* Check for an NdbTransaction */
  if(! i->tx) {
    log_err(r->server, "tx does not exist.");
    response_code = 400;
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
     - There can only be one operation in the transaction. (??)
     - The transaction must be executed "NoCommit" before reading the BLOB
     - BLOBs that are truly binary cannot be returned as Apache notes 
  */
  if(i->flag.has_blob) {
    /* Execute NoCommit */
    if(i->tx->execute(NdbTransaction::NoCommit)) {
      must_restart = handle_exec_error(r, response_code,  i->tx->getNdbError());
      goto cleanup1;
    }
    /* Loop over operations & find BLOBs) */
    for(opn = 0 ; opn < i->n_read_ops ; opn++) {
      struct data_operation *data = i->data + opn ;
      if(data->flag.has_blob && data->result_cols) {
        response_code = build_results(r, data, my_results);
        if(apache_notes) set_note(r, opn, my_results);
      }
    }
  }
  
  /* Execute and Commit the transaction */
  exec_commit:
  i->tx->execute(NdbTransaction::Commit, TX_ABORT_OPT, i->conn->ndb_force_send); 

  if(i->tx->getNdbError().status == NdbError::TemporaryError) {
    register unsigned int sleep_ms = 5 + ( 2 * retries * retries);
    if(total_wait_time + sleep_ms < i->server_config->max_retry_ms) {
      milliSleep(sleep_ms);  
      total_wait_time += sleep_ms;
      retries++;
      goto exec_commit;
    }  /* else: */
    response_code = 503;
    goto cleanup1;
  }

  if(i->tx->getNdbError().classification != NdbError::NoError) {
    must_restart = handle_exec_error(r, response_code, i->tx->getNdbError());
    goto cleanup1;
  }
  
  /* Loop over the operations and build the result page */
  for(opn = 0 ; opn < i->n_read_ops ; opn++) {
    struct data_operation *data = i->data + opn ;
    if(data->result_cols && (! data->flag.has_blob) && data->fmt) {
      if(i->flag.jsonrequest && (! data->fmt->flag.is_JSON))
        response_code = 406;  // "406 NOT ACCEPTABLE"
      else response_code = build_results(r, data, my_results);
      if(apache_notes) set_note(r, opn, my_results);
    }
  }
  
  if(response_code == OK && (! apache_notes)) {
    // Set content-length
    if(my_results.buff)
      ap_set_content_length(r, my_results.sz);
    else {
      ap_set_content_length(r, 0);
      response_code = 204;  // No content
    }
    
    // Set response Content-Type for JSONRequest 
    if(i->flag.jsonrequest) r->content_type = "application/jsonrequest";
    
    // Set ETag
    if(i->flag.use_etag && my_results.buff) {
      char *etag = ap_md5_binary(r->pool, (const unsigned char *) 
                                 my_results.buff, my_results.sz);
      ap_table_setn(r->headers_out, "ETag",  etag);
      // If the ETag matches the client's cache, the page should not be returned 
      response_code = ap_meets_conditions(r);
    }
        
    /* Send the page (but recheck the response code,
      after ap_meets_conditions) */   
    if(response_code == OK) {
      ap_send_http_header(r);
      if(my_results.buff)
        ap_rwrite(my_results.buff, my_results.sz, r);
    }
  }

  cleanup1:
  if(must_restart && i->server_config->force_restart) {
    response_code = ndb_handle_error(r, 503, & i->tx->getNdbError(), "10");
    module_must_restart();
  }
  else if(response_code > 399) 
    response_code = ndb_handle_error(r, response_code, 
                                     & i->tx->getNdbError(), error_message); 
  i->tx->close();
  i->tx = 0;  
  
  cleanup2:
  log_debug(r->server, "Returning %d%s", response_code,
            response_code ? (response_code == -2 ? " (DONE)":"") : " (OK)");
  i->cleanup(); // Clear all used operations
  return response_code;
}

