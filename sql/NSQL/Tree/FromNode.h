/*
 * FromNode.h
 *
 *  Created on: Jul 22, 2009
 *      Author: tulay
 */

#ifndef FROMNODE_H_
#define FROMNODE_H_

#include "Node.h"
#include "NSQLVisitor.h"

class FromNode: public Node {
public:
	FromNode(int i) :
		Node(i) { }



	virtual void accept(NSQLVisitor &visitor);
	virtual void childrenAccept(NSQLVisitor &visitor);

	virtual ~FromNode();
};

#endif /* FROMNODE_H_ */
