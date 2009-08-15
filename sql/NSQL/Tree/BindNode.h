/*
 * BindNode.h
 *
 *  Created on: Jul 12, 2009
 *      Author: tulay
 */

#ifndef BINDNODE_H_
#define BINDNODE_H_

#include "Node.h"

class BindNode: public Node
{
private:
	char *name;
public:
	BindNode(int i) :
		Node(i){}

	~BindNode() {delete[] name;}
	virtual void accept(NSQLVisitor &visitor);
	virtual void childrenAccept(NSQLVisitor &visitor);
	void setName(char *s );
	const char * getName() const;
};
#endif /* BINDNODE_H_ */
