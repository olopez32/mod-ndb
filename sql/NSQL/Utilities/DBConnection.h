/*
 * DBConnection.h
 *
 *  Created on: Aug 3, 2009
 *      Author: tulay
 */

#ifndef DBCONNECTION_H_
#define DBCONNECTION_H_

#include <NdbApi.hpp>

class DBConnection {
public:
	DBConnection(const char *connectstring, const char * databaseName);
	virtual ~DBConnection();
	Ndb * getNdb() {return ndb;}

private:
	Ndb_cluster_connection *cluster_connection;
	Ndb *ndb;

};

#endif /* DBCONNECTION_H_ */
