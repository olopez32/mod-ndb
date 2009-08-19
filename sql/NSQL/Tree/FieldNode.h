/*
 * FieldNode.h
 *
 *  Created on: Jul 12, 2009
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

#ifndef FIELDNODE_H_
#define FIELDNODE_H_

#include "Node.h"
#include <cstdlib>

class FieldNode: public Node
{
private:
	char *name;
	char *table;
	char *alias;
	bool _hasAlias;
	bool _isPrimary;
	int _primaryKeyOrder;

public:
	FieldNode(int i) :
		Node(i){ name=NULL; table=NULL; alias=NULL ;
				_hasAlias  = false; _isPrimary = false;
				_primaryKeyOrder=0;}

	~FieldNode() {delete[] name; delete[] alias; delete[] table;}
	virtual void accept(NSQLVisitor &visitor);
	virtual void childrenAccept(NSQLVisitor &visitor);
	void setName(char *s );
	void setName(const char *s );
	const char * getName() const;
	void setAlias(char *s );
	bool hasAlias(){ return _hasAlias;}
	const char * getAlias() const;
	void setTable(char *s);
	void setTable(const char *s);
	const char * getTable() const;
	bool isPrimary() {return _isPrimary;}
	void setPrimary(bool isPrimary) { _isPrimary=isPrimary;}
	void setPrimaryKeyOrder(int primaryKeyOrder) { _primaryKeyOrder=primaryKeyOrder;}
	int getPrimaryKeyOrder() { return _primaryKeyOrder;}

};
#endif /* FIELDNODE_H_ */
