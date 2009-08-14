/*
 * SelectNode.h
 *
 *  Created on: Jul 12, 2009
 *      Author: tulay
 */

#ifndef SELECTNODE_H_
#define SELECTNODE_H_

#include "Node.h"
#include "NSQLVisitor.h"
#include <iostream>

class NSQLVisitor;

class SelectNode: public Node
{
public:
	SelectNode(int i) :
		Node(i) { }
//	virtual ~SelectNode() { std::cout << "I am deleted\n";}

	virtual void accept(NSQLVisitor &visitor);
	virtual void childrenAccept(NSQLVisitor &visitor);
};

#endif /* SELECTNODE_H_ */
