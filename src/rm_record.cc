#include "rm.h"
#include <cstdlib>
#include <cstring>

RM_Record::RM_Record() {
    this->pData = NULL;
    this->bIsValid = false;
}

RM_Record::~RM_Record() {
    // If the pointer has been allocated we free it
    if(pData != NULL) {
        delete[] (pData);
    }
}

RC RM_Record::Set(char* pData2, int recordSize) {
    if (pData == NULL)
        pData = new char[recordSize];
    memcpy(pData, pData2, recordSize);
    return 0;
}

RC RM_Record::GetData(char *&pData) const {
    if(bIsValid) {
        pData = this->pData;
        return OK_RC;
    }

    return RM_INVALIDRECORD;
}

RC RM_Record::GetRid(RID &rid) const {
    if(bIsValid) {
        rid = this->rid;
        return OK_RC;
    }

    return RM_INVALIDRECORD;
}
