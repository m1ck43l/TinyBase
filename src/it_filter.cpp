/*
 * it_filter.cpp
 *
 *  Created on: Feb 7, 2012
 *      Author: mickael
 */

#include "it_filter.h"

IT_Filter::IT_Filter(QL_Iterator* it, Condition& cond) : it(it), cond(&cond) {
}

IT_Filter::~IT_Filter() {
}

RC IT_Filter::Open() {
	if(bIsOpen) {
		return QL_ITALRDYOPEN;
	}
	RID rid;
	RC rc;

	DataAttrInfo* attrs = it->getRelAttr();
	for(int i = 0; i < it->getAttrCount(); i++) {
		if (strcmp(cond->lhsAttr.relName, attrs[i].relName) == 0 && strcmp(cond->lhsAttr.attrName, attrs[i].attrName) == 0) {
			leftAttr = attrs[i];
		}
	}

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

RC IT_Filter::GetNext(Tuple& tpl) {
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
		Comparator comp(leftAttr.attrType, leftAttr.attrLength, leftAttr.offset, cond->op, pData);

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
