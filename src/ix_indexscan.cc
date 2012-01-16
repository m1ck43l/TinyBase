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
    ix_indexhandle = const_cast<IX_IndexHandle*>(&indexHandle);
    op = compOp;
    val = value;
    type = ix_indexhandle.ix_fileheader.type;
        
    bScanOpen = true;
    
    //Si on cherche quelque chose (value != NULL) on copie exactement la variable value
    //On caste directement pour éviter d'avoir à le refaire plus tard
    if (value != NULL) {
        switch (type) {
            case INT : {
                valInt = *((int*)(value));
                break;
            }
            case FLOAT : {
                valFloat = *((float*)(value));
                break;
            }
            case STRING : {
                if (length > MAXSTRINGLEN) return RM_ATTRTOLONG;
                    valString = (char*)malloc(length);
                memcpy(valString, value, length);
                break;
            }
        }
    }
    

// produce RID of all records whose indexed attributes match to value

// cast type of value (see rm)

    return 0;
}

// Get the next matching entry return IX_EOF if no more matching
// entries.
RC IX_IndexScan::GetNextEntry(RID &rid) {
    
    RC rc;
    PF_PageHandle pf_pagehandle;
    
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

