/*
 * EQNode.h
 *
 *  Created on: Jul 12, 2009
 *      Author: tulay
 */

#ifndef EQNODE_H_
#define EQNODE_H_

#include "Node.h"

class EQNode: public Node
{
public:
	EQNode(int i) :
		Node(i){}

	virtual void accept(NSQLVisitor &visitor);
	virtual void childrenAccept(NSQLVisitor &visitor);
};
#endif /* EQNODE_H_ */
