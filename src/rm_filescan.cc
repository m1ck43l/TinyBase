#include "rm.h"
#include "rm_rid.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <iostream>

RM_FileScan::RM_FileScan()
{
  bOpen = false; //Le scan n'est pas encore ouvert
  val = NULL;
  rm_filehandle = NULL;
}

RM_FileScan::~RM_FileScan()
{
}

RC RM_FileScan::OpenScan(const RM_FileHandle &fileHandle,
			  AttrType   attrType,
			  int        attrLength,
			  int        attrOffset,
			  CompOp     compOp,
			  void       *value,
			  ClientHint pinHint)
{
  //Si le scan est déjà ouvert on ne le réouvre pas
  if (bOpen) return RM_SCANOPEN;

  //On initialise ensuite toutes les variables dont on aura besoin
  //dans la méthode GetNextRec
  rm_filehandle = const_cast<RM_FileHandle*>(&fileHandle);
  type = attrType;
  length = attrLength;
  offset = attrOffset;
  op = compOp;
  val = value;

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

  //On met le numéro de la première page à part le header
  PF_PageHandle pfph;
  int rc;
  rc = rm_filehandle->pf_filehandle->GetFirstPage(pfph);
  if (rc) {

    return rc;
  }

  PageNum pageNum;
  rc = pfph.GetPageNum(pageNum);
  if (rc) return rc;

  rc = rm_filehandle->pf_filehandle->UnpinPage(pageNum);
  if (rc) return rc;

  //On récupère le nombre de record par page
  char *pData;
  rc = pfph.GetData(pData);
  if (rc){
    return rc;
  }

  memcpy(&rmfh,pData,sizeof(RM_FileHeader));
  numMaxRec = rmfh.numberRecords;
  numCurSlot = 0;

  rc = fileHandle.pf_filehandle->GetNextPage(0, pfph); //On obtient la 1ere page après le header
  if (rc == PF_EOF) numCurPage = -1; //On considère qu'il n'y a pas de page
  else if ((rc != 0) && (rc != PF_EOF)) {
    return rc; //S'il y a d'autres erreurs
  }

  // Si numCurPage = -1 alors on ne UnPin pas la page...
  if (numCurPage != -1) {
	rc = pfph.GetPageNum(numCurPage);
	if (rc){
	  return rc;
	}

	rc = fileHandle.pf_filehandle->UnpinPage(numCurPage);
	if (rc) return rc;
  }

  //On a réussi à ouvrir le scan
  bOpen = true;

  return 0;

}

RC RM_FileScan::GetNextRec(RM_Record &record)
{
  //Si le scan n'est pas ouvert on ne peut pas prendre le prochain record
  if (!bOpen) return RM_SCANCLOSED;

  RID rid; //Contiendra le rid courant
  int rc; //Résultat
  int trouve = false;

  //Tant qu'on a pas trouvé de bon record, on continue
  while (!trouve) {

    //On prend le prochain Rid disponible dans le fichier
    //Si on est à la fin du fichier il n'y aura pas de next record
    //Et on sort quand même de la boucle car rc vaudra RM_EOF
    rc = GetNextRID(rid);
    if (rc) return rc;

    //On récupère le record associé
    rc = rm_filehandle->GetRec(rid, record);
    if (rc) return rc;

    //Si le record correspond aux conditions, on le place dans rec
    char *pData;
    rc = record.GetData(pData);
    if (rc) return rc;

    if (ConditionOK(pData)) {
      trouve = true;
    }
  }

  //Si on est sorti de la boucle, c'est qu'on a trouvé un record

  return 0;

}

RC RM_FileScan::CloseScan()
{
  //Il suffit juste de fermer le scan, les variables seront
  //réinitialisées si on réouvre le scan
  //Ce n'est même pas une erreur de fermer un scan déjà fermé
  bOpen = false;

  return 0;
}

bool RM_FileScan::ConditionOK(char *pData)
{
  //Si on a val = NULL alors tous les records sont satisfaisants
  if (val == NULL) return true;

  //Sinon on fait les tests en fonction du type de value
  switch (type) {

  case INT : {
    //On prend la valeur du record
    char* i = pData + offset;
    //Et on switch selon l'opération voulue

    switch (op) {

    case EQ_OP : {
      if (*((int*)i) == valInt) return true;
      break;
    }
    case LT_OP : {
      if (*((int*)i) < valInt) return true;
      break;
    }
    case GT_OP : {
      if (*((int*)i) > valInt) return true;
      break;
    }
    case LE_OP : {
      if (*((int*)i) <= valInt) return true;
      break;
    }
    case GE_OP : {
      if (*((int*)i) >= valInt) return true;
      break;
    }
    case NE_OP : {
      if (*((int*)i) != valInt) return true;
      break;
    }
    case NO_OP : {
      return true;
      break;
    }

    }//Fin du switch(op) sur INT

    break;
  }//Fin du case INT

  case FLOAT : {
    //On récupère la valeur du record
    char* f = pData + offset;

    switch (op) {

    case EQ_OP : {
      if (*((float*)f) == valFloat) return true;
      break;
    }
    case LT_OP : {
      if (*((float*)f) < valFloat) return true;
      break;
    }
    case GT_OP : {
      if (*((float*)f) > valFloat) return true;
      break;
    }
    case LE_OP : {
      if (*((float*)f) <= valFloat) return true;
      break;
    }
    case GE_OP : {
      if (*((float*)f) >= valFloat) return true;
      break;
    }
    case NE_OP : {
      if (*((float*)f) != valFloat) return true;
      break;
    }
    case NO_OP : {
      return true;
      break;
    }

    }//Fin du switch(op) sur FLOAT

    break;
  }//Fin du case FLOAT

  case STRING : {
    //On récupère la valeur du record
    char *str = &pData[offset];
    //On va utiliser strncmp en tenant compte de la longueur pour ne pas aller trop loin en mémoire

    switch (op) {

    case EQ_OP : {
      if (strncmp(str, valString, length) == 0) return true;
      break;
    }
    case LT_OP : {
      if (strncmp(str, valString, length) < 0) return true;
      break;
    }
    case GT_OP : {
      if (strncmp(str, valString, length) > 0) return true;
      break;
    }
    case LE_OP : {
      if (strncmp(str, valString, length) <= 0) return true;
      break;
    }
    case GE_OP : {
      if (strncmp(str, valString, length) >= 0) return true;
      break;
    }
    case NE_OP : {
      if (strncmp(str, valString, length) != 0) return true;
      break;
    }
    case NO_OP : {
      return true;
      break;
    }

    }//Fin du switch(op) sur STRING
    break;
  }//Fin du case STRING

  }//Fin du switch(type)

  //Si on arrive ici, c'est que la condition n'est pas satisfaite
  return false;
}

RC RM_FileScan::GetNextRID(RID &rid)
{
  //On va passer en revue tous les records présents dans le fichier
  int rc; //Résultat

  if (numCurPage < 0) {//num vaut donc -1, il n'y a pas de page suivante
    return RM_EOF; //Il n'y a donc plus de rid
  }
  //Sinon il y a au moins une page

  bool trouve = false;
  PF_PageHandle pfph;// = new PF_PageHandle();
  char *pData;

  while (!trouve) {

    //On prend la page courante
    rc = rm_filehandle->pf_filehandle->GetThisPage(numCurPage, pfph);
    if (rc) {
      return rc;
    }

    rc = rm_filehandle->pf_filehandle->UnpinPage(numCurPage);
    if (rc) {
      return rc;
    }

    rc = pfph.GetData(pData);
    if (rc){
      return rc;
    }

    RM_PageHeader rmph(pData, rmfh.numberRecords);

    while((numCurSlot < numMaxRec) && (!trouve)) { //Tant que notre slot ne sors pas de la page
      if(rmph.getBitmap()->checkSlot(numCurSlot) == OK_RC) {//Le record existe donc bien
	rid.slotNum = numCurSlot;
	rid.pageNum = numCurPage;
	rid.bIsValid = true;
	trouve = true;
      }
      numCurSlot++;
    }

    if(!trouve) { //On est donc dans le cas numCurSlot = numMaxRec
      //On va sur la prochaine page
      rc = rm_filehandle->pf_filehandle->GetNextPage(numCurPage, pfph);
      if (rc == PF_EOF) {
	return RM_EOF; //On a tout parcouru
      }

      rc = pfph.GetPageNum(numCurPage);
      if (rc) {

	return rc;
      }

      rc = rm_filehandle->pf_filehandle->UnpinPage(numCurPage);
      if (rc) {

	return rc; //Si c'est une autre erreur
      }

      numCurSlot = 0;
    }
  }

  //Si on arrive ici c'est que l'on a trouvé un rid
  return 0;

}
