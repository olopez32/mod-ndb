#!/usr/sbin/dtrace -F -s 

/* traces calls in mod_ndb.so -- starting in Query() -- 
   plus entrances into the NDB API   */

/* Usage: trace.d APACHE_PID   */


 pid$1:mod_ndb:Query*:entry         { self->trace = 1; self->in_lib = 0; }
        
 pid$1:mod_ndb::entry , 
 pid$1:mod_ndb::return                
    /self->trace/                   { printf("%s",""); self->in_lib = 0; }

 pid$1:libndbclient::return 
    /self->trace/                   { self->in_lib--; }

 pid$1:libndbclient::entry,
 pid$1:libndbclient::return     
    /self->trace && ! self->in_lib/ { }

 pid$1:libndbclient::entry 
    /self->trace/                   { self->in_lib++; }

 pid$1:mod_ndb:Query*:return        { self->trace = 0; }
