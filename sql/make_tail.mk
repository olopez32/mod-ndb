## make_tail.mk

.PHONY: all prep clean stop start restart configtest tools parsers

VPATH_INC = -I. -ICore -IFormat $(NSQL_INC)
INCLUDES= $(VPATH_INC) -I$(APXS_INCLUDEDIR) $(MY_INC1) $(MY_INC2) $(MY_INC3)
COMPILER_FLAGS=-c $(DEFINE) $(INCLUDES) $(DSO_CC_FLAGS) -Wall $(OPT) 

${OBJDIR}/%.o: %.cc
	$(CC) $(COMPILER_FLAGS) -o $@ $< 

${OBJDIR}/%.o: %.cpp
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
	rm -f mod_ndb.so $(NSQL_TEST_OBJ) $(OBJECTS) $(TOOLS) httpd.conf 

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
