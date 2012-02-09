/*
 * it_projection.h
 *
 *  Created on: Feb 7, 2012
 *      Author: mickael
 */

#ifndef IT_PROJECTION_H_
#define IT_PROJECTION_H_

#include "ql.h"
#include "ql_iterator.h"

class IT_Projection : public virtual QL_Iterator {
public:
	IT_Projection(QL_Iterator* it, int nSelAttrs, const RelAttr selAttrs[]);
	virtual ~IT_Projection();

	RC Open();
	RC GetNext(RM_Record& outRec);
	RC Close();
private:
	QL_Iterator* it;

	int length;
};

#endif /* IT_PROJECTION_H_ */
