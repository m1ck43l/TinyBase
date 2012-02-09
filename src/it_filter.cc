/*
 * it_filter.cpp
 *
 *  Created on: Feb 7, 2012
 *      Author: mickael
 */

#include "it_filter.h"

IT_Filter::IT_Filter(QL_Iterator* it, const Condition& _cond) : it(it), cond(&_cond) {
	attrs = it->getRelAttr();
	attrCount = it->getAttrCount();
	for(int i = 0; i < it->getAttrCount(); i++) {
		// Si le relName de la condition est NULL on ne compare que les attrName
		// On suppose que les verifs ont été faites avant
		if ( ((cond->lhsAttr.relName == NULL) || (cond->lhsAttr.relName != NULL && strcmp(cond->lhsAttr.relName, attrs[i].relName) == 0))
				&& strcmp(cond->lhsAttr.attrName, attrs[i].attrName) == 0) {
			leftAttr = &attrs[i];
		}
	}
}

IT_Filter::~IT_Filter() {
	delete it;
}

// Affiche le query plan
void IT_Filter::Print(std::ostream &output, int spaces) {
	for(int i = 0; i < spaces; i++) {
		output << " ";
	}

	// Affichage des infos
	output << "Filter : " ;
	PrintACond(output, cond);
	output << "\n";
	it->Print(output, spaces + SPACES);
}

RC IT_Filter::Open() {
	if(bIsOpen) {
		return QL_ITALRDYOPEN;
	}
	RID rid;
	RC rc;

	rc = it->Open();
	if(rc) return rc;

	bIsOpen = true;

	return 0;
}

RC IT_Filter::Close() {
	if(!bIsOpen) {
		return QL_ITNOTOPEN;
	}

	RC rc = it->Close();
	if(rc) return rc;

	bIsOpen = false;

	return 0;
}

RC IT_Filter::GetNext(RM_Record& tpl) {
	if(!bIsOpen) {
		return QL_ITNOTOPEN;
	}

	RC rc;
	while ((rc = it->GetNext(tpl)) != QL_EOF) {
		if (rc) return rc;

		char* pData;
		rc = tpl.GetData(pData);
		if(rc) return rc;

		bool result = false;
		Comparator comp(leftAttr->attrType, leftAttr->attrLength, leftAttr->offset, cond->op, pData);

		if (cond->bRhsIsAttr) {
			DataAttrInfo* attrs = it->getRelAttr();
			DataAttrInfo rightAttr;
			for(int i = 0; i < it->getAttrCount(); i++) {
				if (strcmp(cond->rhsAttr.relName, attrs[i].relName) == 0 && strcmp(cond->rhsAttr.attrName, attrs[i].attrName) == 0) {
					rightAttr = attrs[i];
				}
			}

			result = comp.Compare(pData + rightAttr.offset);
		}
	    else {
	    	result = comp.Compare(cond->rhsValue.data);
	    }

		if(result) {
			return 0;
		}
	}

	return QL_EOF;
}
