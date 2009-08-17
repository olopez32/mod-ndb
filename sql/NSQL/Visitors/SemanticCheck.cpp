/*
 * SemanticCheck.cpp
 *
 *  Created on: Jul 19, 2009
 *      Author: tulay
 */
/* refman-5.1-en.pdf p959
 * A PRIMARY KEY is a unique index where all key columns must be defined as NOT NULL. If they are not explicitly declared as
 * NOT NULL, MySQL declares them so implicitly (and silently). A table can have only one PRIMARY KEY. If you do not have a
 * PRIMARY KEY and an application asks for the PRIMARY KEY in your tables, MySQL returns the first UNIQUE index that has no
 * NULL columns as the PRIMARY KEY.
 */
#include "SemanticCheck.h"
#include <iostream>
#include <cstring>

SemanticCheck::~SemanticCheck()
{

}


void
SemanticCheck::setNdb(Ndb *ndb)
{
	myNdb = ndb;
}

/**
 * This method behaves like an automaton
 */
inline
void
SemanticCheck::setStatus(STATUS status)
{
	if (_status != RESOLVED_ERROR){
		_status = status;
	}
}


//Node *
//SemanticCheck::findNode(const char * name, enum NodeType type)
//{
//
//	if( type == TABLE)
//	{
//		std::cout << "LAL\n";
//	}
//	return NULL;
//}


void
SemanticCheck::VisitQueryNode(QueryNode* n)
{
	n->childrenAccept(*this);
	this->setStatus(RESOLVED_OK);
}

void
SemanticCheck::VisitSelectNode(SelectNode* n)
{
	n->childrenAccept(*this);
}

void
SemanticCheck::VisitFieldNode(FieldNode* n)
{

	const NdbDictionary::Dictionary* myDict= myNdb->getDictionary();

	if(n->getTable() != NULL ){
		// Table name is provided in the query
		// Search From clause for this table name
		Node * root = n->getParent();
		while(root->getParent() != NULL ){
			root = root->getParent();
		}
		//Root is the QueryNode
		FromNode * fromNode  = (FromNode *) root->getChild(1);
		int numOfChildren = fromNode->getNumOfChildren();
		bool found = false;
		for(int i = 0; i < numOfChildren; i++){
			if(strcmp(n->getTable(), ((TableNode *) fromNode->getChild(i))->getName()) ==0){
				found = true;
				break;
			}
		}

		if(!found){
			std::cout << n->getTable() << " could not be found in From Clause.\n";
			this->setStatus(RESOLVED_ERROR);
			return;
		}
		const NdbDictionary::Table *myTable= myDict->getTable(n->getTable());
		if(myTable != NULL )
		{
			const NdbDictionary::Column * myColumn = myTable->getColumn(n->getName());
			if (myColumn != NULL ){
				std::cout << "Field exists: " << n->getName() << std::endl;
				this->setStatus(RESOLVED_OK);
			}
			else{
				std::cout << "Field does not exist: " << n->getName() << std::endl;
				this->setStatus(RESOLVED_ERROR);
			}

		}
		else{
			std::cout << (myDict->getNdbError()).message << ":" << n->getTable() << std::endl;
			this->setStatus(RESOLVED_ERROR);

		}
	}
	else
	{
		// Although one table name is allowed in the grammar now, note the following for future.
		// Resolve Table name, Scan From clause for table name
		// Ambiguous cases are not considered. First match is assigned.
		Node * root = n->getParent();
		while(root->getParent() != NULL ){
			root = root->getParent();
		}
		//Root is the QueryNode
		FromNode * fromNode  = (FromNode *) root->getChild(1);
		// TODO: assertion is needed whether this child(1) is FROMNode!
		int numOfChildren = fromNode->getNumOfChildren();
		bool isTableResolved = false;
		for(int i = 0; i < numOfChildren; i++){
			const char * tableCandidate =  ((FieldNode *) fromNode->getChild(i))->getName() ;
			const NdbDictionary::Table *myTable= myDict->getTable(tableCandidate);
			if(myTable != NULL )
			{
				if(strcmp(n->getName(),"PRIMARY") == 0)
				{
					// If the name of the field is PRIMARY after parsing
					// We need to find the table name to get the real field names
					// I will assume that the first table in from clause is related to these fields
					// Get the number of primary key columns
					// Check if the order is smaller than this
					// Get the name of the primary key, assign it as name and assign this as the table
					/*
					// it is usually not necessary to call aggregate() or validate() directly ndbapi-en.pdf p196
					if ( myTable->aggregate(myNdb->getNdbError()) != 0 ){
						std::cout << " Table " << tableCandidate << " is in inconsistant state\n";
					}
					*/
					int nofpk = myTable->getNoOfPrimaryKeys();
					if (nofpk < n->getPrimaryKeyOrder()){
						std::cout << "Primary Key mismatch: " << n->getName() << std::endl;
					}
					else
					{
						n->setName(myTable->getPrimaryKey(n->getPrimaryKeyOrder()));
						isTableResolved = true;
						n->setTable(tableCandidate);

					}
					/**
					 * The first table is assumed to be related to PRIMARY KEY part
					 */
					break;
				}
				const NdbDictionary::Column * myColumn = myTable->getColumn(n->getName());
				if (myColumn != NULL ){
					isTableResolved = true;
					n->setTable(tableCandidate);
					break;
					// TODO: change setTable to duplicate the string, revise code for coco_string functions
					// TODO: Maybe Field should have a pointer to the TableNode?
				}
			}
		}
		if ( isTableResolved){
			std::cout << "Table for the Field "<< n->getName() << " is being resolved as " << n->getTable()<< std::endl;
			std::cout << "Field exists: " << n->getName() << std::endl;
			this->setStatus(RESOLVED_OK);
		}
		else{
			std::cout << "Table for the Field "<< n->getName() << " could not be resolved." << std::endl;
			std::cout << "Field does not exist: " << n->getName() << std::endl;
			this->setStatus(RESOLVED_ERROR);
		}
	}
}




void
SemanticCheck::VisitFromNode(FromNode* n)
{

	n->childrenAccept(*this);
}

void
SemanticCheck::VisitTableNode(TableNode* n)
{
	//Set database

	if (n->getDatabaseName() != NULL){
		myNdb->setDatabaseName(n->getDatabaseName());
	}
	NdbError err = myNdb->getNdbError();
	if ( err.status != err.Success ) {
		std::cout << err.message  << std::endl;
		this->setStatus(RESOLVED_ERROR);
		return;
	}

	const NdbDictionary::Dictionary* myDict= myNdb->getDictionary();
	const NdbDictionary::Table *myTable= myDict->getTable(n->getName());
	if(myTable == NULL )
	{
		std::cout << (myDict->getNdbError()).message << ":" << n->getName() << std::endl;
		this->setStatus(RESOLVED_ERROR);
	}
	else{
		std::cout << "Table exists:" << n->getName() << std::endl;
		this->setStatus(RESOLVED_OK);
	}

	//Table::aggregate() before getNoOfPrimaryKeys()
	//	myTable->getNoOfColumns() ;
	// myTable->getPrimaryKey(0)
	// const_cast<char *>(myNdb->getDatabaseName())
}

void
SemanticCheck::VisitWhereNode(WhereNode* n)
{

	n->childrenAccept(*this);
}

void
SemanticCheck::VisitORNode(ORNode* n)
{

	n->childrenAccept(*this);
}

void
SemanticCheck::VisitANDNode(ANDNode* n)
{

	n->childrenAccept(*this);
}


void
SemanticCheck::VisitEQNode(EQNode* n)
{

	n->childrenAccept(*this);

}

void
SemanticCheck::VisitGTENode(GTENode* n)
{

	n->childrenAccept(*this);
}

void
SemanticCheck::VisitGTNode(GTNode* n)
{

	n->childrenAccept(*this);
}

void
SemanticCheck::VisitLTENode(LTENode* n)
{
	n->childrenAccept(*this);
}

void
SemanticCheck::VisitLTNode(LTNode* n)
{

	n->childrenAccept(*this);
}

void
SemanticCheck::VisitNEQNode(NEQNode* n)
{

	n->childrenAccept(*this);
}

void
SemanticCheck::VisitStarNode(StarNode* n)
{
}

void
SemanticCheck::VisitLiteralNode(LiteralNode* n)
{

}

void
SemanticCheck::VisitBindNode(BindNode* n)
{
}


void
SemanticCheck::VisitDeleteNode(DeleteNode* n)
{
}

void
SemanticCheck::VisitOrderNode(OrderNode* n)
{
}

