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

	// Afficher les query plans
	virtual void Print(std::ostream &output, int spaces) = 0;

	virtual DataAttrInfo* getRelAttr() { return attrs; }
	virtual int getAttrCount() { return attrCount; }

	virtual int getLength() {
		int i = 0, length = 0;
		for(i = 0; i < attrCount; i++) {
			length += attrs[i].attrLength;
		}
		return length;
	}

	void PrintACond(std::ostream &output, const Condition* cond) {
		if(cond->op == NO_OP) {
			output << "No Condition";
		} else {
			output << "{" << cond->lhsAttr << " " << cond->op << " ";
			if(cond->bRhsIsAttr) {
				output << cond->rhsAttr;
			} else {
				if(cond->rhsValue.type == INT) {
					output << *((int*)cond->rhsValue.data);
				} else if(cond->rhsValue.type == FLOAT) {
					output << *((float*)cond->rhsValue.data);
				} else if(cond->rhsValue.type == STRING) {
					output << "\"" << ((char*)cond->rhsValue.data) << "\"";
				}
			}
			output << "}";
		}
	}

protected:
	bool bIsOpen;
	DataAttrInfo* attrs;
	int attrCount;
};

#endif /* QL_TERATOR_H_ */
