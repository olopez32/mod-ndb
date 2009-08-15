/*
 * NEQNode.cpp
 *
 *  Created on: Jul 19, 2009
 *      Author: tulay
 */

#include "NEQNode.h"
#include "NSQLVisitor.h"

class NSQLVisitor;

void
NEQNode::accept (NSQLVisitor& visitor)
{
	visitor.VisitNEQNode(this);
}

void
NEQNode::childrenAccept (NSQLVisitor& visitor)
{
	std::vector<Node*>::iterator itr;
	for ( itr= children.begin(); itr !=children.end(); ++itr) {
		(*itr)->accept(visitor);
	}
}
