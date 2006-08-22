class JSON {
  public:
    static const char * new_array;
    static const char * end_array;
    static const char * new_object;
    static const char * end_object;
    static const char * delimiter ;
    static const char * is        ;
    static char *value(const NdbRecAttr &rec, request_rec *r);
    inline static char *member(const NdbRecAttr &rec, request_rec *r) {
      return ap_pstrcat(r->pool, 
                        rec.getColumn()->getName(), 
                        JSON::is,
                        JSON::value(rec,r),
                        NULL);    
    }
};
