/*
 * it_nestedloopjoin.h
 *
 *  Created on: Feb 7, 2012
 *      Author: mickael
 */

#ifndef IT_NESTEDLOOPJOIN_H_
#define IT_NESTEDLOOPJOIN_H_

#include "ql.h"
#include "ql_iterator.h"

class IT_NestedLoopJoin : public virtual QL_Iterator {
public:
	IT_NestedLoopJoin(QL_Iterator* left, QL_Iterator* right, const Condition& _cond);
	virtual ~IT_NestedLoopJoin();

	RC Open();
	RC GetNext(RM_Record& outRec);
	RC Close();
private:
	QL_Iterator* LeftIterator;
	QL_Iterator* RightIterator;

	CompOp op;
	DataAttrInfo leftAttr, rightAttr;

	const Condition* cond;

	int lengthL, lengthR;
	char* pLeft;
	RM_Record pLeftRec;
};

#endif /* IT_NESTEDLOOPJOIN_H_ */
