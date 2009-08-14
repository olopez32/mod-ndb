/*
 * WhereNode.cpp
 *
 *  Created on: Jul 22, 2009
 *      Author: tulay
 */

#include "WhereNode.h"

WhereNode::~WhereNode() {

}

void WhereNode::accept (NSQLVisitor& visitor)
{
	visitor.VisitWhereNode(this);

}

void
WhereNode::childrenAccept (NSQLVisitor& visitor)
{
	std::vector<Node*>::iterator itr;
		for ( itr= children.begin(); itr !=children.end(); ++itr) {
			(*itr)->accept(visitor);
		}
}
