#ifndef RM_INTERNAL_H
#define RM_INTERNAL_H

typedef struct rm_fileheader {
    int recordSize;     // Size of a single record
    int numberRecords;  // Maximum number of records in a page
    int nextFreePage;   // Number of the next free page
    int nextFullPage;   // Number of the next full page
    int numberPages;    // Number of pages
} RM_FileHeader;

typedef struct rm_pageheader {
    int prevPage;       // Number of the previous page
    int nextPage;       // Number of the next page
    int numberRecords;  // Number of records in the page
    char* bitmap;       // Pointer to the bitmap
                        // Size of the bitmap = [RM_FileHeader.numberRecords] bits
} RM_PageHeader;

#endif
