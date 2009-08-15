/*
 * FieldNode.h
 *
 *  Created on: Jul 12, 2009
 *      Author: tulay
 */

#ifndef FIELDNODE_H_
#define FIELDNODE_H_

#include "Node.h"
#include <cstdlib>

class FieldNode: public Node
{
private:
	char *name;
	char *table;
	char *alias;
	bool _hasAlias;

public:
	FieldNode(int i) :
		Node(i){ name=NULL; table=NULL; alias=NULL ; _hasAlias  = false;}

	~FieldNode() {delete[] name; delete[] alias; delete[] table;}
	virtual void accept(NSQLVisitor &visitor);
	virtual void childrenAccept(NSQLVisitor &visitor);
	void setName(char *s );
	const char * getName() const;
	void setAlias(char *s );
	bool hasAlias(){ return _hasAlias;}
	const char * getAlias() const;
	void setTable(char *s);
	void setTable(const char *s);
	const char * getTable() const;
};
#endif /* FIELDNODE_H_ */
