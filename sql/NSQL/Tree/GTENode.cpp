/*
 * GTENode.cpp
 *
 *  Created on: Jul 19, 2009
 *      Author: tulay
 */

#include "GTENode.h"
#include "NSQLVisitor.h"

class NSQLVisitor;

void
GTENode::accept (NSQLVisitor& visitor)
{
	visitor.VisitGTENode(this);
}

void
GTENode::childrenAccept (NSQLVisitor& visitor)
{
	std::vector<Node*>::iterator itr;
	for ( itr= children.begin(); itr !=children.end(); ++itr) {
		(*itr)->accept(visitor);
	}
}
