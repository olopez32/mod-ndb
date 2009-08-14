/*
 * LTNode.cpp
 *
 *  Created on: Jul 19, 2009
 *      Author: tulay
 */

#include "LTNode.h"
#include "NSQLVisitor.h"

class NSQLVisitor;

void
LTNode::accept (NSQLVisitor& visitor)
{
	visitor.VisitLTNode(this);
}

void
LTNode::childrenAccept (NSQLVisitor& visitor)
{
	std::vector<Node*>::iterator itr;
	for ( itr= children.begin(); itr !=children.end(); ++itr) {
		(*itr)->accept(visitor);
	}
}
