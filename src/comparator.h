/*
 * comparator.h
 *
 *  Created on: Feb 5, 2012
 *      Author: mickael
 */

#ifndef COMPARATOR_H_
#define COMPARATOR_H_

#include "ql.h"

class Comparator {
public:
	Comparator(AttrType attrType, int attrLength, int attrOffset, CompOp op, char* pData);
	virtual ~Comparator();

	bool Compare(const void* pData) const;

private:
	AttrType attrType;
	int attrLength;
	int attrOffset;
	CompOp op;
	const void* value;
};

#endif /* COMPARATOR_H_ */
