/*
 * OrderNode.h
 *
 *  Created on: Jul 12, 2009
 *      Author: tulay
 */

#ifndef ORDERNODE_H_
#define ORDERNODE_H_

#include "Node.h"

class OrderNode: public Node
{
private:
	bool asc;
public:
	OrderNode(int i) :
		Node(i), asc(false){}

	void setAsc() { asc = true;}
	void setDesc() { asc = false;}
	bool isAsc() {return asc;}
	bool isDesc() {return !asc;}
	virtual void accept(NSQLVisitor &visitor);
	virtual void childrenAccept(NSQLVisitor &visitor);
};
#endif /* ORDERNODE_H_ */
