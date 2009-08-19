/*
 * TableNode.cpp
 *
 *  Created on: Jul 22, 2009
 *      Author: tulay
 */

/* Copyright (C) 2009 Sun Microsystems
 All rights reserved. Use is subject to license terms.
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
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
