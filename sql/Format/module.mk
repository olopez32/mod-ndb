## Format/module.mk

FMT_OBJ = format_compiler.o format_dumper.o output_format.o

# Dependencies

output_format.o: output_format.cc output_format.h 
format_compiler.o: format_compiler.cc output_format.h format_compiler.h
format_dumper.o: output_format.h format_compiler.h format_dumper.cc
