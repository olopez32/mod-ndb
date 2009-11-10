## NSQL/module.mk

NSQL_COCO_OBJ=$(OBJDIR)/NSQL_Parser.o $(OBJDIR)/NSQL_Scanner.o 
NSQL_PARS := NSQL/Parser.cpp 
# NSQL_TOOL := test-sql

TREE_SRC := ANDNode.cpp	FieldNode.cpp LTNode.cpp ORNode.cpp Symbol.cpp \
BindNode.cpp FromNode.cpp LiteralNode.cpp OrderNode.cpp	TableNode.cpp \
DeleteNode.cpp GTENode.cpp NEQNode.cpp QueryNode.cpp WhereNode.cpp \
EQNode.cpp GTNode.cpp NSQLState.cpp SelectNode.cpp Environment.cpp \
LTENode.cpp Node.cpp StarNode.cpp

VIS_SRC := PrettyPrintVisitor.cpp SemanticCheck.cpp


# Parser and Scanner .cpp files:

NSQL/Parser.cpp: NSQL/NSQL.atg NSQL/Parser.frame NSQL/Scanner.frame
	( cd NSQL/NSQL ; $(COCO) -namespace NSQL NSQL.atg ) 

# Parser & Scanner object files
$(OBJDIR)/NSQL_Parser.o: NSQL/NSQL/Parser.cpp 
	$(CC) $(COMPILER_FLAGS) -o $@ $< 

$(OBJDIR)/NSQL_Scanner.o: NSQL/NSQL/Scanner.cpp 
	$(CC) $(COMPILER_FLAGS) -o $@ $< 

## fixme: hardcoded -lapr-1
#test-sql: $(NSQL_OBJ) $(NSQL_TEST_OBJ) $(NSQL_LIB) JSON_Scanner.o
#	$(CC) $^ -lreadline -lapr-1 $(LIBS) -o NSQL/$@ 
#
#NSQLtestmain.o: NSQL.cpp
#	$(CC) $(COMPILER_FLAGS) -INSQL/NSQL -INSQL/Utilities -o $@ $<
#


#--------------------------------------------------
# Things required by the main makefile:
## Required:  NSQL_OBJ, NSQL_INC, and NSQL_TOOL:
NSQL_SRC = $(TREE_SRC) $(VIS_SRC) 
NSQL_OBJ = $(patsubst %,${OBJDIR}/%, ${NSQL_SRC:%.cpp=%.o}) $(NSQL_COCO_OBJ)
NSQL_INC := -INSQL -INSQL/Tree -INSQL/Visitors
#--------------------------------------------------

### ----------------------------------------------------------------------

