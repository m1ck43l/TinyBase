/*
 * it_projection.cpp
 *
 *  Created on: Feb 7, 2012
 *      Author: mickael
 */

#include "it_projection.h"

IT_Projection::IT_Projection(QL_Iterator* it, int nSelAttrs, const RelAttr selAttrs[]) : it(it) {
	nProjAttrs = nSelAttrs;
	length = 0;

	projAttrs = new DataAttrInfo[nProjAttrs];
	for(int i = 0; i < nProjAttrs; i++) {
		DataAttrInfo* attrs = it->getRelAttr();
		for(int j = 0; j < it->getAttrCount(); j++) {
			if (strcmp(selAttrs[i].relName, attrs[j].relName) == 0 && strcmp(selAttrs[i].attrName, attrs[j].attrName) == 0) {
				projAttrs[i] = attrs[j];
				length += attrs[j].attrLength;
			}
		}
	}
}

IT_Projection::~IT_Projection() {
	delete[] projAttrs;
}

RC IT_Projection::Open() {
	RC rc;

	if(bIsOpen) {
		return QL_ITALRDYOPEN;
	}

	rc = it->Open();
	if(rc) return rc;

	bIsOpen = true;

	return 0;
}

RC IT_Projection::Close() {
	RC rc;
	if (!bIsOpen) {
		return QL_ITNOTOPEN;
	}

	rc = it->Close();
	if(rc) return rc;

	bIsOpen = false;

	return 0;
}

RC IT_Projection::GetNext(RM_Record& tpl) {
	RC rc;
	if (!bIsOpen) {
		return QL_ITNOTOPEN;
	}

	rc = it->GetNext(tpl);
	if(rc) return rc;

	char* pData;
	rc = tpl.GetData(pData);
	if(rc) return rc;

	int offset = 0;
	char* pDataNew = new char[length];
	for(int i = 0; i < nProjAttrs; i++) {
		memcpy(pDataNew + offset, pData + projAttrs[i].offset, projAttrs[i].attrLength);
		offset += projAttrs[i].attrLength;
	}

	rc = tpl.Set(pDataNew, length);
	if(rc) return rc;

	delete[] pDataNew;

	return 0;
}

