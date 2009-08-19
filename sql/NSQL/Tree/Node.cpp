/*
 * Node.cpp
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

#include "Node.h"
#include <iostream>
#include <typeinfo>


void
Node::addChild(Node *X, int i)
{

	if ((unsigned) i >= children.size())
	{
		children.resize(i+1);
	}
	X->parent = this;
	children[i] = X;
}


const char *
Node::getType() const
{
	return NodeName[id];

}

Node *
Node::getChild(int i)
{
	if( (unsigned) i >= children.size())
		throw "Invalid child index";

	return children[i];

}

void
Node::accept (NSQLVisitor& visitor)
{
	//visitor.VisitNode(this);
}

Node::~Node()
{
	std::cout << "I am deleted: " << typeid(*this).name() << std::endl;

	std::vector<Node*>::iterator iterList;

   for (iterList = children.begin();
        iterList != children.end();
        iterList++)
   {
      delete *iterList;
   }

   children.clear();

}

//void
//Node::childrenAccept (NSQLVisitor& visitor)
//{
//	std::vector<Node*>::iterator itr;
//		for ( itr= children.begin(); itr !=children.end(); ++itr) {
//			itr()->accept(visitor);
//		}
//}
