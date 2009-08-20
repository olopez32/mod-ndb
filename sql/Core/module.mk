## Core/module.mk

CORE_OBJ=mod_ndb.o Query.o Execute.o MySQL_value.o MySQL_result.o \
config.o request_body.o handlers.o result_buffer.o output_format.o \
format_compiler.o format_dumper.o query_source.o  JSON_encoding.o

# Dependencies        

handlers.o: handlers.cc mod_ndb.h query_source.h
request_body.o: request_body.cc
Query.o: Query.cc mod_ndb.h mod_ndb_config.h MySQL_value.h MySQL_result.h index_object.h query_source.h
Execute.o: Execute.cc mod_ndb.h result_buffer.h output_format.h
MySQL_value.o: MySQL_value.cc MySQL_value.h
MySQL_result.o: MySQL_result.cc MySQL_value.h result_buffer.h
config.o: config.cc mod_ndb.h mod_ndb_config.h $(NSQL)/Parser.cpp defaults.h
result_buffer.o: result_buffer.cc mod_ndb.h result_buffer.h
output_format.o: output_format.cc output_format.h 
format_compiler.o: output_format.h format_compiler.h
format_dumper.o: output_format.h format_compiler.h
query_source.o: mod_ndb.h query_source.h 
