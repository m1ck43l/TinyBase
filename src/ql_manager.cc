//
// ql_manager.cc
//

// Note that for the SM component (HW3) the QL is actually a
// simple stub that will allow everything to compile.  Without
// a QL stub, we would need two parsers.

#include <cstdio>
#include <iostream>
#include <sys/times.h>
#include <sys/types.h>
#include <cassert>
#include <unistd.h>
#include <string>
#include "redbase.h"
#include "ql.h"
#include "sm.h"
#include "ix.h"
#include "rm.h"

#include "it_filescan.h"
#include "it_indexscan.h"
#include "it_filter.h"
#include "it_nestedloopjoin.h"
#include "it_projection.h"

using namespace std;

//
// QL_Manager::QL_Manager(SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm)
//
// Constructor for the QL Manager
//
QL_Manager::QL_Manager(SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm) : smm(&smm), ixm(&ixm), rmm(&rmm)
{
    // Can't stand unused variable warnings!
    assert (&smm && &ixm && &rmm);
}

//
// QL_Manager::~QL_Manager()
//
// Destructor for the QL Manager
//
QL_Manager::~QL_Manager()
{
}

//
// Handle the select clause
//
RC QL_Manager::Select(int nSelAttrs, const RelAttr selAttrs[],
                      int nRelations, const char * const relations[],
                      int nConditions, const Condition conditions[])
{
    int i;

    RC rc = checkRelations(nRelations, relations);
    if (rc) {
    	return rc;
    }

	rc = checkSelAttrs(nSelAttrs, selAttrs);
	if (rc) {
		return rc;
	}

	rc = checkWhereAttrs(nConditions, conditions);
	if (rc) {
		return rc;
	}

    // On crée la racine
    QL_Iterator* racine;
    Condition NO_COND = {{NULL,NULL}, NO_OP, 0, {NULL,NULL}, {INT,NULL}};
    rc = SelectPlan(racine, nSelAttrs, selAttrs, nRelations, relations, nConditions, conditions, NO_COND);
    if(rc) return rc;

    // On récupère les attributs de la table
    DataAttrInfo* attributes = racine->getRelAttr();
    int attrNb = racine->getAttrCount();

    // La classe printer
    Printer p(attributes, attrNb);
    p.PrintHeader(cout);

    // On ouvre l'itérateur que l'on récupère grâce au plan
    rc = racine->Open();
    if(rc) return rc;

    // On parcourt l'itérateur et on affiche chaque tuple trouvé
    RM_Record rec;
    while ((rc = racine->GetNext(rec)) != QL_EOF) {
        if(rc) return rc;

        char * pData;
        rc = rec.GetData(pData);
        if(rc) return rc;

        RID rid;
        rc = rec.GetRid(rid);
        if(rc) return rc;

        // On affiche le record
        p.Print(cout, pData);
    }

    p.PrintFooter(cout);

    // On ferme l'itérateur
    rc = racine->Close();
    if(rc) return rc;

    // On détruit les itérateurs
    delete racine;

    cout << "Select\n";

    cout << "   nSelAttrs = " << nSelAttrs << "\n";
    for (i = 0; i < nSelAttrs; i++)
        cout << "   selAttrs[" << i << "]:" << selAttrs[i] << "\n";

    cout << "   nRelations = " << nRelations << "\n";
    for (i = 0; i < nRelations; i++)
        cout << "   relations[" << i << "] " << relations[i] << "\n";

    cout << "   nCondtions = " << nConditions << "\n";
    for (i = 0; i < nConditions; i++)
        cout << "   conditions[" << i << "]:" << conditions[i] << "\n";

    return 0;
}

//
// SelectPLan
//
RC QL_Manager::SelectPlan(QL_Iterator *&racine, int nSelAttrs, const RelAttr selAttrs[],
        int nRelations, const char * const relations[],
        int nConditions, const Condition conditions[],
        const Condition& noCond) {

    RC rc;
    AttrCat attrTpl;
    RID rid;
    QL_Iterator* feuille = NULL;
	// Pour chaque relation,
	//    1) on choisit un itérateur niveau feuille (indexScan ou fileScan) -> leaf
	//    2) on applique les filtres -> leaf = new IT_Filter(leaf, ...)
	// On stocke les leaf par indice de relation
	//    3) on applique les jointures à toutes les relations suivantes -> racine = new IT_Join(leaf1, leaf2, ...)
	// On applique la projection sur root
	for (int i = 0; i < nRelations; i++) {
        int idxCond = -1;
        AttrType idxCondType;

        int fileCond = -1;
        AttrType fileCondType;

        // On essaye de trouver un index parmis les conditions
        // On privilégie un index sur des INT puis FLOAT puis STRING
        // Si l'opération est NE_OP on ne la prend pas en compte
        for(int j = 0; j < nConditions; j++) {
            if(conditions[j].bRhsIsAttr || conditions[j].op == NE_OP) {
                continue;
            }

            if(conditions[j].lhsAttr.relName!=NULL &&
                    strcmp(conditions[j].lhsAttr.relName,relations[i]) !=0 ) {
                continue;
            }

            rc = smm->GetAttrTpl(relations[i], conditions[j].lhsAttr.attrName, attrTpl, rid);
            if(rc) return rc;

            // Vérifions si l'attribut est un index
            if (attrTpl.indexNo == -1)
                continue;

            // On stocke l'indice sinon
            if((idxCond == -1) || (IsBetter(idxCondType, attrTpl.attrType))) {
                idxCond = j;
                idxCondType = attrTpl.attrType;
            }
        }

        // Si on ne trouve pas d'index on choisit une condition
        // selon le type
        if(idxCond == -1) {
            for(int j = 0; j < nConditions; j++) {
                if(conditions[j].bRhsIsAttr) {
                    continue;
                }

                if(conditions[j].lhsAttr.relName!=NULL &&
                        strcmp(conditions[j].lhsAttr.relName,relations[i]) !=0 ){
                    continue;
                }

                if((fileCond == -1) || (IsBetter(fileCondType, conditions[j].rhsValue.type))) {
                    fileCond = j;
                    fileCondType = conditions[j].rhsValue.type;
                }
            }
        }

        // Mise en place de l'itérateur niveau feuille
        if(idxCond != -1) {
            feuille = new IT_IndexScan(rmm, smm, ixm, relations[i], conditions[idxCond].lhsAttr.attrName, conditions[idxCond], rc);
            if(rc) return rc;
        } else if(fileCond != -1) {
            feuille = new IT_FileScan(rmm, smm, relations[i], conditions[fileCond], rc);
            if(rc) return rc;
        } else {
            feuille = new IT_FileScan(rmm, smm, relations[i], noCond, rc);
            if(rc) return rc;
        }

        // Maintenant on doit appliquer un filtre par condition autre que celle du scan
        for(int j = 0; j < nConditions; j++) {
            if(j != idxCond && j != fileCond) {

                if( (strcmp(conditions[j].lhsAttr.relName, relations[i])==0  && !conditions[j].bRhsIsAttr) ||
                    (strcmp(conditions[j].lhsAttr.relName, relations[i])==0  && conditions[j].bRhsIsAttr && strcmp(conditions[j].rhsAttr.relName, relations[i])==0) ) {

                feuille = new IT_Filter(racine, conditions[j]);
                }
            }
        }

        //Il ne reste plus qu'à traiter les jointures

        if (i==0) {
            racine = feuille;
        }
        else {
            for(int j = 0; j < nConditions; j++) {

                if(!conditions[j].bRhsIsAttr) continue;
                if (strcmp(conditions[j].lhsAttr.relName,conditions[j].rhsAttr.relName) == 0) continue;
                if(strcmp(conditions[j].lhsAttr.relName,relations[i]) != 0  && strcmp(conditions[j].rhsAttr.relName,relations[i]) != 0  ) continue;
                int k=0;
                for(k=0;k<i;k++){
                    if (strcmp(conditions[j].lhsAttr.relName, relations[k]) == 0) {
                        break;
                    }
                    if (strcmp(conditions[j].rhsAttr.relName, relations[k]) == 0) {
                        break;
                    }
                }
                if (k==i) {
                    continue;
                }

                racine = new IT_NestedLoopJoin(racine,feuille,conditions[j]);
            }
        }
	}

    racine = new IT_Projection(racine,nSelAttrs,selAttrs);

    return 0;
}

//
// Insert the values into relName
//
RC QL_Manager::Insert(const char *relName,
                      int nValues, const Value values[])
{
	RC rc;
	RM_FileHandle rmfh;
	RID rid;

	int i;

	// cout << "Insert\n";
	//
	//     cout << "   relName = " << relName << "\n";
	//     cout << "   nValues = " << nValues << "\n";
	//     for (i = 0; i < nValues; i++)
	//         cout << "   values[" << i << "]:" << values[i] << "\n";

	// request : INSERT INTO relname VALUES value[Ø], value[1], value[2]

	// verifications

	// 1. verifier que la table existe

	// open relation file
	rc = rmm->OpenFile(relName, rmfh);
	if(rc) return rc;

	// have to get all attributes from relName
	int attrCount;
	DataAttrInfo *attributes;
	rc = smm->GetAttributesFromRel(relName, attributes, attrCount);
	if (rc) return rc;

	// 2. verifier que le nombre de valeurs est le bon

	if (attrCount != nValues) {
		delete [] attributes;
		return QL_INVALIDATTR;
	}

	// 3. verifier que le type des valeur est bon

	for (i = 0; i < attrCount; i++) {
		if (attributes[i].attrType != values[i].type) {
			delete [] attributes;
			return QL_INVALIDATTR;
		}
	}

	IX_IndexHandle *ixih_array = new IX_IndexHandle[attrCount];

	// open index for each index
	for (i = 0; i < attrCount; i++) {
		if (attributes[i].indexNo != -1) {
			rc = ixm->OpenIndex(relName, attributes[i].indexNo, ixih_array[i]);
			if (rc) return rc;
		}
	}

	char *pData;
	int bitmapSize = 0;

	for (int i = 0; i < attrCount; i++) {
		bitmapSize += attributes[i].attrLength;
	}

	// Initialize pData
	pData = new char[bitmapSize];
	memset(pData, 0, bitmapSize); // reinitialise le tuple temporaire

	for (i = 0; i < attrCount; i++) {
		unsigned int attrLength = attributes[i].attrLength;

		// est-ce que le parseur formatte bien les entrees comme il faut ? (par ex. tronquer string trop long)
		memcpy(pData + attributes[i].offset, values[i].data, attrLength);
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

	// Printer
	Printer p(attributes, attrCount);
	p.PrintHeader(cout);
	p.Print(cout, pData);
	p.PrintFooter(cout);

	// close every indexes
	for (int i = 0; i < attrCount; i++) {
		if (attributes[i].indexNo != -1) {
			rc = ixm->CloseIndex(ixih_array[i]);
			if (rc) return rc;
		}
	}

	// close relation file
	rc = rmm->CloseFile(rmfh);
	if(rc) return rc;

	delete[] ixih_array;
	delete[] attributes;

    cout << "Insert\n";

    cout << "   relName = " << relName << "\n";
    cout << "   nValues = " << nValues << "\n";
    for (i = 0; i < nValues; i++)
        cout << "   values[" << i << "]:" << values[i] << "\n";

    return 0;
}

//
// Delete from the relName all tuples that satisfy conditions
//
RC QL_Manager::Delete(const char *relName,
                      int nConditions, const Condition conditions[])
{
    int i;
    RC rc;
    RM_FileHandle rmfh;

    DataAttrInfo* attributes;
    IX_IndexHandle* indexes;

    int attrNb;

    // On met la relation dans le vecteur
    relationsMap.clear();
    relationsMap.push_back(relName);

    // On vérifie les conditions
    rc = checkWhereAttrs(nConditions, conditions);
    if(rc) return rc;

    // On ouvre la relation dans laquelle on veut supprimer les tuples
    rc = rmm->OpenFile(relName, rmfh);
    if(rc) return rc;

    // On crée la racine
    QL_Iterator* racine;
    Condition NO_COND = {{NULL,NULL}, NO_OP, 0, {NULL,NULL}, {INT,NULL}};
	rc = DeletePlan(racine, relName, nConditions, conditions, NO_COND);
	if(rc) return rc;

    // On récupère les attributs de la table
	attributes = racine->getRelAttr();
    attrNb = racine->getAttrCount();

    // On ouvre les index
    indexes = new IX_IndexHandle[attrNb];
    for(int i = 0; i < attrNb; i++) {
    	if(attributes[i].indexNo != -1) {
    		rc = ixm->OpenIndex(relName, attributes[i].indexNo, indexes[i]);
    		if(rc) return rc;
    	}
    }

    // La classe printer
    Printer p(attributes, attrNb);
	p.PrintHeader(cout);

	// On ouvre l'itérateur que l'on récupère grâce au plan
	rc = racine->Open();
	if(rc) return rc;

	// On parcourt l'itérateur et on affiche chaque tuple trouvé
	RM_Record rec;
	while ((rc = racine->GetNext(rec)) != QL_EOF) {
		if(rc) return rc;

		char * pData;
		rc = rec.GetData(pData);
		if(rc) return rc;

		RID rid;
		rc = rec.GetRid(rid);
		if(rc) return rc;

		// On affiche le record
		p.Print(cout, pData);

		// Suppression des records dans les indexes
		for(int i = 0; i < attrNb; i++) {
			if (attributes[i].indexNo != -1) {
				rc = indexes[i].DeleteEntry(pData + attributes[i].offset, rid);
				if(rc) return rc;
			}
		}

		// Finalement on supprime le tuple
		rc = rmfh.DeleteRec(rid);
		if(rc) return rc;
	}

	p.PrintFooter(cout);

	// On ferme l'itérateur
	rc = racine->Close();
	if(rc) return rc;

	// On détruit les itérateurs
	delete racine;

	// On ferme les indexes
	for (int i = 0; i < attrNb; i++) {
		if (attributes[i].indexNo != -1) {
			rc = ixm->CloseIndex(indexes[i]);
			if(rc) return rc;
		}
	}

	// On ferme la relation
	rc = rmm->CloseFile(rmfh);
	if(rc) return rc;

    cout << "Delete\n";

    cout << "   relName = " << relName << "\n";
    cout << "   nCondtions = " << nConditions << "\n";
    for (i = 0; i < nConditions; i++)
        cout << "   conditions[" << i << "]:" << conditions[i] << "\n";

    return 0;
}


//
// Update from the relName all tuples that satisfy conditions
//
RC QL_Manager::Update(const char *relName,
                      const RelAttr &updAttr,
                      const int bIsValue,
                      const RelAttr &rhsRelAttr,
                      const Value &rhsValue,
                      int nConditions, const Condition conditions[])
{
	int i, iLeft = -1, iRight = -1;
	RC rc;
	RM_FileHandle rmfh;

	DataAttrInfo* attributes;
	IX_IndexHandle indexes;

	int attrNb;

	// On met la relation dans le vecteur
	relationsMap.clear();
	relationsMap.push_back(relName);

	// On vérifie les conditions
	rc = checkWhereAttrs(nConditions, conditions);
	if(rc) return rc;

	// On vérifie l'attribut de l'update
	AttrCat left;
	RID rid;
	rc = smm->GetAttrTpl(relName, updAttr.attrName, left, rid);
	if(rc) return rc;

	if(bIsValue) {
		if (left.attrType != rhsValue.type) {
			return QL_INVALIDATTR;
		}
	} else {
		AttrCat right;
		rc = smm->GetAttrTpl(relName, rhsRelAttr.attrName, right, rid);
		if(rc) return rc;

		if (right.attrType != left.attrType)
			return QL_INVALIDATTR;
	}

	// On ouvre la relation dans laquelle on veut supprimer les tuples
	rc = rmm->OpenFile(relName, rmfh);
	if(rc) return rc;

	// On crée la racine
	QL_Iterator* racine;
	Condition NO_COND = {{NULL,NULL}, NO_OP, 0, {NULL,NULL}, {INT,NULL}};
	rc = UpdatePlan(racine, relName, updAttr.attrName, nConditions, conditions, NO_COND);
	if(rc) return rc;

	// On récupère les attributs de la table
	attributes = racine->getRelAttr();
	attrNb = racine->getAttrCount();

	// On récupère les indices des deux attributs
	for(int j = 0; j < attrNb; j++) {
		if(strcmp(attributes[j].attrName, updAttr.attrName) == 0) {
			iLeft = j;
		}
		if(!bIsValue && strcmp(attributes[j].attrName, rhsRelAttr.attrName) == 0) {
			iRight = j;
		}
	}

	// On ouvre juste l'index sur iLeft (modification de la valeur -> clé)
	if(attributes[iLeft].indexNo != -1) {
		rc = ixm->OpenIndex(relName, attributes[iLeft].indexNo, indexes);
		if(rc) return rc;
	}

	// La classe printer
	Printer p(attributes, attrNb);
	p.PrintHeader(cout);

	// On ouvre l'itérateur que l'on récupère grâce au plan
	rc = racine->Open();
	if(rc) return rc;

	// On parcourt l'itérateur et on affiche chaque tuple trouvé
	RM_Record rec;
	while ((rc = racine->GetNext(rec)) != QL_EOF) {
		if(rc) return rc;

		char * pData;
		rc = rec.GetData(pData);
		if(rc) return rc;

		rc = rec.GetRid(rid);
		if(rc) return rc;

		// On affiche le record
		p.Print(cout, pData);

		// Suppression des records dans l'index touché par les modifs
		if (attributes[iLeft].indexNo != -1) {
			rc = indexes.DeleteEntry(pData + attributes[iLeft].offset, rid);
			if(rc) return rc;
		}

		// Mise à jour de la valeur
		// deux cas soit c'est une valeur
		if(bIsValue) {
			memcpy(pData + attributes[iLeft].offset, rhsValue.data, attributes[iLeft].attrLength);
		} else {
			memcpy(pData + attributes[iLeft].offset, pData + attributes[iRight].offset, attributes[iLeft].attrLength);
		}

		// On met à jour le tuple
		rc = rmfh.UpdateRec(rec);
		if(rc) return rc;

		// Finalement on réinsère dans l'index
		if (attributes[iLeft].indexNo != -1) {
			rc = indexes.InsertEntry(pData + attributes[iLeft].offset, rid);
			if(rc) return rc;
		}
	}

	p.PrintFooter(cout);

	// On ferme l'itérateur
	rc = racine->Close();
	if(rc) return rc;

	// On détruit les itérateurs
	delete racine;

	// On ferme l'index
	if (attributes[iLeft].indexNo != -1) {
		rc = ixm->CloseIndex(indexes);
		if(rc) return rc;
	}

	// On ferme la relation
	rc = rmm->CloseFile(rmfh);
	if(rc) return rc;

    cout << "Update\n";

    cout << "   relName = " << relName << "\n";
    cout << "   updAttr:" << updAttr << "\n";
    if (bIsValue)
        cout << "   rhs is value: " << rhsValue << "\n";
    else
        cout << "   rhs is attribute: " << rhsRelAttr << "\n";

    cout << "   nCondtions = " << nConditions << "\n";
    for (i = 0; i < nConditions; i++)
        cout << "   conditions[" << i << "]:" << conditions[i] << "\n";

    return 0;
}

//
// DeletePlan
//
RC QL_Manager::DeletePlan(QL_Iterator*& racine, const char *relName,
        					int nConditions, const Condition conditions[], const Condition& noCond) {
	RC rc;
	AttrCat attrTpl;
	RID rid;



	int idxCond = -1;
	AttrType idxCondType;

	int fileCond = -1;
	AttrType fileCondType;

	// On essaye de trouver un index parmis les conditions
	// On privilégie un index sur des INT puis FLOAT puis STRING
	// Si l'opération est NE_OP on ne la prend pas en compte
	for(int i = 0; i < nConditions; i++) {
		if(conditions[i].bRhsIsAttr || conditions[i].op == NE_OP) {
			continue;
		}

		rc = smm->GetAttrTpl(relName, conditions[i].lhsAttr.attrName, attrTpl, rid);
		if(rc) return rc;

		// Vérifions si l'attribut est un index
		if (attrTpl.indexNo == -1)
			continue;

		// On stocke l'indice sinon
		if((idxCond == -1) || (IsBetter(idxCondType, attrTpl.attrType))) {
			idxCond = i;
			idxCondType = attrTpl.attrType;
		}
	}

	// Si on ne trouve pas d'index on choisit une condition
	// selon le type
	if(idxCond == -1) {
		for(int i = 0; i < nConditions; i++) {
			if(conditions[i].bRhsIsAttr) {
				continue;
			}

			if((fileCond == -1) || (IsBetter(fileCondType, conditions[i].rhsValue.type))) {
				fileCond = i;
				fileCondType = conditions[i].rhsValue.type;
			}
		}
	}

	// Mise en place de l'itérateur niveau feuille
	if(idxCond != -1) {
		racine = new IT_IndexScan(rmm, smm, ixm, relName, conditions[idxCond].lhsAttr.attrName, conditions[idxCond], rc);
		if(rc) return rc;
	} else if(fileCond != -1) {
		racine = new IT_FileScan(rmm, smm, relName, conditions[fileCond], rc);
		if(rc) return rc;
	} else {
		racine = new IT_FileScan(rmm, smm, relName, noCond, rc);
		if(rc) return rc;
	}

	// Maintenant on doit appliquer un filtre par condition autre que celle du scan
	for(int i = 0; i < nConditions; i++) {
		if(i != idxCond && i != fileCond) {
			racine = new IT_Filter(racine, conditions[i]);
		}
	}

	return 0;
}

//
// UpdatePlan
//
RC QL_Manager::UpdatePlan(QL_Iterator*& racine, const char *relName, const char* attrName,
        					int nConditions, const Condition conditions[], const Condition & noCond) {
	RC rc;
	AttrCat attrTpl;
	RID rid;

	int idxCond = -1;
	AttrType idxCondType;

	int fileCond = -1;
	AttrType fileCondType;

	// On essaye de trouver un index parmis les conditions
	// On privilégie un index sur des INT puis FLOAT puis STRING
	// Si l'opération est NE_OP on ne la prend pas en compte
	for(int i = 0; i < nConditions; i++) {
		if(conditions[i].bRhsIsAttr || conditions[i].op == NE_OP) {
			continue;
		}

		// On ne veut pas utiliser un scan sur un index à updater
		// pour éviter des erreurs de mise à jour/scan
		if(strcmp(conditions[i].lhsAttr.attrName, attrName) == 0)
			continue;

		rc = smm->GetAttrTpl(relName, conditions[i].lhsAttr.attrName, attrTpl, rid);
		if(rc) return rc;

		// Vérifions si l'attribut est un index
		if (attrTpl.indexNo == -1)
			continue;

		// On stocke l'indice sinon
		if((idxCond == -1) || (IsBetter(idxCondType, attrTpl.attrType))) {
			idxCond = i;
			idxCondType = attrTpl.attrType;
		}
	}

	// Si on ne trouve pas d'index on choisit une condition
	// selon le type
	if(idxCond == -1) {
		for(int i = 0; i < nConditions; i++) {
			if(conditions[i].bRhsIsAttr) {
				continue;
			}

			if((fileCond == -1) || (IsBetter(fileCondType, conditions[i].rhsValue.type))) {
				fileCond = i;
				fileCondType = conditions[i].rhsValue.type;
			}
		}
	}

	// Mise en place de l'itérateur niveau feuille
	if(idxCond != -1) {
		racine = new IT_IndexScan(rmm, smm, ixm, relName, conditions[idxCond].lhsAttr.attrName, conditions[idxCond], rc);
		if(rc) return rc;
	} else if(fileCond != -1) {
		racine = new IT_FileScan(rmm, smm, relName, conditions[fileCond], rc);
		if(rc) return rc;
	} else {
		racine = new IT_FileScan(rmm, smm, relName, noCond, rc);
		if(rc) return rc;
	}

	// Maintenant on doit appliquer un filtre par condition autre que celle du scan
	for(int i = 0; i < nConditions; i++) {
		if(i != idxCond && i != fileCond) {
			racine = new IT_Filter(racine, conditions[i]);
		}
	}

	return 0;
}

//
// Check if the new index is better
//
bool QL_Manager::IsBetter(AttrType a, AttrType b) {
	switch(a) {
	case INT:
		return false;
	case FLOAT:
		if (b == INT)
			return true;
		else
			return false;
	case STRING:
		return true;
	}

	return false;
}

//
// void QL_PrintError(RC rc)
//
// This function will accept an Error code and output the appropriate
// error.
//
void QL_PrintError(RC rc)
{
    cout << "QL_PrintError\n   rc=" << rc << "\n";
}

//
// Check the list of relations
//
RC QL_Manager::checkRelations(int nRelations, const char * const relations[]) {
	RC rc;
	RID rid;
	RelCat relTpl;

	vector<string>::iterator it;
	relationsMap.clear();

	int i = 0;
	for (i = 0; i < nRelations; i++) {
		// On vérifie l'existence de la relation
		rc = smm->GetRelTpl(relations[i], relTpl, rid);
		if (rc == SM_NOTBLFOUND) {
			// La table n'existe pas
			return QL_NOTBLFOUND;
		} else if (rc) {
			return rc;
		}

		// On vérifie les doublons
		it = find(relationsMap.begin(), relationsMap.end(), relations[i]);
		if (it == relationsMap.end()) {
			relationsMap.push_back(relations[i]);
		} else {
			return QL_RELDBLFOUND;
		}
	}

	// Les relations existent et ne sont pas doublées
	// On valide
	return 0;
}

//
// Check the selAttrs
//
RC QL_Manager::checkSelAttrs (int nSelAttrs, const RelAttr selAttrs[]) {
	RC rc;
	RID rid;
	AttrCat attrTpl;

	// Quand on arrive ici la map des relations a été construite
	// on peut donc utiliser relationsMap

	// Si select * from...
	// on valide
	if (nSelAttrs == 1 && strcmp(selAttrs[0].attrName, "*") == 0) {
		return 0;
	}

	vector<string>::iterator it;
	int i = 0;
	const char * relName;
	for (i = 0; i < nSelAttrs; i++) {
		// On recupere le nom de la relation
		if (selAttrs[i].relName == NULL) {
			if (relationsMap.size() > 1) {
				return QL_INVALIDATTR;
			}
			relName = relationsMap[0].c_str();
		}
		else  {
			relName = selAttrs[i].relName;
		}

		// On vérifie que la relation existe
		it = find(relationsMap.begin(), relationsMap.end(), relName);
		if (it == relationsMap.end()) {
			return QL_NOTBLFOUND;
		}

		// On vérifie que l'attribut existe
		rc = smm->GetAttrTpl(relName, selAttrs[i].attrName, attrTpl, rid);
		if (rc == SM_BADATTR) {
			return QL_ATTRNOTFOUND;
		} else if (rc) {
			return rc;
		}
	}

	// On valide
	return 0;
}

//
// Check the whereAttrs
//
RC QL_Manager::checkWhereAttrs (int nConditions, const Condition conditions[]) {
	RC rc;
	RID rid;
	AttrCat attrTplLeft, attrTplRight;

	// Quand on arrive ici la map des relations a été construite
	// on peut donc utiliser relationsMap

	vector<string>::iterator it;
	int i = 0;
	const char * relNameL;
	const char * relNameR;
	for (i = 0; i < nConditions; i++) {
		// On recupere la nom de la relation gauche
		if (conditions[i].lhsAttr.relName == NULL) {
			if (relationsMap.size() > 1) {
				return QL_INVALIDATTR;
			}
			relNameL = relationsMap[0].c_str();
		}
		else  {
			relNameL = conditions[i].lhsAttr.relName;
		}

		// On vérifie que la relation existe
		it = find(relationsMap.begin(), relationsMap.end(), relNameL);
		if (it == relationsMap.end()) {
			return QL_NOTBLFOUND;
		}

		// On vérifie que l'attribut gauche existe
		rc = smm->GetAttrTpl(relNameL, conditions[i].lhsAttr.attrName, attrTplLeft, rid);
		if (rc == SM_BADATTR) {
			return QL_ATTRNOTFOUND;
		} else if (rc) {
			return rc;
		}

		// Il nous reste à vérifier le membre droite
		if (conditions[i].bRhsIsAttr == TRUE) {
			// On recupere la nom de la relation droite
			if (conditions[i].rhsAttr.relName == NULL) {
				if (relationsMap.size() > 1) {
					return QL_INVALIDATTR;
				}
				relNameR = relationsMap[0].c_str();
			}
			else  {
				relNameR = conditions[i].rhsAttr.relName;
			}

			// On vérifie que la relation existe
			it = find(relationsMap.begin(), relationsMap.end(), relNameR);
			if (it == relationsMap.end()) {
				return QL_NOTBLFOUND;
			}

			// On vérifie que l'attribut droite existe
			rc = smm->GetAttrTpl(relNameR, conditions[i].rhsAttr.attrName, attrTplRight, rid);
			if (rc == SM_BADATTR) {
				return QL_ATTRNOTFOUND;
			} else if (rc) {
				return rc;
			}

			if (attrTplLeft.attrType != attrTplRight.attrType) {
				return QL_WRONGTYPEWHERECLAUSE;
			}
		} else {
			if (conditions[i].rhsValue.type != attrTplLeft.attrType) {
				return QL_WRONGTYPEWHERECLAUSE;
			}
		}
	}

	// On valide
	return 0;
}
