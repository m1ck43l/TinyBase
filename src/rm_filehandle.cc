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

  PF_PageHandle pf_pagehandle;
  RC rc;
  char *pData2; //Notre pointeur vers données

  //On récupère le prochain Rid libre dans le fichier
  rc = GetNextFreeRid(rid);
  if (rc) return rc;

  //On récupère le handle de la page correspondante au RID
  rc = pf_filehandle->GetThisPage(rid.pageNum, pf_pagehandle);
  if (rc) {
    pf_filehandle->UnpinPage(rid.pageNum);
    return rc;
  }

  //On récupère le début des données de la page
  rc = pf_pagehandle.GetData(pData2);
  if (rc){
    pf_filehandle->UnpinPage(rid.pageNum);
    return rc;
  }

  RM_PageHeader rmph(pData2, rm_fileheader.numberRecords);

  //Ecriture du record au bon emplacement
  //On met d'abord pData au bon emplacement
  pData2 += 2*sizeof(int);
  pData2 += rmph.getBitmap()->sizeToChar()*sizeof(char);
  pData2 += rid.slotNum * rm_fileheader.recordSize;

  memcpy(pData2, pData, rm_fileheader.recordSize);

  //Changement du bitmap pour marquer le rid comme occupé
  rmph.getBitmap()->setSlot(rid.slotNum);

  //On marque la page comme dirty
  rc = pf_filehandle->MarkDirty(rid.pageNum);
  if (rc) {
    pf_filehandle->UnpinPage(rid.pageNum);
    return rc;
  }

  //Si le slotnum était le dernier de la page on l'a donc remplie,
  //il faut la retirer de la liste des free pages
  int freeSlots = CountFreeSlots(rmph);
  if(freeSlots == 0) {
    rm_fileheader.nextFreePage = rmph.getNextPage();
  }

  // on reecrit les headers
  rc = WritePageHeader(pf_pagehandle, rmph);
  if(rc) {
    pf_filehandle->UnpinPage(rid.pageNum);
    return rc;
  }

  //Il n'y a plus qu'à unpin la page
  pf_filehandle->UnpinPage(rid.pageNum);

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

RC RM_FileHandle::GetNextFreeRid(RID &rid)
{
  RC rc; //Résultat

  //S'il y au moins une page avec de la place on cherche directement dessus
  if(rm_fileheader.nextFreePage != -1) {

    int i =0; //Sera l'indice de parcours des slots

    //On récupère le handle de la page
    PF_PageHandle pf_pagehandle;
    rc = pf_filehandle->GetThisPage(rm_fileheader.nextFreePage, pf_pagehandle);
    if (rc) {
      pf_filehandle->UnpinPage(rm_fileheader.nextFreePage);
      return rc;
    }

    //On récupère les données de la page
    char *pData;
    rc = pf_pagehandle.GetData(pData);
    if (rc) {
      pf_filehandle->UnpinPage(rm_fileheader.nextFreePage);
      return rc;
    }

    //On récupère le pageheader pour accéder au bitmap
    RM_PageHeader rmph(pData, rm_fileheader.numberRecords);

    //On parcours ensuite le bitmap jusqu'à trouver un rid de libre

    bool trouve = false;
    while ((i<rm_fileheader.numberRecords) && !trouve){
      if(rmph.getBitmap()->checkSlot(i) == RM_RECNOTFOUND) { //La place est donc libre on peut le prendre
	rid.pageNum = rm_fileheader.nextFreePage;
	rid.slotNum = i;
	trouve = true;
      }
      i++;
    }

    if (!trouve) {//Si on a toujours pas trouvé et qu'on a parcouru toute la page c'est qu'elle n'était pas libre
      pf_filehandle->UnpinPage(rm_fileheader.nextFreePage);
      return RM_FILENOTFREE;
    }

    //Sinon on a trouvé, on peut unpin et renvoyer 0
    pf_filehandle->UnpinPage(rm_fileheader.nextFreePage);
    return 0;
  }

  else { //Il n'y avait donc pas de page avec de la place dans le fichier
         //On va donc allouer une page

    PF_PageHandle pf_pagehandle;
    rc = pf_filehandle->AllocatePage(pf_pagehandle);
    if (rc) return rc;

    PageNum pageNum;
    rc = pf_pagehandle.GetPageNum(pageNum);
    if (rc) return rc;

    //On met à jour la liste des pages free dans le fileheader
    rm_fileheader.nextFreePage = pageNum;

    //On récupère les data
    char *pData;
    rc = pf_pagehandle.GetData(pData);
    if (rc) {
      pf_filehandle->UnpinPage(pageNum);
      return rc;
    }

    //On crée le bon page header avec pas de page suivante et le header en précédent
    //Le bitmap est totalement vide

    //On initialise d'abord à 0 toute la plage mémoire du header,
    //On ne change ensuite que les num de pages

    //Je ne sais pas comment obtenir la longueur du bitmap pour l'instant
    int bitmapSize = (rm_fileheader.numberRecords / 8 + ((rm_fileheader.numberRecords % 8) > 0 ? 1 : 0)) * sizeof(char);
    memset(pData, 0, 2*sizeof(int) + bitmapSize);

    //La première page du fichier à le nombre 0 c'est donc déjà bon pour la page précédente
    //La page suivante vaut -1 car inexistante
    int nextPage = -1;
    memcpy(pData + sizeof(int), &nextPage, sizeof(int));

    //Il faut donc maintenant rendre le premier rid de cette page
    rid.pageNum = pageNum;
    rid.slotNum = 0;
    rid.bIsValid = true;

    //On marque dirty la page et on unpin
    rc = pf_filehandle->MarkDirty(pageNum);
    if (rc) {
      pf_filehandle->UnpinPage(pageNum);
      return rc;
    }

    //Si on est arrivé ici c'est qu'il n'y a pas eu de problème
    return pf_filehandle->UnpinPage(pageNum);
  }
}
