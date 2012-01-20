#include "ix.h"
#include <string>
#include <cstring>
#include <iostream>
#include <sstream>

using namespace std;

IX_Manager::IX_Manager(PF_Manager &pfm) : pf_manager(pfm) {}

IX_Manager::~IX_Manager() {

}

RC IX_Manager::CreateIndex (const char *fileName, int indexNo, AttrType attrType, int attrLength) {
    RC rc;

    ostringstream oss;
    oss << fileName << "." << indexNo;

    // On va stocker des couples (attr,RID)
    // on verifie qu'un couple n'est pas plus grand qu'une page
    if (attrLength + (int)sizeof(RID) > PF_PAGE_SIZE)
        return IX_IDXCREATEFAIL;

    // Sanity check, no check for string
    switch(attrType) {
    	case INT:
    		if (attrLength != sizeof(int)) return IX_IDXCREATEFAIL;
    		break;
    	case FLOAT:
    		if (attrLength != sizeof(float)) return IX_IDXCREATEFAIL;
    		break;
    }

    // Creation de la page
    rc = pf_manager.CreateFile(oss.str().c_str());
    if (rc) return rc;

    // Ouverture du fichier pour creer l'en-tete
    PF_FileHandle pf_filehandle;
    rc = pf_manager.OpenFile(oss.str().c_str(), pf_filehandle);
    if (rc) return rc;

    // Creation de l'en-tete
    IX_FileHeader ix_fileheader;
    ix_fileheader.tailleCle = attrLength;
    ix_fileheader.taillePtr = sizeof(PageNum);
    ix_fileheader.numRacine = -1;
    ix_fileheader.type = attrType;

    // TODO

    // Creation de la premiere page
    PF_PageHandle pf_pagehandle;
    rc = pf_filehandle.AllocatePage(pf_pagehandle);
    if (rc) return rc;

    // Recuperation du pointeur de donnees
    char* pData;
    rc = pf_pagehandle.GetData(pData);
    if (rc) return rc;

    // Ecriture du header
    memcpy(pData, &ix_fileheader, sizeof(IX_FileHeader));

    // La page est dirty
    PageNum pageNum;
    rc = pf_pagehandle.GetPageNum(pageNum);
    if (rc) return rc;

    rc = pf_filehandle.MarkDirty(pageNum);
    if (rc) return rc;

    // Unpin
    rc = pf_filehandle.UnpinPage(pageNum);
    if (rc) return rc;

    // Force l'ecriture des pages
    rc = pf_filehandle.ForcePages();
    if (rc) return rc;

    // Fermeture du fichier
    rc = pf_manager.CloseFile(pf_filehandle);

    return rc;
}

RC IX_Manager::DestroyIndex(const char* fileName, int indexNo) {
	ostringstream oss;
	oss << fileName << "." << indexNo;
    return pf_manager.DestroyFile(oss.str().c_str());
}

RC IX_Manager::OpenIndex(const char* fileName, int indexNo, IX_IndexHandle &indexHandle) {
    RC rc;

    ostringstream oss;
    oss << fileName << "." << indexNo;

    // on verifie si le handle est deja ouvert
    if (indexHandle.bFileOpen) return IX_FILEOPEN;

    // ouverture du fichier
    PF_FileHandle pf_filehandle;
    rc = pf_manager.OpenFile(oss.str().c_str(), pf_filehandle);
    if (rc) return rc;

    // le fichier est ouvert
    indexHandle.bFileOpen = true;

    // on associe le pf filehandle au ix indexhandle
    indexHandle.pf_filehandle = new PF_FileHandle(pf_filehandle);

    // on recupere le header et on le stocke dans le indexhandle
    PF_PageHandle pf_pagehandle;
    rc = pf_filehandle.GetThisPage(0, pf_pagehandle);
    if (rc) return rc;

    // Unpin
    rc = pf_filehandle.UnpinPage(0);
    if (rc) return rc;

    char* pData;
    rc = pf_pagehandle.GetData(pData);
    if (rc) return rc;

    memcpy(&(indexHandle.ix_fileheader), pData, sizeof(IX_FileHeader));

    return 0;
}

RC IX_Manager::CloseIndex(IX_IndexHandle &indexHandle) {
    RC rc;

    // le fichier est-il deja ferme?
    if (!indexHandle.bFileOpen) return IX_FILECLOSED;

    // on reecrit le header
    PF_PageHandle pf_pagehandle;
    rc = indexHandle.pf_filehandle->GetThisPage(0, pf_pagehandle);
    if (rc) return rc;

    char* pData;
    rc = pf_pagehandle.GetData(pData);
    if (rc) return rc;

    memcpy(pData, &indexHandle.ix_fileheader, sizeof(IX_FileHeader));

    rc = indexHandle.pf_filehandle->MarkDirty(0);
    if (rc) return rc;

    rc = indexHandle.pf_filehandle->UnpinPage(0);
    if (rc) return rc;

    rc = indexHandle.ForcePages();
    if (rc) return rc;

    // le fichier est clos
    indexHandle.bFileOpen = false;

    if(indexHandle.pf_filehandle != NULL)
    	delete(indexHandle.pf_filehandle);

    return 0;
}
