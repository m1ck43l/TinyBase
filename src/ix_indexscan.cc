#include "ix.h"
#include <cstring>
#include <string>
#include <cstdlib>

using namespace std;

IX_IndexScan::IX_IndexScan() {

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
    taillePtr = indexHandle.ix_fileheader.taillePtr;
    tailleCle = indexHandle.ix_fileheader.tailleCle;
    op = compOp;
    val = value;
    currentRIDpos = 0;
    
    pf_filehandle = indexHandle.pf_filehandle;
        
    bScanOpen = true;
    
    // on cherche la premiere feuille/bucket/rid dont la valeur match avec val
    rc = GetFirstRID(indexHandle.ix_fileheader.numRacine, currentRID);
    if (rc) return rc;
    

// produce RID of all records whose indexed attributes match to value

// cast type of value (see rm)

    return 0;
}

// Get the next matching entry return IX_EOF if no more matching
// entries.
RC IX_IndexScan::GetNextEntry(RID &rid) {
    
    RC rc;
    
    // set rid to be the next record during the scan
    // return IX_EOF if no index entries
    
    // check if scan is open
    
    if (bScanOpen == false)
        return IX_FILECLOSED;
        
    //On vérifie si le currentRID est valable (pageNum != -1)
    PageNum pageRID;
    rc = currentRID.GetPageNum(pageRID);
    if (rc) return rc;

    if (pageRID == -1){
        return IX_EOF;
    }

    //Sinon on modifie le rid en currentRID et on cherche le suivant
    rc = rid.SetPageNum(pageRID);
    if (rc) return rc;
    SlotNum slotRID;

    rc = currentRID.GetSlotNum(slotRID);
    if (rc) return rc;

    rc = rid.SetSlotNum(slotRID);
    if (rc) return rc;

    return GetNextRID(currentRID);
}

RC IX_IndexScan::GetNextRID(RID &rid){

    //Si on était dans un bucket et qu'il n'est pas vide on continue de le vider
    if (!emptyBucket) {
        return GetNextRIDinBucket(rid);
    }
    else {
        //Le bucket est donc vide, on cherche le prochain bucket valable
        return GetNextBucket(rid);
    }
}

// Close index scan
RC IX_IndexScan::CloseScan() {
    
    if (bScanOpen == false)
        return IX_FILECLOSED;
    
    bScanOpen = false;

    return 0;
}

// Find first RID that matches with val
RC IX_IndexScan::GetFirstRID(PageNum pageNum, RID &rid) {

    //Si il n'y a pas de racine dans l'index, on est déjà à IX_EOF
    //IX_EOF peut être testé si on met le pageNum du RID à -1

    if (pageNum == -1){
        rid.SetPageNum(pageNum);
        return 0;
    }

    //Sinon on a une racine et on cherche dans quelle feuille value doit se trouver
    PF_PageHandle pagehandle;
    RC rc;

    rc = pf_filehandle->GetThisPage(pageNum, pagehandle);
    if (rc) return rc;

    //Si on est au niveau 0, c'est ici que l'on doit chercher le premier bucket

    //On récupère le header
    IX_NoeudHeader header;
    char *pData;
    rc = pagehandle.GetData(pData);
    if (rc) return rc;

    memcpy(&header, pData, sizeof(IX_NoeudHeader));

    if (header.niveau == 0) {

        rc = pf_filehandle->UnpinPage(pageNum);
        if (rc) return rc;

        return GetFirstBucket(pageNum, rid);
    }
    else {
        //On doit trouver dans quel noeud propager la récursion
        bool trouve = false;
        int i;
        void *pCle;

        for (i=1; i<=header.nbCle; i++){
            pCle=GetCle(pagehandle, i);

            //Si on est inférieur à la clé comparée, la valeur doit se trouver sur le pointeur précédent
            if (Compare(val, pCle)<0) {
                trouve = true;
                break;
            }
        }
        if (trouve){
            //On cherche la feuille à partir du nouveau noeud
            PageNum nextNum = GetPtr(pagehandle,i);

            rc = pf_filehandle->UnpinPage(pageNum);
            if (rc) return rc;

            return GetFirstRID(nextNum, rid);
        }
        else {
            //On doit chercher dans le dernier pointeur
            PageNum nextNum2 = GetPtr(pagehandle, header.nbCle+1);

            rc = pf_filehandle->UnpinPage(pageNum);
            if (rc) return rc;

            return GetFirstRID(nextNum2, rid);
        }
    }
    
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

PageNum IX_IndexScan::GetPtr(PF_PageHandle &pf_ph, int pos){

    RC rc;
    char *pData;
    PageNum num;

    rc = pf_ph.GetData(pData);
    if (rc) IX_PrintError(rc);

    pData += (sizeof(IX_NoeudHeader) + (pos-1)*(tailleCle + taillePtr));

    memcpy(&num, pData, sizeof(PageNum));

    return num;
}

void* IX_IndexScan::GetCle(PF_PageHandle &pf_ph, int pos){

    RC rc;
    char *pData;
    void *pData2 = malloc(tailleCle);

    rc = pf_ph.GetData(pData);
    if (rc) IX_PrintError(rc);

    pData += (sizeof(IX_NoeudHeader) + pos*taillePtr + (pos-1)*tailleCle);

    memcpy(pData2, pData, tailleCle);

    return pData2;
}
