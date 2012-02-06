/*
 * it_indexscan.h
 *
 *  Created on: Feb 6, 2012
 *      Author: mickael
 */

#ifndef IT_INDEXSCAN_H_
#define IT_INDEXSCAN_H_

#include "ql.h"
#include "ql_iterator.h"

class IT_IndexScan : public virtual QL_Iterator {
public:
	IT_IndexScan(RM_Manager rmm, SM_Manager smm, const char * _relName, const char * _attrName,
			 Condition scanCond, int nbOtherCond, Condition _otherConds[]);
	virtual ~IT_IndexScan();
private:
	const char * relName;
	const char * indexName;

	RM_FileHandle rmfh;

	IX_IndexHandle ixih;
	IX_IndexScan ixis;

	RM_Manager *rmm;
	SM_Manager *smm;
	IX_Manager *ixm;

	Condition& scanCond;
	int nbOtherCond;
	Condition* otherConds;
};

#endif /* IT_INDEXSCAN_H_ */
