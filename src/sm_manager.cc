//
// File:        SM component stubs
// Description: Print parameters of all SM_Manager methods
// Authors:     Dallan Quass (quass@cs.stanford.edu)
//

#include <cstdio>
#include <cstddef>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "redbase.h"
#include "sm.h"
#include "ix.h"
#include "rm.h"
#include "printer.h"

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

    if (relName == NULL || strcmp(relName, "relcat") == 0 || strcmp(relName, "attrcat") == 0)
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

    if (!bIsOpen)
        return SM_DBNOTOPEN;

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

    if (!bIsOpen)
        return SM_DBNOTOPEN;

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
    RC rc;
    RID rid;

    if (!bIsOpen)
        return SM_DBNOTOPEN;

    if (relName == NULL)
        return SM_BADTABLE;

    if (strcmp(relName, "attrcat") == 0 || strcmp(relName, "relcat") == 0)
        return SM_FORBIDDEN;

    // On recupere le tuple dans relcat
    RelCat currentRel;
    rc = GetRelTpl(relName, currentRel, rid);
    if(rc) return rc;

    // Destruction du fichier
    rc = rmm.DestroyFile(relName);
    if(rc) return rc;

    // Destruction du tuple
    rc = relcat_fh.DeleteRec(rid);
    if(rc) return rc;

    // On doit maintenant recuperer tous les attributs et les detruire avec leur index
    RM_FileScan fs;
    RM_Record rec;
    if ((rc=fs.OpenScan(attrcat_fh, STRING, MAXNAME+1, offsetof(AttrCat, relName),
            EQ_OP, (void*) relName, NO_HINT)))
        return rc;

    AttrCat *attrTmp;
    int n;
    for (rc = fs.GetNextRec(rec), n = 0;
         rc == 0;
         rc = fs.GetNextRec(rec), n++) {

        rc = rec.GetData((char*&)attrTmp);
        if(rc) return rc;

        if(attrTmp->indexNo != -1) {
            // un index existe donc on le destroy
            rc = DropIndex(relName, attrTmp->attrName);
            if(rc) return rc;
        }

        // Destruction du tuple
        // TODO --> verifier que le scan n'est pas casser a la suite de la suppression !
        rc = rec.GetRid(rid);
        if(rc) return rc;

        rc = attrcat_fh.DeleteRec(rid);
        if(rc) return rc;
    }

    if (rc != RM_EOF && rc != 0)
        return rc;

    if ((rc=fs.CloseScan()))
        return rc;

    cout << "DropTable\n   relName=" << relName << "\n";
    return (0);
}

RC SM_Manager::CreateIndex(const char *relName,
                           const char *attrName)
{
    RC rc;
    RID rid;

    if (!bIsOpen)
        return SM_DBNOTOPEN;

    if (relName == NULL || attrName == NULL)
        return SM_BADTABLE;

    AttrCat currentAttr;
    rc = GetAttrTpl(relName, attrName, currentAttr, rid);
    if(rc) return rc;

    if (currentAttr.indexNo != -1)
        return SM_IDXALRDYEXISTS;
    // on met a jour indexNo
    currentAttr.indexNo = currentAttr.offset;

    // Sinon on cre l'index
    rc = ixm.CreateIndex(relName, currentAttr.indexNo, currentAttr.attrType, currentAttr.attrLength);
    if(rc) return rc;

    // L'index est cree donc on reecrit le attr dans attrcat
    RM_Record rec;
    rec.Set((char*)&currentAttr, sizeof(AttrCat));
    rec.SetRID(rid);
    rc = attrcat_fh.UpdateRec(rec);
    if(rc) return rc;

    // Maintenant on doit remplir l'index

    // On ouvre l'index
    IX_IndexHandle ixih;
    rc = ixm.OpenIndex(relName, currentAttr.indexNo, ixih);
    if(rc) return rc;

    // On ouvre la relation
    RM_FileHandle rmfh;
    rc = rmm.OpenFile(relName, rmfh);
    if(rc) return rc;

    // On ouvre le scan
    RM_FileScan fs;
    if ((rc=fs.OpenScan(rmfh, currentAttr.attrType, currentAttr.attrLength, currentAttr.offset,
            NO_OP, NULL, NO_HINT)))
        return rc;

    int n;
    for (rc = fs.GetNextRec(rec), n = 0;
         rc == 0;
         rc = fs.GetNextRec(rec), n++) {

        // Chaque tuple trouve doit etre indexe
        char* pData;
        rc = rec.GetData(pData);
        if(rc) return rc;

        rc = ixih.InsertEntry(pData + currentAttr.offset, rid);
    }

    if (rc != RM_EOF && rc != 0)
        return rc;

    if ((rc=fs.CloseScan()))
        return rc;

    // on ferme le fichier
    rc = rmm.CloseFile(rmfh);
    if(rc) return rc;

    // on ferme l'index
    rc = ixm.CloseIndex(ixih);
    if(rc) return rc;

    cout << "CreateIndex\n"
         << "   relName =" << relName << "\n"
         << "   attrName=" << attrName << "\n";
    return (0);
}

RC SM_Manager::DropIndex(const char *relName,
                         const char *attrName)
{
    RC rc;
    RID rid;

    if (!bIsOpen)
        return SM_DBNOTOPEN;

    if (relName == NULL || attrName == NULL)
        return SM_BADTABLE;

    AttrCat currentAttr;
    rc = GetAttrTpl(relName, attrName, currentAttr, rid);
    if(rc) return rc;

    if (currentAttr.indexNo == -1)
        return SM_NOIDXTODESTROY;

    // l'index existe donc on detruit le fichier
    rc = ixm.DestroyIndex(relName, currentAttr.indexNo);
    if(rc) return rc;

    // L'index a ete detruit donc on met a jour le catalogue
    currentAttr.indexNo = -1;
    RM_Record rec;
    rec.Set((char*)&currentAttr, sizeof(AttrCat));
    rec.SetRID(rid);
    rc = attrcat_fh.UpdateRec(rec);
    if(rc) return rc;

    cout << "DropIndex\n"
         << "   relName =" << relName << "\n"
         << "   attrName=" << attrName << "\n";
    return (0);
}

RC SM_Manager::Load(const char *relName,
                    const char *fileName)
{
    //****
    RC rc;
    RM_FileHandle rmfh;
    IX_IndexHandle ixih;
    RID rid;
        
    if (relName == NULL || fileName == NULL)
        return SM_BADTABLE;
    
    // open relation file
    rc = rmm.OpenFile(relName, rmfh);
    if(rc) return rc;
        
    // have to get all attributes from relName
    int attrCount;
    DataAttrInfo *attributes;
    rc = GetAttributesFromRel(relName, attributes, attrCount);
    if (rc) return rc;
    
    IX_IndexHandle *ixih_array = new IX_IndexHandle[attrCount];
    
    // open index for each index
    for (int i = 0; i < attrCount; i++) {
    	if (attributes[i].indexNo != -1) {
    		rc = ixm.OpenIndex(relName, attributes[i].indexNo, ixih_array[i]);
    		if (rc) return rc;
    	}
    }
        
    // open file to read each line
    ifstream fichier;
    string line;
    
    char *pData;
    int bitmapSize = 0;
    
    int j = 0;
        
    fichier.open(fileName, ifstream::in);
    if (fichier.fail()) {
        fichier.close();
        return SM_BADTABLE;
    }
    else if (fichier.is_open()) {
        
        for (int i = 0; i < attrCount; i++) {
            bitmapSize += attributes[i].attrLength;
        }
        
        // Initialize pData
        pData = new char[bitmapSize];
            
        while ( fichier.good() ) {
            // use catalog attrcat information to read tuples in file
            getline (fichier, line);
            
            istringstream split(line);
            string value;
                        
            memset(pData, 0, bitmapSize); // reinitialise le tuple temporaire
                        
            j = 0;
            while (getline(split, value, ',')) {
                
                istringstream stream(value);
                
                unsigned int attrLength = attributes[j].attrLength;
                
                switch (attributes[j].attrType) {
                    
                    case INT:
                        int val_int;
                        stream >> val_int;
                        memcpy(pData + attributes[j].offset, &val_int, attrLength);
                        break;
              
                    case FLOAT:
                        float val_float;
                        stream >> val_float;
                        memcpy(pData + attributes[j].offset, &val_float, attrLength);
                        break;
              
                    case STRING:
                        string val_string = value;
                                                
                        // if entry is too long, let's truncate
                        if (val_string.length() > attrLength) {
                            val_string = val_string.substr(0, attrLength);
                        }
                        
                        if (val_string.length() < attrLength) {
                            memcpy(pData + attributes[j].offset, val_string.c_str(), val_string.length());
                        }
                        else {
                            memcpy(pData + attributes[j].offset, val_string.c_str(), attrLength);
                        }
                        
                        break;
                }
                j++;
            }
            
            // insert temp tuple into the relation
            rc = rmfh.InsertRec(pData, rid);
            if (rc) return rc;

            // make appropriate index entries for the tuple
            for (int i = 0; i < attrCount; i++) {
            	if (attributes[i].indexNo != -1) { 
					rc = ixih_array[i].InsertEntry(pData, rid);
					if (rc) return rc;
            	}
            }
            
        }
    }
    
    // close every indexes
    for (int i = 0; i < attrCount; i++) {
    	if (attributes[i].indexNo != -1) { 
			rc = ixm.CloseIndex(ixih_array[i]);
			if (rc) return rc;
    	}
    }
    
    // close relation file
    rc = rmm.CloseFile(rmfh);
    if(rc) return rc;
    
    // close fileName
    fichier.close();
    
    delete[] ixih_array;
    delete[] attributes;
    
    //****
    
    cout << "Load\n"
         << "   relName =" << relName << "\n"
         << "   fileName=" << fileName << "\n";
    return (0);
}

RC SM_Manager::Print(const char *relName)
{
    if(!bIsOpen)
        return SM_BADTABLE;

    cout << "Print\n"
         << "   relName=" << relName << "\n\n";

    DataAttrInfo* attributes;
    int attrNb;
    RC rc;

    rc = GetAttributesFromRel(relName, attributes, attrNb);
    if(rc) return rc;

    Printer p(attributes, attrNb);
    p.PrintHeader(cout);

    // Contrairement aux fonctions Help, ici nous allons afficher le contenu de la table
    RM_FileHandle* pFileHandle = NULL;
    RM_FileHandle fh;
    if(strcmp(relName, "relcat") == 0)
        pFileHandle = &relcat_fh;
    else if(strcmp(relName, "attrcat") == 0)
        pFileHandle = &attrcat_fh;
    else {
        rc = rmm.OpenFile(relName,fh);
        if(rc) return rc;

        pFileHandle = &fh;
    }

    RM_FileScan fs;
    RM_Record rec;
    if ((rc=fs.OpenScan(*pFileHandle, INT, sizeof(int), 0,
            NO_OP, NULL, NO_HINT)))
        return rc;

    int n;
    for (rc = fs.GetNextRec(rec), n = 0;
         rc == 0;
         rc = fs.GetNextRec(rec), n++) {

        char* pData;
        rec.GetData(pData);
        p.Print(cout, pData);
    }

    if (rc != RM_EOF && rc != 0)
        return rc;

    if ((rc=fs.CloseScan()))
        return rc;

    p.PrintFooter(cout);

    return (0);
}

RC SM_Manager::Set(const char *paramName, const char *value)
{
    if(!bIsOpen)
        return SM_DBNOTOPEN;

    if(paramName == NULL || value == NULL)
        return SM_INVALIDPARAM;

    params[paramName] = std::string(value);

    cout << "Set\n"
         << "   paramName=" << paramName << "\n"
         << "   value    =" << value << "\n";
    return (0);
}

RC SM_Manager::Help()
{
    if(!bIsOpen)
        return SM_BADTABLE;

    cout << "Help\n\n";

    DataAttrInfo* attributes;
    int attrNb;
    RC rc;

    rc = GetAttributesFromRel("relcat", attributes, attrNb);
    if(rc) return rc;

    // On ne veut afficher que l'attribut relName
    for( ;; attributes++) {
        if(strcmp((*attributes).attrName, "relName") == 0)
            break;
    }

    Printer p(attributes, 1);
    p.PrintHeader(cout);

    RM_FileScan fs;
    RM_Record rec;
    if ((rc=fs.OpenScan(relcat_fh, INT, sizeof(int), 0,
            NO_OP, NULL, NO_HINT)))
        return rc;

    int n;
    for (rc = fs.GetNextRec(rec), n = 0;
         rc == 0;
         rc = fs.GetNextRec(rec), n++) {

        char* pData;
        rec.GetData(pData);
        p.Print(cout, pData);
    }

    if (rc != RM_EOF && rc != 0)
        return rc;

    if ((rc=fs.CloseScan()))
        return rc;

    p.PrintFooter(cout);

    return (0);
}

RC SM_Manager::Help(const char *relName)
{
    if(!bIsOpen)
        return SM_BADTABLE;

    cout << "Help\n"
         << "   relName=" << relName << "\n\n";

    DataAttrInfo* attributes;
    int attrNb;
    RC rc;

    rc = GetAttributesFromRel(relName, attributes, attrNb);
    if(rc) return rc;

    Printer p(attributes, attrNb);
    p.PrintHeader(cout);

    RM_FileScan fs;
    RM_Record rec;
    if ((rc=fs.OpenScan(attrcat_fh, STRING, MAXNAME+1, offsetof(AttrCat,relName),
            EQ_OP, (void*) relName, NO_HINT)))
        return rc;

    int n;
    for (rc = fs.GetNextRec(rec), n = 0;
         rc == 0;
         rc = fs.GetNextRec(rec), n++) {

        char* pData;
        rec.GetData(pData);
        p.Print(cout, pData);
    }

    if (rc != RM_EOF && rc != 0)
        return rc;

    if ((rc=fs.CloseScan()))
        return rc;

    p.PrintFooter(cout);

    return (0);
}

RC SM_Manager::GetAttributesFromRel(const char* relName, DataAttrInfo* & attributes, int& attrNb) const {
    RC rc;
    RID rid;

    if(!bIsOpen)
        return SM_DBNOTOPEN;

    if(relName == NULL)
        return SM_BADTABLE;

    // On recupere les infos sur la relation
    RelCat currentRel;
    rc = GetRelTpl(relName, currentRel, rid);
    if(rc) return rc;

    // On construit les objets de sortie
    attrNb = currentRel.attrNb;
    attributes = new DataAttrInfo[attrNb];

    // On scan les attr
    RM_FileScan fs;
    RM_Record rec;
    if ((rc=fs.OpenScan(attrcat_fh, STRING, MAXNAME+1, offsetof(AttrCat, relName),
            EQ_OP, (void*) relName, NO_HINT)))
        return rc;

    AttrCat *attrTmp;
    int n;
    for (rc = fs.GetNextRec(rec), n = 0;
         rc == 0;
         rc = fs.GetNextRec(rec), n++) {

        rc = rec.GetData((char*&)attrTmp);
        if (rc) return rc;

        // Copie de l'attribut
        attributes[n].indexNo = attrTmp->indexNo;
        attributes[n].attrLength = attrTmp->attrLength;
        attributes[n].attrType = attrTmp->attrType;
        attributes[n].offset = attrTmp->offset;
        strcpy(attributes[n].relName, attrTmp->relName);
        strcpy(attributes[n].attrName, attrTmp->attrName);
    }

    if (rc != RM_EOF && rc != 0)
        return rc;

    if ((rc=fs.CloseScan()))
        return rc;

    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// Error management
///////////////////////////////////////////////////////////////////////////////

//
// Error table
//
static char *SM_WarnMsg[] = {
    (char*)"index already exists",
    (char*)"no index to destroy",
    (char*)"forbidden operation"
};

static char *SM_ErrorMsg[] = {
    (char*)"Fail to open DB",
    (char*)"DB already openned",
    (char*)"DB not found",
    (char*)"DB not openned",
    (char*)"Invalid number of attributes",
    (char*)"Bad Table",
    (char*)"No Table found",
    (char*)"Bad attribute",
    (char*)"Invalid parameter"
};

//
// SM_PrintError
//
// Desc: Send a message corresponding to a SM return code to cerr
// In:   rc - return code for which a message is desired
//
void SM_PrintError(RC rc)
{
    // Check the return code is within proper limits
	if (rc >= START_SM_WARN && rc <= SM_LASTWARN)
		// Print warning
		cerr << "SM warning: " << SM_WarnMsg[rc - START_SM_WARN] << "\n";

	// Error codes are negative, so invert everything
	else if (-rc >= -START_SM_ERR && -rc <= -SM_LASTERROR)
		// Print error
		cerr << "SM error: " << SM_ErrorMsg[-rc + START_SM_ERR] << "\n";

	else if ((rc >= START_IX_WARN && rc <= IX_LASTWARN) || (-rc >= -START_IX_ERR && -rc <= -IX_LASTERROR))
		IX_PrintError(rc);
	
	else if ((rc >= START_PF_WARN && rc <= PF_LASTWARN) ||
		 (-rc >= -START_PF_ERR && -rc < -PF_LASTERROR) ||
			  (rc == PF_UNIX))
		PF_PrintError(rc);

	else if ((rc >= START_RM_WARN && rc <= RM_LASTWARN) ||
		 (-rc >= -START_RM_ERR && -rc < -RM_LASTERROR))
		RM_PrintError(rc);

	else if (rc == 0)
		cerr << "SM_PrintError called with return code of 0\n";

	else
		cerr << "SM error: " << rc << " is out of bounds\n";
}
