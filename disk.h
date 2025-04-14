#ifndef STORAGE_MGR_H
#define STORAGE_MGR_H

#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
using namespace std;
static const int PAGE_SIZE = 4096; // 4Kb pages 
class SM_FileHandle {
public:
    string fileName;
    int totalNumPages;
    int curPagePos; // cur pointer pos where we write bytes 
    void *mgmtInfo;

    SM_FileHandle(const string &name)
        : fileName(name), totalNumPages(0), curPagePos(0), mgmtInfo(nullptr) {}
};

using SM_PageHandle = char*;

class DiskManager {
public:
    void initStorageManager();
    bool createPageFile(const string &fileName, int numPages);
    bool openPageFile(const string &fileName, SM_FileHandle &fHandle);
    bool closePageFile(SM_FileHandle &fHandle);
    bool destroyPageFile(const string &fileName);

    bool readBlock(int pageNum, SM_FileHandle &fHandle, SM_PageHandle memPage);
    int getBlockPos(const SM_FileHandle &fHandle);
    bool readFirstBlock(SM_FileHandle &fHandle, SM_PageHandle memPage);
    bool readPreviousBlock(SM_FileHandle &fHandle, SM_PageHandle memPage);
    bool readCurrentBlock(SM_FileHandle &fHandle, SM_PageHandle memPage);
    bool readNextBlock(SM_FileHandle &fHandle, SM_PageHandle memPage);
    bool readLastBlock(SM_FileHandle &fHandle, SM_PageHandle memPage);

    bool writeBlock(int pageNum, SM_FileHandle &fHandle, SM_PageHandle memPage);
    bool writeCurrentBlock(SM_FileHandle &fHandle, SM_PageHandle memPage);
    bool appendEmptyBlock(SM_FileHandle &fHandle);
    bool ensureCapacity(int numberOfPages, SM_FileHandle &fHandle);
};

class DiskScheduler {
public:
    void scheduleReads();
    void scheduleWrites();
};

#endif


