//
// File:        SM component stubs
// Description: Print parameters of all SM_Manager methods
// Authors:     Dallan Quass (quass@cs.stanford.edu)
//

#include <cstdio>
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

