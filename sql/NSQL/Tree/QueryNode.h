/*
 * QueryNode.h
 *
 *  Created on: Jul 12, 2009
 *      Author: tulay
 */

#ifndef QUERYNODE_H_
#define QUERYNODE_H_


#include "Node.h"
#include "NSQLVisitor.h"

class NSQLVisitor;

class QueryNode: public Node
{
public:
	QueryNode(int i) :
		Node(i)
		{ }

//	virtual ~QueryNode();
	virtual void accept(NSQLVisitor &visitor);
	virtual void childrenAccept(NSQLVisitor &visitor);

};

#endif /* QUERYNODE_H_ */
