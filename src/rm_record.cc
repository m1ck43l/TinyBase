#include "rm.h"

RM_Record::RM_Record() {
    this->pData = NULL;
    this->bIsValid = false;
}

RM_Record::~RM_Record() {
    // If the pointer has been allocated we free it
    if(bIsValid) {
        delete(pData);
    }
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
