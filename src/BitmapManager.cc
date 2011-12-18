#include "rm.h"
#include <cstring>

BitmapManager::BitmapManager(char* _bitmap, int _size) {
    this->size = _size;
    this->bitmap = new char[this->sizeToChar()];
    memcpy(this->bitmap, _bitmap, this->sizeToChar());
}

BitmapManager::~BitmapManager() {
    delete[] this->bitmap;
}

void BitmapManager::setSlot(SlotNum num) {
    if(num >= this->size) return;

    int slotChar = num / 8;
    int slotBit = num % 8;

    this->bitmap[slotChar] |= (1 << slotBit);
}

void BitmapManager::clearSlot(SlotNum num) {
    if(num >= this->size) return;

    int slotChar = num / 8;
    int slotBit = num % 8;

    this->bitmap[slotChar] &= ~(1 << slotBit);
}

RC BitmapManager::checkSlot(SlotNum num) {
    if(num >= this->size) return RM_RECNOTFOUND;

    int slotChar = num / 8;
    int slotBit = num % 8;

    return (this->bitmap[slotChar] & (1 << slotBit) ? OK_RC : RM_RECNOTFOUND);
}

int BitmapManager::sizeToChar() {
    return this->size/8 + ((this->size % 8) > 0 ? 1 : 0);
}

char* BitmapManager::getChar() {
    return this->bitmap;
}

int BitmapManager::getSize() {
    return size;
}

RC BitmapManager::getSlot(SlotNum num, int& value) {
    if(num >= this->size) return RM_RECNOTFOUND;

    int slotChar = num / 8;
    int slotBit = num % 8;

    value = this->bitmap[slotChar] & (1 << slotBit);
    return OK_RC;
}
