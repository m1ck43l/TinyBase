/*
 * it_nestedloopjoin.cpp
 *
 *  Created on: Feb 7, 2012
 *      Author: mickael
 */

#include "it_nestedloopjoin.h"

IT_NestedLoopJoin::~IT_NestedLoopJoin() {
	// TODO Auto-generated destructor stub
}

RC IT_NestedLoopJoin::Open() {
	attrCount = LeftIterator->getAttrCount() + RightIterator->getAttrCount();
	attrs = new DataAttrInfo[attrCount];

	// On a besoin de remettre les attributs correctement
	// i.e. mettre à jour l'offset pour les attributs de l'iterateur droit
	int i = 0;

	DataAttrInfo* left = LeftIterator->getRelAttr();
	for (i = 0; i < LeftIterator->getAttrCount(); i++) {
		attrs[i] = left[i];
	}

	DataAttrInfo* right = RightIterator->getRelAttr();
	for (i = 0; i < RightIterator->getAttrCount(); i++) {
		attrs[LeftIterator->getAttrCount() + i] = right[i];
		attrs[LeftIterator->getAttrCount() + i].offset += LeftIterator->getLength();
	}

	// On ouvre les deux itérateurs
	RC rc;
	rc = LeftIterator->Open();
	if(rc) return rc;

	rc = RightIterator->Open();
	if(rc) return rc;

	rc = LeftIterator->GetNext(leftTpl);
	if(rc) return rc;

	bIsOpen = true;

	return 0;
}

RC IT_NestedLoopJoin::Close() {
	if(!bIsOpen) {
		return QL_ITNOTOPEN;
	}

	RC rc = LeftIterator->Close();
	if(rc) return rc;

	rc = RightIterator->Close();
	if(rc) return rc;

	bIsOpen = false;
	return 0;
}

RC IT_NestedLoopJoin::GetNext(Tuple& outRec) {
	RC rc;

	if(!bIsOpen) {
		return QL_ITNOTOPEN;
	}

	return rc;
}

