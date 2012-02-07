/*
 * comparator.cpp
 *
 *  Created on: Feb 5, 2012
 *      Author: mickael
 */

#include "comparator.h"

Comparator::Comparator(AttrType _attrType, int _attrLength, int _attrOffset, CompOp _op, char* _pData) {
	attrType = _attrType;
	attrLength = _attrLength;
	attrOffset = _attrOffset;
	op = _op;
	value = (_pData == NULL ? NULL : _pData + attrOffset);
}

Comparator::~Comparator() {}

bool Comparator::Compare(const void* pData) const {
	if(op == NO_OP)
		return true;

	switch (attrType) {
	case INT:
		switch(op) {
		case LT_OP:
			return *((int *)value) < *((int *)value);
		case LE_OP:
			return *((int *)value) <= *((int *)value);
		case EQ_OP:
			return *((int *)value) == *((int *)value);
		case NE_OP:
			return *((int *)value) != *((int *)value);
		case GT_OP:
			return *((int *)value) > *((int *)value);
		case GE_OP:
			return *((int *)value) >= *((int *)value);
		default:
			return true;
		}
		break;
	case FLOAT:
		switch(op) {
		case LT_OP:
			return *((float *)value) < *((float *)value);
		case LE_OP:
			return *((float *)value) <= *((float *)value);
		case EQ_OP:
			return *((float *)value) == *((float *)value);
		case NE_OP:
			return *((float *)value) != *((float *)value);
		case GT_OP:
			return *((float *)value) > *((float *)value);
		case GE_OP:
			return *((float *)value) >= *((float *)value);
		default:
			return true;
		}
		break;
	case STRING:
		switch(op) {
		case LT_OP:
			return (strcmp(((char *)value), ((char *)value)) < 0);
		case LE_OP:
			return (strcmp(((char *)value), ((char *)value)) <= 0);
		case EQ_OP:
			return (strcmp(((char *)value), ((char *)value)) == 0);
		case NE_OP:
			return (strcmp(((char *)value), ((char *)value)) != 0);
		case GT_OP:
			return (strcmp(((char *)value), ((char *)value)) > 0);
		case GE_OP:
			return (strcmp(((char *)value), ((char *)value)) >= 0);
		default:
			return true;
		}
		break;
	}

	return false;
}

