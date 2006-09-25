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
    virtual int set_key_part(config::key_col &, mvalue &) = 0;
    virtual const NdbDictionary::Column *get_column() = 0; 
};


class PK_index_object : public index_object {
  public:
    PK_index_object(struct QueryItems *queryitems, request_rec *r) :
      index_object(queryitems, r) { } ;
      
    NdbOperation *get_ndb_operation(NdbTransaction *tx) {
      n_parts = q->tab->getNoOfPrimaryKeys();
      NdbOperation *op = tx->getNdbOperation(q->tab);
      log_debug(server, "Using primary key lookup; key %d", q->active_index);
      return op;
    };
     
    int set_key_part(config::key_col &keycol, mvalue &mval) {
      int col_id = this->get_column()->getColumnNo();
        switch(mval.use_value) {
          case use_char:
            return q->op->equal(col_id, mval.u.val_char);
          default:
            return q->op->equal(col_id, (const char *) (&mval.u.val_char));
        }
    };
    
    const NdbDictionary::Column *get_column() {
      return q->tab->getColumn(q->tab->getPrimaryKey(key_part));
    };
};
  
class Unique_index_object : public index_object {
  public:
    Unique_index_object(struct QueryItems *queryitems, request_rec *r) :
      index_object(queryitems, r) { } ;

    NdbOperation *get_ndb_operation(NdbTransaction *tx) {
      n_parts = q->idx->getNoOfColumns(); 
      NdbOperation *op = tx->getNdbIndexOperation(q->idx);
      log_debug(server, "Using UniqueIndexAccess; key %s", q->idx->getName());
      return op;
    };
    
    int set_key_part(config::key_col &keycol, mvalue &mval) {
      const char *col_name = q->idx->getColumn(key_part)->getName();
      switch(mval.use_value) {
        case use_char:
          return q->op->equal(col_name, mval.u.val_char);
        default:
          return q->op->equal(col_name, (const char *) (&mval.u.val_char)); 
      }
    };

    const NdbDictionary::Column *get_column() {
      return q->idx->getColumn(key_part);
    };    
};

class Ordered_index_object : public index_object {
  public:
    Ordered_index_object(struct QueryItems *queryitems, request_rec *r) :
      index_object(queryitems, r) { } ;

    NdbOperation *get_ndb_operation(NdbTransaction *tx) {
      n_parts = q->idx->getNoOfColumns(); 
      q->scanop = tx->getNdbIndexScanOperation(q->idx);
      log_debug(server, "Using OrderedIndexScan; key %s", q->idx->getName());
      return q->scanop;
    };
    
    int set_key_part(config::key_col &keycol, mvalue &mval) {
      int col_id = q->idx->getColumn(key_part)->getColumnNo();
      
      return q->scanop->setBound(col_id, keycol.filter_op, &mval.u.val_char);      
    };

    const NdbDictionary::Column *get_column() {
      return q->idx->getColumn(key_part);
    };
    
};

