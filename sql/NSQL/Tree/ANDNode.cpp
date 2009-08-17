/*
 * ANDNode.cpp
 *
 *  Created on: Jul 19, 2009
 *      Author: tulay
 */
//TODO: PrimaryKey production will generate n children for AND node, Evaluation should behave accordingly


#include "ANDNode.h"
#include "NSQLVisitor.h"

class NSQLVisitor;

void
ANDNode::accept (NSQLVisitor& visitor)
{
	visitor.VisitANDNode(this);
}

void
ANDNode::childrenAccept (NSQLVisitor& visitor)
{
	std::vector<Node*>::iterator itr;
	for ( itr= children.begin(); itr !=children.end(); ++itr) {
		(*itr)->accept(visitor);
	}
}
