/*
 * Environment.h
 *
 *  Created on: Aug 5, 2009
 *      Author: tulay
 */

#ifndef ENVIRONMENT_H_
#define ENVIRONMENT_H_

#include "Symbol.h"
#include <map.h>
#include <string>


class Environment {
	typedef map<std::string, Symbol > SymTable;
public:
	Environment();
	virtual ~Environment();
	void put(std::string s, Symbol symbol);
	Symbol & get(std::string s) ;

protected:
	SymTable symtable;
};

#endif /* ENVIRONMENT_H_ */
