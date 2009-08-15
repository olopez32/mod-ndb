/*
 * WhereNode.h
 *
 *  Created on: Jul 22, 2009
 *      Author: tulay
 */

#ifndef WHERENODE_H_
#define WHERENODE_H_

#include "Node.h"
#include "NSQLVisitor.h"

class WhereNode: public Node {
public:
	WhereNode(int i) :
		Node(i)
		{ }

	virtual void accept(NSQLVisitor &visitor);
	virtual void childrenAccept(NSQLVisitor &visitor);
	virtual ~WhereNode();


};

#endif /* WHERENODE_H_ */
