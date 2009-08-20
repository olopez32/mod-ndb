## JSON/module.mk

JSON_PARS = JSON_Parser.o 

JSON_OBJ = JSON_Parser.o JSON_Scanner.o JSON_encoding.o

JSON_TOOL = test_encoding

JSON/Parser.cpp: JSON/JSON.atg JSON/Parser.frame JSON/Scanner.frame
	( cd JSON ; $(COCO) -namespace JSON JSON.atg )
      
JSON_Parser.o: JSON/Parser.cpp 
	$(CC) $(COMPILER_FLAGS) -o $@ $< 
        
JSON_Scanner.o: JSON/Scanner.cpp
	$(CC) $(COMPILER_FLAGS) -o $@ $< 

test_encoding: test_encoding.c  
	$(CC) $< -lreadline -o JSON/$@ 
