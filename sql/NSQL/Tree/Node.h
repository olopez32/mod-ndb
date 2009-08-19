/*
 * Node.h
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

#ifndef NODE_H_
#define NODE_H_

#include <cstdlib>
#include <vector>

#include "TreeConstants.h"

class NSQLVisitor;

class Node
{
public:
	Node(){}
	virtual ~Node();
	explicit Node(int i): id(i), parent(NULL){}
	const char * getType() const;
	void setParent(Node *X) { parent = X;}
	Node *getParent() const {return parent;}
	void addChild(Node *X, int i);
	Node *getChild(int i);
	int getNumOfChildren() const {return children.size();}
	void open() {}
	void close() {}

	virtual void accept (NSQLVisitor& visitor);





	//const Node &Node::operator= (const Node & rhs);


protected:
	int id;
	Node *parent;
	std::vector<Node*> children;


};

#endif /* NODE_H_ */
