#include "ix.h"

IX_IndexScan::IX_IndexScan() {
    currentHeaderNode = NULL;
}

IX_IndexScan::~IX_IndexScan() {

}

// Open index scan
RC IX_IndexScan::OpenScan(const IX_IndexHandle &indexHandle, CompOp compOp, void *value, ClientHint  pinHint) {
    
    RC rc;
    // initialize condtion-based scan over entries
    // compop possible values     NO_OP, EQ_OP, NE_OP, LT_OP, GT_OP, LE_OP, GE_OP
    
    // check if scan already open
    if (bScanOpen == true)
        return IX_FILEOPEN;
    
    // check if compop is OK
    if (compOp < NO_OP || compOp > GE_OP)
        return IX_IDXCREATEFAIL;
        
    //On initialise ensuite toutes les variables dont on aura besoin
    //dans la méthode GetNextEntry
    type = indexHandle.ix_fileheader.type;
    length = indexHandle.ix_fileheader.length;
    taillePtr = indexHandle.ix_fileheader.taillePtr;
    tailleCle = indexHandle.ix_fileheader.tailleCle;
    op = compOp;
    val = value;
    
    pf_filehandle = indexHandle.pf_filehandle;
        
    bScanOpen = true;
    
    // on cherche la premiere feuille/bucket/rid dont la valeur match avec val
    rc = GetFirstRID(rid);
    if (rc) return rc;
    
    currentRID = rid;
    

// produce RID of all records whose indexed attributes match to value

// cast type of value (see rm)

    return 0;
}

// Get the next matching entry return IX_EOF if no more matching
// entries.
RC IX_IndexScan::GetNextEntry(RID &rid) {
    
    RC rc;
    PF_PageHandle *pf_pagehandle;
    
    // set rid to be the next record during the scan
    // return IX_EOF if no index entries
    
    // check if scan is open
    
    if (bScanOpen == false)
        return IX_FILECLOSED;
        

        
    // check if value match
    
    
    // set entry to next
    
    // reach end of file
    
    
    

    return 0;
}

// Close index scan
RC IX_IndexScan::CloseScan() {
    
    if (bScanOpen == false)
        return IX_FILECLOSED;
    
    bScanOpen = false;

    return 0;
}

// Find first RID that matches with val
RC IX_IndexScan::GetFirstRID(&rid) {
    
    PF_PageHandle *pf_pagehandle;
    
    return 0;
}

int IX_IndexScan::Compare(void *pData1, void *pData2) {
    switch(type){

        case INT : {

            int i1, i2;
            memcpy(&i1, pData1, sizeof(int));
            memcpy(&i2, pData2, sizeof(int));
            if (i1 < i2) return -1;
            if (i1 > i2) return +1;
            return 0;
            break;
        }
        case FLOAT : {

            float f1, f2;
            memcpy(&f1, pData1, sizeof(float));
            memcpy(&f2, pData2, sizeof(float));
            if (f1 < f2) return -1;
            if (f1 > f2) return +1;
            return 0;
            break;
        }
        case STRING : {

            string s1, s2;
            s1.reserve(length);
            s2.reserve(length);
            memcpy(&s1, pData1, length);
            memcpy(&s2, pData2, length);

            return s1.compare(s2);
        }
    }
}
