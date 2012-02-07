/*
 * it_filescan.cpp
 *
 *  Created on: Feb 5, 2012
 *      Author: mickael
 */

#include "it_filescan.h"

IT_FileScan::IT_FileScan(RM_Manager rmm, SM_Manager smm, const char * _relName,
						 Condition scanCond)
					: rmm(&rmm), smm(&smm), relName(_relName), scanCond(scanCond) {
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

	// Si on a une condition sur le scan on récupère l'attribut
	rc = smm->GetAttrTpl(relName, scanCond.lhsAttr.attrName, attrCat, rid);
	if(rc) return rc;

	rc = smm->GetAttributesFromRel(relName, attrs, attrCount);
	if(rc) return rc;

	rc = rmm->OpenFile(relName, rmfh);
	if(rc) return rc;

	rc = rmfs.OpenScan(rmfh,
	                    attrCat.attrType,
	                    attrCat.attrLength,
	                    attrCat.offset,
	                    scanCond.op,
	                    scanCond.rhsValue.data,
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

