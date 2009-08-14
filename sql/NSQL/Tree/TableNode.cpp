/*
 * TableNode.cpp
 *
 *  Created on: Jul 22, 2009
 *      Author: tulay
 */

#include "TableNode.h"


void TableNode::accept (NSQLVisitor& visitor)
{
	visitor.VisitTableNode(this);

}

void
TableNode::childrenAccept (NSQLVisitor& visitor)
{
	std::vector<Node*>::iterator itr;
		for ( itr= children.begin(); itr !=children.end(); ++itr) {
			(*itr)->accept(visitor);
		}
}

void
TableNode::setName(char *s )
{
	if(name != NULL) {
		delete [] name;
		}
	//Assertion:
	//Relies on COCO function coco_string_create_char(t->val);
	//that allocates memory
	name = s;
}


const char *
TableNode::getName() const
{
	return (const char *)name;
}

void
TableNode::setAlias(char *s )
{
	_hasAlias = true;
	delete [] alias;
	alias = s;
}


const char *
TableNode::getAlias() const
{
	return (const char *)alias;
}


void
TableNode::setDatabaseName(char *s )
{
	delete [] database;
	database = s;
}

const char *
TableNode::getDatabaseName() const
{
	return (const char *)database;
}
