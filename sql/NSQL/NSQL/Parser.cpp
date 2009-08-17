

#include <wchar.h>
#include "Parser.h"
#include "Scanner.h"


namespace NSQL {


void Parser::SynErr(int n) throw (ParseException){
	if (errDist >= minErrDist) errors->SynErr(la->line, la->col, n);
	errDist = 0;
	throw ParseException();
}

void Parser::SemErr(const wchar_t* msg) {
	if (errDist >= minErrDist) errors->Error(t->line, t->col, msg);
	errDist = 0;
}

void Parser::Get() {
	for (;;) {
		t = la;
		la = scanner->Scan();
		if (la->kind <= maxT) { ++errDist; break; }

		if (dummyToken != t) {
			dummyToken->kind = t->kind;
			dummyToken->pos = t->pos;
			dummyToken->col = t->col;
			dummyToken->line = t->line;
			dummyToken->next = NULL;
			coco_string_delete(dummyToken->val);
			dummyToken->val = coco_string_create(t->val);
			t = dummyToken;
		}
		la = t;
	}
}

void Parser::Expect(int n) {
	if (la->kind==n) Get(); else { SynErr(n); }
}

void Parser::ExpectWeak(int n, int follow) {
	if (la->kind == n) Get();
	else {
		SynErr(n);
		while (!StartOf(follow)) Get();
	}
}

bool Parser::WeakSeparator(int n, int syFol, int repFol) {
	if (la->kind == n) {Get(); return true;}
	else if (StartOf(repFol)) {return false;}
	else {
		SynErr(n);
		while (!(StartOf(syFol) || StartOf(repFol) || StartOf(0))) {
			Get();
		}
		return StartOf(syFol);
	}
}

void Parser::NSQL() {
		Query();
}

void Parser::Query() {
		QueryNode *root = new QueryNode(QUERY); 
		bool shouldCloseRoot = true;
		nsqlTree->openNodeScope(root);
		try{ 
		if (la->kind == 3) {
			SelectQuery();
		} else if (la->kind == 2) {
			DeleteQuery();
		} else SynErr(33);
		}
		catch(ParseException & excp){
			if(shouldCloseRoot) { 
				nsqlTree->clearNodeScope(root);
				shouldCloseRoot = false;
			}
			else {
				nsqlTree->popNode();
			}
			delete root;
			throw;
		}
		
		if(shouldCloseRoot) { 
				nsqlTree->closeNodeScope(root,true);
		}
		
		
		
}

void Parser::SelectQuery() {
		SelectClause();
		FromClause();
		if (la->kind == 5) {
			WhereClause();
		}
		if (la->kind == 6) {
			OrderClause();
		}
		Expect(12);
}

void Parser::DeleteQuery() {
		Expect(2);
		DeleteNode *deleteNode = new DeleteNode(DELETE);
		bool shouldCloseDeleteNode = true;
		 nsqlTree->openNodeScope(deleteNode); 
		nsqlTree->closeNodeScope(deleteNode,true);
		shouldCloseDeleteNode = false;
		
													try{ 
		FromClause();
		if (la->kind == 5) {
			WhereClause();
		}
		Expect(12);
		}
		catch(ParseException & excp){
			if(shouldCloseDeleteNode) { 
				nsqlTree->clearNodeScope(deleteNode);
				shouldCloseDeleteNode = false;
			}
			else {
				nsqlTree->popNode();
			}
			delete deleteNode;
			throw;
		}
		
		if(shouldCloseDeleteNode) { 
				nsqlTree->closeNodeScope(deleteNode,true);
		}
													
		
}

void Parser::FromClause() {
		Expect(4);
		FromNode *from = new FromNode(FROM);
		nsqlTree->openNodeScope(from); 
		bool shouldCloseFrom = true;
		try{ 
		FromList();
		}
		catch(ParseException & excp){
			if(shouldCloseFrom) { 
				nsqlTree->clearNodeScope(from);
				shouldCloseFrom = false;
			}
			else {
				nsqlTree->popNode();
			}
			delete from;
			throw;
		}
		if(shouldCloseFrom) { 
				nsqlTree->closeNodeScope(from,true);
		} 
		
}

void Parser::WhereClause() {
		Expect(5);
		WhereNode *where = new WhereNode(WHERE);
		nsqlTree->openNodeScope(where); 
		bool shouldCloseWhere = true;
		try {
		if (StartOf(1)) {
			SelectCondition();
		} else if (la->kind == 18 || la->kind == 21) {
			IndexCondition();
		} else SynErr(34);
		}
		catch(ParseException & excp){
			if(shouldCloseWhere) { 
				nsqlTree->clearNodeScope(where);
				shouldCloseWhere = false;
			}
			else {
				nsqlTree->popNode();
			}
			delete where;
			throw;
		}
		if(shouldCloseWhere) { 
				nsqlTree->closeNodeScope(where,true);
		} 
}

void Parser::SelectClause() {
		Expect(3);
		SelectNode *select = new SelectNode(SELECT);
		bool shouldCloseSelectNode = true;
		nsqlTree->openNodeScope(select);
		try{ 
		if (la->kind == 1) {
			SelectList();
		} else if (la->kind == 13) {
			Get();
			StarNode *star = new StarNode(STAR);
			nsqlTree->openNodeScope(star); 
			nsqlTree->closeNodeScope(star,true);
		} else SynErr(35);
		}
		catch(ParseException & excp){
			if(shouldCloseSelectNode) { 
				nsqlTree->clearNodeScope(select);
				shouldCloseSelectNode = false;
			}
			else {
				nsqlTree->popNode();
			}
			delete select;
			throw;
		}
		if(shouldCloseSelectNode) { 
				nsqlTree->closeNodeScope(select,true);
		}  
}

void Parser::OrderClause() {
		Expect(6);
		OrderNode *order = new OrderNode(ORDER);
		nsqlTree->openNodeScope(order); 
		if (la->kind == 16) {
			Get();
			order->setAsc();				
		} else if (la->kind == 17) {
			Get();
			order->setDesc();			
		} else SynErr(36);
		nsqlTree->closeNodeScope(order,true); 
}

void Parser::SelectList() {
		Field();
		while (la->kind == 11) {
			Get();
			Field();
		}
}

void Parser::Field() {
		char * value, * value1 = NULL;
		Identifier(value);
		if (la->kind == 14) {
			Get();
			Identifier(value1);
		}
		FieldNode *field = new FieldNode(FIELD);
		nsqlTree->openNodeScope(field);
		bool shouldCloseField = true;
		if (value1 != NULL) {
			field->setTable(value);
			field->setName(value1);
		}
		else{
			field->setName(value);
		}
			 
		try{ 
		if (la->kind == 1 || la->kind == 15) {
			if (la->kind == 15) {
				Get();
			}
			Identifier(value);
			field->setAlias(value); 
		}
		}
		catch(ParseException & excp){
			if(shouldCloseField) { 
				nsqlTree->clearNodeScope(field);
				shouldCloseField = false;
			}
			else {
				nsqlTree->popNode();
			}
			delete field;
			throw;
		}
		if(shouldCloseField) { 
				nsqlTree->closeNodeScope(field,true);
		} 
		
}

void Parser::Identifier(char* &v) {
		Expect(1);
		v = coco_string_create_char(t->val); 
}

void Parser::FromList() {
		Table();
}

void Parser::Table() {
		char * value, *value1 = NULL;
		Identifier(value);
		if (la->kind == 14) {
			Get();
			Identifier(value1);
		}
		TableNode *table = new TableNode(TABLE);
		nsqlTree->openNodeScope(table);
		bool shouldCloseTable = true;
		/*end auto*/
		if (value1 != NULL) {
			table->setDatabaseName(value);
			table->setName(value1);
		}
		else{
			table->setName(value);
		}
		/*auto*/ 
		try{ 
		/*end auto*/
		
		if (la->kind == 1 || la->kind == 15) {
			if (la->kind == 15) {
				Get();
			}
			Identifier(value);
			table->setAlias(value); 
		}
		}
		catch(ParseException & excp){
			if(shouldCloseTable) { 
				nsqlTree->clearNodeScope(table);
				shouldCloseTable = false;
			}
			else {
				nsqlTree->popNode();
			}
			delete table;
			throw;
		}
		if(shouldCloseTable) { 
				nsqlTree->closeNodeScope(table,true);
		} 
		/*end auto*/
		
}

void Parser::SelectCondition() {
		Expression();
}

void Parser::IndexCondition() {
		if (la->kind == 18) {
			PrimaryKey();
		} else if (la->kind == 21) {
			UniqueIndex();
		} else SynErr(37);
}

void Parser::PrimaryKey() {
		Expect(18);
		Expect(19);
		Expect(20);
		ANDNode *andNode = new ANDNode(AND);
			bool shouldCloseANDNode = true;
		nsqlTree->openNodeScope(andNode); 
		try{
		 int order = 0;
		
		KeyValue(order);
		if (la->kind == 11) {
			Get();
			order++; 
			KeyValue(order);
		}
		}
		catch(ParseException & excp){
			if(shouldCloseANDNode) { 
				nsqlTree->clearNodeScope(andNode);
				shouldCloseANDNode = false;
			}
			else {
				nsqlTree->popNode();
			}
			delete andNode;
			throw;
		}
		if(shouldCloseANDNode) { 
				nsqlTree->closeNodeScope(andNode,true);
		}
		
}

void Parser::UniqueIndex() {
		Expect(21);
		Expect(22);
}

void Parser::KeyValue(int &order) {
		EQNode *eq = new EQNode(EQ);
		bool shouldCloseEQ = true;
		nsqlTree->openNodeScope(eq);  
		try { 
		 	FieldNode *field = new FieldNode(FIELD);
		 	field->setName("PRIMARY");
		 	field->setPrimary(true);
		 	field->setPrimaryKeyOrder(order);
			nsqlTree->openNodeScope(field); 
			nsqlTree->closeNodeScope(field,true);
		
		char * value = NULL;
		if (la->kind == 9 || la->kind == 10) {
			Literal(value);
			LiteralNode *literal = new LiteralNode(LITERAL);
			literal->setValue(value);
			nsqlTree->openNodeScope(literal); 
			nsqlTree->closeNodeScope(literal,true);  
		} else if (la->kind == 25) {
			BindVariable(value);
			BindNode *bind = new BindNode(BIND);
			bind->setName(value);
			nsqlTree->openNodeScope(bind); 
			nsqlTree->closeNodeScope(bind,true);
		} else SynErr(38);
		}
		catch(ParseException & excp){
			if(shouldCloseEQ) { 
				nsqlTree->clearNodeScope(eq);
				shouldCloseEQ = false;
			}
			else {
				nsqlTree->popNode();
			}
			delete eq;
			throw;
		}
		if(shouldCloseEQ) { 
				nsqlTree->closeNodeScope(eq,2);
		}
		
}

void Parser::Literal(char* &v) {
		if (la->kind == 10) {
			Get();
			v = coco_string_create_char(t->val); 
		} else if (la->kind == 9) {
			Get();
			v = coco_string_create_char(t->val); 
		} else SynErr(39);
}

void Parser::BindVariable(char * &value) {
		Expect(25);
		Identifier(value);
}

void Parser::Expression() {
		OrExpression();
}

void Parser::PrimaryExpression() {
		char * value = NULL;
		if (la->kind == 1) {
			Identifier(value);
			FieldNode *field = new FieldNode(FIELD);
			field->setName(value);
			nsqlTree->openNodeScope(field); 
			nsqlTree->closeNodeScope(field,true);
		} else if (la->kind == 9 || la->kind == 10) {
			Literal(value);
			LiteralNode *literal = new LiteralNode(LITERAL);
			literal->setValue(value);
			nsqlTree->openNodeScope(literal); 
			nsqlTree->closeNodeScope(literal,true);  
		} else if (la->kind == 25) {
			BindVariable(value);
			BindNode *bind = new BindNode(BIND);
			bind->setName(value);
			nsqlTree->openNodeScope(bind); 
			nsqlTree->closeNodeScope(bind,true);
		} else if (la->kind == 23) {
			Lparen();
			Expression();
			RParen();
		} else SynErr(40);
}

void Parser::Lparen() {
		Expect(23);
}

void Parser::RParen() {
		Expect(24);
}

void Parser::RelationalExpression() {
		PrimaryExpression();
		while (StartOf(2)) {
			switch (la->kind) {
			case 26: {
				Get();
				GTNode *gt = new GTNode(GT);
				bool shouldCloseGT = true;
				nsqlTree->openNodeScope(gt);  
				try{ 
				PrimaryExpression();
				}
				catch(ParseException & excp){
					if(shouldCloseGT) { 
						nsqlTree->clearNodeScope(gt);
						shouldCloseGT = false;
					}
					else {
						nsqlTree->popNode();
					}
					delete gt;
					throw;
				}
				if(shouldCloseGT) { 
						nsqlTree->closeNodeScope(gt,2);
				} 
				
				break;
			}
			case 27: {
				Get();
				GTENode *gte = new GTENode(GTE);
				bool shouldCloseGTE = true;
				nsqlTree->openNodeScope(gte); 
				try{ 
				 
				PrimaryExpression();
				}
				catch(ParseException & excp){
					if(shouldCloseGTE) { 
						nsqlTree->clearNodeScope(gte);
						shouldCloseGTE = false;
					}
					else {
						nsqlTree->popNode();
					}
					delete gte;
					throw;
				}
				if(shouldCloseGTE) { 
						nsqlTree->closeNodeScope(gte,2);
				}
				
				break;
			}
			case 28: {
				Get();
				LTNode *lt = new LTNode(LT);
				bool shouldCloseLT = true;
				nsqlTree->openNodeScope(lt);
				try{   
				
				PrimaryExpression();
				}
				catch(ParseException & excp){
					if(shouldCloseLT) { 
						nsqlTree->clearNodeScope(lt);
						shouldCloseLT = false;
					}
					else {
						nsqlTree->popNode();
					}
					delete lt;
					throw;
				}
				if(shouldCloseLT) { 
						nsqlTree->closeNodeScope(lt,2);
				} 
				
				break;
			}
			case 29: {
				Get();
				LTENode *lte = new LTENode(LTE);
				bool shouldCloseLTE = true;
				nsqlTree->openNodeScope(lte); 
				try{ 
				
				PrimaryExpression();
				}
				catch(ParseException & excp){
					if(shouldCloseLTE) { 
						nsqlTree->clearNodeScope(lte);
						shouldCloseLTE = false;
					}
					else {
						nsqlTree->popNode();
					}
					delete lte;
					throw;
				}
				if(shouldCloseLTE) { 
						nsqlTree->closeNodeScope(lte,2);
				}
				
				break;
			}
			case 30: {
				Get();
				EQNode *eq = new EQNode(EQ);
				bool shouldCloseEQ = true;
				nsqlTree->openNodeScope(eq);  
				try { 
				
				PrimaryExpression();
				}
				catch(ParseException & excp){
					if(shouldCloseEQ) { 
						nsqlTree->clearNodeScope(eq);
						shouldCloseEQ = false;
					}
					else {
						nsqlTree->popNode();
					}
					delete eq;
					throw;
				}
				if(shouldCloseEQ) { 
						nsqlTree->closeNodeScope(eq,2);
				}
				
				break;
			}
			case 31: {
				Get();
				NEQNode *neq = new NEQNode(NEQ);
				bool shouldCloseNEQ = true;
				nsqlTree->openNodeScope(neq);
				try{
				 
				PrimaryExpression();
				}
				catch(ParseException & excp){
					if(shouldCloseNEQ) { 
						nsqlTree->clearNodeScope(neq);
						shouldCloseNEQ = false;
					}
					else {
						nsqlTree->popNode();
					}
					delete neq;
					throw;
				}
				if(shouldCloseNEQ) { 
						nsqlTree->closeNodeScope(neq,2);
				}
				
				break;
			}
			}
		}
}

void Parser::AndExpression() {
		RelationalExpression();
		while (la->kind == 8) {
			Get();
			ANDNode *andNode = new ANDNode(AND);
			bool shouldCloseANDNode = true;
			nsqlTree->openNodeScope(andNode); 
			try{
			
			RelationalExpression();
			}
			catch(ParseException & excp){
				if(shouldCloseANDNode) { 
					nsqlTree->clearNodeScope(andNode);
					shouldCloseANDNode = false;
				}
				else {
					nsqlTree->popNode();
				}
				delete andNode;
				throw;
			}
			if(shouldCloseANDNode) { 
					nsqlTree->closeNodeScope(andNode,2);
			}
			
		}
}

void Parser::OrExpression() {
		AndExpression();
		while (la->kind == 7) {
			Get();
			ORNode *orNode = new ORNode(OR);
			bool shouldCloseORNode = true;
			nsqlTree->openNodeScope(orNode);  
			try{ 
			
			AndExpression();
			}
			catch(ParseException & excp){
				if(shouldCloseORNode) { 
					nsqlTree->clearNodeScope(orNode);
					shouldCloseORNode = false;
				}
				else {
					nsqlTree->popNode();
				}
				delete orNode;
				throw;
			}
			if(shouldCloseORNode) { 
					nsqlTree->closeNodeScope(orNode,2);
			}
			
		}
}



void Parser::Parse() {
	t = NULL;
	la = dummyToken = new Token();
	la->val = coco_string_create(L"Dummy Token");
	Get();
	NSQL();

	Expect(0);
}

Parser::Parser(Scanner *scanner) {
	maxT = 32;

	dummyToken = NULL;
	t = la = NULL;
	minErrDist = 2;
	errDist = minErrDist;
	this->scanner = scanner;
	errors = new Errors();
}

bool Parser::StartOf(int s) {
	const bool T = true;
	const bool x = false;

	static bool set[3][34] = {
		{T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x},
		{x,T,x,x, x,x,x,x, x,T,T,x, x,x,x,x, x,x,x,x, x,x,x,T, x,T,x,x, x,x,x,x, x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,T, T,T,T,T, x,x}
	};



	return set[s][la->kind];
}

Parser::~Parser() {
	delete errors;
	delete dummyToken;
}

Errors::Errors() {
	count = 0;
}

void Errors::SynErr(int line, int col, int n) {
	wchar_t* s;
	switch (n) {
			case 0: s = coco_string_create(L"EOF expected"); break;
			case 1: s = coco_string_create(L"ident expected"); break;
			case 2: s = coco_string_create(L"delete expected"); break;
			case 3: s = coco_string_create(L"select expected"); break;
			case 4: s = coco_string_create(L"from expected"); break;
			case 5: s = coco_string_create(L"where expected"); break;
			case 6: s = coco_string_create(L"order expected"); break;
			case 7: s = coco_string_create(L"o_or expected"); break;
			case 8: s = coco_string_create(L"o_and expected"); break;
			case 9: s = coco_string_create(L"qstring expected"); break;
			case 10: s = coco_string_create(L"number expected"); break;
			case 11: s = coco_string_create(L"comma expected"); break;
			case 12: s = coco_string_create(L"semicolon expected"); break;
			case 13: s = coco_string_create(L"\"*\" expected"); break;
			case 14: s = coco_string_create(L"\".\" expected"); break;
			case 15: s = coco_string_create(L"\"as\" expected"); break;
			case 16: s = coco_string_create(L"\"asc\" expected"); break;
			case 17: s = coco_string_create(L"\"desc\" expected"); break;
			case 18: s = coco_string_create(L"\"primary\" expected"); break;
			case 19: s = coco_string_create(L"\"key\" expected"); break;
			case 20: s = coco_string_create(L"\"=\" expected"); break;
			case 21: s = coco_string_create(L"\"unique\" expected"); break;
			case 22: s = coco_string_create(L"\"index\" expected"); break;
			case 23: s = coco_string_create(L"\"(\" expected"); break;
			case 24: s = coco_string_create(L"\")\" expected"); break;
			case 25: s = coco_string_create(L"\"$\" expected"); break;
			case 26: s = coco_string_create(L"\">\" expected"); break;
			case 27: s = coco_string_create(L"\">=\" expected"); break;
			case 28: s = coco_string_create(L"\"<\" expected"); break;
			case 29: s = coco_string_create(L"\"<=\" expected"); break;
			case 30: s = coco_string_create(L"\"==\" expected"); break;
			case 31: s = coco_string_create(L"\"!=\" expected"); break;
			case 32: s = coco_string_create(L"??? expected"); break;
			case 33: s = coco_string_create(L"invalid Query"); break;
			case 34: s = coco_string_create(L"invalid WhereClause"); break;
			case 35: s = coco_string_create(L"invalid SelectClause"); break;
			case 36: s = coco_string_create(L"invalid OrderClause"); break;
			case 37: s = coco_string_create(L"invalid IndexCondition"); break;
			case 38: s = coco_string_create(L"invalid KeyValue"); break;
			case 39: s = coco_string_create(L"invalid Literal"); break;
			case 40: s = coco_string_create(L"invalid PrimaryExpression"); break;

		default:
		{
			wchar_t format[20];
			coco_swprintf(format, 20, L"error %d", n);
			s = coco_string_create(format);
		}
		break;
	}
	wprintf(L"-- line %d col %d: %ls\n", line, col, s);
	coco_string_delete(s);
	count++;
}

void Errors::Error(int line, int col, const wchar_t *s) {
	wprintf(L"-- line %d col %d: %ls\n", line, col, s);
	count++;
}

void Errors::Warning(int line, int col, const wchar_t *s) {
	wprintf(L"-- line %d col %d: %ls\n", line, col, s);
}

void Errors::Warning(const wchar_t *s) {
	wprintf(L"%ls\n", s);
}

void Errors::Exception(const wchar_t* s) {
	wprintf(L"%ls", s); 
	exit(1);
}

} // namespace


