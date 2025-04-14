/*
    A custom SQLite Virtual File System (VFS) implementation that routes
    file I/O operations through your Buffer Manager instead of directly
    using the DiskManager. This allows your page replacement strategies (LRU,
    CLOCK, MRU) in the Buffer Manager to be active.
*/

#include "sqlite3.h"
#include "sqlite_vfs.h"  // Contains MySqliteFile structure and function prototypes
#include "disk.h"        // Contains DiskManager and PAGE_SIZE definition
#include "buffer_mgr.h"  // Contains BufferPool, BM_PageHandle, and related declarations
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <unistd.h>      // For usleep()

// External DiskManager instance declared in disk.cpp
extern DiskManager dm;

#define NUM_FRAMES 1000
#define STRATEGY static_cast<ReplacementStrategy>(1)  // Cast to ReplacementStrategy; adjust as needed

// --- VFS File I/O methods using Buffer Manager ---

// xClose: Frees the BufferPool object if present.
static int myvfsClose(sqlite3_file *pFile) {
    MySqliteFile *myFile = (MySqliteFile*)pFile;
    if (myFile->bp) {
        myFile->bp->shutdownBufferPool();
        delete myFile->bp;
        myFile->bp = nullptr;
    }
    return SQLITE_OK;
}

// xRead: Read iAmt bytes from the file at offset using the BufferPool.
// This version handles reads spanning across page boundaries.
static int myvfsRead(sqlite3_file *pFile, void *zBuf, int iAmt, sqlite3_int64 offset) {
    MySqliteFile *myFile = (MySqliteFile*)pFile;
    int bytesRemaining = iAmt;
    int totalCopied = 0;
    while (bytesRemaining > 0) {
        int pageNum = offset / PAGE_SIZE;
        int offsetWithinPage = offset % PAGE_SIZE;
        int available = PAGE_SIZE - offsetWithinPage;
        int toCopy = (bytesRemaining < available) ? bytesRemaining : available;
        
        BM_PageHandle bmPage;
        if (!myFile->bp->pinPage(bmPage, pageNum, myFile->fh.fileName))
            return SQLITE_IOERR_READ;
        
        memcpy(static_cast<char*>(zBuf) + totalCopied, bmPage.data + offsetWithinPage, toCopy);
        myFile->bp->unpinPage(pageNum, myFile->fh.fileName);
        
        totalCopied += toCopy;
        offset += toCopy;
        bytesRemaining -= toCopy;
    }
    return SQLITE_OK;
}

// xWrite: Write iAmt bytes from zBuf into the file at offset using the Buffer Manager.
// This version handles writes spanning across page boundaries.
static int myvfsWrite(sqlite3_file *pFile, const void *zBuf, int iAmt, sqlite3_int64 offset) {
    MySqliteFile *myFile = (MySqliteFile*)pFile;
    int bytesRemaining = iAmt;
    int totalCopied = 0;
    while (bytesRemaining > 0) {
        int pageNum = offset / PAGE_SIZE;
        int offsetWithinPage = offset % PAGE_SIZE;
        int available = PAGE_SIZE - offsetWithinPage;
        int toCopy = (bytesRemaining < available) ? bytesRemaining : available;
        
        BM_PageHandle bmPage;
        if (!myFile->bp->pinPage(bmPage, pageNum, myFile->fh.fileName))
            return SQLITE_IOERR_WRITE;
        
        memcpy(bmPage.data + offsetWithinPage, static_cast<const char*>(zBuf) + totalCopied, toCopy);
        myFile->bp->markDirty(pageNum, myFile->fh.fileName);
        myFile->bp->unpinPage(pageNum, myFile->fh.fileName);
        
        totalCopied += toCopy;
        offset += toCopy;
        bytesRemaining -= toCopy;
    }
    return SQLITE_OK;
}

// xTruncate: Not implemented in this simulation.
// static int myvfsTruncate(sqlite3_file *pFile, sqlite3_int64 size) {
//     return SQLITE_OK;
// }

static int myvfsTruncate(sqlite3_file *pFile, sqlite3_int64 size) {
    // Get our custom file structure.
    MySqliteFile *myFile = (MySqliteFile*)pFile;
    SM_FileHandle &fh = myFile->fh;
    
    // Calculate the current file size based on total number of pages.
    sqlite3_int64 currentSize = static_cast<sqlite3_int64>(fh.totalNumPages) * PAGE_SIZE;
    
    // If the requested size is larger than current size, extend the file.
    if (size > currentSize) {
        // Calculate the number of pages required.
        int requiredPages = static_cast<int>((size + PAGE_SIZE - 1) / PAGE_SIZE);
        // Append empty pages until the file reaches the required number of pages.
        while (fh.totalNumPages < requiredPages) {
            if (!dm.appendEmptyBlock(fh)) {
                // Failed to append an empty block.
                return SQLITE_IOERR_WRITE;
            }
        }
    }
    // If the size is smaller, you could implement truncation here.
    // For many simulation purposes, shrinking is not required.
    // Otherwise, you could use platform-specific calls (e.g., ftruncate) to shrink the file.
    
    return SQLITE_OK;
}

// xSync: Nothing extra to do in simulation.
static int myvfsSync(sqlite3_file *pFile, int flags) {
    return SQLITE_OK;
}

// xFileSize: Return the size of the file (number of pages * PAGE_SIZE).
static int myvfsFileSize(sqlite3_file *pFile, sqlite3_int64 *pSize) {
    MySqliteFile *myFile = (MySqliteFile*)pFile;
    *pSize = static_cast<sqlite3_int64>(myFile->fh.totalNumPages) * PAGE_SIZE;
    return SQLITE_OK;
}

// The following functions are kept as in the original implementation.
static int myvfsLock(sqlite3_file *pFile, int eLock) { return SQLITE_OK; }
static int myvfsUnlock(sqlite3_file *pFile, int eLock) { return SQLITE_OK; }
static int myvfsCheckReservedLock(sqlite3_file *pFile, int *pResOut) { *pResOut = 0; return SQLITE_OK; }
static int myvfsFileControl(sqlite3_file *pFile, int op, void *pArg) { return SQLITE_NOTFOUND; }
static int myvfsSectorSize(sqlite3_file *pFile) { return PAGE_SIZE; }
static int myvfsDeviceCharacteristics(sqlite3_file *pFile) { return 0; }

// sqlite3_io_methods structure holding our I/O callbacks.
static sqlite3_io_methods myIoMethods = {
    1,                          // iVersion
    myvfsClose,                 // xClose
    myvfsRead,                  // xRead
    myvfsWrite,                 // xWrite
    myvfsTruncate,              // xTruncate
    myvfsSync,                  // xSync
    myvfsFileSize,              // xFileSize
    myvfsLock,                  // xLock
    myvfsUnlock,                // xUnlock
    myvfsCheckReservedLock,     // xCheckReservedLock
    myvfsFileControl,           // xFileControl
    myvfsSectorSize,            // xSectorSize
    myvfsDeviceCharacteristics  // xDeviceCharacteristics
};

static int myvfsOpen(sqlite3_vfs *vfs, const char *zName, sqlite3_file *pFile, int flags, int *pOutFlags) {
    // Clear the sqlite3_file struct.
    memset(pFile, 0, vfs->szOsFile);
    MySqliteFile *myFile = (MySqliteFile*)pFile;
    string filename(zName);

    // Try to open the file if it exists.
    bool fileExists = dm.openPageFile(filename, myFile->fh);

    // If the file does not exist and the CREATE flag is set, create a new file.
    if (!fileExists) {
        if (flags & SQLITE_OPEN_CREATE) {
            // Create a file with at least one page.
            if (!dm.createPageFile(filename, 1)) {
                cerr << "[myvfsOpen] Failed to create file: " << filename << "\n";
                return SQLITE_CANTOPEN;
            }
            // Reopen the file to initialize the file handle.
            if (!dm.openPageFile(filename, myFile->fh)) {
                cerr << "[myvfsOpen] Failed to open newly created file: " << filename << "\n";
                return SQLITE_CANTOPEN;
            }
            fileExists = true;
        } else {
            cerr << "[myvfsOpen] File does not exist and CREATE flag not set: " << filename << "\n";
            return SQLITE_CANTOPEN;
        }
    }

    // Set up BufferPool for this file.
    // (If you want each file to have its own BufferPool, you can create a new one;
    // here we create one per file using NUM_FRAMES and STRATEGY.)
    myFile->bp = new BufferPool(NUM_FRAMES, STRATEGY);
    myFile->base.pMethods = &myIoMethods;

    if (pOutFlags)
        *pOutFlags = flags;

    cout << "[myvfsOpen] Opened file (exists=" << (fileExists ? 1 : 0) << "): " << filename << "\n";

    return SQLITE_OK;
}

static int myvfsDelete(sqlite3_vfs *vfs, const char *zName, int syncDir) {
    return dm.destroyPageFile(zName) ? SQLITE_OK : SQLITE_IOERR_DELETE;
}

static int myvfsAccess(sqlite3_vfs *vfs, const char *zName, int flags, int *pResOut) {
    SM_FileHandle fh{string(zName)};  // Use braces to construct an SM_FileHandle with zName.
    bool exists = dm.openPageFile(zName, fh);
    *pResOut = exists ? 1 : 0;
    return SQLITE_OK;
}

static int myvfsFullPathname(sqlite3_vfs *vfs, const char *zName, int nOut, char *zOut) {
    strncpy(zOut, zName, nOut);
    zOut[nOut - 1] = '\0';
    return SQLITE_OK;
}

static void* myvfsDlOpen(sqlite3_vfs *vfs, const char *zPath) {
    return nullptr;
}

static void myvfsDlError(sqlite3_vfs *vfs, int nByte, char *zErrMsg) {
    strncpy(zErrMsg, "Dynamic loading not supported", nByte);
}

static void (*myvfsDlSym(sqlite3_vfs *vfs, void *p, const char *zSymbol))(void) {
    return nullptr;
}

static void myvfsDlClose(sqlite3_vfs *vfs, void *pHandle) {
}

static int myvfsRandomness(sqlite3_vfs *vfs, int nByte, char *zOut) {
    for (int i = 0; i < nByte; i++)
        zOut[i] = (char)(rand() % 256);
    return nByte;
}

static int myvfsSleep(sqlite3_vfs *vfs, int microseconds) {
    usleep(microseconds);
    return microseconds;
}

static int myvfsCurrentTime(sqlite3_vfs *vfs, double *pOut) {
    time_t t = time(nullptr);
    *pOut = t / 86400.0 + 2440587.5;
    return SQLITE_OK;
}

static int myvfsGetLastError(sqlite3_vfs *vfs, int nByte, char *zErrMsg) {
    if(nByte > 0)
        zErrMsg[0] = '\0';
    return SQLITE_OK;
}

// Define the sqlite3_vfs structure.
static sqlite3_vfs myVfs = {
    1,                        // iVersion
    sizeof(MySqliteFile),     // szOsFile
    PAGE_SIZE,                // mxPathname
    nullptr,                  // pNext
    "myvfs",                  // zName
    nullptr,                  // pAppData
    myvfsOpen,
    myvfsDelete,
    myvfsAccess,
    myvfsFullPathname,
    myvfsDlOpen,
    myvfsDlError,
    myvfsDlSym,
    myvfsDlClose,
    myvfsRandomness,
    myvfsSleep,
    myvfsCurrentTime,
    myvfsGetLastError,
    nullptr                   // xCurrentTimeInt64 (optional)
};

extern "C" {
int sqlite3_os_init(void) {
    return sqlite3_vfs_register(&myVfs, 1);  // Register as the default VFS.
}
int sqlite3_os_end(void) {
    return SQLITE_OK;
}
} // extern "C"
