/*
 * NEQNode.h
 *
 *  Created on: Jul 12, 2009
 *      Author: tulay
 */

#ifndef NEQNODE_H_
#define NEQNODE_H_

#include "Node.h"

class NEQNode: public Node
{
public:
	NEQNode(int i) :
		Node(i){}

	virtual void accept(NSQLVisitor &visitor);
	virtual void childrenAccept(NSQLVisitor &visitor);
};
#endif /* NEQNODE_H_ */
