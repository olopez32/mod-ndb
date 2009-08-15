/*
 * GTNode.h
 *
 *  Created on: Jul 12, 2009
 *      Author: tulay
 */

#ifndef GTNODE_H_
#define GTNODE_H_

#include "Node.h"

class GTNode: public Node
{
public:
	GTNode(int i) :
		Node(i){}

	virtual void accept(NSQLVisitor &visitor);
	virtual void childrenAccept(NSQLVisitor &visitor);
};
#endif /* GTNODE_H_ */
