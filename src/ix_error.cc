#include <cerrno>
#include <cstdio>
#include <iostream>
#include "ix.h"
#include "rm.h"
#include "redbase.h"

using namespace std;

//
// Error table
//
static char *IX_WarnMsg[] = {
    (char*)"file is already open",
    (char*)"file is already closed",
    (char*)"end of file",
    (char*)"last warn before eof"
};

static char *IX_ErrorMsg[] = {
    (char*)"Fail to create index file",
    (char*)"Key doesn't exist in index",
    (char*)"CompOp error in searching first rid"
};

//
// IX_PrintError
//
// Desc: Send a message corresponding to a IX return code to cerr
// In:   rc - return code for which a message is desired
//
void IX_PrintError(RC rc)
{
    // Check the return code is within proper limits
    if (rc >= START_IX_WARN && rc <= IX_LASTWARN)
        // Print warning
        cerr << "IX warning: " << IX_WarnMsg[rc - START_IX_WARN] << "\n";

    // Error codes are negative, so invert everything
    else if (-rc >= -START_IX_ERR && -rc <= -IX_LASTERROR)
        // Print error
        cerr << "IX error: " << IX_ErrorMsg[-rc + START_IX_ERR] << "\n";

    else if ((rc >= START_PF_WARN && rc <= PF_LASTWARN) ||
         (-rc >= -START_PF_ERR && -rc < -PF_LASTERROR) ||
              (rc == PF_UNIX))
    PF_PrintError(rc);

    else if ((rc >= START_RM_WARN && rc <= RM_LASTWARN) ||
         (-rc >= -START_RM_ERR && -rc < -RM_LASTERROR))
    RM_PrintError(rc);

    else if (rc == 0)
        cerr << "IX_PrintError called with return code of 0\n";

    else
        cerr << "IX error: " << rc << " is out of bounds\n";
}

