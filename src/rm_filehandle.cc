#include "rm.h"
#include "pf.h"
#include "rm_internal.h"
#include <cstring>


RM_FileHandle::RM_FileHandle () {
    bFileOpen = false;
    pf_filehandle = new PF_FileHandle();
}

RM_FileHandle::~RM_FileHandle() {
    delete pf_filehandle;
}

// Given a RID, copy the record into rec
RC RM_FileHandle::GetRec (const RID &rid, RM_Record &rec) const {
    RC rc;
    PageNum pageNum;
    SlotNum slotNum;
    PF_PageHandle pfph;
    char *pData;

    // récupère le numéro de page à partir de rid
    rc = rid.GetPageNum(pageNum);
    if (rc) {
        return rc;
    }

    // récupère le numéro de slot à partir de rid
    rc = rid.GetSlotNum(slotNum);
    if (rc) {
        return rc;
    }

    // récupère la page à partir du numéro de page
    rc = pf_filehandle->GetThisPage(pageNum, pfph);
    if (rc) {
        return rc;
    }

    // récupère les données de la page
    rc = pfph.GetData(pData);
    if (rc) {
        pf_filehandle->UnpinPage(pageNum);
        return rc;
    }
    RM_PageHeader pageHeader(pData, rm_fileheader.numberRecords);

    rc = pageHeader.getBitmap()->checkSlot(slotNum);
    if (rc) {
        pf_filehandle->UnpinPage(pageNum);
        return rc;
    }

    // on ajuste le pointeur
    rc = pfph.GetData(pData);
    if (rc) {
        pf_filehandle->UnpinPage(pageNum);
        return rc;
    }
    // header
    pData += 2*sizeof(int);
    pData += pageHeader.getBitmap()->sizeToChar()*sizeof(char);
    // records
    pData += slotNum * rm_fileheader.recordSize;

    // stocke les données dans rec
    rec.rid = rid;
    memcpy(rec.pData, pData, rm_fileheader.recordSize);
    rec.bIsValid = true;

    // Unpin la page
    rc = pf_filehandle->UnpinPage(pageNum);
    if (rc) {
        return rc;
    }

    return 0;
}

RC RM_FileHandle::InsertRec  (const char *pData, RID &rid) { // Insert a new record
    RC rc;
    PageNum pageNum;
    SlotNum slotNum;
    PF_PageHandle pfph;
    // char *pData2;

    // check if record is ok
    if (false) {
        return rc;
    }

    // check if there's still place on the page, otherwise allocate new page
    if (false) {
        // get the first free page
        rc = pf_filehandle->GetFirstPage(pfph);
        if (rc) {
            return rc;
        }
    }
    else {
        // allocate the new page
        rc = pf_filehandle->AllocatePage(pfph);
        if (rc) {
            return rc;
        }
    }

    // get the page
    rc = pf_filehandle->GetThisPage(pageNum, pfph);
    if (rc) {
        return rc;
    }

    // get data
    // rc = pfph.GetData(pData);
    // if (rc) {
    //     pf_filehandle.UnpinPage(pageNum);
    //     return rc;
    // }

    // find the first empty slot


    // set rid
    rid.SetPageNum(pageNum);
    rid.SetSlotNum(slotNum);

    // copy the record into the buffer

    // set header

    // check if page became full of not

    // mark the header page as dirty
    rc = pf_filehandle->MarkDirty(pageNum);
    if (rc) {
        pf_filehandle->UnpinPage(pageNum);
        return rc;
    }

    // unpin the page
    rc = pf_filehandle->UnpinPage(pageNum);
    if (rc) {
        return rc;
    }

    return 0;
}

RC RM_FileHandle::DeleteRec  (const RID &rid) { // Delete a record
    RC rc;
    PageNum pageNum;
    SlotNum slotNum;
    PF_PageHandle pfph;
    char *pData;

    // récupère le numéro de page à partir de rid
    rc = rid.GetPageNum(pageNum);
    if (rc) {
        return rc;
    }

    // récupère le numéro de slot à partir de rid
    rc = rid.GetSlotNum(slotNum);
    if (rc) {
        return rc;
    }

    // récupère la page à partir du numéro de page
    rc = pf_filehandle->GetThisPage(pageNum, pfph);
    if (rc) {
        return rc;
    }

    // récupère les données de la page
    rc = pfph.GetData(pData);
    if (rc) {
        pf_filehandle->UnpinPage(pageNum);
        return rc;
    }
    RM_PageHeader pageHeader(pData, rm_fileheader.numberRecords);

    rc = pageHeader.getBitmap()->checkSlot(slotNum);
    if (rc) {
        pf_filehandle->UnpinPage(pageNum);
        return rc;
    }

    pageHeader.getBitmap()->clearSlot(slotNum);
    int numFreeSlots = CountFreeSlots(pageHeader);
    if(numFreeSlots == 0) {
        pageHeader.nextPage = rm_fileheader.nextFreePage;
        rm_fileheader.nextFreePage = pageNum;
    }

    // Rewrite page header
    rc = WritePageHeader(pfph, pageHeader);
    if(rc) {
        pf_filehandle->UnpinPage(pageNum);
        return rc;
    }

    // mark as dirty
    rc = pf_filehandle->MarkDirty(pageNum);
    if (rc) {
        pf_filehandle->UnpinPage(pageNum);
        return rc;
    }

    // unpine
    rc = pf_filehandle->UnpinPage(pageNum);
    if (rc) {
        return rc;
    }

    return 0;
}

int RM_FileHandle::CountFreeSlots(RM_PageHeader& pageHeader) {
    int freeSlots = 0;
    int value = 0;
    for(int i = 0; i < pageHeader.getBitmap()->getSize(); i++) {
        RC rc = pageHeader.getBitmap()->getSlot(i, value);
        if(rc) continue;
        freeSlots += (value == 1 ? 0 : 1);
    }
    return freeSlots;
}

RC RM_FileHandle::WritePageHeader(PF_PageHandle pf_ph, RM_PageHeader& pageHeader) {
    char* pData;
    RC rc = pf_ph.GetData(pData);
    if(rc) return rc;

    int nextPage = pageHeader.getNextPage();
    int prevPage = pageHeader.getPrevPage();
    memcpy(pData, &prevPage, sizeof(PageNum));
    memcpy(pData+sizeof(PageNum), &nextPage, sizeof(PageNum));
    memcpy(pData+2*sizeof(PageNum), pageHeader.getBitmap()->getChar(), pageHeader.getBitmap()->sizeToChar()*sizeof(char));

    return OK_RC;
}

RC RM_FileHandle::UpdateRec  (const RM_Record &rec) { // Update a record
    RC rc;
    PageNum pageNum;
    SlotNum slotNum;
    PF_PageHandle pfph;
    char *pData;

    RID rid;
    rc = rec.GetRid(rid);
    if(rc) return rc;

    // récupère le numéro de page à partir de rid
    rc = rid.GetPageNum(pageNum);
    if (rc) {
        return rc;
    }

    // récupère le numéro de slot à partir de rid
    rc = rid.GetSlotNum(slotNum);
    if (rc) {
        return rc;
    }

    // récupère la page à partir du numéro de page
    rc = pf_filehandle->GetThisPage(pageNum, pfph);
    if (rc) {
        return rc;
    }

    // récupère les données de la page
    rc = pfph.GetData(pData);
    if (rc) {
        pf_filehandle->UnpinPage(pageNum);
        return rc;
    }
    RM_PageHeader pageHeader(pData, rm_fileheader.numberRecords);

    rc = pageHeader.getBitmap()->checkSlot(slotNum);
    if (rc) {
        pf_filehandle->UnpinPage(pageNum);
        return rc;
    }

    char* recData = NULL;
    rc = rec.GetData(recData);
    if (rc) {
        pf_filehandle->UnpinPage(pageNum);
        return rc;
    }

    // on ajuste le pointeur
    rc = pfph.GetData(pData);
    if (rc) {
        pf_filehandle->UnpinPage(pageNum);
        return rc;
    }
    // header
    pData += 2*sizeof(int);
    pData += pageHeader.getBitmap()->sizeToChar()*sizeof(char);
    // records
    pData += slotNum * rm_fileheader.recordSize;

    memcpy(pData, recData, rm_fileheader.recordSize);

    // mark as dirty
    rc = pf_filehandle->MarkDirty(pageNum);
    if (rc) {
        pf_filehandle->UnpinPage(pageNum);
        return rc;
    }

    // unpin
    rc = pf_filehandle->UnpinPage(pageNum);
    if (rc) {
        return rc;
    }

    return 0;
}

// Forces a page (along with any contents stored in this class)
// from the buffer pool to disk.  Default value forces all pages.
RC RM_FileHandle::ForcePages (PageNum pageNum) const {
    // force page
    RC rc = pf_filehandle->ForcePages(pageNum);
    if (rc) return rc;
    return OK_RC;
}
