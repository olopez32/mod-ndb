## NSQL/module.mk

TREE_OBJ=ANDNode.o FieldNode.o LTNode.o ORNode.o \
Symbol.o BindNode.o FromNode.o LiteralNode.o \
OrderNode.o TableNode.o DeleteNode.o GTENode.o \
NEQNode.o QueryNode.o WhereNode.o EQNode.o GTNode.o \
NSQLState.o SelectNode.o Environment.o LTENode.o \
Node.o StarNode.o

VIS_OBJ=NSQLVisitor.o PrettyPrintVisitor.o SemanticCheck.o 

UTIL_OBJ=DBConnection.o 

NSQL_OBJ=NSQL_Parser.o NSQL_Scanner.o $(TREE_OBJ) $(VIS_OBJ) $(UTIL_OBJ) 

NSQL_PARS = NSQL/Parser.cpp 

NSQL_INC=-INSQL -INSQL/Tree -INSQL/Utilities -INSQL/Visitors

NSQL_FLAGS=$(COMPILER_FLAGS) $(NSQL_INC)

NSQL/Parser.cpp: NSQL/NSQL.atg NSQL/Parser.frame NSQL/Scanner.frame
	( cd NSQL/NSQL ; $(COCO) -namespace NSQL NSQL.atg ) 

NSQL_Parser.o: NSQL/Parser.cpp 
	$(CC) $(NSQL_FLAGS) -o $@ $< 

NSQL_Scanner.o: NSQL/Scanner.cpp 
	$(CC) $(NSQL_FLAGS) -o $@ $< 
        
