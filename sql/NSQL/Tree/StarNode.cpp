/*
 * StarNode.cpp
 *
 *  Created on: Jul 19, 2009
 *      Author: tulay
 */

#include "StarNode.h"
#include "NSQLVisitor.h"

class NSQLVisitor;

void
StarNode::accept (NSQLVisitor& visitor)
{
	visitor.VisitStarNode(this);
}

void
StarNode::childrenAccept (NSQLVisitor& visitor)
{

}
