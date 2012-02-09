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
	IT_IndexScan(RM_Manager* rmm, SM_Manager* smm, IX_Manager* ixm, const char * _relName, const char * _attrName,
			 const Condition& scanCond, RC& rc);
	virtual ~IT_IndexScan();

	RC Open();
	RC GetNext(RM_Record& outRec);
	RC Close();
	void Print(std::ostream &output, int spaces);

private:
	RM_Manager *rmm;
	SM_Manager *smm;
	IX_Manager *ixm;

	const char * relName;
	const char * indexName;

	RM_FileHandle rmfh;

	IX_IndexHandle ixih;
	IX_IndexScan ixis;

	const Condition* scanCond;
    RID rid;
    AttrCat attrCat;
};

#endif /* IT_INDEXSCAN_H_ */
