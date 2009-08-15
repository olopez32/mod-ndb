/*
 * PrettyPrintVisitor.h
 *
 *  Created on: Jul 19, 2009
 *      Author: tulay
 */

#ifndef PRETTYPRINTVISITOR_H_
#define PRETTYPRINTVISITOR_H_

#include "NSQLVisitor.h"

class PrettyPrintVisitor: public NSQLVisitor {
public:
	PrettyPrintVisitor();
	virtual ~PrettyPrintVisitor();

	virtual void PrettyIndent();
	virtual void PrettyPrint(const char *);
	virtual void PrettyPrint(const char * s, const char * t);


	virtual void VisitQueryNode(QueryNode*);
	virtual void VisitSelectNode(SelectNode*);
	virtual void VisitFieldNode(FieldNode*);
	virtual void VisitFromNode(FromNode* n);
	virtual void VisitTableNode(TableNode* n);
	virtual void VisitWhereNode(WhereNode* n);
	virtual void VisitANDNode(ANDNode* n);
	virtual void VisitORNode(ORNode* n);
	virtual void VisitEQNode(EQNode* n);
	virtual void VisitGTENode(GTENode* n);
	virtual void VisitGTNode(GTNode* n);
	virtual void VisitLTENode(LTENode* n);
	virtual void VisitLTNode(LTNode* n);
	virtual void VisitNEQNode(NEQNode* n);
	virtual void VisitStarNode(StarNode* n);
	virtual void VisitLiteralNode(LiteralNode* n);
	virtual void VisitBindNode(BindNode* n);
	virtual void VisitDeleteNode(DeleteNode* n);
	virtual void VisitOrderNode(OrderNode* n);


private:
	int indent;

};




#endif /* PRETTYPRINTVISITOR_H_ */
