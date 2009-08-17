

#if !defined(COCO_PARSER_H__)
#define COCO_PARSER_H__

#include  "NSQLState.h"
#include  "FieldNode.h"
#include  "Node.h"
#include  "QueryNode.h"
#include  "SelectNode.h"
#include "StarNode.h"
#include  "FromNode.h"
#include  "TableNode.h"
#include  "WhereNode.h"
#include  "ANDNode.h"
#include  "ORNode.h"
#include "EQNode.h"
#include "GTENode.h"
#include "GTNode.h"
#include "LTENode.h"
#include "LTNode.h"
#include "NEQNode.h"
#include "LiteralNode.h"
#include "BindNode.h"
#include "DeleteNode.h"
#include "OrderNode.h"

#include  "TreeConstants.h"
#include <iostream>


#include "Scanner.h"

namespace NSQL {



class ParseException {

public:
	ParseException(){}
	~ParseException(){}
	const char *what() const {
	      return "Parse Exception.";
	   }
}; // Exception

class Errors {
public:
	int count;			// number of errors detected

	Errors();
	void SynErr(int line, int col, int n);
	void Error(int line, int col, const wchar_t *s);
	void Warning(int line, int col, const wchar_t *s);
	void Warning(const wchar_t *s);
	void Exception(const wchar_t *s);

}; // Errors

class Parser {
private:
	enum {
		_EOF=0,
		_ident=1,
		_delete=2,
		_select=3,
		_from=4,
		_where=5,
		_order=6,
		_o_or=7,
		_o_and=8,
		_qstring=9,
		_number=10,
		_comma=11,
		_semicolon=12,
	};
	int maxT;

	Token *dummyToken;
	int errDist;
	int minErrDist;

	void SynErr(int n) throw (ParseException);
	void Get();
	void Expect(int n);
	bool StartOf(int s);
	void ExpectWeak(int n, int follow);
	bool WeakSeparator(int n, int syFol, int repFol);

public:
	Scanner *scanner;
	Errors  *errors;

	Token *t;			// last recognized token
	Token *la;			// lookahead token

	NSQLState *nsqlTree;        //Call getRoot to get a pointer to the syntax tree



	Parser(Scanner *scanner);
	~Parser();
	void SemErr(const wchar_t* msg);

	void NSQL();
	void Query();
	void SelectQuery();
	void DeleteQuery();
	void FromClause();
	void WhereClause();
	void SelectClause();
	void OrderClause();
	void SelectList();
	void Field();
	void Identifier(char* &v);
	void FromList();
	void Table();
	void SelectCondition();
	void IndexCondition();
	void PrimaryKey();
	void UniqueIndex();
	void KeyValue(int &order);
	void Literal(char* &v);
	void BindVariable(char * &value);
	void Expression();
	void PrimaryExpression();
	void Lparen();
	void RParen();
	void RelationalExpression();
	void AndExpression();
	void OrExpression();

	void Parse();

}; // end Parser

} // namespace


#endif // !defined(COCO_PARSER_H__)

