#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include <string>
#include <cstring>
#include <vector>
#include <iostream>
#include "disk.h"

using namespace std;

// Replacement Strategies
typedef enum ReplacementStrategy {
  RS_LRU = 1,
  RS_CLOCK = 2,
  RS_MRU = 3,
} ReplacementStrategy;

// Data Types and Structures
typedef int PageNumber;
#define NO_PAGE -1
#define NO_HIT -1

//This class use to store all data for a Frame in buffer
class Frame {
	  char * data; // Actual data of the page + null character at end
  
    PageNumber pageNum; // Page number currently loaded in this frame
    string fileName; //name of file whose page is currently in disk
    bool dirtyBit; // Used to indicate whether the contents of the page (loaded in this frame) has been modified by the client
    int pinCount; // Used to indicate the number of processes(internal to the DBMS) using the page (loaded in this frame) at a given instance
    int last_hit;  // Used by LRU algorithm to get the least recently used page	
    bool use_bit; //Used by CLOCK algorithm
    
    public:
    Frame():pageNum(NO_PAGE),dirtyBit(0),pinCount(0),last_hit(NO_HIT){
      data=(char*)malloc(PAGE_SIZE+1);
      data[PAGE_SIZE]='\0';
    }
    ~Frame(){
      if(data!=NULL)free(data);
      data=NULL;
    }

  friend class BufferPool;
};

typedef struct BM_PageHandle {
  PageNumber pageNum;
  char *data;
} BM_PageHandle;


class BufferPool{
    int numPages; //total number of frames in buffer (maximum pages it can hold)
    ReplacementStrategy strategy;//strategy used for page replacement

    vector<Frame> mgmtData; //use to store metadata for each frame (as struct)

    public:
    BufferPool(int numPages, ReplacementStrategy strategy);
    bool markDirty (int page_num,string& pageFile);
    bool unpinPage (int pageNum,string& pageFile);
    bool pinPage (BM_PageHandle& page, const PageNumber pageNum,string& pageFile);

    void forcePage (int indx,string& pageFile);
    void readPage (int pageNum,int indx,string& pageFile);

    bool LRU(int pageNum,string& pageFile);
    bool CLOCK(int pageNum,string& pageFile);
    bool MRU(int pageNum,string& pageFile);

    bool shutdownBufferPool();
    ~BufferPool();
};

#endif