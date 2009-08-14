//============================================================================
// Name        : NSQL.cpp
// Author      :
// Version     :
// Copyright   :
// Description :
//
//  Case 1: Correct query
//  "SELECT col as T, to  FROM table WHERE ( col1 < $id  AND ( col2 > 9 or col3 == \"re\")) order asc;";
//	Case 2: Syntax error
//	FIXED: Throw exception and exception handling
//	in this particular case the memory assigned to col3 (FIELD) will never be returned.
//	"SELECT col, to  FROM table WHERE ( col1 < a  AND ( col2 > b or col3 = c));";
//	test(const_cast<unsigned char *>(selectQuery));
//	"DELETE FROM table where  col1 != \"test9\" ;";
//
//
//
//============================================================================

#include <iostream>

#include "Parser.h"
#include "Scanner.h"
#include "Node.h"
#include "PrettyPrintVisitor.h"
#include "SemanticCheck.h"
#include "DBConnection.h"
#include <readline/readline.h> //compile with -lreadline
#include <readline/history.h>

using namespace std;

#define HISTFILE ".SQLtest.history"

std::ostream & operator<< (std::ostream & out, const Node & rhs)
{
	out << "type=" << rhs.getType();
	return out;
}

int test(const unsigned char*, DBConnection &);

int main()
{
	char *cmdline;
	using_history();
	read_history(HISTFILE);

	DBConnection conn("192.168.1.3:1186", "mod_ndb_tests");

	cmdline = readline("> ");
	while(cmdline) {
		int cmd_len = strlen(cmdline);
		if(cmd_len) {
			add_history(cmdline);

			test((const unsigned char*) cmdline, conn);

		}
		cmdline = readline("> ");
	}

	return 0;
}


int test(const unsigned char* query, DBConnection &conn)
{
	NSQLState *tree = new NSQLState();
	int len = strlen((char*)query);
	NSQL::Scanner *scanner = new NSQL::Scanner(query,len);
	NSQL::Parser *parser = new NSQL::Parser(scanner);
	parser->nsqlTree = tree;
	try{
		parser->Parse();
	}
	catch(...){
		cout<< "Parser Exception" << endl;
		//		return -1;
	}

	if(tree->getRoot() != 0){
		cout<< "Printing Syntax Tree" << endl;
		PrettyPrintVisitor v;
		(tree->getRoot())->accept(v);
	}


	if(tree->getRoot() != 0){
		cout<< "Semantic Analysis" << endl;
		SemanticCheck s;
		s.setNdb(conn.getNdb());
		(tree->getRoot())->accept(s);
		if (s->getStatus())
			std::cout << "Semantic is OK\n";

	}
	delete parser;
	delete scanner;
	delete tree;

	return 0;
}
