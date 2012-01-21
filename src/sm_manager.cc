//
// File:        SM component stubs
// Description: Print parameters of all SM_Manager methods
// Authors:     Dallan Quass (quass@cs.stanford.edu)
//

#include <cstdio>
#include <cstddef>
#include <iostream>
#include "redbase.h"
#include "sm.h"
#include "ix.h"
#include "rm.h"

using namespace std;

SM_Manager::SM_Manager(IX_Manager &ixm, RM_Manager &rmm):ixm(ixm), rmm(rmm)
{
    bIsOpen = false;
}

SM_Manager::~SM_Manager()
{
}

RC SM_Manager::OpenDb(const char *dbName)
{
    RC rc;

    if(dbName == NULL)
        return SM_OPENFAILED;

    if(bIsOpen)
        return SM_ALREADYOPEN;

    // On change le repertoire courant
    if(chdir(dbName)<0)
        return SM_DBNOTFOUND;

    // On ouvre les catalogues
    rc = rmm.OpenFile("relcat", relcat_fh);
    if(rc) return rc;

    rc = rmm.OpenFile("attrcat", attrcat_fh);
    if(rc) return rc;

    // La base est ouverte
    bIsOpen = true;

    return (0);
}

RC SM_Manager::CloseDb()
{
    RC rc;

    if(!bIsOpen)
        return SM_DBNOTOPEN;

    rc = rmm.CloseFile(relcat_fh);
    if(rc) return rc;

    rc = rmm.CloseFile(attrcat_fh);
    if(rc) return rc;

    bIsOpen = false;

    // On retourne au bon path
    rc = chdir("..");

    return (0);
}

RC SM_Manager::CreateTable(const char *relName,
                           int        attrCount,
                           AttrInfo   *attributes)
{
    RC rc;
    RID rid;

    // Sanity check
    if (!bIsOpen)
        return SM_DBNOTOPEN;

    if (relName == NULL || strcmp(relName, "relcat") == 0 || strcmp (relName, "attrcat"))
        return SM_BADTABLE;

    if (attrCount < 1 || attrCount > MAXATTRS)
        return SM_INVALIDATTRNB;

    cout << "CreateTable\n"
         << "   relName     =" << relName << "\n"
         << "   attrCount   =" << attrCount << "\n";

    int recordLength = 0;
    for (int i = 0; i < attrCount; i++) {
        // On ajoute chacun des attributes ds le catalogue
        AttrCat newAttr;
        newAttr.offset = recordLength;
        newAttr.attrType = attributes[i].attrType;
        newAttr.attrLength = attributes[i].attrLength;
        newAttr.indexNo = -1;
        strcpy(newAttr.relName, relName);
        strcpy(newAttr.attrName, attributes[i].attrName);
        rc = attrcat_fh.InsertRec((char*)&newAttr, rid);
        if(rc) return rc;

        cout << "   attributes[" << i << "].attrName=" << attributes[i].attrName
             << "   attrType="
             << (attributes[i].attrType == INT ? "INT" :
                 attributes[i].attrType == FLOAT ? "FLOAT" : "STRING")
             << "   attrLength=" << attributes[i].attrLength << "\n";
        recordLength += attributes[i].attrLength;
    }

    // On ajoute la relation au catalogue
    RelCat newRel;
    newRel.recordLength = recordLength;
    newRel.attrNb = attrCount;
    newRel.pageNb = 1;
    newRel.recordNb = 0;
    strcpy(newRel.relName, relName);
    rc = relcat_fh.InsertRec((char*)&newRel, rid);
    if(rc) return rc;

    // On cre le nouveau fichier
    rc = rmm.CreateFile(relName, recordLength);
    if(rc) return rc;

    return (0);
}

RC SM_Manager::GetRelTpl(const char* relName, RelCat& relTpl, RID& rid) const {
    RC rc;

    if(relName == NULL)
        return SM_BADTABLE;

    RM_FileScan fs;
    if ((rc=fs.OpenScan(relcat_fh, STRING, MAXNAME+1, offsetof(RelCat, relName),
            EQ_OP, (void*) relName, NO_HINT)))
        return rc;

    RM_Record rec;
    rc = fs.GetNextRec(rec);
    if(rc == RM_EOF) return SM_NOTBLFOUND;
    else if(rc) return rc;

    if ((rc=fs.CloseScan()))
        return (rc);

    // On a trouve la relation
    char* pData;
    rc = rec.GetData(pData);
    if(rc) return rc;

    memcpy(&relTpl, pData, sizeof(RelCat));
    rec.GetRid(rid);

    return 0;
}

RC SM_Manager::GetAttrTpl(const char* relName, const char* attrName, AttrCat& attrTpl, RID& rid) const {
    RC rc;

    if (relName == NULL || attrName == NULL)
        return SM_BADTABLE;

    RM_FileScan fs;
    RM_Record rec;
    if ((rc=fs.OpenScan(attrcat_fh, STRING, MAXNAME+1, offsetof(AttrCat, relName),
            EQ_OP, (void*) relName, NO_HINT)))
        return rc;

    AttrCat *attrTmp;
    int n;
    bool found = false;
    for (rc = fs.GetNextRec(rec), n = 0;
         rc == 0;
         rc = fs.GetNextRec(rec), n++) {

        rc = rec.GetData((char*&)attrTmp);
        if(strcmp(attrName, attrTmp->attrName) == 0) {
            found = true;
            break;
        }
    }

    if (rc != RM_EOF && rc != 0)
        return rc;

    if ((rc=fs.CloseScan()))
        return rc;

    if (!found)
        return SM_BADATTR;

    memcpy(&attrTpl, attrTmp, sizeof(AttrCat));
    rec.GetRid(rid);

    return 0;
}

RC SM_Manager::DropTable(const char *relName)
{
    cout << "DropTable\n   relName=" << relName << "\n";
    return (0);
}

RC SM_Manager::CreateIndex(const char *relName,
                           const char *attrName)
{
    cout << "CreateIndex\n"
         << "   relName =" << relName << "\n"
         << "   attrName=" << attrName << "\n";
    return (0);
}

RC SM_Manager::DropIndex(const char *relName,
                         const char *attrName)
{
    cout << "DropIndex\n"
         << "   relName =" << relName << "\n"
         << "   attrName=" << attrName << "\n";
    return (0);
}

RC SM_Manager::Load(const char *relName,
                    const char *fileName)
{
    cout << "Load\n"
         << "   relName =" << relName << "\n"
         << "   fileName=" << fileName << "\n";
    return (0);
}

RC SM_Manager::Print(const char *relName)
{
    cout << "Print\n"
         << "   relName=" << relName << "\n";
    return (0);
}

RC SM_Manager::Set(const char *paramName, const char *value)
{
    cout << "Set\n"
         << "   paramName=" << paramName << "\n"
         << "   value    =" << value << "\n";
    return (0);
}

RC SM_Manager::Help()
{
    cout << "Help\n";
    return (0);
}

RC SM_Manager::Help(const char *relName)
{
    cout << "Help\n"
         << "   relName=" << relName << "\n";
    return (0);
}

void SM_PrintError(RC rc)
{
    cout << "SM_PrintError\n   rc=" << rc << "\n";
}

