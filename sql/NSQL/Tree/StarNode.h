/*
 * StarNode.h
 *
 *  Created on: Jul 12, 2009
 *      Author: tulay
 */

#ifndef STARNODE_H_
#define STARNODE_H_

#include "Node.h"

class StarNode: public Node
{
public:
	StarNode(int i) :
		Node(i){}

	virtual void accept(NSQLVisitor &visitor);
	virtual void childrenAccept(NSQLVisitor &visitor);
};
#endif /* STARNODE_H_ */
