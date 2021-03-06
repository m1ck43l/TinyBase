/*
 * it_indexscan.cpp
 *
 *  Created on: Feb 6, 2012
 *      Author: mickael
 */

#include "it_indexscan.h"

IT_IndexScan::IT_IndexScan(RM_Manager* rmm, SM_Manager* _smm, IX_Manager* ixm, const char * _relName, const char * _indexName,
		 const Condition& _scanCond, RC& rc)
	: rmm(rmm), smm(_smm), ixm(ixm), relName(_relName), indexName(_indexName), scanCond(&_scanCond) {
	rc = smm->GetAttributesFromRel(relName, attrs, attrCount);
    rc = smm->GetAttrTpl(relName, indexName, attrCat, rid);
}

IT_IndexScan::~IT_IndexScan() {
	delete[] attrs;
}

// Affiche le query plan
void IT_IndexScan::Print(std::ostream &output, int spaces) {
	for(int i = 0; i < spaces; i++) {
		output << " ";
	}

	// Affichage des infos
	output << "IndexScan : [" << relName << "], [" << indexName << "] : " ;
	PrintACond(output, scanCond);
	output << "\n";
}

RC IT_IndexScan::Open() {
	if(bIsOpen) {
		return IX_FILEOPEN;
	}

	RC rc;

	rc = rmm->OpenFile(relName, rmfh);
	if(rc) return rc;

	rc = ixm->OpenIndex(relName, attrCat.indexNo, ixih);
	if(rc) return rc;

	rc = ixis.OpenScan(ixih,
	                    scanCond->op,
	                    scanCond->rhsValue.data,
	                    NO_HINT);

	bIsOpen = true;

	return 0;
}

RC IT_IndexScan::Close() {
	if(!bIsOpen) {
		return QL_ITNOTOPEN;
	}

	RC rc = ixis.CloseScan();
	if(rc) return rc;

	rc = ixm->CloseIndex(ixih);
	if(rc) return rc;

	rc = rmm->CloseFile(rmfh);
	if(rc) return rc;

	bIsOpen = false;
	return 0;
}

RC IT_IndexScan::GetNext(RM_Record& rec) {
	RC rc;

	if(!bIsOpen) {
		return QL_ITNOTOPEN;
	}

	RID rid;

	rc = ixis.GetNextEntry(rid);
	if (rc != IX_EOF && rc != 0)
		return rc;

	if (rc == IX_EOF)
		return QL_EOF;

	rc = rmfh.GetRec(rid, rec);
	if (rc != 0 ) return rc;

	return rc;
}
