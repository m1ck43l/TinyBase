#include <cerrno>
#include <cstdio>
#include <iostream>
#include "rm.h"
#include "redbase.h"

using namespace std;

//
// Error table
//
static char *RM_WarnMsg[] = {
    (char*)"invalid record (the record has not yet been read)",
    (char*)"file is already opened",
    (char*)"file is already closed",
    (char*)"scan is already opened",
    (char*)"scan is already closed",
    (char*)"no more record in file",
    (char*)"attribute is too long",
    (char*)"record not found",
    (char*)"file is not free"
};

static char *RM_ErrorMsg[] = {
    (char*)"invalid RID",
    (char*)"record too long to fit in one page"
};

//
// RM_PrintError
//
// Desc: Send a message corresponding to a RM return code to cerr
// In:   rc - return code for which a message is desired
//
void RM_PrintError(RC rc)
{
    // Check the return code is within proper limits
    if (rc >= START_RM_WARN && rc <= RM_LASTWARN)
        // Print warning
        cerr << "RM warning: " << RM_WarnMsg[rc - START_RM_WARN] << "\n";

    // Error codes are negative, so invert everything
    else if (-rc >= -START_RM_ERR && -rc < -RM_LASTERROR)
        // Print error
        cerr << "RM error: " << RM_ErrorMsg[-rc + START_RM_ERR] << "\n";

    else if ((rc >= START_PF_WARN && rc <= PF_LASTWARN) ||
	     (-rc >= -START_PF_ERR && -rc < -PF_LASTERROR) ||
              (rc == PF_UNIX))
	PF_PrintError(rc);

    else if (rc == 0)
        cerr << "RM_PrintError called with return code of 0\n";

    else
        cerr << "RM error: " << rc << " is out of bounds\n";
}
