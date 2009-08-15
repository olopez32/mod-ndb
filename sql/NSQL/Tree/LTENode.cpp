/*
 * LTENode.cpp
 *
 *  Created on: Jul 19, 2009
 *      Author: tulay
 */

#include "LTENode.h"
#include "NSQLVisitor.h"

class NSQLVisitor;

void
LTENode::accept (NSQLVisitor& visitor)
{
	visitor.VisitLTENode(this);
}

void
LTENode::childrenAccept (NSQLVisitor& visitor)
{
	std::vector<Node*>::iterator itr;
	for ( itr= children.begin(); itr !=children.end(); ++itr) {
		(*itr)->accept(visitor);
	}
}
