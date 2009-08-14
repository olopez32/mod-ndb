/*
 * ANDNode.cpp
 *
 *  Created on: Jul 19, 2009
 *      Author: tulay
 */

#include "EQNode.h"
#include "NSQLVisitor.h"

class NSQLVisitor;

void
EQNode::accept (NSQLVisitor& visitor)
{
	visitor.VisitEQNode(this);
}

void
EQNode::childrenAccept (NSQLVisitor& visitor)
{
	std::vector<Node*>::iterator itr;
	for ( itr= children.begin(); itr !=children.end(); ++itr) {
		(*itr)->accept(visitor);
	}
}
