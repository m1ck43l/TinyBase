/*
 * it_projection.cpp
 *
 *  Created on: Feb 7, 2012
 *      Author: mickael
 */

#include "it_projection.h"

IT_Projection::IT_Projection(QL_Iterator* it, int nSelAttrs, const RelAttr selAttrs[]) : it(it) {
    attrCount = nSelAttrs;
	length = 0;

    attrs = new DataAttrInfo[attrCount];
    for(int i = 0; i < attrCount; i++) {
        DataAttrInfo* tempattrs = it->getRelAttr();
		for(int j = 0; j < it->getAttrCount(); j++) {
            if (strcmp(selAttrs[i].relName, tempattrs[j].relName) == 0 && strcmp(selAttrs[i].attrName, tempattrs[j].attrName) == 0) {
                attrs[i] = tempattrs[j];
                length += tempattrs[j].attrLength;
			}
		}
	}
}

IT_Projection::~IT_Projection() {
    delete[] attrs;
	delete it;
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
    for(int i = 0; i < attrCount; i++) {
        memcpy(pDataNew + offset, pData + attrs[i].offset, attrs[i].attrLength);
        offset += attrs[i].attrLength;
	}

	rc = tpl.Set(pDataNew, length);
	if(rc) return rc;

	delete[] pDataNew;

	return 0;
}

