/*
 * it_filter.h
 *
 *  Created on: Feb 7, 2012
 *      Author: mickael
 */

#ifndef IT_FILTER_H_
#define IT_FILTER_H_

#include "ql.h"

class IT_Filter : public virtual QL_Iterator {
public:
	IT_Filter(QL_Iterator* it, Condition& cond);
	virtual ~IT_Filter();
private:
	QL_Iterator* it;
	Condition* cond;

	DataAttrInfo leftAttr;
};

#endif /* IT_FILTER_H_ */
