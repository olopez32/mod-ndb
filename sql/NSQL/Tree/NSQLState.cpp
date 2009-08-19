/*
 * NSQLState.cpp
 *
 *  Created on: Jul 13, 2009
 *      Author: tulay
 *      TODO: Replace all list with apr_array_t
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

#include "NSQLState.h"


void
NSQLState::pushNode(Node *n) {
	nodes.push_back(n);
	++sp;
}


Node *
NSQLState::popNode() {
	if (--sp < mk) {
		mk = marks.back();
		marks.pop_back();
	}
	Node *node = nodes.back();
	nodes.pop_back();
	return node;
}


void
NSQLState::clearNodeScope(Node *n) {
	while (sp > mk) {
		Node *top = popNode();
		delete top;
	}
	mk = marks.back();
	marks.pop_back();
}


void
NSQLState::openNodeScope(Node *n) {
	marks.push_back(mk);
	mk = sp;
	n->open();
}




void
NSQLState::closeNodeScope(Node *n, int num) {
	mk = marks.back();
	marks.pop_back();
	while (num-- > 0) {
		Node *c = popNode();
		c->setParent(n);
		n->addChild(c, num);
	}
	n->close();
	pushNode(n);
	node_created = true;
}



void
NSQLState::closeNodeScope(Node *n, bool condition) {
	if (condition) {
		int a = nodeArity();
		mk = marks.back();
		marks.pop_back();
		while (a-- > 0) {
			Node *c = popNode();
			c->setParent(n);
			n->addChild(c, a);
		}
		n->close();
		pushNode(n);
		node_created = true;
	} else {
		mk = marks.back();
		marks.pop_back();
		node_created = false;
	}
}


void
NSQLState::reset() {

	while(!nodes.empty())
		nodes.pop_back();

	while(!marks.empty())
			marks.pop_back();
	sp = 0;
	mk = 0;
}
