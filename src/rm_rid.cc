#include "rm_rid.h"
#include "rm.h"

RID::RID() {
    this->bIsValid = false;
}

RID::RID(PageNum pageNum, SlotNum slotNum) {
    this->pageNum = pageNum;
    this->slotNum = slotNum;
    this->bIsValid = true;
}

RC RID::GetPageNum(PageNum &pageNum) const {
    if(bIsValid) {
        pageNum = this->pageNum;
        return OK_RC;
    }

    return RM_INVALIDRID;
}

RC RID::GetSlotNum(SlotNum &slotNum) const {
    if(bIsValid) {
        slotNum = this->slotNum;
        return OK_RC;
    }

    return RM_INVALIDRID;
}

void RID::SetPageNum(PageNum &pageNum) {
    this->pageNum = pageNum;
}

void RID::SetSlotNum(SlotNum &slotNum) {
    this->slotNum = slotNum;
}

RID::~RID() {}
