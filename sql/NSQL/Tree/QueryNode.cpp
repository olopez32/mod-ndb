/*
 * QueryNode.cpp
 *
 *  Created on: Jul 19, 2009
 *      Author: tulay
 */

#include "QueryNode.h"

void QueryNode::accept (NSQLVisitor& visitor)
{
	visitor.VisitQueryNode(this);
}

void
QueryNode::childrenAccept (NSQLVisitor& visitor)
{
	std::vector<Node*>::iterator itr;
		for ( itr= children.begin(); itr !=children.end(); ++itr) {
			(*itr)->accept(visitor);
		}
}



