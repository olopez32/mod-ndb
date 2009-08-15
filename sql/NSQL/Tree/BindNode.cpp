/*
 * BindNode.cpp
 *
 *  Created on: Jul 19, 2009
 *      Author: tulay
 */

#include "BindNode.h"
#include "NSQLVisitor.h"

class NSQLVisitor;

void
BindNode::accept (NSQLVisitor& visitor)
{
	visitor.VisitBindNode(this);
}

void
BindNode::childrenAccept (NSQLVisitor& visitor)
{

}

void
BindNode::setName(char *s )
{
	delete [] name;
	name = s;
}


const char *
BindNode::getName() const
{
	return (const char *)name;
}
