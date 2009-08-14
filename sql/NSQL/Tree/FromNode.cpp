/*
 * FromNode.cpp
 *
 *  Created on: Jul 22, 2009
 *      Author: tulay
 */

#include "FromNode.h"

FromNode::~FromNode() {

}


void FromNode::accept (NSQLVisitor& visitor)
{
	visitor.VisitFromNode(this);

}

void
FromNode::childrenAccept (NSQLVisitor& visitor)
{
	std::vector<Node*>::iterator itr;
		for ( itr= children.begin(); itr !=children.end(); ++itr) {
			(*itr)->accept(visitor);
		}
}
