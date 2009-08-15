/*
 * Node.h
 *
 *  Created on: Jul 12, 2009
 *      Author: tulay
 */

#ifndef NODE_H_
#define NODE_H_

#include <cstdlib>
#include <vector>

#include "TreeConstants.h"

class NSQLVisitor;

class Node
{
public:
	Node(){}
	virtual ~Node();
	explicit Node(int i): id(i), parent(NULL){}
	const char * getType() const;
	void setParent(Node *X) { parent = X;}
	Node *getParent() const {return parent;}
	void addChild(Node *X, int i);
	Node *getChild(int i);
	int getNumOfChildren() const {return children.size();}
	void open() {}
	void close() {}

	virtual void accept (NSQLVisitor& visitor);





	//const Node &Node::operator= (const Node & rhs);


protected:
	int id;
	Node *parent;
	std::vector<Node*> children;


};

#endif /* NODE_H_ */
