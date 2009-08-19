/*
 * Environment.h
 *
 *  Created on: Aug 5, 2009
 *      Author: tulay
 */

/* Copyright (C) 2009 Sun Microsystems
 All rights reserved. Use is subject to license terms.
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
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
