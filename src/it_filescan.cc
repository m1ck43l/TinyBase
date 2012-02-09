/*
 * it_filescan.cpp
 *
 *  Created on: Feb 5, 2012
 *      Author: mickael
 */

#include "it_filescan.h"

IT_FileScan::IT_FileScan(RM_Manager* _rmm, SM_Manager* _smm, const char * _relName,
						 const Condition& _scanCond, RC& rc)
					: rmm(_rmm), smm(_smm), relName(_relName), scanCond(&_scanCond) {

	rc = smm->GetAttributesFromRel(relName, attrs, attrCount);
}

IT_FileScan::~IT_FileScan() {
	delete[] attrs;
}

RC IT_FileScan::Open() {
	if(bIsOpen) {
		return RM_SCANOPEN;
	}

	RC rc;
	RID rid;
	AttrCat attrCat;

	std::cout << "Before --> " << (scanCond->op != NO_OP) << std::endl;

	// Si on a une condition sur le scan on récupère l'attribut
	if (scanCond->op != NO_OP) {
		rc = smm->GetAttrTpl(relName, scanCond->lhsAttr.attrName, attrCat, rid);
		if(rc) return rc;
	}

	std::cout << "After --> " << (scanCond->op != NO_OP) << std::endl;

	rc = rmm->OpenFile(relName, rmfh);
	if(rc) return rc;

	rc = rmfs.OpenScan(rmfh,
	                    attrCat.attrType,
	                    attrCat.attrLength,
	                    attrCat.offset,
	                    scanCond->op,
	                    scanCond->rhsValue.data,
	                    NO_HINT);

	bIsOpen = true;

	return 0;
}

RC IT_FileScan::GetNext(RM_Record& rec) {
	RC rc;

	if(!bIsOpen) {
		return QL_ITNOTOPEN;
	}

	RID rid;

	rc = rmfs.GetNextRec(rec);
	if (rc != RM_EOF && rc != 0)
		return rc;

	if (rc == RM_EOF)
		return QL_EOF;

	return rc;
}

RC IT_FileScan::Close() {
	if(!bIsOpen) {
		return QL_ITNOTOPEN;
	}

	RC rc = rmfs.CloseScan();
	if(rc) return rc;

	rc = rmm->CloseFile(rmfh);
	if(rc) return rc;

	bIsOpen = false;
	return 0;
}

