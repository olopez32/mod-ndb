/*
 * SelectNode.cpp
 *
 *  Created on: Jul 19, 2009
 *      Author: tulay
 */

#include "ORNode.h"


void ORNode::accept (NSQLVisitor& visitor)
{
	visitor.VisitORNode(this);

}

void
ORNode::childrenAccept (NSQLVisitor& visitor)
{
	std::vector<Node*>::iterator itr;
		for ( itr= children.begin(); itr !=children.end(); ++itr) {
			(*itr)->accept(visitor);
		}
}
