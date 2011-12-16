#include "rm.h"
#include "pf.h"
#include "rm_internal.h"

RM_Manager::RM_Manager(PF_Manager &pfm)
{
  //Initialisation des variables, pfm ayant déjà été instancié avant dans le programme
  pf_manager = &pfm;
  pf_filehandle = new PF_FileHandle();
  pf_pagehandle = new PF_PageHandle();
}

RM_Manager::~RM_Manager()
{
  //On libère les instances de PF_FileHandle et de PF_PageHandle créées avec le constructeur
  delete pf_filehandle;
  delete pf_pagehandle;
  pf_filehandle = NULL;
  pf_pagehandle = NULL;
}

//Cette méthode crée un fichier et crée la page d'entête
RC RM_Manager::CreateFile(const char *fileName, int recordSize)
{ 
  int rc; //Résultat
  PageNum pageNum; //numéro de la page que l'on va créer
  
  rc = pf_manager->CreateFile(fileName);
  
  //Si la création du fichier ne s'est pas bien déroulée, on renvoie tout de suite le rc
  if (rc) return rc;
  
  //On doit ouvrir le fichier pour écrire la page d'entête
  rc = pf_manager->OpenFile(fileName, pf_filehandle);
  
  //On vérifie si l'ouverture s'est bien passée
  if (rc) return rc;
  
  //On crée l'entête
  RM_FileHeader rm_fileheader;
  
  rm_fileheader.recordSize = recordSize;
  
  //On définit le nombre de record que l'on peut mettre dans la page, sans compter l'entête.
  //Il faut d'abord vérifier que l'on peut mettre au moins un record dans une page
  if (recordSize > (PF_PAGE_SIZE - sizeof(RM_PageHeader))) return RM_RECORD_TOO_LONG; //code erreur à ajouter
  else {
    rm_fileheader.numberRecords = ((PF_PAGE_SIZE - sizeof(RM_PageHeader)) / recordSize);
  }
  
  rm_fileheader.nextFreePage = -1; //Quand on ne pointe vers aucune page, on donne le numéro -1
  rm_fileheader.nextFullPage = -1;
  rm_fileheader.numberPages = 1;
  
  //On doit maintenant insérer ce header dans la première page, on crée donc la première page
  rc = pf_filehandle->AllocatePage(pf_pagehandle);
  
  //On vérifie qu'il n'y a pas eu d'erreur
  if (rc) return rc;
  
  
  //Il faut maintenant écrire dans cette page l'entête
  //Je ne sais pas vraiment comment faire
  //Ma seule idée est d'écrire dans le fichier la struct mais à partir de où?
  //Il faudrait donc décaler le pData de sizeof(RM_FileHeader)
  
  //Quand on a fini d'écrire le header, on unpin la page
  rc = pf_pagehandle->GetPageNum(&pageNum);
  if (rc) return rc;
  
  rc = pf_filehandle->UnpinPage(pageNum);
  if (rc) return rc; 
  
  //Il nous reste maintenant à fermer le fichier. Si cela ce passe bien ici (rc = 0) c'est que tout s'est bien passé avant
  return pf_manager->CloseFile(pf_filehandle);
}

RC RM_Manager::DestroyFile(const char *fileName) {
  
  //Il n'y a qu'à détruire le fichier avec la fonction de PF_Manager, il n'y a rien à vérifier
  return pf_manager->DestroyFile(fileName);
  
}

RC RM_Manager::OpenFile(const char *fileName, RM_FileHandle &fileHandle) {
  
  //On vérifie si le fileHandle n'est pas déjà ouvert
  if (fileHandle.bFileOpen) return RM_FILE_OPEN;

  rc = pf_manager->OpenFile(fileName, pf_filehandle);
  if (rc) return rc;

  //On associe ensuite le pf_filehandle à celui de rm
  fileHandle.pf_filehandle = pf_filehandle;

  //Tout s'est bien passé
  return 0;

}

RC RM_Manager::CloseFile(RM_FileHandle &fileHandle) {
  
  int rc; //Résultat

  rc = pf_manager->CloseFile(fileHandle.pf_filehandle);

  //On supprime ensuite l'objet pointé par le rm_filehandle
  delete &fileHandle;
  return rc;

}
