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
	IT_FileScan(RM_Manager rmm, SM_Manager smm, const char * relName, Condition scanCond, int nbOtherCond, Condition _otherConds[]);
	virtual ~IT_FileScan();

private:
	const char * relName;
	RM_FileHandle rmfh;
	RM_FileScan rmfs;

	RM_Manager *rmm;
	SM_Manager *smm;

	Condition& scanCond;
	int nbOtherCond;
	Condition* otherConds;
};

#endif /* IT_FILESCAN_H_ */
