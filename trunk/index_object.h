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


class index_object {
public:
    int key_part;
    server_rec *server;
    struct QueryItems *q;
    int n_parts;
    
    index_object(struct QueryItems *queryitems, request_rec *r) {  
      q = queryitems;
      server = r->server;
      key_part = 0; 
    };
    virtual ~index_object() {};

    virtual NdbOperation *get_ndb_operation(NdbTransaction *) = 0;
    bool next_key_part() {  return (key_part++ < n_parts); };
    virtual int set(int col_id, int, mvalue &mval) {
      if(mval.use_value == use_char) {
        log_debug(server, "Set key: %d = %s", col_id, mval.u.val_char);
        return q->data->op->equal(col_id, mval.u.val_char);
      }
      else {
        log_debug(server, "Set key: %d = %d", col_id, mval.u.val_signed);
        return q->data->op->equal(col_id, (const char *) (&mval.u.val_char));      
      }
    };    
    virtual const NdbDictionary::Column *get_column(base_expr &) {
      return q->idx->getColumn(key_part);
    };
};


class PK_index_object : public index_object {
  public:
    PK_index_object(struct QueryItems *queryitems, request_rec *r) :
      index_object(queryitems, r) { } ;
      
    NdbOperation *get_ndb_operation(NdbTransaction *tx) {
      log_debug(server, "Using primary key lookup; key %d", q->active_index);
      n_parts = q->tab->getNoOfPrimaryKeys();
      return tx->getNdbOperation(q->tab);
    };
     
    const NdbDictionary::Column *get_column(base_expr &) {
      return q->tab->getColumn(q->tab->getPrimaryKey(key_part));
    };
};

  
class Unique_index_object : public index_object {
  public:
    Unique_index_object(struct QueryItems *queryitems, request_rec *r) :
      index_object(queryitems, r) { } ;

    NdbOperation *get_ndb_operation(NdbTransaction *tx) {
      log_debug(server, "Using UniqueIndexAccess; key %s", q->idx->getName());
      n_parts = q->idx->getNoOfColumns(); 
      NdbOperation *op = tx->getNdbIndexOperation(q->idx);
      return op;
    };
};


class Ordered_index_object : public index_object {
  public:
    Ordered_index_object(struct QueryItems *queryitems, request_rec *r) :
      index_object(queryitems, r) { } ;

    NdbOperation *get_ndb_operation(NdbTransaction *tx) {
      n_parts = q->idx->getNoOfColumns(); 
      q->data->scanop = tx->getNdbIndexScanOperation(q->idx);
      log_debug(server, "Using OrderedIndexScan; key %s", q->idx->getName());
      return q->data->scanop;
    };
    
    const NdbDictionary::Column *get_column(base_expr &) {
      return q->idx->getColumn(key_part);
    };

    int set(int col_id, int rel_op, mvalue &mval) {
      if(mval.use_value == use_char) {
        log_debug(server, "OI set: %d %d %s", col_id, rel_op, mval.u.val_char);
        return q->data->scanop->setBound(col_id, rel_op, mval.u.val_char);
      }
      else {
        log_debug(server, "OI set: %d %d %d", col_id, rel_op, mval.u.val_signed);       
        return q->data->scanop->setBound(col_id, rel_op, &mval.u.val_char);    
      }
    };
};


class Table_Scan_object : public index_object {
public:
  Table_Scan_object(struct QueryItems *queryitems, request_rec *r) :
  index_object(queryitems, r) { } ;  
  NdbScanOperation *ts_op;
  
  NdbOperation *get_ndb_operation(NdbTransaction *tx) {
    n_parts = 0; 
    ts_op = tx->getNdbScanOperation(q->tab);
    q->data->scanop = static_cast<NdbIndexScanOperation *> (ts_op);
    return ts_op;
  };

  int set(int col_id, int rel_op, mvalue &mval) {   
    log_debug(server, "In Table_Scan_Object::set()");
    return 0;
  };
};


