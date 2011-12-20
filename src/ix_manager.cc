#include "ix.h"
#include <string>
#include <iostream>
#include <sstream>

using namespace std;

IX_Manager::IX_Manager(PF_Manager &pfm) : pf_manager(pfm) {}

IX_Manager::~IX_Manager() {}

RC IX_Manager::CreateIndex (const char *fileName, int indexNo, AttrType attrType, int attrLength) {
    const char* realFilename;
    ComputeFilename(fileName, indexNo, realFilename);

    return 0;
}

RC IX_Manager::DestroyIndex(const char* fileName, int indexNo) {
    const char* realFilename;
    ComputeFilename(fileName, indexNo, realFilename);

    return 0;
}

RC IX_Manager::OpenIndex(const char* fileName, int indexNo, IX_IndexHandle &indexHandle) {
    const char* realFilename;
    ComputeFilename(fileName, indexNo, realFilename);

    return 0;
}

RC IX_Manager::CloseIndex(IX_IndexHandle &indexHandle) {

    return 0;
}

RC IX_Manager::ComputeFilename(const char* fileName, int indexNo, const char*& computedFilename) {
    ostringstream oss;

    oss << fileName;
    oss << ".";
    oss << indexNo;

    computedFilename = oss.str().c_str();
    return 0;
}
