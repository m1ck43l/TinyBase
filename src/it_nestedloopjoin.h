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
	IT_NestedLoopJoin(QL_Iterator* left, QL_Iterator* right, Condition _cond)
			: LeftIterator(left), RightIterator(right), cond(_cond), leftTpl(left->getLength()), rightTpl(right->getLength()) {};
	virtual ~IT_NestedLoopJoin();

private:
	QL_Iterator* LeftIterator;
	QL_Iterator* RightIterator;

	Condition cond;
	Tuple leftTpl, rightTpl;
};

#endif /* IT_NESTEDLOOPJOIN_H_ */
