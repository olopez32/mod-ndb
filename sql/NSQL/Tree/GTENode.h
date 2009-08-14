/*
 * GTENode.h
 *
 *  Created on: Jul 12, 2009
 *      Author: tulay
 */

#ifndef GTENODE_H_
#define GTENODE_H_

#include "Node.h"

class GTENode: public Node
{
public:
	GTENode(int i) :
		Node(i){}

	virtual void accept(NSQLVisitor &visitor);
	virtual void childrenAccept(NSQLVisitor &visitor);
};
#endif /* GTENODE_H_ */
