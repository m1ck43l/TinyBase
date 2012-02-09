//
// ql.h
//   Query Language Component Interface
//

// This file only gives the stub for the QL component

#ifndef QL_H
#define QL_H


#include <stdlib.h>
#include <string.h>
#include "redbase.h"
#include "parser.h"
#include "rm.h"
#include "ix.h"
#include "sm.h"
#include "comparator.h"

#undef min
#undef max
#include <vector>
#include <algorithm>

#include "ql_iterator.h"

#define SPACES 4

//
// QL_Manager: query language (DML)
//
class QL_Manager {
public:
    QL_Manager (SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm);
    ~QL_Manager();                       // Destructor

    RC Select  (int nSelAttrs,           // # attrs in select clause
        const RelAttr selAttrs[],        // attrs in select clause
        int   nRelations,                // # relations in from clause
        const char * const relations[],  // relations in from clause
        int   nConditions,               // # conditions in where clause
        const Condition conditions[]);   // conditions in where clause

    RC Insert  (const char *relName,     // relation to insert into
        int   nValues,                   // # values
        const Value values[]);           // values to insert

    RC Delete  (const char *relName,     // relation to delete from
        int   nConditions,               // # conditions in where clause
        const Condition conditions[]);   // conditions in where clause

    RC Update  (const char *relName,     // relation to update
        const RelAttr &updAttr,          // attribute to update
        const int bIsValue,              // 1 if RHS is a value, 0 if attribute
        const RelAttr &rhsRelAttr,       // attr on RHS to set LHS equal to
        const Value &rhsValue,           // or value to set attr equal to
        int   nConditions,               // # conditions in where clause
        const Condition conditions[]);   // conditions in where clause

private:
    RC checkRelations(int nRelations, const char * const relations[]);
    RC checkSelAttrs (int nSelAttrs, const RelAttr selAttrs[]);
    RC checkWhereAttrs (int nConditions, const Condition conditions[]);

    // SelectPLan
    RC SelectPlan(QL_Iterator*& racine, int nSelAttrs, const RelAttr selAttrs[],
            int nRelations, const char * const relations[],
            int nConditions, const Condition conditions[], const Condition &noCond);

    // UpdatePLan
    RC UpdatePlan(QL_Iterator*& racine, const char *relName, const char* attrName,
			int nConditions, const Condition conditions[], const Condition& noCond);

    // DeletePlan
    RC DeletePlan(QL_Iterator*& racine, const char *relName,
			int nConditions, const Condition conditions[], const Condition& noCond);
    bool IsBetter(AttrType a, AttrType b);

    SM_Manager *smm;
    IX_Manager *ixm;
    RM_Manager *rmm;

    std::vector<std::string> relationsMap;
};

//
// Print-error function
//
void QL_PrintError(RC rc);

#define QL_NOTBLFOUND       	(START_QL_WARN + 0)
#define QL_RELDBLFOUND			(START_QL_WARN + 1)
#define QL_INVALIDATTR			(START_QL_WARN + 2)
#define QL_ATTRNOTFOUND			(START_QL_WARN + 3)
#define QL_WRONGTYPEWHERECLAUSE	(START_QL_WARN + 4)
#define QL_ITNOTOPEN			(START_QL_WARN + 5)
#define QL_EOF					(START_QL_WARN + 6)
#define QL_ITALRDYOPEN			(START_QL_WARN + 7)
#define QL_LASTWARN				QL_ITALRDYOPEN

#define QL_LASTERROR			START_QL_ERR

#endif
