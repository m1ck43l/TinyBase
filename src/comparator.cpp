/*
 * comparator.cpp
 *
 *  Created on: Feb 5, 2012
 *      Author: mickael
 */

#include "comparator.h"

Comparator::Comparator(AttrType _attrType, int _attrLength, int _attrOffset, CompOp _op, void* _pData) {
	attrType = _attrType;
	attrLength = _attrLength;
	attrOffset = _attrOffset;
	op = _op;
	value = _pData;
}

Comparator::~Comparator() {}

bool Comparator::Compare(const char* pData) const {
	if(op == NO_OP || value == NULL)
		return true;

	const char * valueToTest = pData + attrOffset;
	switch (attrType) {
	case INT:
		switch(op) {
		case LT_OP:
			return *((int *)valueToTest) < *((int *)value);
		case LE_OP:
			return *((int *)valueToTest) <= *((int *)value);
		case EQ_OP:
			return *((int *)valueToTest) == *((int *)value);
		case NE_OP:
			return *((int *)valueToTest) != *((int *)value);
		case GT_OP:
			return *((int *)valueToTest) > *((int *)value);
		case GE_OP:
			return *((int *)valueToTest) >= *((int *)value);
		}
		break;
	case FLOAT:
		switch(op) {
		case LT_OP:
			return *((float *)valueToTest) < *((float *)value);
		case LE_OP:
			return *((float *)valueToTest) <= *((float *)value);
		case EQ_OP:
			return *((float *)valueToTest) == *((float *)value);
		case NE_OP:
			return *((float *)valueToTest) != *((float *)value);
		case GT_OP:
			return *((float *)valueToTest) > *((float *)value);
		case GE_OP:
			return *((float *)valueToTest) >= *((float *)value);
		}
		break;
	case STRING:
		switch(op) {
		case LT_OP:
			return (strcmp(((char *)valueToTest), ((char *)value)) < 0);
		case LE_OP:
			return (strcmp(((char *)valueToTest), ((char *)value)) <= 0);
		case EQ_OP:
			return (strcmp(((char *)valueToTest), ((char *)value)) == 0);
		case NE_OP:
			return (strcmp(((char *)valueToTest), ((char *)value)) != 0);
		case GT_OP:
			return (strcmp(((char *)valueToTest), ((char *)value)) > 0);
		case GE_OP:
			return (strcmp(((char *)valueToTest), ((char *)value)) >= 0);
		}
		break;
	}
}

