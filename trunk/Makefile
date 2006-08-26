APXS=apxs

MY_INC1=-I/Users/jdd/mysql-builds/5ndb/include/mysql
MY_INC2=-I/Users/jdd/mysql-builds/5ndb/include/mysql/ndb
MY_INC3=-I/Users/jdd/mysql-builds/5ndb/include/mysql/ndb/ndbapi
MY_LIBS=-L/Users/jdd/mysql-builds/5ndb/lib/mysql -lz -lm

APXS_INCLUDEDIR=/usr/include/httpd
APXS_SYSCONFDIR=/private/etc/httpd
APXS_CC=gcc
APXS_LD_SHLIB=cc
APXS_CFLAGS=-DDARWIN -DUSE_HSREGEX -DUSE_EXPAT -I../lib/expat-lite -g -Os -pipe -DHARD_SERVER_LIMIT=2048 -DEAPI
APXS_CFLAGS_SHLIB=-DSHARED_MODULE
APXS_LDFLAGS_SHLIB=-bundle -undefined suppress -flat_namespace -Wl,-bind_at_load
APXS_LIBS_SHLIB=
OBJECTS=mod_ndb.o JSON.o Query.o MySQL_Field.o config.o read_http_post.o

INCLUDES=-I$(APXS_INCLUDEDIR) $(MY_INC1) $(MY_INC2) $(MY_INC3)
LIBS=$(APXS_LIBS_SHLIB) $(MY_LIBS) -lndbclient -lmystrings -lmysys -lstdc++

mod_ndb.so: $(OBJECTS)
	$(APXS_LD_SHLIB) $(APXS_LDFLAGS_SHLIB) -o $@ $(OBJECTS) $(LIBS)

.cc.o:
	g++ -c $(INCLUDES) $(APXS_CFLAGS) $(APXS_CFLAGS_SHLIB) -Wall -O0 -o $@ $< 


mod_ndb.o: mod_ndb.cc mod_ndb.h mod_ndb_config.h
read_http_post.o: read_http_post.cc
JSON.o: JSON.cc mod_ndb.h MySQL_Field.h JSON.h
Query.o: Query.cc mod_ndb.h mod_ndb_config.h MySQL_Field.h JSON.h 
MySQL_Field.o: MySQL_Field.cc MySQL_Field.h
config.o: config.cc mod_ndb.h mod_ndb_config.h

install: mod_ndb.so
	$(APXS) -i -n 'ndb' mod_ndb.so

clean:
	-rm -f *.so *.o 

stop:
	sudo apachectl stop

start:
	sudo apachectl start

restart: install
	apachectl restart
