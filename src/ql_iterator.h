/*
 * ql_terator.h
 *
 *  Created on: Feb 5, 2012
 *      Author: mickael
 */

#ifndef QL_TERATOR_H_
#define QL_TERATOR_H_

#include "ql.h"

// PLus ou moins la même chose que RM_Record mais avec plus d'infos pour permettre
// une meilleure gestion par les itérateurs
// On aura une structure permettant de récupérer et mettre à jour un attribut directement
class Tuple {
public:
	Tuple(int _length) : length(_length), rid(-1, -1) {
		pData = new char[length];
	}

	Tuple(const Tuple& tpl) : length(tpl.length), rid(tpl.rid) {
		pData = new char[length];
		memcpy(pData, tpl.pData, length);
		this->SetAttributes(tpl.GetAttributes());
	}

	~Tuple() {
		delete[] pData;
	}

	void SetAttributes(DataAttrInfo* _pAttrs) {
		pAttrs = _pAttrs;
	}

	DataAttrInfo* GetAttributes() {
		return pAttrs;
	}

	int GetLength() {
		return length;
	}

	RC GetRID(RID& rid) {
		rid = this->rid;
		return 0;
	}

	RC SetRID(RID& rid) {
	    this->rid = rid;
	    return 0;
	}

	RC GetData(char *&pData) const {
		pData = this->pData;
		return OK_RC;
	}

	RC Set(char* pData2) {
	    memcpy(pData, pData2, length);
	    return 0;
	}

private:
	char *pData;
	RID rid;
	int length;

	DataAttrInfo * pAttrs;
};

class QL_Iterator {
public:
	QL_Iterator():bIsOpen(false) {};
	virtual ~QL_Iterator();

	virtual RC Open() = 0;
	virtual RC GetNext(Tuple& outRec) = 0;
	virtual RC Close() = 0;

	virtual DataAttrInfo* getRelAttr() { return attrs; }
	virtual int getAttrCount() { return attrCount; }

protected:
	bool bIsOpen;
	DataAttrInfo* attrs;
	int attrCount;
};

#endif /* QL_TERATOR_H_ */
