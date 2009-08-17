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
	bool _isPrimary;
	int _primaryKeyOrder;

public:
	FieldNode(int i) :
		Node(i){ name=NULL; table=NULL; alias=NULL ;
				_hasAlias  = false; _isPrimary = false;
				_primaryKeyOrder=0;}

	~FieldNode() {delete[] name; delete[] alias; delete[] table;}
	virtual void accept(NSQLVisitor &visitor);
	virtual void childrenAccept(NSQLVisitor &visitor);
	void setName(char *s );
	void setName(const char *s );
	const char * getName() const;
	void setAlias(char *s );
	bool hasAlias(){ return _hasAlias;}
	const char * getAlias() const;
	void setTable(char *s);
	void setTable(const char *s);
	const char * getTable() const;
	bool isPrimary() {return _isPrimary;}
	void setPrimary(bool isPrimary) { _isPrimary=isPrimary;}
	void setPrimaryKeyOrder(int primaryKeyOrder) { _primaryKeyOrder=primaryKeyOrder;}
	int getPrimaryKeyOrder() { return _primaryKeyOrder;}

};
#endif /* FIELDNODE_H_ */
