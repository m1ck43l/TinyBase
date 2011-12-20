#include "ix.h"

IX_IndexHandle::IX_IndexHandle() {
    bFileOpen = false;
    pf_filehandle = NULL;
}

IX_IndexHandle::~IX_IndexHandle() {
    if (pf_filehandle != NULL)
        delete pf_filehandle;
}

 // Insert a new index entry
 RC IX_IndexHandle::InsertEntry(void *pData, const RID &rid) {

     return 0;
 }

// Delete a new index entry
RC IX_IndexHandle::DeleteEntry(void *pData, const RID &rid) {

    return 0;
}

// Force index files to disk
RC IX_IndexHandle::ForcePages() {

    return 0;
}
