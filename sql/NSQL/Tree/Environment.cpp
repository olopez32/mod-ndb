/*
 * Environment.cpp
 *
 *  Created on: Aug 5, 2009
 *      Author: tulay
 */

#include "Environment.h"

Environment::Environment() {
	// TODO Auto-generated constructor stub

}

Environment::~Environment() {
	// TODO Auto-generated destructor stub
}

void
Environment::put(std::string s, Symbol symbol)
{
	symtable.insert(SymTable::value_type(s,symbol));

}

Symbol &
Environment::get(std::string  s)
{

	SymTable::iterator found = symtable.find(s);
	return (*found).second;
}
