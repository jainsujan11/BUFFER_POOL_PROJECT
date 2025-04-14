#ifndef SQLITE_VFS_H
#define SQLITE_VFS_H

#include "sqlite3.h"
#include "disk.h"       // For SM_FileHandle
#include "buffer_mgr.h" // For BufferPool

// Structure that extends sqlite3_file. It holds a DiskManager file handle and a BufferPool pointer.
struct MySqliteFile {
    sqlite3_file base;   // Base class. Must be the first element.
    SM_FileHandle fh;    // Our custom file handle managed by DiskManager.
    BufferPool *bp;      // Pointer to the buffer manager handling pages.
};

 // Forward declarations for VFS callbacks.
static int myvfsOpen(sqlite3_vfs*, const char*, sqlite3_file*, int, int*);
static int myvfsDelete(sqlite3_vfs*, const char*, int);
static int myvfsAccess(sqlite3_vfs*, const char*, int, int*);
static int myvfsFullPathname(sqlite3_vfs*, const char*, int, char*);
static void* myvfsDlOpen(sqlite3_vfs*, const char*);
static void myvfsDlError(sqlite3_vfs*, int, char*);
static void (*myvfsDlSym(sqlite3_vfs*, void*, const char*))(void);
static void myvfsDlClose(sqlite3_vfs*, void*);
static int myvfsRandomness(sqlite3_vfs*, int, char*);
static int myvfsSleep(sqlite3_vfs*, int);
static int myvfsCurrentTime(sqlite3_vfs*, double*);
static int myvfsGetLastError(sqlite3_vfs*, int, char*);

#endif
