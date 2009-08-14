/*
 * Node.cpp
 *
 *  Created on: Jul 12, 2009
 *      Author: tulay
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
