/*
 * SelectNode.cpp
 *
 *  Created on: Jul 19, 2009
 *      Author: tulay
 */

#include "SelectNode.h"


void SelectNode::accept (NSQLVisitor& visitor)
{
	visitor.VisitSelectNode(this);

}

void
SelectNode::childrenAccept (NSQLVisitor& visitor)
{
	std::vector<Node*>::iterator itr;
		for ( itr= children.begin(); itr !=children.end(); ++itr) {
			(*itr)->accept(visitor);
		}
}
