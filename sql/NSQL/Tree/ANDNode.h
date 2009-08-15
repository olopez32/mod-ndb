/*
 * ANDNode.h
 *
 *  Created on: Jul 12, 2009
 *      Author: tulay
 */

#ifndef ANDNODE_H_
#define ANDNODE_H_

#include "Node.h"

class ANDNode: public Node
{
public:
	ANDNode(int i) :
		Node(i){}

	virtual void accept(NSQLVisitor &visitor);
	virtual void childrenAccept(NSQLVisitor &visitor);
};
#endif /* ANDNODE_H_ */
