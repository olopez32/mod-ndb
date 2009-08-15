/*
 * LiteralNode.cpp
 *
 *  Created on: Jul 19, 2009
 *      Author: tulay
 */

#include "LiteralNode.h"
#include "NSQLVisitor.h"
#include <string>

class NSQLVisitor;

void
LiteralNode::accept (NSQLVisitor& visitor)
{
	visitor.VisitLiteralNode(this);
}

void
LiteralNode::childrenAccept (NSQLVisitor& visitor)
{

}



void
LiteralNode::setValue(char *s )
{
	//Deleting a null pointer is safe. 5.3.5/2 of the Standard
	delete [] value;
	value = s;
}

//void
//LiteralNode::setValue(const char *s )
//{
//	//Deleting a null pointer is safe. 5.3.5/2 of the Standard
//	delete [] value;
//	value = 0;
//	int len = 0;
//	if (s) { len = strlen(s); }
//	value = new char [len + 1];
//	for (int i = 0; i < len; ++i) { value[i] = (char) s[i]; }
//	value[len] = 0;
//}

const char *
LiteralNode::getValue() const
{
	return (const char *)value;
}
