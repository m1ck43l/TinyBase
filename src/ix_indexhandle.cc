#include "ix.h"
#include <cstring>

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
        if (CleExiste(pf_pagehandle)) {
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

            //Il faut vérifier si ce n'est pas la racine

            //On crée une nouvelle feuille
            PF_PageHandle new_pagehandle;
            rc = pf_filehandle->AllocatePage(new_pagehandle);
            if (rc) return rc;

            //On crée le header de la feuille
            IX_NoeudHeader new_header;
            new_header.nbCle = 0;
            new_header.nbMaxPtr = header.nbMaxPtr;
            new_header.niveau = header.niveau;

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

            //On regarde dans laquelle des 2 feuilles insérer la clé
            if (Compare(pData, GetCle(new_pagehandle, 1))<0) {
                //La clé a insérer est plus petite que la première de la nouvelle feuille
                rc = InsererFeuille(numPage, pData, rid);
                if (rc) return rc;
            }
            else {
                PageNum newPageNum;
                rc = new_pagehandle.GetPageNum(newPageNum);
                if (rc) return rc;

                rc = InsererFeuille(newPageNum, pData, rid);
            }

            //On remonte la 1ere clé de la nouvelle feuille

        }
    }

}

// Delete a new index entry
RC IX_IndexHandle::DeleteEntry(void *pData, const RID &rid) {

    return 0;
}

// Force index files to disk
RC IX_IndexHandle::ForcePages() {

    return 0;
}
