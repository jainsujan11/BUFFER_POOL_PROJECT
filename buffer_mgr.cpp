#include<stdio.h>
#include<stdlib.h>
#include "disk.h"
#include "buffer_mgr.h"
#include <math.h>

// "bufferSize" represents the total number of frames in buffer i.e. maximum number of pages that can be kept into the buffer pool
int bufferSize = 0;

// "readCount" stores the count of number of pages read from the disk.
int readCount = 0;

// "writeCount" counts the number of I/O write to the disk i.e. number of pages writen to the disk
int writeCount = 0;

// "hit" is a counter which is incremented whenever a page is accessed and its value
// is stored in Frame::last_hit so that LRU algorithm can compare these relative times of last_hit to determine LRU page
int hit = 0;

// "clockPointer" is used by CLOCK algorithm to point to the last added page in the buffer pool.
int clockPointer = 0;

extern DiskManager dm;


// This function evicts MRU page in buffer(if possible)
// Args : page number of the new page to be loaded
// Return value : false (if all pages have pinCount>0) else true
bool BufferPool::MRU(int pageNum,string& pageFile){
	int mru_indx=-1,mru_value=-1;

	for(int i = 0; i < numPages; i++){
		// Finding frame whose pinCount = 0 i.e. no client is using that the page loaded in that frame
		if(mgmtData[i].pinCount == 0){
			if(mru_value<mgmtData[i].last_hit){
				mru_indx=i;
				mru_value=mgmtData[i].last_hit;
			}
		}
	}	

	//currently the buffer is full and all pages loaded in all frames are in use (i.e. pinCount!=0 for all frames)
	if(mru_indx==-1){
		return false;
	}

	cout<<"\tEvicted page "<<mgmtData[mru_indx].pageNum<<" of file "<<mgmtData[mru_indx].fileName<<" to load page "<<pageNum<<" of file "<<pageFile<<"\n";

	// If page in memory has been modified (dirtyBit = 1), then write page to disk
	if(mgmtData[mru_indx].dirtyBit == 1){
		forcePage(mru_indx,mgmtData[mru_indx].fileName);
	}

	// Setting page frame's content to new page's content
	readPage(pageNum,mru_indx,pageFile);
	mgmtData[mru_indx].pageNum = pageNum;
	mgmtData[mru_indx].fileName = pageFile;
	mgmtData[mru_indx].dirtyBit = 0;
	mgmtData[mru_indx].pinCount = 1;
	mgmtData[mru_indx].last_hit = hit++;


	return true;
}

// This function evicts LRU page in buffer(if possible)
// Args : page number of the new page to be loaded
// Return value : false (if all pages have pinCount>0) else true
bool BufferPool::LRU(int pageNum,string& pageFile){	
	int lru_indx=-1,lru_value=INT32_MAX;

	for(int i = 0; i < numPages; i++){
		// Finding frame whose pinCount = 0 i.e. no client is using that the page loaded in that frame
		if(mgmtData[i].pinCount == 0){
			if(lru_value>mgmtData[i].last_hit){
				lru_indx=i;
				lru_value=mgmtData[i].last_hit;
			}
		}
	}	

	//currently the buffer is full and all pages loaded in all frames are in use (i.e. pinCount!=0 for all frames)
	if(lru_indx==-1){
		return false;
	}

	cout<<"\tEvicted page "<<mgmtData[lru_indx].pageNum<<" of file "<<mgmtData[lru_indx].fileName<<" to load page "<<pageNum<<" of file "<<pageFile<<"\n";

	// If page in memory has been modified (dirtyBit = 1), then write page to disk
	if(mgmtData[lru_indx].dirtyBit == 1){
		forcePage(lru_indx,mgmtData[lru_indx].fileName);
	}

	// Setting page frame's content to new page's content
	readPage(pageNum,lru_indx,pageFile);
	mgmtData[lru_indx].pageNum = pageNum;
	mgmtData[lru_indx].fileName = pageFile;
	mgmtData[lru_indx].dirtyBit = 0;
	mgmtData[lru_indx].pinCount = 1;
	mgmtData[lru_indx].last_hit = hit++;


	return true;
}

// This function evicts a page in buffer(if possible) by using CLOCK algorithm
// Args : page number of the new page to be loaded
// Return value : false (if all pages have pinCount>0) else true
bool BufferPool::CLOCK(int pageNum,string& pageFile){	

	bool check=0;
	//check if any page has pinCount==0
	for(int i=0;i<numPages;i++){
		if(mgmtData[i].pinCount==0){
			check=1;
			break;
		}
	}

	if(!check)return false;

	while(1){
		if(clockPointer==numPages){
			clockPointer=0;
		}

		if(mgmtData[clockPointer].use_bit == 0){
			// If page in memory has been modified (dirtyBit = 1), then write page to disk
			if(mgmtData[clockPointer].dirtyBit == 1){
				forcePage(clockPointer,mgmtData[clockPointer].fileName);
			}

			cout<<"\tEvicted page "<<mgmtData[clockPointer].pageNum<<" of file "<<mgmtData[clockPointer].fileName<<" to load page "<<pageNum<<" of file "<<pageFile<<"\n";
			
			// Setting page frame's content to new page's content
			readPage(pageNum,clockPointer,pageFile);
			mgmtData[clockPointer].pageNum = pageNum;
			mgmtData[clockPointer].fileName = pageFile;
			mgmtData[clockPointer].dirtyBit = 0;
			mgmtData[clockPointer].pinCount = 1;
			mgmtData[clockPointer].use_bit = 1;

			// clockPointer++;//!
			return true;	
		}
		else{
			//unset use_bit of this frame
			mgmtData[clockPointer++].use_bit = 0;		
		}
	}
	return true;
}


// // ***** BUFFER POOL FUNCTIONS ***** //

BufferPool::~BufferPool(){
	for(int i=0;i<numPages;i++){
		if(mgmtData[i].data!=NULL){
			free(mgmtData[i].data);
			mgmtData[i].data=NULL;
		}
	}
	cout<<"BufferPool cleared\n";
}

BufferPool::BufferPool( int numPages_, ReplacementStrategy strategy_){
	numPages=numPages_;
	strategy=strategy_;

	mgmtData.resize(numPages);
		
	//Setting global variables for storing metadata of replacement strategies and disk read/write counts
	bufferSize = numPages;	
	clockPointer=0;
	writeCount=0;
	readCount=0;
	hit=0;

	//******** Initialize dm using dm_ */
	dm.initStorageManager();
		
}


//This function Clear all the memory allocated to buffer pool 
// Return value : true (if pincount of all frames == 0) else false
bool BufferPool::shutdownBufferPool(){
	
	for(int i = 0; i < bufferSize; i++){
		// If fixCount != 0, it means that the contents of the page was modified by some client and has not been written back to disk.
		if(mgmtData[i].pinCount != 0){
			return false;
		}
	}

	for(int i=0;i<bufferSize;i++){
		if(mgmtData[i].dirtyBit){
			forcePage(i,mgmtData[i].fileName);
		}
	}

	mgmtData.clear();  //check if calls destructor of Frame //?*
	return true;
}

// ***** PAGE MANAGEMENT FUNCTIONS ***** //

// This function marks a page as dirty indicating that the data of the page has been modified by the client
//  Args : pageNum (which is to be marked dirty)
//  Return value : true (if page is present in buffer pool) else false
bool BufferPool::markDirty (int pageNum,string& pageFile){
	for(int i = 0; i < bufferSize; i++){
		if(mgmtData[i].pageNum == pageNum && mgmtData[i].fileName==pageFile){
			mgmtData[i].dirtyBit = 1;
			return true;
		}			
	}		
	return false;
}

// This function unpins a page (indicating that one of the processes using this pageNum is not using it anymore)
//  Args : pageNum (which is to be unpinned)
//  Return value : true (if page is present in buffer pool) else false
bool BufferPool::unpinPage (int pageNum,string& pageFile){	
	for(int i = 0; i < bufferSize; i++){
		if(mgmtData[i].pageNum == pageNum && mgmtData[i].fileName==pageFile){
			mgmtData[i].pinCount--;
			//should we record hit here for this page ?
			// mgmtData[i].last_hit=hit++;
			return true;	
		}		
	}
	return false;
}

// This function writes the contents of the modified page back to the page file on disk
//  Args : indx (index of Frame to be written on disk)
//  Return value : true (if page is present in buffer pool) else false
void BufferPool::forcePage (int indx,string& pageFile){	

	// Actual writing of data 
	SM_FileHandle fh(pageFile);
	dm.openPageFile(pageFile, fh);
	dm.writeBlock(mgmtData[indx].pageNum, fh, mgmtData[indx].data);
		
	writeCount++;
}

// // This function reads data of a given page from disk into a given frame in buffer pool
// //  Args : pageNum (which is to be read from disk) , indx (index of frame to which data is to be written)
// //  Return value : true (if page is present in buffer pool) else false
void BufferPool::readPage (int pageNum, int indx, string& pageFile){	
	// Actual reading of data 
	SM_FileHandle fh(pageFile);
	dm.openPageFile(pageFile, fh);
	dm.readBlock(pageNum, fh, mgmtData[indx].data);
	
	readCount++;
}

//This function pins a page in buffer pool
//Args : BM_PageHandle reference (used to return the pointer of loaded page data) , pageNum
//Return value : false (if page cannot be loaded to buffer pool) else true
bool BufferPool::pinPage (BM_PageHandle& page, const PageNumber pageNum, string& pageFile){	
	//see if this page is already present in buffer pool
	cout<<"Request for page "<<pageNum<<" of file "<<pageFile<<"\n";
	for(int i=0;i<numPages;i++){
		if(mgmtData[i].pageNum == pageNum && mgmtData[i].fileName==pageFile){
			// Increasing pinCount 
			mgmtData[i].pinCount++;

			//update metadata for replacement strategies
			mgmtData[i].last_hit=hit++;
			mgmtData[i].use_bit=1;
			
			page.pageNum = pageNum;
			page.data = mgmtData[i].data;


			cout<<"\tPage "<<pageNum<<" of file "<<pageFile<<" already in buffer\n";


			return true;
		}
	}
	// page not in buffer pool
	//see if a frame is empty
	for(int i = 0; i < numPages; i++){
		if(mgmtData[i].pageNum==-1){
			
			if (mgmtData[i].data == nullptr) {
				mgmtData[i].data = (char*) malloc(PAGE_SIZE);  // or new char[PAGE_SIZE];
			}
			readPage(pageNum,i,pageFile);

			mgmtData[i].pageNum = pageNum;
			mgmtData[i].fileName=pageFile;
			mgmtData[i].pinCount = 1;
			
			mgmtData[i].last_hit=hit++;
			mgmtData[i].use_bit=1;

			page.pageNum = pageNum;
			page.data = mgmtData[i].data;

			cout<<"\tPage "<<pageNum<<" of file "<<pageFile<<" loaded in free frame "<<i<<"\n";

			return true;
		}
	}
			
	if(strategy == RS_LRU) return LRU(pageNum,pageFile);		
	else if(strategy == RS_CLOCK) return CLOCK(pageNum,pageFile);
	else if (strategy == RS_MRU) return MRU(pageNum,pageFile);	
	else {cout<<"Invalid strategy\n";return false;}				
}


// int main(){
// 	BufferPool bb(5,RS_CLOCK);

// }