## JSON/module.mk

JSON_SRC := JSON_encoding.cc
JSON_COCO_OBJ = $(OBJDIR)/JSON_Parser.o $(OBJDIR)/JSON_Scanner.o
JSON_OBJ := $(OBJDIR)/JSON_encoding.o $(JSON_COCO_OBJ)
JSON_TOOL = test_encoding

#### Parser and Scanner rules
JSON/Parser.cpp: JSON/JSON.atg JSON/Parser.frame JSON/Scanner.frame
	( cd JSON ; $(COCO) -namespace JSON JSON.atg )
      
$(OBJDIR)/JSON_Parser.o: JSON/Parser.cpp 
	$(CC) $(COMPILER_FLAGS) -o $@ $< 
        
$(OBJDIR)/JSON_Scanner.o: JSON/Scanner.cpp
	$(CC) $(COMPILER_FLAGS) -o $@ $< 
####  ------------------------

test_encoding: test_encoding.c  
	$(CC) $< -lreadline -o $@ 
