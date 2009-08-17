/*
 * SemanticCheck.h
 *
 *  Created on: Jul 19, 2009
 *      Author: tulay
 */

#ifndef SEMANTICCHECK_H_
#define SEMANTICCHECK_H_

#include "NSQLVisitor.h"
#include <NdbApi.hpp>


class SemanticCheck: public NSQLVisitor {
	typedef enum STATUS {RESOLVED_ERROR=-1, UNRESOLVED=0, RESOLVED_OK=1};
public:


	SemanticCheck(){_status = UNRESOLVED;}
	virtual ~SemanticCheck();

	virtual void setNdb(Ndb *ndb);
	virtual void setStatus(STATUS status);
	virtual STATUS getStatus() {return _status;}

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
	Ndb *myNdb;
	STATUS _status;


};




#endif /* SEMANTICCHECK_H_ */
