#include "ix.h"
#include <cstring>
#include <string>

using namespace std;

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

     RC rc;
     //On essaie d'insérer l'entrée à la racine si elle n'existe pas encore, on la crée
     if(ix_fileheader.numRacine == -1)
     {
         PF_PageHandle pf_pagehandle;
         rc = pf_filehandle->AllocatePage(pf_pagehandle);
         if (rc) return rc;

         //On crée le header de la racine
         IX_NoeudHeader header;
         header.nbCle = 0;
         header.niveau = 0;
         header.nbMaxPtr = ((PF_PAGE_SIZE+ix_fileheader.tailleCle)/(ix_fileheader.tailleCle + ix_fileheader.taillePtr));
         header.pageMere = -1;

         //On copie le header en mémoire
         char* pData2;
         rc = pf_pagehandle.GetData(pData2);
         if (rc) return rc;

         memcpy(pData2, &header, sizeof(IX_NoeudHeader));

         //On change le numéro de la racine
         PageNum pageNum;
         rc = pf_pagehandle.GetPageNum(pageNum);
         ix_fileheader.numRacine = pageNum;

         //La page est dirty
         rc = pf_filehandle->MarkDirty(pageNum);
         if (rc) return rc;

         //On unpin la page
         rc = pf_filehandle->UnpinPage(pageNum);
         if (rc) return rc;
     }

     //Une fois arrivé ici, la racine existe forcément
     rc = Inserer(ix_fileheader.numRacine, pData, rid);

     return rc;
 }

RC IX_IndexHandle::Inserer(PageNum pageNum, void *pData, const RID &rid){

    RC rc;
    //On prend d'abord un pagehandle du noeud où l'on veut insérer
    PF_PageHandle pf_pagehandle;
    rc = pf_filehandle->GetThisPage(pageNum, pf_pagehandle);
    if (rc) return rc;

    //On récupère le header du noeud
    IX_NoeudHeader header;
    char* pData2;
    rc = pf_pagehandle.GetData(pData2);
    if (rc) return rc;

    memcpy(&header, pData2, sizeof(IX_NoeudHeader));

    //On récupère tout de suite le numéro de la page courante
    PageNum numPage;
    rc = pf_pagehandle.GetPageNum(numPage);
    if (rc) return rc;

    //Si le noeud n'est pas une feuille, alors on cherche dans quelle feuille l'insérer
    if (header.niveau != 0) {
        int i;
        void* pCle;
        bool trouve = false;
        for (i=1; i<=header.nbCle; i++){
            pCle=GetCle(pf_pagehandle, i);

            //Si on est inférieur à la clé comparée, l'entrée doit être ajoutée sur le pointeur précédent
            if (Compare(pData, pCle)<0) {
                trouve = true;
                break;
            }
        }
        //On peut unpin la page et inserer dans le noeud suivant

        if (trouve){ //On est donc plus petit qu'une clé
            PageNum pageNum = GetPtr(pf_pagehandle, i);

            rc = pf_filehandle->UnpinPage(numPage);
            if (rc) return rc;

            rc = Inserer(pageNum, pData, rid);
            if (rc) return rc;
        }
        else { //Sinon on doit insérer l'entrée sur le dernier pointeur
            PageNum pageNum = GetPtr(pf_pagehandle, header.nbCle+1);

            rc = pf_filehandle->UnpinPage(numPage);
            if (rc) return rc;

            rc = Inserer(pageNum, pData, rid);
            if (rc) return rc;
        }
    }

    else { //On est au niveau d'une feuille

        //Si la clé existe déjà dans la feuille, c'est plus simple à insérer
        if (CleExiste(pf_pagehandle, header, pData)) {
            rc = pf_filehandle->UnpinPage(numPage);
            if (rc) return rc;

            rc = InsererFeuilleExiste(numPage, pData, rid);
            if (rc) return rc;
        }
        //Si on a de la place on peut l'insérer
        else if (header.nbCle<(header.nbMaxPtr - 1)) {
            rc = pf_filehandle->UnpinPage(numPage);
            if (rc) return rc;

            rc = InsererFeuille(numPage, pData, rid);
            if (rc) return rc;
        }
        else { //Sinon on doit splitter la feuille

            //On crée une nouvelle feuille
            PF_PageHandle new_pagehandle;
            rc = pf_filehandle->AllocatePage(new_pagehandle);
            if (rc) return rc;

            //On crée le header de la feuille
            IX_NoeudHeader new_header;
            new_header.nbCle = 0;
            new_header.nbMaxPtr = header.nbMaxPtr;
            new_header.niveau = header.niveau;
            new_header.pageMere = header.pageMere;

            char *pData2;
            rc = new_pagehandle.GetData(pData2);
            if (rc) return rc;

            memcpy(pData2, &new_header, sizeof(IX_NoeudHeader));

            //On la rempli avec la moitié de l'ancienne
            int max = header.nbMaxPtr;
            int ordre = max/2, i;

            for (i=(ordre+1); i<=max; i++){
                SetCle(new_pagehandle, i - (ordre+1), GetCle(pf_pagehandle, i));
                SetPtr(new_pagehandle, i - (ordre+1), GetPtr(pf_pagehandle, i));
                new_header.nbCle++;
                header.nbCle--;
            }
            //On marque les pages en dirty
            rc = pf_filehandle->MarkDirty(numPage);
            if (rc) return rc;

            PageNum newPageNum;
            rc = new_pagehandle.GetPageNum(newPageNum);
            if (rc) return rc;

            rc = pf_filehandle->MarkDirty(newPageNum);
            if (rc) return rc;

            //On regarde dans laquelle des 2 feuilles insérer la clé
            if (Compare(pData, GetCle(new_pagehandle, 1))<0) {
                //La clé a insérer est plus petite que la première de la nouvelle feuille
                rc = pf_filehandle->UnpinPage(numPage);
                if (rc) return rc;

                rc = pf_filehandle->UnpinPage(newPageNum);
                if (rc) return rc;

                rc = InsererFeuille(numPage, pData, rid);
                if (rc) return rc;
            }
            else {
                rc = pf_filehandle->UnpinPage(numPage);
                if (rc) return rc;

                rc = pf_filehandle->UnpinPage(newPageNum);
                if (rc) return rc;

                rc = InsererFeuille(newPageNum, pData, rid);
                if (rc) return rc;
            }

            //On récupère la clé à remonter
            void *pData3 = GetCle(new_pagehandle,1);

            //Si les feuilles étaient la racine, on doit en créer une nouvelle
            if (header.pageMere == -1) {

                PF_PageHandle newRacine;
                rc = pf_filehandle->AllocatePage(newRacine);
                if (rc) return rc;

                PageNum racNum;
                rc = newRacine.GetPageNum(racNum);
                if (rc) return rc;

                IX_NoeudHeader racHeader;
                racHeader.nbCle = 0;
                racHeader.nbMaxPtr = header.nbMaxPtr;
                racHeader.niveau = header.niveau + 1;
                racHeader.pageMere = -1;

                char *pData4;
                rc = newRacine.GetData(pData4);
                if (rc) return rc;

                memcpy(pData4, &racHeader, sizeof(IX_NoeudHeader));

                header.pageMere = racNum;
                new_header.pageMere = racNum;

                rc = pf_filehandle->UnpinPage(newPageNum);
                if (rc) return rc;

                rc = pf_filehandle->UnpinPage(numPage);
                if (rc) return rc;

                rc = pf_filehandle->MarkDirty(racNum);
                if (rc) return rc;

                rc = pf_filehandle->UnpinPage(racNum);
                if (rc) return rc;

                rc = InsererNoeudInterne(racNum,pData4);
                if (rc) return rc;
            }

            //On remonte la 1ere clé de la nouvelle feuille
            rc = pf_filehandle->UnpinPage(numPage);
            if (rc) return rc;

            rc = pf_filehandle->UnpinPage(newPageNum);
            if (rc) return rc;

            rc = InsererNoeudInterne(header.pageMere, pData3);
            if (rc) return rc;
        }
    }

}

RC IX_IndexHandle::InsererFeuilleExiste(PageNum pageNum, void *pData, const RID &rid){

    //On insère la clé et le rid dans une feuille ou la clé existe déjà
    //Il faut simplement ajouter le rid au bucket

    RC rc;
    PF_PageHandle pagehandle;

    rc = pf_filehandle->GetThisPage(pageNum, pagehandle);
    if (rc) return rc;

    //On recopie le header
    IX_NoeudHeader header;
    char *pData2;

    rc = pagehandle.GetData(pData2);
    if (rc) return rc;

    memcpy(&header, pData2, sizeof(IX_NoeudHeader));

    int pos = 0, i=1;
    bool trouve = false;

    while ( (i<=header.nbCle) && !trouve ){
        if(Compare(pData, GetCle(pagehandle, i)) == 0) {
            //On est donc sur la bonne clé
            trouve = true;
            pos = i;
        }
        i++;
    }

    if (pos == 0) return IX_KEYNOTEXISTS; //Ne devrait jamais arriver

    PageNum nextPage = GetPtr(pagehandle, pos);

    //On unpin la page
    rc = pf_filehandle->UnpinPage(pageNum);
    if (rc) return rc;

    return InsererBucket(nextPage, rid);

}

RC IX_IndexHandle::InsererBucket(PageNum pageNum, const RID &rid){

    RC rc;
    //On récupère d'abord la page
    PF_PageHandle pagehandle;

    rc = pf_filehandle->GetThisPage(pageNum, pagehandle);
    if (rc) return rc;

    //On récupère le header
    IX_BucketHeader header;
    char *pData;

    rc = pagehandle.GetData(pData);
    if (rc) return rc;

    memcpy(&header, pData, sizeof(IX_BucketHeader));

    if (header.nbRid == header.nbMax){
        //Page de débordement à traiter plus tard
        return IX_BUCKETFULL;
    }

    //On décale le pointeur
    pData += (sizeof(IX_BucketHeader) + header.nbRid*sizeof(RID));

    memcpy(pData, &rid, sizeof(RID));

    header.nbRid++;

    rc = pf_filehandle->MarkDirty(pageNum);
    if (rc) return rc;

    return pf_filehandle->UnpinPage(pageNum);
}

RC IX_IndexHandle::InsererFeuille(PageNum pageNum, void *pData, const RID &rid){
    //La feuille a assez de place pour l'entrée supplémentaire

    //On récupère la page
    RC rc;
    PF_PageHandle pagehandle;

    rc = pf_filehandle->GetThisPage(pageNum, pagehandle);
    if (rc) return rc;

    //On récupère le header
    IX_NoeudHeader header;
    char *pData2;

    rc = pagehandle.GetData(pData2);
    if (rc) return rc;

    memcpy(&header, pData2, sizeof(IX_NoeudHeader));

    //On cherche la position à laquelle nous devons insérer l'entrée
    int pos = 0, i;

    for (i=1; i<=header.nbCle; i++){
        if (Compare(pData, GetCle(pagehandle, i)) < 0){
            //Cela veut dire qu'avant nous étions supérieur, et plus maintenant
            break;
        }
    }
    //Si on est jamais passé dans la boucle if, c'est que la clé est supérieure à toutes
    pos = i;

    //On déplace maintenant toutes les clés et les pointeurs qui sont après la nouvelle position
    for (i=header.nbCle; i>=pos; i--){
        SetCle(pagehandle, i+1, GetCle(pagehandle, i));
        SetPtr(pagehandle, i+1, GetPtr(pagehandle, i));
    }

    //On insère la nouvelle valeur à la position pos
    SetCle(pagehandle, pos, pData);

    //On crée un nouveau bucket pour insérer le rid
    PF_PageHandle bucket;

    rc = pf_filehandle->AllocatePage(bucket);
    if (rc) return rc;

    PageNum numBucket;
    rc = bucket.GetPageNum(numBucket);
    if (rc) return rc;

    //On n'oublie pas le pointeur vers le bucket dans la feuille
    SetPtr(pagehandle, pos, numBucket);

    //On crée le header du bucket
    IX_BucketHeader bHeader;
    bHeader.nbRid = 0;
    bHeader.nbMax = (PF_PAGE_SIZE-sizeof(IX_BucketHeader))/sizeof(RID);

    //On écrit le header sur la page
    char *pData3;
    rc = bucket.GetData(pData3);
    if (rc) return rc;

    memcpy(pData3, &bHeader, sizeof(IX_BucketHeader));

    //Les 2 pages sont dirty, et on les unpin avant d'insérer le rid dans le bucket
    rc = pf_filehandle->MarkDirty(pageNum);
    if (rc) return rc;

    rc = pf_filehandle->MarkDirty(numBucket);
    if (rc) return rc;

    rc = pf_filehandle->UnpinPage(pageNum);
    if (rc) return rc;

    rc = pf_filehandle->UnpinPage(numBucket);
    if (rc) return rc;

    return InsererBucket(numBucket, rid);

}

bool IX_IndexHandle::CleExiste(PF_PageHandle &pf_ph, IX_NoeudHeader header, void *pData){
    int i=1;
    bool trouve = false;

    while( (i<=header.nbCle) && (!trouve)){
        if (Compare(GetCle(pf_ph,i), pData)) trouve = true;
        i++;
    }
    return trouve;
}

void IX_IndexHandle::SetCle(PF_PageHandle &pf_ph, int pos, void *pData){
    //On récupère d'abord le pointeur de la page
    char *pData2;
    RC rc;

    rc = pf_ph.GetData(pData2);
    if (rc) IX_PrintError(rc);

    //On règle le décalage
    pData2 += (sizeof(IX_NoeudHeader) + pos*ix_fileheader.taillePtr + (pos-1)*ix_fileheader.tailleCle);

    memcpy(pData2, pData, ix_fileheader.length);
}

void IX_IndexHandle::SetPtr(PF_PageHandle &pf_ph, int pos, PageNum pageNum){
    //On récupère le pointeur de la page
    char *pData;
    RC rc;

    rc = pf_ph.GetData(pData);
    if (rc) IX_PrintError(rc);

    //On règle le décalage
    pData += (sizeof(IX_NoeudHeader) + (pos-1)*(ix_fileheader.tailleCle + ix_fileheader.taillePtr));

    memcpy(pData, &pageNum, sizeof(PageNum));
}

PageNum IX_IndexHandle::GetPtr(PF_PageHandle &pf_ph, int pos){

    RC rc;
    char *pData;
    PageNum num;

    rc = pf_ph.GetData(pData);
    if (rc) IX_PrintError(rc);

    pData += (sizeof(IX_NoeudHeader) + (pos-1)*(ix_fileheader.tailleCle + ix_fileheader.taillePtr));

    memcpy(&num, pData, sizeof(PageNum));

    return num;
}

void* IX_IndexHandle::GetCle(PF_PageHandle &pf_ph, int pos){

    RC rc;
    char *pData;
    void *pData2;

    rc = pf_ph.GetData(pData);
    if (rc) IX_PrintError(rc);

    pData += (sizeof(IX_NoeudHeader) + pos*ix_fileheader.taillePtr + (pos-1)*ix_fileheader.tailleCle);

    memcpy(pData2, pData, ix_fileheader.length);

    return pData2;
}

int IX_IndexHandle::Compare(void* pData1, void*pData2){

    switch(ix_fileheader.type){

        case INT : {

        int i1, i2;
        memcpy(&i1, pData1, sizeof(int));
        memcpy(&i2, pData2, sizeof(int));
        if (i1<i2) return -1;
        if (i1>i2) return +1;
        return 0;
        break;
    }
        case FLOAT : {

        float f1, f2;
        memcpy(&f1, pData1, sizeof(float));
        memcpy(&f2, pData2, sizeof(float));
        if (f1<f2) return -1;
        if (f1>f2) return +1;
        return 0;
        break;
    }
        case STRING : {

        string s1, s2;
        s1.reserve(ix_fileheader.length);
        s2.reserve(ix_fileheader.length);
        memcpy(&s1, pData1, ix_fileheader.length);
        memcpy(&s2, pData2, ix_fileheader.length);

        return s1.compare(s2);
    }
    }
}

// Delete a new index entry
RC IX_IndexHandle::DeleteEntry(void *pData, const RID &rid) {

    return 0;
}

// Force index files to disk
RC IX_IndexHandle::ForcePages() {

    return pf_filehandle->ForcePages();
}
