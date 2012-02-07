//
// ql_manager_stub.cc
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

using namespace std;

//
// QL_Manager::QL_Manager(SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm)
//
// Constructor for the QL Manager
//
QL_Manager::QL_Manager(SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm) : smm(smm), ixm(ixm), rmm(rmm)
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

    cout << "Vérification des relations..." << endl;
    RC rc = checkRelations(nRelations, relations);
    if (rc) {
    	cout << "   Erreur dans les relations\n";
    	return rc;
    }
    cout << "Relations validées" << endl;

    cout << "Vérification des attributs..." << endl;
	rc = checkSelAttrs(nSelAttrs, selAttrs);
	if (rc) {
		cout << "   Erreur dans les relations\n";
		return rc;
	}
	cout << "Attributs validés" << endl;

	cout << "Vérification de la clause where..." << endl;
	rc = checkWhereAttrs(nConditions, conditions);
	if (rc) {
		cout << "   Erreur dans les relations\n";
		return rc;
	}
	cout << "Clause where validée" << endl;

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
// Insert the values into relName
//
RC QL_Manager::Insert(const char *relName,
                      int nValues, const Value values[])
{
    int i;

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
    int i;

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
		rc = smm.GetRelTpl(relations[i], relTpl, rid);
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
		rc = smm.GetAttrTpl(relName, selAttrs[i].attrName, attrTpl, rid);
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
		rc = smm.GetAttrTpl(relNameL, conditions[i].lhsAttr.attrName, attrTplLeft, rid);
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
			rc = smm.GetAttrTpl(relNameR, conditions[i].rhsAttr.attrName, attrTplRight, rid);
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
