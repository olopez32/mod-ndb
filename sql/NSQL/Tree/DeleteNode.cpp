/*
 * DeleteNode.cpp
 *
 *  Created on: Jul 19, 2009
 *      Author: tulay
 */

#include "DeleteNode.h"
#include "NSQLVisitor.h"

class NSQLVisitor;

void
DeleteNode::accept (NSQLVisitor& visitor)
{
	visitor.VisitDeleteNode(this);
}

void
DeleteNode::childrenAccept (NSQLVisitor& visitor)
{

}
