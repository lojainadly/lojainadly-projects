/////////////////////////////////////////////////////////////////////////////////
// Main File:        buf.C
// Semester:         CS 564 Lecture 001   FALL 2024
// Instructor:       AnHai
//
// Purpose: To handle the requests of reading and writing from the database, whilst
// maintaining the buffer.
//
// Authors:          Lojain Adly 
//                   Henry Burke 
//                   Tze Khye Tan 
// Emails:           ladly@wisc.edu
//                   hpburke@wisc.edu
//                   ttan38@wisc.edu
/////////////////////////////////////////////////////////////////////////////////

#include <memory.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include "page.h"
#include "buf.h"

#define ASSERT(c)  { if (!(c)) { \
                       cerr << "At line " << __LINE__ << ":" << endl << "  "; \
                       cerr << "This condition should hold: " #c << endl; \
                       exit(1); \
                     } \
                   }

//----------------------------------------
// Constructor of the class BufMgr
//----------------------------------------

BufMgr::BufMgr(const int bufs)
{
    numBufs = bufs;

    bufTable = new BufDesc[bufs];
    memset(bufTable, 0, bufs * sizeof(BufDesc));
    for (int i = 0; i < bufs; i++)
    {
        bufTable[i].frameNo = i;
        bufTable[i].valid = false;
    }

    bufPool = new Page[bufs];
    memset(bufPool, 0, bufs * sizeof(Page));

    int htsize = ((((int) (bufs * 1.2))*2)/2)+1;
    hashTable = new BufHashTbl (htsize);  // allocate the buffer hash table

    clockHand = bufs - 1;
}


BufMgr::~BufMgr() {

    // flush out all unwritten pages
    for (int i = 0; i < numBufs; i++)
    {
        BufDesc* tmpbuf = &bufTable[i];
        if (tmpbuf->valid == true && tmpbuf->dirty == true) {

#ifdef DEBUGBUF
            cout << "flushing page " << tmpbuf->pageNo
                 << " from frame " << i << endl;
#endif

            tmpbuf->file->writePage(tmpbuf->pageNo, &(bufPool[i]));
        }
    }

    delete [] bufTable;
    delete [] bufPool;
}

/*
Allocates a free buffer frame using the clock algorithm to load a page into memory.
Input Parameter: int &frame – Output parameter that will be set to the allocated frame number.
Return Values: Frame updated successfully. If no free frames are available, returns BUFFEREXCEEDED.
*/
const Status BufMgr::allocBuf(int & frame)
{
    int attempts = 2 * numBufs; // Attempts finding a free frame
    while (attempts >= 0) {
        clockHand = (clockHand + 1) % numBufs;
        BufDesc* tmpbuf = &bufTable[clockHand];
        if (!tmpbuf->valid) {
            // Use frame if not valid
            frame = tmpbuf->frameNo;
            return OK;
        } else if (!tmpbuf->refbit && tmpbuf->pinCnt == 0) {
            // If frame is not referenced & pinCnt > 0, use it after writing if dirty
            if (tmpbuf->dirty) {
                // Write page back to disk
                tmpbuf->file->writePage(tmpbuf->pageNo, &bufPool[clockHand]);
                bufStats.accesses++;
                bufStats.diskwrites++; // increment disk writes
                tmpbuf->dirty = false;

            }
            hashTable->remove(bufTable[clockHand].file, bufTable[clockHand].pageNo);
            bufTable[clockHand].Clear();
            // Update buffer entry for the new page
            frame = tmpbuf->frameNo;
            return OK;
        }else {
            // Set reference bit = false then continue searching
            tmpbuf->refbit = false;
        }
        attempts--;
    }
    return BUFFEREXCEEDED; // All buffer frames are pinned
}

/*
    readPage: If a page exists in the buffer, returns it to be read. If not, reads from disk
            into the buffer and returns it to be read.

    inputs:
    File* file: a file object to read specific page from
    const int PageNo: page number in file
    Page*& page: page object to pass read page into

    output:
    page: returns OK with the page read from buffer/disk

    errors:
    UNIXERR: unix error occurred
    BUFFEREXCEEDED: all buffer frames are pinned
    HASHTBLERROR: hash table error ocurred
*/
const Status BufMgr::readPage(File* file, const int PageNo, Page*& page)
{
    // first check whether page is already in buffer pool
    int frameNo;
    Status status = hashTable->lookup(file, PageNo, frameNo);
    bufStats.diskreads++;

    // case 1: page is not in buffer pool
    if (status == HASHNOTFOUND) {
            // allocate a buffer frame
            status = allocBuf(frameNo);
            if (status != OK) {
                    return status;
            }

            // call file->readPage() to read page from disk into buffer pool frame
            status = file->readPage(PageNo, &bufPool[frameNo]);

            if (status != OK) {
                    return status;
            }

            // insert page into the hashtable
            status = hashTable->insert(file, PageNo, frameNo);

            if (status != OK) {
                    return status;
            }

            // invoke Set() on the frame to set it up properly
            bufTable[frameNo].Set(file, PageNo);
            bufTable[frameNo].pinCnt = 1;
            page = &bufPool[frameNo];
            return OK;
    } else if (status == OK) {
            // case 2: page is in buffer pool
            // set appropriate refbit
            bufTable[frameNo].refbit = true;
            // increment the pinCnt for the page
            bufTable[frameNo].pinCnt++;
            page = &bufPool[frameNo];
            return OK;
    }
    return status;
}

/**
 * Decrements the pinCnt of the frame containing (file, PageNo) and, if dirty == true, sets the dirty bit.
 * Returns OK if no errors occurred, HASHNOTFOUND if the page is not in the buffer pool hash table,
 * PAGENOTPINNED if the pin count is already 0.
 *
 * File *file: the file that the page is in
 * int PageNo: the page number of the page
 * bool dirty: whether the page is dirty
 * Returns: OK if no errors occurred, HASHNOTFOUND if the page is not in the buffer pool hash table,
 */
const Status BufMgr::unPinPage(File* file, const int PageNo,
                               const bool dirty)
{
    int frameNo;
    // see if it is in the buffer pool
    Status status = hashTable->lookup(file, PageNo, frameNo);
    // if it is not in the buffer pool, return HASHNOTFOUND
    if (status != OK) {
        return status;
    }
    // check if the page is already unpinned or dirty, if dirty, set the dirty bit
    if (dirty == true) {
        bufTable[frameNo].dirty = true;
    }
    if (bufTable[frameNo].pinCnt == 0) {
        return PAGENOTPINNED;
    }
    // decrement the pin count
    bufTable[frameNo].pinCnt--;

    return OK;
}


/**
 * This call is kind of weird.  The first step is to to allocate an empty page in the specified file by invoking
 *  the file->allocatePage() method. This method will return the page number of the newly allocated page.
 * Then allocBuf() is called to obtain a buffer pool frame.  Next, an entry is inserted into the hash table and
 *  Set() is invoked on the frame to set it up properly.  The method returns both the page number of the newly
 *  allocated page to the caller via the pageNo parameter and a pointer to the buffer frame allocated for the
 * page via the page parameter. Returns OK if no errors occurred, UNIXERR if a Unix error occurred,
 *  BUFFEREXCEEDED if all buffer frames are pinned and HASHTBLERROR if a hash table error occurred.
 *
 * File *file: the file that the page is in
 * int pageNo: the page number of the page
 * Page *page: the page that is being allocated
 * Returns: OK if no errors occurred, UNIXERR if a Unix error occurred, BUFFEREXCEEDED if all buffer frames
 * are pinned, HASHTBLERROR if a hash table error occurred.
 */
const Status BufMgr::allocPage(File* file, int& pageNo, Page*& page)
{
    // allocate an empty page in the specified file, call file->allocatePage()
    Status status = file->allocatePage(pageNo);
    // return UNIXERR if a Unix error occurred
    if (status != OK) {
        return status;
    }
    // get a buffer pool frame, call allocBuf()
    int frameNo;
    status = allocBuf(frameNo);
    // return BUFFEREXCEEDED if all buffer frames are pinned
    if (status != OK) {
        return status;
    }
    // insert an entry into the hash table and set it up properly
    status = hashTable->insert(file, pageNo, frameNo);
    // return HASHTBLERROR if a hash table error occurred
    if (status != OK) {
        return status;
    }
    bufTable[frameNo].Set(file, pageNo);
    // return the page number of the newly allocated page and a pointer to the buffer frame allocated for the page
    page = &bufPool[frameNo];
    pageNo = bufTable[frameNo].pageNo;
    return OK;
}

const Status BufMgr::disposePage(File* file, const int pageNo)
{
    // see if it is in the buffer pool
    Status status = OK;
    int frameNo = 0;
    status = hashTable->lookup(file, pageNo, frameNo);
    if (status == OK)
    {
        // clear the page
        bufTable[frameNo].Clear();
    }
    status = hashTable->remove(file, pageNo);

    // deallocate it in the file
    return file->disposePage(pageNo);
}

const Status BufMgr::flushFile(const File* file)
{
  Status status;

  for (int i = 0; i < numBufs; i++) {
    BufDesc* tmpbuf = &(bufTable[i]);
    if (tmpbuf->valid == true && tmpbuf->file == file) {

      if (tmpbuf->pinCnt > 0)
          return PAGEPINNED;

      if (tmpbuf->dirty == true) {
#ifdef DEBUGBUF
        cout << "flushing page " << tmpbuf->pageNo
             << " from frame " << i << endl;
#endif
        if ((status = tmpbuf->file->writePage(tmpbuf->pageNo,
                                              &(bufPool[i]))) != OK)
          return status;

        tmpbuf->dirty = false;
      }

      hashTable->remove(file,tmpbuf->pageNo);

      tmpbuf->file = NULL;
      tmpbuf->pageNo = -1;
      tmpbuf->valid = false;
    }

    else if (tmpbuf->valid == false && tmpbuf->file == file)
      return BADBUFFER;
  }

  return OK;
}


void BufMgr::printSelf(void)
{
    BufDesc* tmpbuf;

    cout << endl << "Print buffer...\n";
    for (int i=0; i<numBufs; i++) {
        tmpbuf = &(bufTable[i]);
        cout << i << "\t" << (char*)(&bufPool[i])
             << "\tpinCnt: " << tmpbuf->pinCnt;

        if (tmpbuf->valid == true)
            cout << "\tvalid\n";
        cout << endl;
    };
}
