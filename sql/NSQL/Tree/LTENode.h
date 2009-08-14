/*
 * LTENode.h
 *
 *  Created on: Jul 12, 2009
 *      Author: tulay
 */

#ifndef LTENODE_H_
#define LTENODE_H_

#include "Node.h"

class LTENode: public Node
{
public:
	LTENode(int i) :
		Node(i){}

	virtual void accept(NSQLVisitor &visitor);
	virtual void childrenAccept(NSQLVisitor &visitor);
};
#endif /* LTENODE_H_ */
