/*
 * SelectNode.h
 *
 *  Created on: Jul 12, 2009
 *      Author: tulay
 */

#ifndef ORNODE_H_
#define ORNODE_H_

#include "Node.h"
#include "NSQLVisitor.h"


class NSQLVisitor;

class ORNode: public Node
{
public:
	ORNode(int i) :
		Node(i) { }

	virtual void accept(NSQLVisitor &visitor);
	virtual void childrenAccept(NSQLVisitor &visitor);
};

#endif /* ORNODE_H_ */
