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

/*  This code is only used in Query.cc, and, following GCC's "vague linkage" 
    rules, it puts the vtable and object code in Query.o
*/

class index_object {
  protected:
    int key_part;
    server_rec *server;
    struct QueryItems *q;
    int n_parts;
    int set_key_num(int num, mvalue &mval) {
      if(mval.use_value == use_char) 
        return q->data->op->equal(num, mval.u.val_char);
      else 
        return q->data->op->equal(num, (const char *) (&mval.u.val_char)); 
    };
    
  public:
    index_object(struct QueryItems *queryitems, request_rec *r) {  
      q = queryitems;
      server = r->server;
      key_part = 0; 
    };
    virtual ~index_object() {};
    virtual NdbOperation *get_ndb_operation(NdbTransaction *) = 0;
    virtual bool next_key_part() {  return (key_part++ < n_parts); };
    virtual const NdbDictionary::Column *get_column(base_expr &) = 0;
    virtual int set_key_part(int, mvalue &mval) = 0;
};


class PK_index_object : public index_object {
  private:
    int attr_id;
  public:
    PK_index_object(struct QueryItems *queryitems, request_rec *r) :
      index_object(queryitems, r) { } ;
      
    NdbOperation *get_ndb_operation(NdbTransaction *tx) {
      log_debug(server, "Using primary key lookup.");
      n_parts = q->tab->getNoOfPrimaryKeys();
      return tx->getNdbOperation(q->tab);
    };
     
    const NdbDictionary::Column *get_column(base_expr &) {
      const NdbDictionary::Column *col = 
        q->tab->getColumn(q->tab->getPrimaryKey(key_part));
      attr_id = col->getColumnNo();
      return col;
    };

    int set_key_part(int, mvalue &mval) {
      return set_key_num(attr_id, mval);
    }
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

    const NdbDictionary::Column *get_column(base_expr &) {
      return q->idx->getColumn(key_part);
    };

    int set_key_part(int, mvalue &mval) {
      return set_key_num(key_part, mval);
    }
};


class Ordered_index_object : public index_object {
  private:
    int parts_used;
    const char *key_part_name;
  public:
    Ordered_index_object(struct QueryItems *queryitems, request_rec *r) :
      index_object(queryitems, r) 
    { 
      parts_used = 0;
    };

    NdbOperation *get_ndb_operation(NdbTransaction *tx) {
      n_parts = q->idx->getNoOfColumns(); 
      q->data->scanop = tx->getNdbIndexScanOperation(q->idx);
      log_debug(server, "Using OrderedIndexScan; key %s", q->idx->getName());
      return q->data->scanop;
    };
    
    const NdbDictionary::Column *get_column(base_expr &expr) {
      key_part_name = expr.base_col_name;
      return (*key_part_name ? 
              q->tab->getColumn(key_part_name) : 
              q->idx->getColumn(key_part) );
    };

    int set_key_part(int rel_op, mvalue &mval) {
      parts_used++;
      if(*key_part_name) {
        if(mval.use_value == use_char) 
          return q->data->scanop->setBound(key_part_name, rel_op, mval.u.val_char);
        else 
          return q->data->scanop->setBound(key_part_name, rel_op, &mval.u.val_char);    
      }
      else {
        if(mval.use_value == use_char) 
          return q->data->scanop->setBound(key_part, rel_op, mval.u.val_char);
        else 
          return q->data->scanop->setBound(key_part, rel_op, &mval.u.val_char);
      }
    };
    
    bool next_key_part() {  
      key_part++;
      return (parts_used < n_parts); 
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

  const NdbDictionary::Column *get_column(base_expr &) { assert(0); return 0; };
  int set_key_part(int rel_op, mvalue &mval) { assert(0); return 0; };
  
};


