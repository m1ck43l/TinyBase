/*
 * ql_terator.h
 *
 *  Created on: Feb 5, 2012
 *      Author: mickael
 */

#ifndef QL_TERATOR_H_
#define QL_TERATOR_H_

#include "ql.h"

class QL_Iterator {
public:
	QL_Iterator():bIsOpen(false) {};
	virtual ~QL_Iterator() {};

	virtual RC Open() = 0;
	virtual RC GetNext(RM_Record& outRec) = 0;
	virtual RC Close() = 0;

	virtual DataAttrInfo* getRelAttr() { return attrs; }
	virtual int getAttrCount() { return attrCount; }

	virtual int getLength() {
		int i = 0, length = 0;
		for(i = 0; i < attrCount; i++) {
			length += attrs[i].attrLength;
		}
		return length;
	}

protected:
	bool bIsOpen;
	DataAttrInfo* attrs;
	int attrCount;
};

#endif /* QL_TERATOR_H_ */
