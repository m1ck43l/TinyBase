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

    // stocke les données dans rec
    rec.GetData(pData);

    // Unpine la page
    rc = pf_filehandle->UnpinPage(pageNum);
    if (rc) {
        return rc;
    }

    // libère la mémoire

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

    // find pageNumber from rid
    rc = rid.GetPageNum(pageNum);
    if (rc) {
        return rc;
    }

    // find Slot Number from rid
    rc = rid.GetSlotNum(slotNum);
    if (rc) {
        return rc;
    }

    // check if slot number is ok

    // get the page from rid
    rc = pf_filehandle->GetThisPage(pageNum, pfph);
    if (rc) {
        return rc;
    }

    // get data
    rc = pfph.GetData(pData);
    if (rc) {
        pf_filehandle->UnpinPage(pageNum);
        return rc;
    }

    // test on rid

    // clear Bitmap

    // find a not empty slot


    if (false) { // dispose page if empty
        pf_filehandle->UnpinPage(pageNum);

        rc = pf_filehandle->DisposePage(pageNum);
        if (rc) {
            return rc;
        }
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

RC RM_FileHandle::UpdateRec  (const RM_Record &rec) { // Update a record
    RC rc;
    RID rid;
    PF_PageHandle pfph;
    char *recData;
    char *pData;
    PageNum pageNum;
    SlotNum slotNum;

    // get rid from record
    rc = rec.GetRid(rid);
    if (rc) return rc;

    // get data from record
    rc = rec.GetData(recData);
    if (rc) {
        return rc;
    }

    // get pagenumber from rid
    rc = rid.GetPageNum(pageNum);
    if (rc) {
        return rc;
    }

    // get slot number
    rc = rid.GetSlotNum(slotNum);
    if (rc) {
        return rc;
    }

    // check record is ok


    // get page
    rc = pf_filehandle->GetThisPage(pageNum, pfph);
    if (rc) {
        return rc;
    }

    // get data
    rc = pfph.GetData(pData);
    if (rc) return rc;

    // check if rid exists

    // update !!

    // mark page as dirty
    rc = pf_filehandle->UnpinPage(pageNum);
    if (rc) {
        return rc;
    }

    // unpin the page

    return 0;
}

// Forces a page (along with any contents stored in this class)
// from the buffer pool to disk.  Default value forces all pages.
RC RM_FileHandle::ForcePages (PageNum pageNum) const {
    RC rc;
    PF_PageHandle pfph;
    char *pData;

    // get header page
    rc = pf_filehandle->GetFirstPage(pfph);
    if (rc) return rc;

    // get data
    rc = pfph.GetData(pData);
    if (rc) return rc;

    // set file header
    if (false) return rc;

    // mark header page as dirty
    if (false) return rc;

    // unpin header page



    // force page
    rc = pf_filehandle->ForcePages(pageNum);
    if (rc) return rc;

    //
    return 0;
}
