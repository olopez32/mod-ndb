/*
 * DBConnection.cpp
 *
 *  Created on: Aug 3, 2009
 *      Author: tulay
 */

#include "DBConnection.h"
#include <iostream>
#include <cstdlib>

#define PRINT_ERROR(code,msg) \
	std::cout << "Error in " << __FILE__ << ", line: " << __LINE__ \
	<< ", code: " << code \
	<< ", msg: " << msg << "." << std::endl

#define APIERROR(error) { \
	PRINT_ERROR(error.code,error.message); \
	exit(-1); }

DBConnection::DBConnection(const char *connectstring, const char * databaseName) {
	ndb_init();

	// Object representing the cluster
	cluster_connection = new Ndb_cluster_connection(connectstring);

	// Connect to cluster management server (ndb_mgmd)
	if (cluster_connection->connect(4 /* retries               */,
			5 /* delay between retries */,
			1 /* verbose               */))
	{
		std::cout << "Cluster management server was not ready within 30 secs.\n";
		exit(-1);
	}

	// Connect and wait for the storage nodes (ndbd's)
	if (cluster_connection->wait_until_ready(30,0) < 0)
	{
		std::cout << "Cluster was not ready within 30 secs.\n";
		exit(-1);
	}

	cluster_connection->set_name("Semantic Check");

	ndb = new Ndb( cluster_connection, databaseName );
	if (ndb->init()) APIERROR(ndb->getNdbError());

	std::cout << "Connection to cluster management server established\n";


}

DBConnection::~DBConnection() {
	delete ndb;
	delete cluster_connection;
	ndb_end(0);

}
