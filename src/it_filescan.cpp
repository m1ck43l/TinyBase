/*
 * it_filescan.cpp
 *
 *  Created on: Feb 5, 2012
 *      Author: mickael
 */

#include "it_filescan.h"

IT_FileScan::IT_FileScan(RM_Manager rmm, SM_Manager smm, const char * _relName,
						 Condition scanCond, int nbOtherCond, Condition _otherConds[])
					: rmm(&rmm), smm(&smm), relName(_relName), scanCond(scanCond), nbOtherCond(nbOtherCond) {

	// Plusieurs cas sont possibles lorsque l'on va ouvrir l'iterateur
	// Soit on possède une condition sur l'attribut que l'on scan -> scanCond
	// Soit non -> otherConds

	otherConds = new Condition[nbOtherCond];

	int i = 0;
	for(i = 0; i < nbOtherCond; i++) {
		otherConds[i] = _otherConds[i];
	}
}

IT_FileScan::~IT_FileScan() {
	delete[] otherConds;
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
	if(scanCond.rhsValue != NULL) {
		rc = smm->GetAttrTpl(relName, scanCond.lhsAttr.attrName, attrCat, rid);
		if(rc) return rc;
	}

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

RC IT_FileScan::GetNext(RM_Record& outRec) {
	RC rc;

	if(!bIsOpen) {
		return QL_ITNOTOPEN;
	}

	RM_Record rec;
	RID rid;
	bool trouve = false;

	while (!trouve) {
		rc = rmfs.GetNextRec(rec);
		if (rc != RM_EOF && rc != 0)
			return rc;

		if (rc == RM_EOF)
			break;

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
			outRec.rid = rid;
			outRec.bIsValid = true;
			outRec.Set(pData, rec.GetLength());
			trouve = true;
		}
	}

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

