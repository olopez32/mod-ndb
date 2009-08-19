/*
 * TreeConstants.h
 *
 *  Created on: Jul 12, 2009
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

#ifndef TREECONSTANTS_H_
#define TREECONSTANTS_H_



enum NodeType {VOID=0, QUERY=1, SELECT=2, FIELD=3, FROM=4, WHERE=5, TABLE=6, AND=7, OR=8,
	NEQ=9, EQ=10, LTE=11, LT=12,GTE=13, GT=14, STAR=15, LITERAL=16, BIND=17, DELETE=18, ORDER=19};

namespace{
const char*  NodeName[] = {"VOID", "QUERY","SELECT", "FIELD", "FROM", "WHERE", "TABLE", "AND", "OR",
		"NEQ", "EQ", "LTE", "LT", "GTE", "GT", "STAR" ,"LITERAL", "BIND" , "DELETE", "ORDER"};
}

#endif /* TREECONSTANTS_H_ */
