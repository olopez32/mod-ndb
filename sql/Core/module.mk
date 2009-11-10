## Core/module.mk

CORE_SRC := Execute.cc config.cc query_source.cc MySQL_result.cc \
handlers.cc	request_body.cc MySQL_value.cc result_buffer.cc Query.cc

CORE_OBJ := $(patsubst %,${OBJDIR}/%, ${CORE_SRC:%.cc=%.o}) $(OBJDIR)/mod_ndb.o

# Dependencies        

$(OBJDIR)/handlers.o: handlers.cc mod_ndb.h query_source.h
$(OBJDIR)/request_body.o: request_body.cc JSON/Parser.cpp
$(OBJDIR)/Query.o: Query.cc mod_ndb.h mod_ndb_config.h MySQL_value.h \
 MySQL_result.h index_object.h query_source.h
$(OBJDIR)/Execute.o: Execute.cc mod_ndb.h result_buffer.h output_format.h
$(OBJDIR)/MySQL_value.o: MySQL_value.cc MySQL_value.h
$(OBJDIR)/MySQL_result.o: MySQL_result.cc MySQL_value.h result_buffer.h
$(OBJDIR)/config.o: config.cc mod_ndb.h mod_ndb_config.h $(NSQL)/Parser.cpp defaults.h
$(OBJDIR)/result_buffer.o: result_buffer.cc mod_ndb.h result_buffer.h
$(OBJDIR)/query_source.o: mod_ndb.h query_source.h 

# Special rule for mod_ndb.o

$(OBJDIR)/mod_ndb.o: $(MOD_SOURCE) mod_ndb.h mod_ndb_config.h output_format.h
	$(CC) $(COMPILER_FLAGS) -o $@ $<
