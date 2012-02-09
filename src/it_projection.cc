/*
 * it_projection.cpp
 *
 *  Created on: Feb 7, 2012
 *      Author: mickael
 */

#include "it_projection.h"

IT_Projection::IT_Projection(QL_Iterator* it, int nSelAttrs, const RelAttr selAttrs[]) : it(it) {
	length = 0;

	// Select *
	if(nSelAttrs == 1 && strcmp(selAttrs[0].attrName, "*") == 0) {
		attrCount = it->getAttrCount();
		attrs = new DataAttrInfo[attrCount];
		projAttrs = new DataAttrInfo[attrCount];

		DataAttrInfo* tempattrs = it->getRelAttr();
		for(int i = 0; i < attrCount; i++) {
			projAttrs[i] = tempattrs[i];
			attrs[i] = tempattrs[i];
			length += tempattrs[i].attrLength;
		}

	} else {
		attrCount = nSelAttrs;
		attrs = new DataAttrInfo[attrCount];
		projAttrs = new DataAttrInfo[attrCount];

		DataAttrInfo* tempattrs = it->getRelAttr();
		for(int i = 0; i < attrCount; i++) {
			for(int j = 0; j < it->getAttrCount(); j++) {
				if (((selAttrs[i].relName == NULL) || (selAttrs[i].relName != NULL && strcmp(selAttrs[i].relName, tempattrs[j].relName) == 0))
						&& strcmp(selAttrs[i].attrName, tempattrs[j].attrName) == 0) {
					projAttrs[i] = tempattrs[j];

					// construction de attrs reel
					attrs[i] = tempattrs[j];
					attrs[i].offset = length;

					length += tempattrs[j].attrLength;
				}
			}
		}
	}
}

IT_Projection::~IT_Projection() {
    delete[] attrs;
    delete[] projAttrs;
	delete it;
}

// Affiche le query plan
void IT_Projection::Print(std::ostream &output, int spaces) {
	for(int i = 0; i < spaces; i++) {
		output << " ";
	}

	// Affichage des infos
	output << "Projection \n";
	it->Print(output, spaces + SPACES);
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
        memcpy(pDataNew + offset, pData + projAttrs[i].offset, projAttrs[i].attrLength);
        offset += projAttrs[i].attrLength;
	}

	rc = tpl.Set(pDataNew, length);
	if(rc) return rc;

	delete[] pDataNew;

	return 0;
}

