## make_tail.mk

VPATH_INC = -I. -ICore -IFormat $(NSQL_INC)
INCLUDES= $(VPATH_INC) -I$(APXS_INCLUDEDIR) $(MY_INC1) $(MY_INC2) $(MY_INC3)
COMPILER_FLAGS=-c $(DEFINE) $(INCLUDES) $(DSO_CC_FLAGS) -Wall $(OPT) 

mod_ndb.o: $(MOD_SOURCE) mod_ndb.h mod_ndb_config.h output_format.h
	$(CC) $(COMPILER_FLAGS) -o $@ $<

.cc.o:
	$(CC) $(COMPILER_FLAGS) -o $@ $< 

.cpp.o:
	$(CC) $(COMPILER_FLAGS) -o $@ $< 

parsers: $(NSQL_PARS) $(JSON_PARS) 

# Link

OBJECTS=$(CORE_OBJ) $(FMT_OBJ) $(NSQL_OBJ) $(JSON_OBJ)

mod_ndb.so: $(OBJECTS)
	LD_RUN_PATH=$(LDSO_PATH) $(CC) $(DSO_LD_FLAGS) -o $@ $^ $(LIBS)

# Tools

TOOLS = $(NSQL_TOOL) $(JSON_TOOL)

tools: $(TOOLS)

# Other rules

install: mod_ndb.so
	$(APXS) -i -n 'ndb' mod_ndb.so

clean:
	rm -f mod_ndb.so $(OBJECTS) $(TOOLS) httpd.conf 

httpd.conf:
	sed -f template.sed $(TEMPLATE) > httpd.conf

start: httpd.conf
	$(START_HTTPD)
        
stop:
	$(STOP_HTTPD)
      
restart:
	$(RESTART_HTTPD)

configtest:
	$(START_HTTPD) -t