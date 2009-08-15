/*
 * Symbol.h
 *
 *  Created on: Aug 5, 2009
 *      Author: tulay
 */

#ifndef SYMBOL_H_
#define SYMBOL_H_

#include <string>
class Symbol {
public:
	Symbol() {}
	virtual ~Symbol();

private:
	std::string value;
};

#endif /* SYMBOL_H_ */
