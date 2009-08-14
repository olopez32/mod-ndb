/*
 * NSQLState.cpp
 *
 *  Created on: Jul 13, 2009
 *      Author: tulay
 *      TODO: Replace all list with apr_array_t
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
