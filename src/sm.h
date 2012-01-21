//
// sm.h
//   Data Manager Component Interface
//

#ifndef SM_H
#define SM_H

// Please do not include any other files than the ones below in this file.

#include <stdlib.h>
#include <string.h>
#include "redbase.h"  // Please don't change these lines
#include "parser.h"
#include "rm.h"
#include "ix.h"

//
// Catalog attr
//
typedef struct attr_cat {
    int offset;
    AttrType attrType;
    int attrLength;
    int indexNo;
    char relName[MAXNAME+1];
    char attrName[MAXNAME+1];
} AttrCat;

//
// Catalog Rel
//
typedef struct rel_cat {
    int recordLength;
    int attrNb;
    int pageNb; // nombre de pages de la relation
    int recordNb;
    char relName[MAXNAME+1];
} RelCat;

//
// SM_Manager: provides data management
//
class SM_Manager {
    friend class QL_Manager;
public:
    SM_Manager    (IX_Manager &ixm, RM_Manager &rmm);
    ~SM_Manager   ();                             // Destructor

    RC OpenDb     (const char *dbName);           // Open the database
    RC CloseDb    ();                             // close the database

    RC CreateTable(const char *relName,           // create relation relName
                   int        attrCount,          //   number of attributes
                   AttrInfo   *attributes);       //   attribute data
    RC CreateIndex(const char *relName,           // create an index for
                   const char *attrName);         //   relName.attrName
    RC DropTable  (const char *relName);          // destroy a relation

    RC DropIndex  (const char *relName,           // destroy index on
                   const char *attrName);         //   relName.attrName
    RC Load       (const char *relName,           // load relName from
                   const char *fileName);         //   fileName
    RC Help       ();                             // Print relations in db
    RC Help       (const char *relName);          // print schema of relName

    RC Print      (const char *relName);          // print relName contents

    RC Set        (const char *paramName,         // set parameter to
                   const char *value);            //   value

private:
    bool bIsOpen;
    IX_Manager& ixm;
    RM_Manager& rmm;

    RM_FileHandle relcat_fh, attrcat_fh;

    // Recupere un tuple de la table relcat
    RC GetRelTpl(const char* relName, RelCat& relTpl, RID& rid) const;

    // Recupere un tuple de la table attrcat
    RC GetAttrTpl(const char* relName, const char* attrName, AttrCat& attrTpl, RID& rid) const;
};

//
// Print-error function
//
void SM_PrintError(RC rc);

#define SM_OPENFAILED       (START_SM_ERR - 0) // Fail to open DB
#define SM_ALREADYOPEN      (START_SM_ERR - 1)
#define SM_DBNOTFOUND       (START_SM_ERR - 2)
#define SM_DBNOTOPEN        (START_SM_ERR - 3)
#define SM_INVALIDATTRNB    (START_SM_ERR - 4)
#define SM_BADTABLE         (START_SM_ERR - 5)
#define SM_NOTBLFOUND       (START_SM_ERR - 6)
#define SM_BADATTR          (START_SM_ERR - 7)
#define SM_LASTERROR        SM_BADTABLE

#endif
