/*
 * it_indexscan.cpp
 *
 *  Created on: Feb 6, 2012
 *      Author: mickael
 */

#include "it_indexscan.h"

IT_IndexScan::IT_IndexScan(RM_Manager rmm, SM_Manager smm, const char * _relName, const char * _indexName,
		 Condition scanCond, int nbOtherCond, Condition _otherConds[])
	: rmm(&rmm), smm(&smm), ixm(&ixm), relName(_relName), indexName(_indexName), scanCond(scanCond), nbOtherCond(nbOtherCond) {

	// Plusieurs cas sont possibles lorsque l'on va ouvrir l'iterateur
	// Soit on possède une condition sur l'attribut que l'on scan -> scanCond
	// Soit non -> otherConds

	otherConds = new Condition[nbOtherCond];

	int i = 0;
	for(i = 0; i < nbOtherCond; i++) {
		otherConds[i] = _otherConds[i];
	}
}

IT_IndexScan::~IT_IndexScan() {
	delete[] otherConds;
	delete[] attrs;
}

RC IT_IndexScan::Open() {
	if(bIsOpen) {
		return IX_FILEOPEN;
	}

	RC rc;
	RID rid;
	AttrCat attrCat;

	rc = smm->GetAttrTpl(relName, indexName, attrCat, rid);
	if(rc) return rc;

	rc = smm->GetAttributesFromRel(relName, attrs, attrCount);
	if(rc) return rc;

	rc = rmm->OpenFile(relName, rmfh);
	if(rc) return rc;

	rc = ixm->OpenIndex(relName, attrCat.indexNo, ixih);
	if(rc) return rc;

	rc = ixis.OpenScan(ixih,
	                    scanCond.op,
	                    scanCond.rhsValue.data,
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

RC IT_IndexScan::GetNext(Tuple& outRec) {
	RC rc;

	if(!bIsOpen) {
		return QL_ITNOTOPEN;
	}

	RM_Record rec;
	RID rid;
	bool trouve = false;

	while (!trouve) {
		rc = ixis.GetNextEntry(rid);
		if (rc != IX_EOF && rc != 0)
			return rc;

		if (rc == IX_EOF)
			break;

		rc = rmfh.GetRec(rid, rec);
		if (rc != 0 ) return rc;

		// On recupere le record
		char * pData;
		rec.GetData(pData);
		rec.GetRid(rid);

		// On doit maintenant vérifier si ce record satifait toutes les conditions
		bool isRecordValid = true;
		for (int i = 0; i < nbOtherCond; i++) {
			Condition currentCond = otherConds[i];
			AttrCat attrCat;
			RID tmpRid;

			rc = smm->GetAttrTpl(relName, currentCond.lhsAttr.attrName, attrCat, tmpRid);
			if (rc) return rc;

			// On crée le comparateur
			Comparator comp(attrCat.attrType, attrCat.attrLength, attrCat.offset, currentCond.op, currentCond.rhsValue.data);

			if(!comp.Compare(pData)) {
				isRecordValid = false;
				break;
			}
		}

		if(isRecordValid) {
			Tuple tpl(rec.GetLength());
			tpl.Set(pData);
			tpl.SetRID(rid);
			tpl.SetAttributes(attrs);

			outRec(tpl);
			trouve = true;
		}
	}

	return rc;
}
