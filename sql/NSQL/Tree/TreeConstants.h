/*
 * TreeConstants.h
 *
 *  Created on: Jul 12, 2009
 *      Author: tulay
 */

#ifndef TREECONSTANTS_H_
#define TREECONSTANTS_H_



enum NodeType {VOID=0, QUERY=1, SELECT=2, FIELD=3, FROM=4, WHERE=5, TABLE=6, AND=7, OR=8,
	NEQ=9, EQ=10, LTE=11, LT=12,GTE=13, GT=14, STAR=15, LITERAL=16, BIND=17, DELETE=18, ORDER=19};

namespace{
const char*  NodeName[] = {"VOID", "QUERY","SELECT", "FIELD", "FROM", "WHERE", "TABLE", "AND", "OR",
		"NEQ", "EQ", "LTE", "LT", "GTE", "GT", "STAR" ,"LITERAL", "BIND" , "DELETE", "ORDER"};
}

#endif /* TREECONSTANTS_H_ */
