/*
 * TableNode.h
 *
 *  Created on: Jul 22, 2009
 *      Author: tulay
 */

#ifndef TABLENODE_H_
#define TABLENODE_H_

#include "Node.h"
#include "NSQLVisitor.h"

class TableNode: public Node
{
private:
	char *name;
	char *alias;
	bool _hasAlias;
	char *database;
public:
	TableNode(int i) :
		Node(i)
		{name = NULL ; database = NULL; alias = NULL; _hasAlias = false; }

	~TableNode() {delete[] name; delete[] alias; delete[] database;}
	virtual void accept(NSQLVisitor &visitor);
	virtual void childrenAccept(NSQLVisitor &visitor);
	void setName(char *s );
	const char * getName() const;
	void setAlias(char *s );
	bool hasAlias(){ return _hasAlias;}
	const char * getAlias() const;
	void setDatabaseName(char *s );
	const char * getDatabaseName() const;
};

#endif /* TABLENODE_H_ */
