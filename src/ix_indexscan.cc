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
    rid.SetPageNum(pageRID);
    SlotNum slotRID;

    rc = currentRID.GetSlotNum(slotRID);
    if (rc) return rc;

    rid.SetSlotNum(slotRID);

    return GetNextRID(currentRID);
}

RC IX_IndexScan::GetNextRIDinBucket(RID &rid){

    //On récupère le pagehandle du bucket courant
    RC rc;
    PF_PageHandle pagehandle;

    rc = pf_filehandle->GetThisPage(currentBucketNum, pagehandle);
    if (rc) return rc;

    //On récupère le header
    IX_BucketHeader header;
    char *pData;

    rc = pagehandle.GetData(pData);
    if (rc) return rc;

    memcpy(&header, pData, sizeof(IX_BucketHeader));

    if (currentRIDpos > header.nbRid){
        //Cela signifie que l'on était sur le dernier rid du bucket
        //On regarde si il y a un bucket de débordement
        if(header.nextBuck == -1) {
            //On était donc sur le tout dernier rid pour cette clé
            rc = pf_filehandle->UnpinPage(currentBucketNum);
            if (rc) return rc;

            rc = this->GetNextBucket(rid);
            return rc;
        }
        else {
            currentBucketNum = header.nextBuck;
            currentRIDpos = 1;

            rc = pf_filehandle->UnpinPage(currentBucketNum);
            if (rc) return rc;

            return GetNextRIDinBucket(rid);
        }
    }

    else {
        //On peut donc récupérer le bon rid
        RID RIDres;

        //On se décale du bon nombre de position
        pData += sizeof(IX_BucketHeader) + (currentRIDpos-1)*sizeof(RID);

        memcpy(&RIDres, pData, sizeof(RID));

        //On met maintenant le bon rid dans celui cherché
        PageNum numRID;
        SlotNum slotRID;
        rc = RIDres.GetPageNum(numRID);
        if (rc) return rc;

        rc = RIDres.GetSlotNum(slotRID);
        if (rc) return rc;

        rid.SetPageNum(numRID);
        rid.SetSlotNum(slotRID);

        //On se décale dans le bucket pour l'appel suivant
        currentRIDpos++;

        return pf_filehandle->UnpinPage(currentBucketNum);
    }

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

RC IX_IndexScan::GetFirstBucket(PageNum pageNum, RID &rid){
    return 0;
}

RC IX_IndexScan::GetNextBucket(RID &rid){
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
