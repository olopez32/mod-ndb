## N-SQL/Module.mk

NSQL_OBJ = NSQL_Parser.o NSQL_Scanner.o

N-SQL/Parser.cpp: N-SQL/NSQL.atg N-SQL/Parser.frame N-SQL/Scanner.frame
	( cd N-SQL ; $(COCO) -namespace NSQL NSQL.atg ) 

NSQL_Parser.o: N-SQL/Parser.cpp 
	$(CC) $(COMPILER_FLAGS) -o $@ $< 

NSQL_Scanner.o: N-SQL/Scanner.cpp 
	$(CC) $(COMPILER_FLAGS) -o $@ $< 


