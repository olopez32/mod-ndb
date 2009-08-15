/*
 * DeleteNode.cpp
 *
 *  Created on: Jul 19, 2009
 *      Author: tulay
 */

#include "OrderNode.h"
#include "NSQLVisitor.h"

class NSQLVisitor;

void
OrderNode::accept (NSQLVisitor& visitor)
{
	visitor.VisitOrderNode(this);
}

void
OrderNode::childrenAccept (NSQLVisitor& visitor)
{

}
