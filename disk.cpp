/* our header with DiskManager & DiskScheduler declarations */
#include "disk.h"  
#include <fstream>
#include <cstring>      
#include <sys/stat.h>   
#include <cstdio>       
#include <iostream>
using namespace std; 

DiskManager dm;

/* ----------- DiskManager Implementation ------------- */

void DiskManager::initStorageManager() {
    // Nothing to initialize for our C++ version.
}

// Create a new page file with one empty page.
bool DiskManager::createPageFile(const string &fileName, int numPages) {
    ofstream ofs(fileName, ios::trunc | ios::binary);
    if (!ofs)
        return false;

    char page[PAGE_SIZE];
    memset(page, 0, PAGE_SIZE);

    // Build a complete 100-byte database file header
    unsigned char header[100] = {0};

    // Bytes 0-15: Magic header
    const char *magic = "SQLite format 3\000";
    memcpy(header, magic, 16);

    // Bytes 16-17: Page size (big-endian)
    unsigned short pageSize = PAGE_SIZE;
    header[16] = (pageSize >> 8) & 0xFF;
    header[17] = pageSize & 0xFF;
    header[18] = 1;
    header[19] = 1;
    header[20] = 0;
    header[21] = 64;
    header[22] = 32;
    header[23] = 32;
    header[28] = 0;
    header[29] = 0;
    header[30] = 0;
    header[31] = 1;
    header[32] = 0;
    header[33] = 0;
    header[34] = 0;
    // The rest of the header is left zero (bytes 36-99)

    // Copy 100-byte file header to beginning of page 1
    memcpy(page, header, 100);

    // Create a valid table B-tree leaf page (starts at byte 100)
    page[100] = 0x0D;  // Page type: 0x0D = table leaf page
    page[101] = 0x00;  // Number of cells (high byte)
    page[102] = 0x00;  // Number of cells (low byte)
    page[103] = (PAGE_SIZE >> 8) & 0xFF;  // Start of content area (high byte)
    page[104] = PAGE_SIZE & 0xFF;         // Start of content area (low byte)
    page[105] = 0x00;  // Fragmented free space
    page[106] = 0x00;  // Right child pointer (0 for leaf)
    page[107] = 0x00;

    // Write the first page (with header + B-tree page)
    ofs.write(page, PAGE_SIZE);
    if (!ofs)
        return false;

    // Write the rest of the pages (if any)
    for (int i = 1; i < numPages; ++i) {
        memset(page, 0, PAGE_SIZE);
        ofs.write(page, PAGE_SIZE);
    }

    ofs.flush();
    ofs.close();
    return ofs.good();
}


// Open an existing page file and update the file handle.
bool DiskManager::openPageFile(const string &fileName, SM_FileHandle &fHandle) {
    ifstream ifs(fileName, ios::binary);
    if (!ifs)
        return false;
    
    // Update the file handle with file name and reset current page position.
    fHandle.fileName = fileName;
    fHandle.curPagePos = 0;
    
    // Use stat to get the file size.
    struct stat fileInfo;
    if(stat(fileName.c_str(), &fileInfo) < 0)
        return false;
    
    fHandle.totalNumPages = fileInfo.st_size / PAGE_SIZE;
    ifs.close();
    return true;
}

// In this design, each function opens/closes the file as needed so closing is a no-op.
bool DiskManager::closePageFile(SM_FileHandle &fHandle) {
    return true;
}

// Remove the file from the file system.
bool DiskManager::destroyPageFile(const string &fileName) {
    ifstream ifs(fileName, ios::binary);
    if (!ifs)
        return false;
    ifs.close();
    return (remove(fileName.c_str()) == 0);
}

// Read a block (page) from the file.
bool DiskManager::readBlock(int pageNum, SM_FileHandle &fHandle, SM_PageHandle memPage) {
    if (pageNum < 0 || pageNum >= fHandle.totalNumPages)
        return false;
    
    ifstream ifs(fHandle.fileName, ios::binary);
    if (!ifs)
        return false;
    
    // Seek to the correct block offset.
    ifs.seekg(pageNum * PAGE_SIZE, ios::beg);
    if (!ifs)
        return false;
    
    // Read PAGE_SIZE bytes into memPage.
    ifs.read(memPage, PAGE_SIZE);
    if (!ifs)
        return false;
    
    cout << "[readBlock] Reading page " << pageNum << " of " << fHandle.fileName << endl;
    // Update the file handle's current page position.
    fHandle.curPagePos = static_cast<int>(ifs.tellg());
    ifs.close();
    return true;
}

// Return the current block position (byte offset) in the file.
int DiskManager::getBlockPos(const SM_FileHandle &fHandle) {
    return fHandle.curPagePos;
}

// Read the first block.
bool DiskManager::readFirstBlock(SM_FileHandle &fHandle, SM_PageHandle memPage) {
    return readBlock(0, fHandle, memPage);
}

// Read the previous block relative to current position.
bool DiskManager::readPreviousBlock(SM_FileHandle &fHandle, SM_PageHandle memPage) {
    int currentPageNumber = fHandle.curPagePos / PAGE_SIZE;
    return readBlock(currentPageNumber - 1, fHandle, memPage);
}

// Read the current block.
bool DiskManager::readCurrentBlock(SM_FileHandle &fHandle, SM_PageHandle memPage) {
    int currentPageNumber = fHandle.curPagePos / PAGE_SIZE;
    return readBlock(currentPageNumber, fHandle, memPage);
}

// Read the next block relative to current position.
bool DiskManager::readNextBlock(SM_FileHandle &fHandle, SM_PageHandle memPage) {
    int currentPageNumber = fHandle.curPagePos / PAGE_SIZE;
    return readBlock(currentPageNumber + 1, fHandle, memPage);
}

// Read the last block.
bool DiskManager::readLastBlock(SM_FileHandle &fHandle, SM_PageHandle memPage) {
    return readBlock(fHandle.totalNumPages - 1, fHandle, memPage);
}

// Write a block to the file at a given page number.
bool DiskManager::writeBlock(int pageNum, SM_FileHandle &fHandle, SM_PageHandle memPage) {
    if (pageNum < 0)
        return false;

    if (pageNum >= fHandle.totalNumPages) {
        while (fHandle.totalNumPages <= pageNum) {
            if (!appendEmptyBlock(fHandle))
                return false;
        }
    }

    fstream fs(fHandle.fileName, ios::in | ios::out | ios::binary);
    if (!fs) {
        cerr << "[writeBlock] Failed to open file " << fHandle.fileName << "\n";
        return false;
    }

    fs.seekp(pageNum * PAGE_SIZE, ios::beg);
    if (!fs) {
        cerr << "[writeBlock] Failed to seek to page " << pageNum << "\n";
        return false;
    }

    fs.write(memPage, PAGE_SIZE);
    if (!fs) {
        cerr << "[writeBlock] Failed to write page " << pageNum << "\n";
        return false;
    }

    fs.flush();
    fHandle.curPagePos = pageNum * PAGE_SIZE;
    fs.close();
    return true;
}


// Write the current block (calculated from the current position).
bool DiskManager::writeCurrentBlock(SM_FileHandle &fHandle, SM_PageHandle memPage) {
    int currentPageNumber = fHandle.curPagePos / PAGE_SIZE;
    // In this version we assume that writing to the current block doesn't change total number of pages.
    return writeBlock(currentPageNumber, fHandle, memPage);
}

// Append an empty block (page) to the file.
bool DiskManager::appendEmptyBlock(SM_FileHandle &fHandle) {
    ofstream ofs(fHandle.fileName, ios::binary | ios::app);
    if (!ofs)
        return false;
    
    char emptyBlock[PAGE_SIZE];
    memset(emptyBlock, 0, PAGE_SIZE);
    ofs.write(emptyBlock, PAGE_SIZE);
    ofs.flush();
    if (!ofs)
        return false;
    
    // Update the total number of pages.
    fHandle.totalNumPages++;
    ofs.close();
    return true;
}

// Ensure the file has at least the given number of pages.
bool DiskManager::ensureCapacity(int numberOfPages, SM_FileHandle &fHandle) {
    // Open file to check.
    ifstream ifs(fHandle.fileName, ios::binary);
    if (!ifs)
        return false;
    ifs.close();
    
    while (fHandle.totalNumPages < numberOfPages) {
        if (!appendEmptyBlock(fHandle))
            return false;
    }
    return true;
}
