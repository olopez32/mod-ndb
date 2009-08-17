/*
 * FieldNode.cpp
 *
 *  Created on: Jul 19, 2009
 *      Author: tulay
 */

#include "FieldNode.h"
#include "NSQLVisitor.h"

class NSQLVisitor;

void
FieldNode::accept (NSQLVisitor& visitor)
{
	visitor.VisitFieldNode(this);
}

void
FieldNode::childrenAccept (NSQLVisitor& visitor)
{

}

void
FieldNode::setName(char *s )
{
	if(name != NULL)
	{
		delete [] name;
	}

	name = s;
}

/*
 * Duplicates input string
 */
void
FieldNode::setName(const char *s )
{
	if(name != NULL)
	{
		delete [] name;
	}

	name =  0;
	int len = 0;
	if (s) { len = strlen(s); }
	name  = new char [len + 1];
	for (int i = 0; i < len; ++i) { name [i] = (char) s[i]; }
	name [len] = 0;
}


const char *
FieldNode::getName() const
{
	return (const char *)name;
}

void
FieldNode::setAlias(char *s )
{
	_hasAlias = true;
	delete [] alias;
	alias = s;
}


const char *
FieldNode::getAlias() const
{
	return (const char *)alias;
}

void
FieldNode::setTable(char *s)
{
	delete [] table;
	table = s;
}

void
FieldNode::setTable(const char *s)
{
	delete [] table;
	table =  0;
	int len = 0;
	if (s) { len = strlen(s); }
	table  = new char [len + 1];
	for (int i = 0; i < len; ++i) { table [i] = (char) s[i]; }
	table [len] = 0;
}

const char *
FieldNode::getTable() const
{
	return (const char *)table;
}
