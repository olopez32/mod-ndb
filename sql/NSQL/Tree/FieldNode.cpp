/*
 * FieldNode.cpp
 *
 *  Created on: Jul 19, 2009
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
