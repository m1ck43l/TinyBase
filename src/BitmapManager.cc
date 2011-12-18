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

int BitmapManager::sizeToChar() {
    return this->size/8 + ((this->size % 8) > 0 ? 1 : 0);
}
