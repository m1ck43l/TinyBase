/*
 * it_nestedloopjoin.cpp
 *
 *  Created on: Feb 7, 2012
 *      Author: mickael
 */

#include "it_nestedloopjoin.h"

IT_NestedLoopJoin::IT_NestedLoopJoin(QL_Iterator* _left, QL_Iterator* _right, const Condition& _cond)
											: LeftIterator(_left), RightIterator(_right), cond(&_cond) {
	// Nous avons besoin de savoir dans quel itérateur est le membre gauche/droit
	// de notre condition...
	bool trouve = false;
	DataAttrInfo* attrsL = LeftIterator->getRelAttr();
	for(int j = 0; j < LeftIterator->getAttrCount(); j++) {
		if (strcmp(cond->lhsAttr.relName, attrsL[j].relName) == 0 && strcmp(cond->lhsAttr.attrName, attrsL[j].attrName) == 0) {
			leftAttr = attrsL[j];
			trouve = true;
		}

		if (strcmp(cond->rhsAttr.relName, attrsL[j].relName) == 0 && strcmp(cond->rhsAttr.attrName, attrsL[j].attrName) == 0) {
			rightAttr = attrsL[j];
		}
	}

	DataAttrInfo* attrsR = RightIterator->getRelAttr();
	for(int j = 0; j < RightIterator->getAttrCount(); j++) {
		if(!trouve) {
			// L'attribut gauche de la condition est dans l'arbre droit...
			if (strcmp(cond->lhsAttr.relName, attrsR[j].relName) == 0 && strcmp(cond->lhsAttr.attrName, attrsR[j].attrName) == 0) {
				leftAttr = attrsR[j];
			}
		} else {
			// L'attribut droit de la condition est dans l'arbre droit...
			if (strcmp(cond->rhsAttr.relName, attrsR[j].relName) == 0 && strcmp(cond->rhsAttr.attrName, attrsR[j].attrName) == 0) {
				rightAttr = attrsR[j];
			}
		}
	}

	// A ce niveau: leftAttr -> attribut de l'arbre de gauche, rightAttr -> attribut de l'arbre de droite
	// Nous devons maintenant changer ou non l'opérateur
	op = cond->op;
	if(!trouve) {
		switch(cond->op) {
		case NO_OP:
		case EQ_OP:
		case NE_OP:
			break;
		case LT_OP:
			op = GT_OP;
			break;
		case GT_OP:
			op = LT_OP;
			break;
		case LE_OP:
			op = GE_OP;
			break;
		case GE_OP:
			op = LE_OP;
			break;
		}
	}

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

	// Taille d'un tuple
	lengthL = LeftIterator->getLength();
	lengthR = RightIterator->getLength();
}

IT_NestedLoopJoin::~IT_NestedLoopJoin() {
	delete[] attrs;
	delete LeftIterator;
	delete RightIterator;
}

// Affiche le query plan
void IT_NestedLoopJoin::Print(std::ostream &output, int spaces) {
	for(int i = 0; i < spaces; i++) {
		output << " ";
	}

	// Affichage des infos
	output << "NestedLoopJoin : " ;
	PrintACond(output, cond);
	output << "\n";
	LeftIterator->Print(output, spaces + SPACES);
	RightIterator->Print(output, spaces + SPACES);
}

RC IT_NestedLoopJoin::Open() {
	// On ouvre les deux itérateurs
	RC rc;
	rc = LeftIterator->Open();
	if(rc) return rc;

	// Initialisation du nested loop join
	pLeft = NULL;

	bIsOpen = true;

	return 0;
}

RC IT_NestedLoopJoin::Close() {
	if(!bIsOpen) {
		return QL_ITNOTOPEN;
	}

	RC rc = LeftIterator->Close();
	if(rc) return rc;

	bIsOpen = false;
	return 0;
}

RC IT_NestedLoopJoin::GetNext(RM_Record& rec) {
	RC rc;

	if(!bIsOpen) {
		return QL_ITNOTOPEN;
	}

	while(true) {
		if (pLeft == NULL) {
			// Première phase: ouverture de right et getNext sur left
			rc = RightIterator->Open();
			if(rc) return rc;

			rc = LeftIterator->GetNext(pLeftRec);
			if(rc) return rc;

			rc = pLeftRec.GetData(pLeft);
			if(rc) return rc;

			// Phase 1 ok
		}

		while ((rc = RightIterator->GetNext(rec)) != QL_EOF) {
			if(rc) return rc;

			char* pRight;
			rc = rec.GetData(pRight);
			if(rc) return rc;

			Comparator comp(leftAttr.attrType, leftAttr.attrLength, leftAttr.offset, op, pLeft);
			if(comp.Compare(pRight + rightAttr.offset)) {
				// Condition de jointure validée
				// On retourne le nouveau tuple
				// Pour cela on doit le construire avec pLeft a gauche et pRight a droite
				char* pDataNew = new char[lengthL + lengthR];
				memcpy(pDataNew, pLeft, lengthL);
				memcpy(pDataNew + lengthL, pRight, lengthR);

				rc = rec.Set(pDataNew, lengthL + lengthR);
				if(rc) return rc;

				return 0;
			}
		}

		rc = RightIterator->Close();
		if(rc) return rc;

		pLeft = NULL;
	}

	return rc;
}

