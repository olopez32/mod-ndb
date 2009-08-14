/*
 * DeleteNode.h
 *
 *  Created on: Jul 12, 2009
 *      Author: tulay
 */

#ifndef DELETENODE_H_
#define DELETENODE_H_

#include "Node.h"

class DeleteNode: public Node
{
public:
	DeleteNode(int i) :
		Node(i){}

	virtual void accept(NSQLVisitor &visitor);
	virtual void childrenAccept(NSQLVisitor &visitor);
};
#endif /* DELETENODE_H_ */
