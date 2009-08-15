/*
 * NSQLVisitor.h
 *
 *  Created on: Jul 19, 2009
 *      Author: tulay
 */

#ifndef NSQLVISITOR_H_
#define NSQLVISITOR_H_


#include "Node.h"
#include "QueryNode.h"
#include "SelectNode.h"
#include "FieldNode.h"
#include "FromNode.h"
#include "TableNode.h"
#include "WhereNode.h"
#include "ORNode.h"
#include "ANDNode.h"
#include "EQNode.h"
#include "GTENode.h"
#include "GTNode.h"
#include "LTENode.h"
#include "LTNode.h"
#include "NEQNode.h"
#include "StarNode.h"
#include "BindNode.h"
#include "LiteralNode.h"
#include "DeleteNode.h"
#include "OrderNode.h"

/**
 * All of these methods do nothing by default
 */


class FieldNode;
class QueryNode;
class SelectNode;
class FromNode;
class TableNode;
class WhereNode;
class ANDNode;
class ORNode;
class EQNode;
class GTENode;
class GTNode;
class LTENode;
class LTNode;
class NEQNode;
class StarNode;
class LiteralNode;
class BindNode;
class DeleteNode;
class OrderNode;

class NSQLVisitor {
public:
	virtual ~NSQLVisitor();

	virtual void VisitQueryNode(QueryNode* n);
	virtual void VisitSelectNode(SelectNode* n);
	virtual void VisitFieldNode(FieldNode* n);
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


//protected:
	NSQLVisitor();
};

#endif /* NSQLVISITOR_H_ */
