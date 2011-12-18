#include "rm.h"
#include <cstring>

RM_PageHeader::RM_PageHeader(char* pData, int size) {
        memcpy(&this->prevPage, pData, sizeof(int));
        memcpy(&this->nextPage, pData + sizeof(int), sizeof(int));
        this->bitmap = new BitmapManager(pData + 2*sizeof(int), size);
}

PageNum RM_PageHeader::getNextPage() {
    return this->nextPage;
}

PageNum RM_PageHeader::getPrevPage() {
    return this->prevPage;
}

BitmapManager* RM_PageHeader::getBitmap() {
    return this->bitmap;
}
