/*
 * it_filter.h
 *
 *  Created on: Feb 7, 2012
 *      Author: mickael
 */

#ifndef IT_FILTER_H_
#define IT_FILTER_H_

#include "ql.h"
#include "ql_iterator.h"

class IT_Filter : public virtual QL_Iterator {
public:
	IT_Filter(QL_Iterator* it, const Condition& cond);
	virtual ~IT_Filter();

	RC Open();
	RC GetNext(RM_Record& outRec);
	RC Close();
private:
	QL_Iterator* it;
	const Condition* cond;

	DataAttrInfo leftAttr;
};

#endif /* IT_FILTER_H_ */
