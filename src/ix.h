//
// ix.h
//
//   Index Manager Component Interface
//

#ifndef IX_H
#define IX_H

// Please do not include any other files than the ones below in this file.

#include "redbase.h"  // Please don't change these lines
#include "rm_rid.h"  // Please don't change these lines
#include "pf.h"

//
// IX_FileHeader
//
typedef struct ix_fileheader {
    int tailleCle;
    int taillePtr;
    PageNum numRacine;
    AttrType type;
} IX_FileHeader;

//
// IX_NoeudHeader
//
typedef struct ix_noeudHeader {
    int niveau; // Niveau sert à savoir si l'on est une feuille ou un noeud interne, niveau=0 correspond à une feuille
    int nbMaxPtr;  //Nombre maximum de pointeurs dans le noeud, il y aura donc nbMaxPtr-1 clés au max dans le noeud
    int nbCle;//Nombre de clé actuellement dans le noeud (il y a donc nbCle+2 pointeurs)
    PageNum pageMere;
    PageNum prevPage;
    PageNum nextPage;
    
} IX_NoeudHeader;

//
// IX_BucketHeader
//
typedef struct ix_bucketheader {
    // Nombre actuel de RID dans le bucket
    int nbRid;
    // Nombre maximal de RIC pouvant être stockés dans le bucket
    int nbMax;
    PageNum nextBuck;
} IX_BucketHeader;

//
// IX_IndexHandle: IX Index File interface
//
class IX_IndexHandle {
    friend class IX_Manager;
    friend class IX_IndexScan;
    
public:
    IX_IndexHandle();
    ~IX_IndexHandle();

    // Insert a new index entry
    RC InsertEntry(void *pData, const RID &rid);

    // Delete a new index entry
    RC DeleteEntry(void *pData, const RID &rid);

    // Force index files to disk
    RC ForcePages();

    //Insère récursivement l'entrée dans la page pageNum
    RC Inserer(PageNum pageNum, void *pData, const RID &rid);

    //Fonctions permettant d'obtenir ou de modifier la clé ou le pointeur d'un noeud à la position pos
    void SetCle(PF_PageHandle &pf_ph, int pos, void *pData);
    void SetPtr(PF_PageHandle &pf_ph, int pos, PageNum pageNum);
    PageNum GetPtr(PF_PageHandle &pf_ph, int pos);
    void* GetCle(PF_PageHandle &pf_ph, int pos);

    //Fonction qui compare les clés selon les types
    int Compare(void* pData1, void*pData2);

    //Renvoie vrai si la clé est déjà dans la feuille
    bool CleExiste(PF_PageHandle &pf_ph, IX_NoeudHeader header, void *pData);

    //Methode d'insertion dans les feuilles selon l'existence ou non de la clé
    RC InsererFeuille(PageNum pageNum, void *pData, const RID &rid);
    RC InsererFeuilleExiste(PageNum pageNum, void *pData, const RID &rid);
    RC InsererNoeudInterne(PageNum pageNum, void *pData, PageNum numPageGauche, PageNum numPageDroite);
    RC InsererBucket(PageNum pageNum, const RID &rid);
    
    //Fonction qui change le lien de parenté dans l'arbe
    RC ChangerParent(PageNum pageNum, PageNum numParent);

private:
    bool bFileOpen;
    PF_FileHandle* pf_filehandle;
    IX_FileHeader ix_fileheader;
};

//
// IX_IndexScan: condition-based scan of index entries
//
class IX_IndexScan {
public:
    IX_IndexScan();
    ~IX_IndexScan();

    // Open index scan
    RC OpenScan(const IX_IndexHandle &indexHandle,
                CompOp compOp,
                void *value,
                ClientHint  pinHint = NO_HINT);

    // Get the next matching entry return IX_EOF if no more matching
    // entries.
    RC GetNextEntry(RID &rid);

    // Close index scan
    RC CloseScan();
    
    RC GetFirstRID(PageNum pageNum, RID &rid);
    RC GetNextRID(RID &rid);
    RC GetNextRIDinBucket(RID &rid);
    
    RC GetFirstBucket(PageNum pageNum, RID &rid);
    RC GetNextBucket(RID &rid);
    
    int Compare(void *pData1, void *pData2);
    PageNum GetPtr(PF_PageHandle &pf_ph, int pos);
    void* GetCle(PF_PageHandle &pf_ph, int pos);
    
private:
    bool bScanOpen;
    
    PF_FileHandle *pf_filehandle;

    RID currentRID;
    int currentPagePos;
    PageNum currentPageNum;
    PageNum currentBucketNum;
    bool emptyBucket;
    int currentRIDpos;
    
    //On crée une copie de tous les paramètres pour les utilise
    void *val; //Contiendra la valeur que l'on devra comparer
               //C'est un void* car on ne connait pas encore le type de value
    AttrType type;
    int length;
    int taillePtr;
    int tailleCle;
    CompOp op;
};

//
// IX_Manager: provides IX index file management
//
class IX_Manager {
public:
    IX_Manager(PF_Manager &pfm);
    ~IX_Manager();

    // Create a new Index
    RC CreateIndex(const char *fileName, int indexNo,
                   AttrType attrType, int attrLength);

    // Destroy and Index
    RC DestroyIndex(const char *fileName, int indexNo);

    // Open an Index
    RC OpenIndex(const char *fileName, int indexNo,
                 IX_IndexHandle &indexHandle);

    // Close an Index
    RC CloseIndex(IX_IndexHandle &indexHandle);
private:
    PF_Manager& pf_manager;

    RC ComputeFilename(const char*, int, const char*&);
};

//
// Print-error function
//
void IX_PrintError(RC rc);

#define IX_FILEOPEN             (START_IX_WARN + 0) // File already opened
#define IX_FILECLOSED           (START_IX_WARN + 1) // File already closed
#define IX_EOF                  (START_IX_WARN + 2) // End of file
#define IX_LASTWARN             IX_EOF

#define IX_IDXCREATEFAIL        (START_IX_ERR - 0) // Fail to create index file
#define IX_KEYNOTEXISTS         (START_IX_ERR - 1) // La clé devrait exister
#define IX_LASTERROR            IX_KEYNOTEXISTS
#endif
