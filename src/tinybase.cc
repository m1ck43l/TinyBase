//
// redbase.cc
//
// Author: Jason McHugh (mchughj@cs.stanford.edu)
//
// This shell is provided for the student.

#include <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "redbase.h"
#include "rm.h"
#include "sm.h"
#include "ql.h"
#include "parser.h"

using namespace std;

//
// main
//
int main(int argc, char *argv[])
{
    char *dbname;
    RC rc;

    // Look for 2 arguments.  The first is always the name of the program
    // that was executed, and the second should be the name of the
    // database.
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " dbname \n";
        exit(1);
    }

    dbname = argv[1];

    // On declare tous les managers pour les passer à RBparse
    PF_Manager pfm;
    RM_Manager rmm(pfm);
    IX_Manager ixm(pfm);
    SM_Manager smm(ixm, rmm);
    QL_Manager qlm(smm, ixm, rmm);

    // On ouvre la DB
    rc = smm.OpenDb(dbname);
    if(rc) { SM_PrintError(rc); exit(rc); }

    // On appelle le parser
    RBparse(pfm, smm, qlm);

    // On referme la DB
    rc = smm.CloseDb();
    if(rc) { SM_PrintError(rc); exit(rc); }

    cout << "Bye.\n";

    return 0;
}
