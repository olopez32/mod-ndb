/*
 * PrettyPrintVisitor.cpp
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

#include "PrettyPrintVisitor.h"
#include <iostream>

PrettyPrintVisitor::PrettyPrintVisitor()
{
	indent=0;
}

PrettyPrintVisitor::~PrettyPrintVisitor()
{

}

void
PrettyPrintVisitor::PrettyIndent()
{
	for (int i=0; i< indent ; i++)
			std::cout << "\t";
}

void
PrettyPrintVisitor::PrettyPrint(const char * s)
{
	for (int i=0; i< indent ; i++)
		std::cout << "\t";
	std::cout <<s <<std::endl;
}

void
PrettyPrintVisitor::PrettyPrint(const char * s, const char * t)
{
	for (int i=0; i< indent ; i++)
		std::cout << "\t";
	std::cout <<s <<" = " << t << std::endl;
}

void
PrettyPrintVisitor::VisitQueryNode(QueryNode* n)
{
	indent++;
	PrettyPrint(n->getType());
	n->childrenAccept(*this);
	indent--;

}

void
PrettyPrintVisitor::VisitSelectNode(SelectNode* n)
{
	indent++;
	PrettyPrint(n->getType());
	n->childrenAccept(*this);
	indent--;
}

void
PrettyPrintVisitor::VisitFieldNode(FieldNode* n)
{
	indent++;

	PrettyIndent();

	std::cout <<n->getType() << " = " << n->getName();
	if(n->isPrimary()){
		std::cout<< " ; PK Order = " << n->getPrimaryKeyOrder();
	}
	if (n->hasAlias())
		std::cout<< " ; Alias = " << n->getAlias();
	if(n->getTable() != NULL){
		std::cout<< " ; Table = " << n->getTable();
	}
	std::cout << std::endl;

	indent--;
}

void
PrettyPrintVisitor::VisitFromNode(FromNode* n)
{
	indent++;
	PrettyPrint(n->getType());
	n->childrenAccept(*this);
	indent--;
}

void
PrettyPrintVisitor::VisitTableNode(TableNode* n)
{
	indent++;
	PrettyIndent();
	std::cout <<n->getType() << " = " << n->getName();
	if (n->hasAlias())
		std::cout<< " ; Alias = " << n->getAlias();
	if(n->getDatabaseName() != NULL){
			std::cout<< " ; Database = " << n->getDatabaseName();
		}
	std::cout << std::endl;
	indent--;
}

void
PrettyPrintVisitor::VisitWhereNode(WhereNode* n)
{
	indent++;
	PrettyPrint(n->getType());
	n->childrenAccept(*this);
	indent--;
}

void
PrettyPrintVisitor::VisitORNode(ORNode* n)
{
	indent++;
	PrettyPrint(n->getType());
	n->childrenAccept(*this);
	indent--;
}

void
PrettyPrintVisitor::VisitANDNode(ANDNode* n)
{
	indent++;
	PrettyPrint(n->getType());
	n->childrenAccept(*this);
	indent--;
}


void
PrettyPrintVisitor::VisitEQNode(EQNode* n)
{
	indent++;
	PrettyPrint(n->getType());
	n->childrenAccept(*this);
	indent--;

}

void
PrettyPrintVisitor::VisitGTENode(GTENode* n)
{
	indent++;
	PrettyPrint(n->getType());
	n->childrenAccept(*this);
	indent--;
}

void
PrettyPrintVisitor::VisitGTNode(GTNode* n)
{
	indent++;
	PrettyPrint(n->getType());
	n->childrenAccept(*this);
	indent--;
}

void
PrettyPrintVisitor::VisitLTENode(LTENode* n)
{
	indent++;
	PrettyPrint(n->getType());
	n->childrenAccept(*this);
	indent--;
}

void
PrettyPrintVisitor::VisitLTNode(LTNode* n)
{
	indent++;
	PrettyPrint(n->getType());
	n->childrenAccept(*this);
	indent--;
}

void
PrettyPrintVisitor::VisitNEQNode(NEQNode* n)
{
	indent++;
	PrettyPrint(n->getType());
	n->childrenAccept(*this);
	indent--;
}

void
PrettyPrintVisitor::VisitStarNode(StarNode* n)
{
	indent++;
	PrettyPrint(n->getType());
	indent--;
}

void
PrettyPrintVisitor::VisitLiteralNode(LiteralNode* n)
{
	indent++;
	PrettyPrint(n->getType(), n->getValue());
	indent--;
}

void
PrettyPrintVisitor::VisitBindNode(BindNode* n)
{
	indent++;
	PrettyPrint(n->getType(),n->getName());
	indent--;
}


void
PrettyPrintVisitor::VisitDeleteNode(DeleteNode* n)
{
	indent++;
	PrettyPrint(n->getType());
	indent--;
}

void
PrettyPrintVisitor::VisitOrderNode(OrderNode* n)
{
	indent++;

	PrettyPrint(n->getType(), (n->isAsc()) ? "ASC":"DESC");
	indent--;
}

