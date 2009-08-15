/*
 * GTENode.cpp
 *
 *  Created on: Jul 19, 2009
 *      Author: tulay
 */

#include "GTNode.h"
#include "NSQLVisitor.h"

class NSQLVisitor;

void
GTNode::accept (NSQLVisitor& visitor)
{
	visitor.VisitGTNode(this);
}

void
GTNode::childrenAccept (NSQLVisitor& visitor)
{
	std::vector<Node*>::iterator itr;
	for ( itr= children.begin(); itr !=children.end(); ++itr) {
		(*itr)->accept(visitor);
	}
}
