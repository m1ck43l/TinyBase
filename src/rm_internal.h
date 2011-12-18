#ifndef RM_INTERNAL_H
#define RM_INTERNAL_H

class BitmapManager {
    private:
        char* bitmap;
        int size;

    public:
        BitmapManager(char*,int);
        ~BitmapManager();
        void setSlot(SlotNum);
        void clearSlot(SlotNum);
        RC checkSlot(SlotNum);
        int sizeToChar();
        char* getChar();
        int getSize();
        RC getSlot(SlotNum, int&);
};

typedef struct rm_fileheader {
    int recordSize;     // Size of a single record
    int numberRecords;  // Number of records in a page
    int nextFreePage;   // Number of the next free page
    int nextFullPage;   // Number of the next full page
    int numberPages;    // Number of pages
} RM_FileHeader;

class RM_PageHeader {
    friend class RM_FileHandle;
    private:
        int prevPage;       // Number of the previous page
        int nextPage;       // Number of the next page
        BitmapManager* bitmap;      // bitmap

    public:
        RM_PageHeader(char*, int);
        PageNum getNextPage();
        PageNum getPrevPage();
        BitmapManager* getBitmap();
};

#endif
