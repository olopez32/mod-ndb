/*
 * LiteralNode.h
 *
 *  Created on: Jul 12, 2009
 *      Author: tulay
 */

#ifndef LITERALNODE_H_
#define LITERALNODE_H_

#include "Node.h"
#include <cstdlib>

class LiteralNode: public Node
{
private:
	char * value;
public:
	LiteralNode(int i) :
		Node(i), value(NULL){}
	~LiteralNode() { delete[] value;}
	virtual void accept(NSQLVisitor &visitor);
	virtual void childrenAccept(NSQLVisitor &visitor);
	void setValue(char *s );
	const char * getValue() const;
};
#endif /* LITERALNODE_H_ */
