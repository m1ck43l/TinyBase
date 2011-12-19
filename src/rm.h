//
// rm.h
//
//   Record Manager component interface
//
// This file does not include the interface for the RID class.  This is
// found in rm_rid.h
//

#ifndef RM_H
#define RM_H

// Please DO NOT include any files other than redbase.h and pf.h in this
// file.  When you submit your code, the test program will be compiled
// with your rm.h and your redbase.h, along with the standard pf.h that
// was given to you.  Your rm.h, your redbase.h, and the standard pf.h
// should therefore be self-contained (i.e., should not depend upon
// declarations in any other file).

// Do not change the following includes
#include "redbase.h"
#include "rm_rid.h"
#include "pf.h"

#include "rm_internal.h"

//
// RM_Record: RM Record interface
//
class RM_Record {
    // Both need to be friend so that we can set the private fields
    // directly in these classes
    friend class RM_FileHandle;
    friend class RM_FileScan;

public:
    RM_Record ();
    ~RM_Record();

    // Return the data corresponding to the record.  Sets *pData to the
    // record contents.
    RC GetData(char *&pData) const;

    // Return the RID associated with the record
    RC GetRid (RID &rid) const;

private:
    char *pData;
    RID rid;
    bool bIsValid;
};

//
// RM_FileHandle: RM File interface
//
class RM_FileHandle {
  //RM_Manager et RM_FileScan ont besoin des attributs de RM_FileHandle
  friend class RM_Manager;
  friend class RM_FileScan;
public:
    RM_FileHandle ();
    ~RM_FileHandle();

    // Given a RID, return the record
    RC GetRec     (const RID &rid, RM_Record &rec) const;

    RC InsertRec  (const char *pData, RID &rid);       // Insert a new record

    RC DeleteRec  (const RID &rid);                    // Delete a record
    RC UpdateRec  (const RM_Record &rec);              // Update a record

    // Forces a page (along with any contents stored in this class)
    // from the buffer pool to disk.  Default value forces all pages.
    RC ForcePages (PageNum pageNum = ALL_PAGES) const;
private:
    PF_FileHandle *pf_filehandle;
    RM_FileHeader rm_fileheader;
    bool bFileOpen;

    RC WritePageHeader(PF_PageHandle, RM_PageHeader& );
    int CountFreeSlots(RM_PageHeader&);
    RC GetNextFreeRid(RID &rid);
};

//
// RM_FileScan: condition-based scan of records in the file
//
class RM_FileScan {
public:
    RM_FileScan  ();
    ~RM_FileScan ();

    RC OpenScan  (const RM_FileHandle &fileHandle,
                  AttrType   attrType,
                  int        attrLength,
                  int        attrOffset,
                  CompOp     compOp,
                  void       *value,
                  ClientHint pinHint = NO_HINT); // Initialize a file scan
    RC GetNextRec(RM_Record &rec);               // Get next matching record
    RC CloseScan ();                             // Close the scan
private:
    bool bOpen; //Savoir si le Scan ouvert
    bool ConditionOK(char *pData); //Renvoie vrai si le record satisfait les conditions demandées

    RM_FileHandle rm_filehandle;
    RM_FileHeader rmfh;
    RC GetNextRID(RID &rid);

    //On crée une copie de tous les paramètres pour les utilise
    void *val; //Contiendra la valeur que l'on devra comparer
               //C'est un void* car on ne connait pas encore le type de value
    AttrType type;
    int length;
    int offset;
    CompOp op;

    //On ajoute des variables qui serviront à éviter trop de cast
    int valInt;
    float valFloat;
    char *valString; //C'est un char* pour pouvoir utiliser strncmp

    PageNum numCurPage; //Numéro de la page courante
    int numCurSlot; //Numéro du slot sourant
    int numMaxRec; //Nombre max de record par page
};

//
// RM_Manager: provides RM file management
//
class RM_Manager {
public:
    RM_Manager    (PF_Manager &pfm);
    ~RM_Manager   ();

    RC CreateFile (const char *fileName, int recordSize);
    RC DestroyFile(const char *fileName);
    RC OpenFile   (const char *fileName, RM_FileHandle &fileHandle);

    RC CloseFile  (RM_FileHandle &fileHandle);
private:
    PF_Manager pf_manager;
};

//
// Print-error function
//
void RM_PrintError(RC rc);

#define RM_INVALIDRECORD    (START_RM_WARN + 0) // record has not yet been read
#define RM_FILEOPEN         (START_RM_WARN + 1) // File is already opened
#define RM_FILECLOSED       (START_RM_WARN + 2) // File is already closed
#define RM_SCANOPEN         (START_RM_WARN + 3) // Scan is already opened
#define RM_SCANCLOSED       (START_RM_WARN + 4) // Scan is already closed
#define RM_EOF              (START_RM_WARN + 5) // There is no more Record in file
#define RM_ATTRTOLONG       (START_RM_WARN + 6) // Attribute too long
#define RM_RECNOTFOUND      (START_RM_WARN + 7) // Record not found
#define RM_LASTWARN         RM_RECNOTFOUND
#define RM_FILENOTFREE      (START_RM_WARN + 8) // File is not free

#define RM_INVALIDRID       (START_RM_ERR - 0) // RID invalid
#define RM_RECORDTOOLONG    (START_RM_ERR - 1) // Record too long to fit in one page
#define RM_LASTERROR        RM_RECORDTOOLONG

#endif
