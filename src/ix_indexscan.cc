#include "ix.h"
#include <cstring>
#include <string>
#include <cstdlib>
#include <iostream>
#include <cstdio>

using namespace std;

IX_IndexScan::IX_IndexScan() {
    bScanOpen = false;
    pf_filehandle = NULL;
}

IX_IndexScan::~IX_IndexScan() {
    if (pf_filehandle != NULL)
        delete pf_filehandle;
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

    //On copie le filehandle
    //PF_FileHandle filehandle;
    //filehandle = *indexHandle.pf_filehandle;
    pf_filehandle = new PF_FileHandle(*indexHandle.pf_filehandle);

    bScanOpen = true;

    // on cherche la premiere feuille/bucket/rid dont la valeur match avec val
    rc = GetFirstRID(indexHandle.ix_fileheader.numRacine, currentRID);
    if (rc) return rc;

    return 0;
}

// Close index scan
RC IX_IndexScan::CloseScan() {

    if (bScanOpen == false)
        return IX_FILECLOSED;

    bScanOpen = false;

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

    //On vérifie si le currentRID est valable
    if (!currentRID.bIsValid) {
        return IX_EOF;
    }

    //Sinon on modifie le rid en currentRID et on cherche le suivant
    rid.bIsValid=true;
    rid.SetPageNum(currentRID.pageNum);
    SlotNum slotRID;

    rc = currentRID.GetSlotNum(slotRID);
    if (rc) return rc;

    rid.SetSlotNum(slotRID);

    return GetNextRIDinBucket(currentRID);
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

        rid.bIsValid = true;
        rid.SetPageNum(numRID);
        rid.SetSlotNum(slotRID);

        //On se décale dans le bucket pour l'appel suivant
        currentRIDpos++;

        return pf_filehandle->UnpinPage(currentBucketNum);
    }

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

        //Si on a NO_OP ou NE_OP on doit tout sortir
        //On prend donc le premier bucket
        if ((op == NO_OP) || (op == NE_OP)){
            if (op == NE_OP) {
                //On vérifie que ce n'est pas la première valeur de la feuille
                if (Compare(val, GetCle(pagehandle,1)) == 0) {
                    if (header.nbCle > 2) {
                        currentBucketNum = GetPtr(pagehandle, 2);
                        currentRIDpos = 1;

                        rc = pf_filehandle->UnpinPage(pageNum);
                        if (rc) return rc;

                        return GetNextRIDinBucket(rid);
                    }
                    else {
                        //Il n'y a donc aucune valeur non égale dans l'index
                        rc = pf_filehandle->UnpinPage(pageNum);
                        if (rc) return rc;

                        //On met le rid en invalide et on sors normalement de la fonction
                        PageNum numInv = -1;
                        rid.SetPageNum(numInv);
                        return 0;
                    }
                }
            }
            currentBucketNum = GetPtr(pagehandle, 1);
            currentRIDpos = 1;

            rc = pf_filehandle->UnpinPage(pageNum);
            if (rc) return rc;

            return GetNextRIDinBucket(rid);
        }

        //Sinon on cherche le premier bucket valable
        rc = pf_filehandle->UnpinPage(pageNum);
        if (rc) return rc;

        return GetFirstBucket(pageNum, rid);
    }
    else {//On est au niveau d'un noeud interne

        //Si on a NO_OP ou NE_OP on doit chercher la première feuille à gauche
        if ((op == NO_OP) || (op == NE_OP)){
            PageNum leftPage = GetPtr(pagehandle, 1);

            rc = pf_filehandle->UnpinPage(pageNum);
            if (rc) return rc;

            return GetFirstRID(leftPage, rid);
        }

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

RC IX_IndexScan::GetFirstBucket(PageNum pageNum, RID &rid) {
    //pageNum correspond à la feuille où devrait se trouver la valeur val
    PF_PageHandle pagehandle;
    RC rc;

    currentPageNum = pageNum;

    rc = pf_filehandle->GetThisPage(pageNum, pagehandle);
    if (rc) return rc;

    //On récupère le header de la feuille
    IX_NoeudHeader header;
    char* pData2;
    rc = pagehandle.GetData(pData2);
    if (rc) return rc;

    memcpy(&header, pData2, sizeof(IX_NoeudHeader));

    //On cherche la position que devrait prendre la clé
    int i;
    void* pCle;
    bool valIsFound = false;

    //Si on est inférieur à la clé comparée, la valeur doit se trouver sur le pointeur précédent
    for (i=1; i<=header.nbCle; i++){
        pCle=GetCle(pagehandle, i);

        if (Compare(val, pCle) <= 0) {
            if (Compare(val, pCle) == 0) {
                valIsFound = true;
            }
            break;
        }
    }

    switch(op) {
        case EQ_OP: {
            if (valIsFound) {
                currentBucketNum = GetPtr(pagehandle,i);
                currentPagePos = i;
            }
            else {
                //Il n'y a donc aucune valeur qui correspond dans l'index
                rid.bIsValid = false;

                rc = pf_filehandle->UnpinPage(pageNum);
                if (rc) return rc;

                return 0;
            }
            break;
        }
        case GE_OP: {
            if (valIsFound) {
                currentBucketNum = GetPtr(pagehandle,i);
                currentPagePos = i;
            }
            // si la cle sur laquelle on est est deja superieure a la valeur voulue
            else if (!valIsFound && (i != header.nbCle+1)) {
                currentBucketNum = GetPtr(pagehandle,i);
                currentPagePos = i;
            }
            else { //i est supérieur toutes les clés sans jamais être égal
                //On regarder s'il n'existe pas une page liée à droite
                if (header.nextPage == -1) {
                    rid.bIsValid = false;

                    rc = pf_filehandle->UnpinPage(pageNum);
                    if (rc) return rc;

                    return 0;
                }
                else {
                    //Il existe donc une nextpage
                    rc = pf_filehandle->UnpinPage(pageNum);
                    if (rc) return rc;

                    return GetFirstBucket(header.nextPage, rid);
                }
            }
            break;
        }
        case LE_OP: {
            if (valIsFound) {
                currentBucketNum = GetPtr(pagehandle,i);
                currentPagePos = i;
            }
            else if (!valIsFound && (i != 1)) {
                currentBucketNum = GetPtr(pagehandle, i-1);
                currentPagePos = i-1;
            }
            else if (!valIsFound && (i == 1) && (header.prevPage != -1)) {
                rc = pf_filehandle->UnpinPage(pageNum);
                if (rc) return rc;

                return GetFirstBucket(header.prevPage, rid);
            }
            else {
                rid.bIsValid = false;

                rc = pf_filehandle->UnpinPage(pageNum);
                if (rc) return rc;

                return 0;
            }
            break;
        }

        case GT_OP: {
            // s'il y a encore une cle a droite
            if (valIsFound && (i != header.nbCle)) {
                currentBucketNum = GetPtr(pagehandle,i+1);
                currentPagePos = i+1;
            }
            // s'il n'y a plus de cle a droite, On regarde s'il y a une page suivante
            else if (valIsFound && (i == header.nbCle)) {
                if (header.nextPage == -1) {
                    rid.bIsValid = false;

                    rc = pf_filehandle->UnpinPage(pageNum);
                    if (rc) return rc;

                    return 0;
                }
                else {
                    rc = pf_filehandle->UnpinPage(pageNum);
                    if (rc) return rc;

                    return GetFirstBucket(header.nextPage, rid);
                }
            }
            // si la cle sur laquelle on est est deja superieure a la valeur voulue
            else if (!valIsFound && (i != header.nbCle+1)) {
                currentBucketNum = GetPtr(pagehandle,i);
                currentPagePos = i;
            }
            else if (!valIsFound && (i == header.nbCle+1) && (header.nextPage != -1)){
                rc = pf_filehandle->UnpinPage(pageNum);
                if (rc) return rc;

                return GetFirstBucket(header.nextPage, rid);
            }
            else {
                rid.bIsValid = false;

                rc = pf_filehandle->UnpinPage(pageNum);
                if (rc) return rc;

                return 0;
            }
            break;
        }
        case LT_OP: {
            // s'il y a encore une cle a gauche
            if (valIsFound && (i != 1)) {
                currentBucketNum = GetPtr(pagehandle,i-1);
                currentPagePos = i-1;
            }
            // s'il n'y a plus de cle a gauche, on regarde s'il y a une page precedente
            else if (valIsFound && (i == 1)) {
                if (header.prevPage != -1){
                    rc = pf_filehandle->UnpinPage(pageNum);
                    if (rc) return rc;

                    return GetFirstBucket(header.prevPage, rid);
                }
                else {
                    rid.bIsValid = false;

                    rc = pf_filehandle->UnpinPage(pageNum);
                    if (rc) return rc;

                    return 0;
                }

            }

            else if (!valIsFound && i != 1) {
                currentBucketNum = GetPtr(pagehandle, i-1);
                currentPagePos = i-1;
            }
            else if (!valIsFound && i == 1 && header.prevPage != -1) {
                rc = pf_filehandle->UnpinPage(pageNum);
                if (rc) return rc;

                return GetFirstBucket(header.prevPage, rid);
            }
            else {
                rid.bIsValid = false;

                rc = pf_filehandle->UnpinPage(pageNum);
                if (rc) return rc;

                return 0;
            }
            break;
        }
        default : return IX_ERROROP; //On ne devrait pas avoir d'autre OP
    }

    currentRIDpos = 1;

    rc = pf_filehandle->UnpinPage(pageNum);
    if (rc) return rc;

    return GetNextRIDinBucket(rid);
}

RC IX_IndexScan::GetNextBucket(RID &rid) {
    PF_PageHandle pf_pagehandle;
    RC rc;
    PageNum pageNum = currentPageNum;

    rc = pf_filehandle->GetThisPage(pageNum, pf_pagehandle);
    if (rc) return rc;

    //On récupère le header du noeud
    IX_NoeudHeader header;
    char* pData2;
    rc = pf_pagehandle.GetData(pData2);
    if (rc) return rc;

    memcpy(&header, pData2, sizeof(IX_NoeudHeader));

    switch(op) {

    case NO_OP: {
        //On a juste à prendre le bucket suivant
        if (currentPagePos == header.nbCle) {
            if (header.nextPage != -1){
                PF_PageHandle next_pagehandle;

                rc = pf_filehandle->GetThisPage(header.nextPage, next_pagehandle);
                if (rc) return rc;

                currentBucketNum = GetPtr(next_pagehandle, 1);
                currentPagePos = 1;
                currentPageNum = header.nextPage;
                rc = pf_filehandle->UnpinPage(header.nextPage);
                if (rc) return rc;
            }
            else{
                rid.bIsValid = false;

                rc = pf_filehandle->UnpinPage(pageNum);
                if (rc) return rc;

                return 0;
            }
        }
        else {//Il reste des clés dans la feuille courante
            currentPagePos++;
            currentBucketNum = GetPtr(pf_pagehandle,currentPagePos);
        }
    }
    case NE_OP : {
        //On a juste à prendre le bucket suivant
        if (currentPagePos == header.nbCle) {
            if (header.nextPage != -1){
                PF_PageHandle next_pagehandle2;

                rc = pf_filehandle->GetThisPage(header.nextPage, next_pagehandle2);
                if (rc) return rc;

                if (Compare(val,GetCle(next_pagehandle2,1)) == 0) {
                    currentBucketNum = GetPtr(next_pagehandle2, 2);
                    currentPagePos = 2;
                    currentPageNum = header.nextPage;

                    rc = pf_filehandle->UnpinPage(header.nextPage);
                    if (rc) return rc;
                }
                else {
                    currentBucketNum = GetPtr(next_pagehandle2, 1);
                    currentPagePos = 1;
                    currentPageNum = header.nextPage;
                    rc = pf_filehandle->UnpinPage(header.nextPage);
                    if (rc) return rc;
                }
            }
            else{
                rid.bIsValid = false;

                rc = pf_filehandle->UnpinPage(pageNum);
                if (rc) return rc;

                return 0;
            }
        }
        else {//Il reste des clés dans la feuille courante
            currentPagePos++;
            if ( (Compare(val, GetCle(pf_pagehandle, currentPagePos)) == 0) && (currentPagePos!=header.nbCle)) {
                currentPagePos++;
                currentBucketNum = GetPtr(pf_pagehandle,currentPagePos);
            }
            else if ((Compare(val, GetCle(pf_pagehandle, currentPagePos)) == 0) && (currentPagePos==header.nbCle)){
                //On doit regarder s'il reste des pages
                if (header.nextPage == -1){
                    rid.bIsValid = false;

                    rc = pf_filehandle->UnpinPage(pageNum);
                    if (rc) return rc;

                    return 0;
                }
                else {
                    PF_PageHandle next_handle;
                    rc = pf_filehandle->GetThisPage(header.nextPage,next_handle);
                    if (rc) return rc;

                    currentBucketNum = GetPtr(next_handle, 1);
                    currentPagePos = 1;
                    currentPageNum = header.nextPage;

                    rc = pf_filehandle->UnpinPage(header.nextPage);
                    if (rc) return rc;
                }
            }
            else {
                currentBucketNum = GetPtr(pf_pagehandle,currentPagePos);
            }
        }
        break;
    }
    case GE_OP:
    case GT_OP: {
        if (currentPagePos < header.nbCle) {
            currentPagePos++;
            currentBucketNum = GetPtr(pf_pagehandle, currentPagePos);
        }
        else if (currentPagePos == header.nbCle && header.nextPage != -1) {
            PF_PageHandle next_handle2;

            rc = pf_filehandle->GetThisPage(header.nextPage, next_handle2);
            if (rc) return rc;

            currentBucketNum = GetPtr(next_handle2,1);
            currentPagePos = 1;
            currentPageNum = header.nextPage;

            rc = pf_filehandle->UnpinPage(header.nextPage);
            if (rc) return rc;
        }
        else {
            rid.bIsValid = false;

            rc = pf_filehandle->UnpinPage(pageNum);
            if (rc) return rc;

            return 0;
        }
        break;
    }
    case LE_OP:
    case LT_OP: {
        if (currentPagePos > 1) {
            currentPagePos--;
            currentBucketNum = GetPtr(pf_pagehandle, currentPagePos);
        }
        else if (currentPagePos == 1 && header.prevPage != -1) {
            PF_PageHandle next_handle3;

            rc = pf_filehandle->GetThisPage(header.prevPage, next_handle3);
            if (rc) return rc;

            //On récupère le header pour connaitre la dernière clé
            IX_NoeudHeader prevHeader;
            char* pData3;

            rc = next_handle3.GetData(pData3);
            if (rc) return rc;

            memcpy(&prevHeader, pData3, sizeof(IX_NoeudHeader));

            currentBucketNum = GetPtr(next_handle3, prevHeader.nbCle);
            currentPagePos = prevHeader.nbCle;
            currentPageNum = header.prevPage;

            rc = pf_filehandle->UnpinPage(header.prevPage);
            if (rc) return rc;
        }
        else {
            rid.bIsValid = false;

            rc = pf_filehandle->UnpinPage(pageNum);
            if (rc) return rc;

            return 0;
        }
        break;
    }
    case EQ_OP: {
        rid.bIsValid = false;

        rc = pf_filehandle->UnpinPage(pageNum);
        if (rc) return rc;

        return 0;
    }
    }
    currentRIDpos = 1;

    rc = pf_filehandle->UnpinPage(pageNum);
    if(rc) return rc;

    return GetNextRIDinBucket(rid);
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
            char *s1 = (char*) pData1;
            char *s2 = (char*) pData2;
            return strncmp(s1,s2,tailleCle);
        }
    }
    //On ne doit pas arriver jusqu'ici
    return 0;
}

PageNum IX_IndexScan::GetPtr(PF_PageHandle &pf_ph, int pos) {

    RC rc;
    char *pData;
    PageNum num;

    rc = pf_ph.GetData(pData);
    if (rc) IX_PrintError(rc);

    pData += (sizeof(IX_NoeudHeader) + (pos-1)*(tailleCle + taillePtr));

    memcpy(&num, pData, sizeof(PageNum));

    return num;
}

void* IX_IndexScan::GetCle(PF_PageHandle &pf_ph, int pos) {

    RC rc;
    char *pData;
    void *pData2 = malloc(tailleCle);

    rc = pf_ph.GetData(pData);
    if (rc) IX_PrintError(rc);

    pData += (sizeof(IX_NoeudHeader) + pos*taillePtr + (pos-1)*tailleCle);

    memcpy(pData2, pData, tailleCle);

    return pData2;
}
