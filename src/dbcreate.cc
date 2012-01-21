//
// dbcreate.cc
//
// Author: Jason McHugh (mchughj@cs.stanford.edu)
//
// This shell is provided for the student.

#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstddef>
#include <unistd.h>
#include "rm.h"
#include "sm.h"
#include "redbase.h"

using namespace std;

//
// DB_PrintError
//
void DB_PrintError(RC rc) {
    SM_PrintError(rc);
    exit(rc);
}

//
// main
//
int main(int argc, char *argv[])
{
    char *dbname;
    char command[255] = "mkdir ";
    RC rc;
    RID rid;

    // Look for 2 arguments. The first is always the name of the program
    // that was executed, and the second should be the name of the
    // database.
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " dbname \n";
        exit(1);
    }

    // The database name is the second argument
    dbname = argv[1];

    // Create a subdirectory for the database
    system (strcat(command,dbname));

    if (chdir(dbname) < 0) {
        cerr << argv[0] << " chdir error to " << dbname << "\n";
        exit(1);
    }

    // Create the system catalogs...

    // Link pf and rm
    PF_Manager pfm;
    RM_Manager rmm(pfm);

    // File handle for rel and attr
    RM_FileHandle relcat_fh, attrcat_fh;

    // Create and open relcat and attrecat files
    rc = rmm.CreateFile("relcat", sizeof(RelCat));
    if(rc) DB_PrintError(rc);

    rc = rmm.OpenFile("relcat", relcat_fh);
    if(rc) DB_PrintError(rc);

    rc = rmm.CreateFile("attrcat", sizeof(AttrCat));
    if(rc) DB_PrintError(rc);

    rc = rmm.OpenFile("attrcat", attrcat_fh);
    if(rc) DB_PrintError(rc);

    // Now we need to insert the two above relations in relcat
    RelCat relcat_tbl;
    relcat_tbl.recordLength = sizeof(RelCat);
    relcat_tbl.attrNb = 3;
    strcpy(relcat_tbl.relName, "relcat");
    rc = relcat_fh.InsertRec((char*)&relcat_tbl, rid);
    if(rc) DB_PrintError(rc);

    RelCat attrcat_tbl;
    attrcat_tbl.recordLength = sizeof(AttrCat);
    attrcat_tbl.attrNb = 6;
    strcpy(attrcat_tbl.relName, "attrcat");
    rc = relcat_fh.InsertRec((char*)&attrcat_tbl, rid);
    if(rc) DB_PrintError(rc);

    // Now we will insert all the attr in the attrcat relation
    AttrCat recordLength;
    recordLength.offset = offsetof(RelCat, recordLength);
    recordLength.attrType = INT;
    recordLength.attrLength = sizeof(int);
    recordLength.indexNo = -1;
    strcpy(recordLength.relName, "relcat");
    strcpy(recordLength.attrName, "recordLength");
    rc = attrcat_fh.InsertRec((char*)&recordLength, rid);
    if(rc) DB_PrintError(rc);

    AttrCat attrNb;
    attrNb.offset = offsetof(RelCat, attrNb);
    attrNb.attrType = INT;
    attrNb.attrLength = sizeof(int);
    attrNb.indexNo = -1;
    strcpy(attrNb.relName, "relcat");
    strcpy(attrNb.attrName, "attrNb");
    rc = attrcat_fh.InsertRec((char*)&attrNb, rid);
    if(rc) DB_PrintError(rc);

    AttrCat relName;
    relName.offset = offsetof(RelCat, relName);
    relName.attrType = STRING;
    relName.attrLength = MAXNAME+1;
    relName.indexNo = -1;
    strcpy(relName.relName, "relcat");
    strcpy(relName.attrName, "relName");
    rc = attrcat_fh.InsertRec((char*)&relName, rid);
    if(rc) DB_PrintError(rc);

    // Done with relcat attrs, do it for attrcat
    AttrCat offset;
    offset.offset = offsetof(AttrCat, offset);
    offset.attrType = INT;
    offset.attrLength = sizeof(int);
    offset.indexNo = -1;
    strcpy(offset.relName, "attrcat");
    strcpy(offset.attrName, "offset");
    rc = attrcat_fh.InsertRec((char*)&offset, rid);
    if(rc) DB_PrintError(rc);

    AttrCat attrType;
    attrType.offset = offsetof(AttrCat, attrType);
    attrType.attrType = INT;
    attrType.attrLength = sizeof(AttrType);
    attrType.indexNo = -1;
    strcpy(attrType.relName, "attrcat");
    strcpy(attrType.attrName, "attrType");
    rc = attrcat_fh.InsertRec((char*)&attrType, rid);
    if(rc) DB_PrintError(rc);

    AttrCat attrLength;
    attrLength.offset = offsetof(AttrCat, attrLength);
    attrLength.attrType = INT;
    attrLength.attrLength = sizeof(int);
    attrLength.indexNo = -1;
    strcpy(attrLength.relName, "attrcat");
    strcpy(attrLength.attrName, "attrLength");
    rc = attrcat_fh.InsertRec((char*)&attrLength, rid);
    if(rc) DB_PrintError(rc);

    AttrCat indexNo;
    indexNo.offset = offsetof(AttrCat, indexNo);
    indexNo.attrType = INT;
    indexNo.attrLength = sizeof(int);
    indexNo.indexNo = -1;
    strcpy(indexNo.relName, "attrcat");
    strcpy(indexNo.attrName, "indexNo");
    rc = attrcat_fh.InsertRec((char*)&indexNo, rid);
    if(rc) DB_PrintError(rc);

    relName.offset = offsetof(AttrCat, relName);
    relName.attrType = STRING;
    relName.attrLength = MAXNAME+1;
    relName.indexNo = -1;
    strcpy(relName.relName, "attrcat");
    strcpy(relName.attrName, "relName");
    rc = attrcat_fh.InsertRec((char*)&relName, rid);
    if(rc) DB_PrintError(rc);

    AttrCat attrName;
    attrName.offset = offsetof(AttrCat, attrName);
    attrName.attrType = STRING;
    attrName.attrLength = MAXNAME+1;
    attrName.indexNo = -1;
    strcpy(attrName.relName, "attrcat");
    strcpy(attrName.attrName, "attrName");
    rc = attrcat_fh.InsertRec((char*)&attrName, rid);
    if(rc) DB_PrintError(rc);

    // End of writing, close the files
    rc = rmm.CloseFile(relcat_fh);
    if(rc) DB_PrintError(rc);

    rc = rmm.CloseFile(attrcat_fh);
    if(rc) DB_PrintError(rc);

    return(0);
}
