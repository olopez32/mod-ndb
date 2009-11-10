## Format/module.mk

FMT_SRC := output_format.cc format_compiler.cc format_dumper.cc
FMT_OBJ := $(patsubst %,${OBJDIR}/%, ${FMT_SRC:%.cc=%.o})

# Dependencies

FMT_HEADERS := output_format.h format_compiler.h 
$(OBJDIR)/output_format.o: $(FMT_HEADERS)
$(OBJDIR)/format_compiler.o: $(FMT_HEADERS)
$(OBJDIR)/format_dumper.o: $(FMT_HEADERS)
