/*
 * SemanticCheck.h
 *
 *  Created on: Jul 19, 2009
 *      Author: tulay
 */

/* Copyright (C) 2009 Sun Microsystems
 All rights reserved. Use is subject to license terms.
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
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
