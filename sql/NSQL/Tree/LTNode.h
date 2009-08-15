/*
 * LTNode.h
 *
 *  Created on: Jul 12, 2009
 *      Author: tulay
 */

#ifndef LTNODE_H_
#define LTNODE_H_

#include "Node.h"

class LTNode: public Node
{
public:
	LTNode(int i) :
		Node(i){}

	virtual void accept(NSQLVisitor &visitor);
	virtual void childrenAccept(NSQLVisitor &visitor);
};
#endif /* LTNODE_H_ */
