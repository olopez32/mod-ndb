/*
 * NSQLState.h
 *
 *  Created on: Jul 13, 2009
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

#ifndef NSQLSTATE_H_
#define NSQLSTATE_H_

#include <list>
#include "Node.h"



class NSQLState
{
public:


	NSQLState():sp(0), mk(0),node_created(false){}

	/*
	 * Pushes a node on to the stack.
	 */
	void pushNode(Node *n);

	/*
	 * Returns the node on the top of the stack, and remove it from the stack.
	 */
	Node *NSQLState::popNode();

	/*
	 * Returns the node currently on the top of the stack.
	 */
	 Node *peekNode() {return nodes.back();}


	 /*
	  * Returns the number of children on the stack in the current node scope.
	  */
	 int nodeArity() {return sp - mk;}


	 /*
	  *
	  */
	 void clearNodeScope(Node *n);



	 /*
	  *
	  */
	 void openNodeScope(Node *n);



	 /* A definite node is constructed from a specified number of children.
	  * That number of nodes are popped from the stack and made the children
	  * of the definite node.  Then the definite node is pushed on to the stack.
	  */
	 void closeNodeScope(Node *n, int num);


	 /*
	  * A conditional node is constructed if its condition is true.
	  * All the nodes that have been pushed since the node was opened are
	  * made children of the conditional node, which is then pushed
	  * on to the stack.  If the condition is false the node is not
	  * constructed and they are left on the stack.
	  */
	 void closeNodeScope(Node *n, bool condition);

	/*
	 * Returns the root node of the AST.  It only makes sense to call
	 * this after a successful parse.
	 */
	Node *getRoot() const{ if(nodes.empty())  return NULL; else return nodes.back(); }


	 /*
	  * Call this to reinitialize the node stack. */
	  void reset();


private:
	std::list<Node*> nodes;
	std::list<int> marks;
	int sp; //number of nodes in stack
	int mk ; // current mark
	bool node_created ;
};

#endif /* NSQLSTATE_H_ */
