/*
 * it_filescan.h
 *
 *  Created on: Feb 5, 2012
 *      Author: mickael
 */

#ifndef IT_FILESCAN_H_
#define IT_FILESCAN_H_

#include "ql.h"
#include "ql_iterator.h"

class IT_FileScan : public virtual QL_Iterator {
public:
	IT_FileScan(RM_Manager* rmm, SM_Manager* smm, const char * relName, const Condition& scanCond, RC& rc);
	virtual ~IT_FileScan();

	RC Open();
	RC GetNext(RM_Record& outRec);
	RC Close();
	void Print(std::ostream &c, int spaces);

private:
	RM_Manager *rmm;
	SM_Manager *smm;

	const char * relName;
	RM_FileHandle rmfh;
	RM_FileScan rmfs;

	const Condition* scanCond;
    RID rid;
    AttrCat attrCat;
};

#endif /* IT_FILESCAN_H_ */
