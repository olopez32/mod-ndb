/*
 * PrettyPrintVisitor.h
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
