#include "rm.h"
#include "pf.h"
#include "rm_internal.h"
#include <cstring>

RM_Manager::RM_Manager(PF_Manager &pfm) : pf_manager(pfm)
{

}

RM_Manager::~RM_Manager()
{
  //Il n'y a rien à libérer, on s'est assuré qu'avant chaque sortie de méthode
  //nous libérions la mémoire des variables
}

//Cette méthode crée un fichier et crée la page d'entête
RC RM_Manager::CreateFile(const char *fileName, int recordSize)
{
  int rc; //Résultat
  PageNum pageNum; //numéro de la page que l'on va créer
  char* pData; //Début des données

  rc = pf_manager.CreateFile(fileName);

  //Si la création du fichier ne s'est pas bien déroulée, on renvoie tout de suite le rc
  if (rc) return rc;

  //On doit ouvrir le fichier pour écrire la page d'entête
  PF_FileHandle *pf_filehandle = new PF_FileHandle();
  rc = pf_manager.OpenFile(fileName, *pf_filehandle);

  //On vérifie si l'ouverture s'est bien passée
  if (rc) {
    delete pf_filehandle; //On libère la mémoire qui n'aura finalement pas été utilisée
    return rc;
  }

  //On crée l'entête
  RM_FileHeader rm_fileheader;

  rm_fileheader.recordSize = recordSize;

  int numberRecords = 8 * (PF_PAGE_SIZE - 2*sizeof(int) - sizeof(char)) / (8*recordSize + 1); // la taille du bitmap depend du nombre de records d'ou l'equation
  if (numberRecords == 0) return RM_RECORDTOOLONG;
  else {
    rm_fileheader.numberRecords = numberRecords;
  }

  rm_fileheader.nextFreePage = -1; //Quand on ne pointe vers aucune page, on donne le numéro -1
  rm_fileheader.nextFullPage = -1;
  rm_fileheader.numberPages = 1;

  //On doit maintenant insérer ce header dans la première page, on crée donc la première page
  PF_PageHandle *pf_pagehandle = new PF_PageHandle();
  rc = pf_filehandle->AllocatePage(*pf_pagehandle);

  //On vérifie qu'il n'y a pas eu d'erreur sinon on libère la mémoire
  if (rc) {
    delete pf_pagehandle;
    delete pf_filehandle;
    return rc;
  }

  //On récupère l'adresse du début des données de la page
  rc = pf_pagehandle->GetData(pData);
  if (rc) {
    delete pf_pagehandle;
    delete pf_filehandle;
    return rc;
  }

  //On copie en mémoire le fileheader
  memcpy(&pData[0], &rm_fileheader, sizeof(RM_FileHeader));

  //On marque la page comme dirty
  rc = pf_pagehandle->GetPageNum(pageNum);

  //On n'a plus besoin du pagehandle
  delete pf_pagehandle;

  if (rc) {
    delete pf_filehandle;
    return rc;
  }

  rc = pf_filehandle->MarkDirty(pageNum);
  if (rc) {
    delete pf_filehandle;
    return rc;
  }

  //On unpin la page
  rc = pf_filehandle->UnpinPage(pageNum);
  if (rc) {
    delete pf_filehandle;
    return rc;
  }

  //On force les pages
  rc = pf_filehandle->ForcePages(ALL_PAGES);

  if (rc){
    delete pf_filehandle;
    return rc;
  }

  //Il nous reste maintenant à fermer le fichier.
  rc = pf_manager.CloseFile(*pf_filehandle);

  //On libère filehandle
  delete pf_filehandle;

  return rc;
}

RC RM_Manager::DestroyFile(const char *fileName) {

  //Il n'y a qu'à détruire le fichier avec la fonction de PF_Manager, il n'y a rien à vérifier
  return pf_manager.DestroyFile(fileName);

}

RC RM_Manager::OpenFile(const char *fileName, RM_FileHandle &fileHandle) {

  int rc; // Résultat

  //On vérifie si le fileHandle n'est pas déjà ouvert
  if (fileHandle.bFileOpen) return RM_FILEOPEN;

  PF_FileHandle *pf_filehandle = new PF_FileHandle();

  rc = pf_manager.OpenFile(fileName, *pf_filehandle);
  if (rc) {
    delete pf_filehandle;
    return rc;
  }

  //Le fichier est bien ouvert
  fileHandle.bFileOpen = TRUE;

  //On associe ensuite le pf_filehandle à celui de rm, on est obligé de créer une
  //Nouvelle instance sinon la variable pf_filehandle est détruite à la fin de
  //l'exécution de la méthode, il ne faudra pas oublier de libérer le pf_filehandle
  //du RM_filehandle
  fileHandle.pf_filehandle = new PF_FileHandle(*pf_filehandle);

  PF_PageHandle pf_ph;
  rc = pf_filehandle->GetThisPage(0, pf_ph);
  if(rc) return rc;
  pf_filehandle->UnpinPage(0);

  char* pData;
  rc = pf_ph.GetData(pData);
  if(rc) return rc;
  memcpy(&(fileHandle.rm_fileheader), pData, sizeof(RM_FileHeader));

  //Tout s'est bien passé
  delete pf_filehandle;
  return 0;

}

RC RM_Manager::CloseFile(RM_FileHandle &fileHandle) {

  int rc; //Résultat

  //On vérifie si le fichier n'est pas déjà fermé
  if (!fileHandle.bFileOpen) return RM_FILECLOSED;

  rc = fileHandle.ForcePages(ALL_PAGES);
  if (rc) return rc;

  rc = pf_manager.CloseFile(*(fileHandle.pf_filehandle));
  if (rc) return rc;

  fileHandle.bFileOpen = FALSE;

  return 0;

}
