## NSQL/module.mk

NSQL_COCO_OBJ := $(OBJDIR)/NSQL_Parser.o $(OBJDIR)/NSQL_Scanner.o 

TREE_SRC := ANDNode.cpp	FieldNode.cpp LTNode.cpp ORNode.cpp Symbol.cpp \
BindNode.cpp FromNode.cpp LiteralNode.cpp OrderNode.cpp	TableNode.cpp \
DeleteNode.cpp GTENode.cpp NEQNode.cpp QueryNode.cpp WhereNode.cpp \
EQNode.cpp GTNode.cpp NSQLState.cpp SelectNode.cpp Environment.cpp \
LTENode.cpp Node.cpp StarNode.cpp

VIS_SRC := PrettyPrintVisitor.cpp SemanticCheck.cpp NSQLVisitor.cpp

TREE_OBJ := $(patsubst %,${OBJDIR}/%, ${TREE_SRC:%.cpp=%.o})

VIS_OBJ  := $(patsubst %,${OBJDIR}/%, ${VIS_SRC:%.cpp=%.o})

#--------------------------------------------------
# Things required by the main makefile:
# NSQL_OBJ, NSQL_INC, NSQL_PARS, and NSQL_TOOL
NSQL_OBJ = $(TREE_OBJ) $(VIS_OBJ) $(NSQL_COCO_OBJ)
NSQL_INC := -INSQL/NSQL -INSQL/Tree -INSQL/Visitors
NSQL_PARS := $(OBJDIR)/NSQL_Parser.o  $(OBJDIR)/NSQL_Scanner.o
NSQL_TOOL := test-sql
#--------------------------------------------------


#--------------------- NSQL Parser and Scanner -------------------
# Parser and Scanner .cpp files:
NSQL/Parser.cpp: NSQL/NSQL.atg NSQL/Parser.frame 
	( cd NSQL/NSQL ; $(COCO) -namespace NSQL NSQL.atg ) 

NSQL/Scanner.cpp: NSQL/NSQL.atg NSQL/Scanner.frame
	( cd NSQL/NSQL ; $(COCO) -namespace NSQL NSQL.atg ) 

# Parser & Scanner object files.
$(OBJDIR)/NSQL_Parser.o: NSQL/NSQL/Parser.cpp 
	$(CC) $(COMPILER_FLAGS) -o $@ $< 

$(OBJDIR)/NSQL_Scanner.o: NSQL/NSQL/Scanner.cpp 
	$(CC) $(COMPILER_FLAGS) -o $@ $<
### --------------------------------------------------------------



#--------------------- test tools --------------------------------
$(OBJDIR)/DBConnection.o: Utilities/DBConnection.cpp
	$(CC) $(COMPILER_FLAGS) -o $@ $< 

$(OBJDIR)/NSQLtestmain.o: NSQL.cpp 
	$(CC) $(COMPILER_FLAGS) -INSQL/Utilities -o $@ $<

$(OBJDIR)/nsql_test.o: nsql_test.cpp
	$(CC) $(COMPILER_FLAGS) -INSQL/Utilities -o $@ $<

test-sql-1: NSQLtestmain.o DBConnection.o $(NSQL_OBJ) 
	$(CC) -lreadline -lapr-1 NSQLtestmain.o $(NSQL_OBJ) -o $@ 

test-sql-2: $(OBJDIR)/nsql_test.o $(NSQL_OBJ) 
	$(CC) -lreadline -lapr-1 $< $(TREE_OBJ) $(NSQL_COCO_OBJ) -o $@


### --------------------------------------------------------------

