/////////////////////////////////////////////////////////////////////////////////
// Main File:        heapfile.C
// Semester:         CS 564 Lecture 001   FALL 2024
// Instructor:       AnHai Doan
//
// Purpose: To handle creating and managing a file manager for heap files that
//          provides a scan mechanism to search a heap file for records that
//          satisfy a predicate.
// Authors:          Lojain Adly
//                   Henry Burke
//                   Tze Khye Tan
// Emails:           ladly@wisc.edu
//                   hpburke@wisc.edu
//                   ttan38@wisc.edu
/////////////////////////////////////////////////////////////////////////////////

#include "heapfile.h"
#include "error.h"

/**
 * Creates a heap file with the specified file name.
 * This function attempts to create a new heap file with a header page and an initial data page.
 * If the file already exists, it returns an error indicating the file exists.
 * If any step in the file creation or page allocation fails, it returns the corresponding error status.
 *
 * @param fileName The name of the file to be created.
 * @return Status The status of the operation, indicating success or the type of error encountered.
 */
const Status createHeapFile(const string fileName)
{
    File *file;
    Status status;
    FileHdrPage *hdrPage;
    // int hdrPageNo;
    int newPageNo;
    Page *newPage;

    // try to open the file. This should return an error
    status = db.openFile(fileName, file);
    if (status != OK)
    {
        // file doesn't exist. First create it and allocate
        // an empty header page and data page.

        status = db.createFile(fileName);
        if (status != OK)
        {
            return status; // return if file creation fails
        }

        status = db.openFile(fileName, file); // open the newly created file
        if (status != OK)
        {
            return status; // return if file opening fails
        }

        int tempPageNo;
        Page *tempPage;

        // allocate for a header page
        status = bufMgr->allocPage(file, tempPageNo, tempPage);
        if (status == OK)
        {
            // initialzie the header page
            hdrPage = (FileHdrPage *)tempPage;
            fileName.copy(hdrPage->fileName, min((const unsigned int)fileName.size(), MAXNAMESIZE));

            // allocate the first data page
            status = bufMgr->allocPage(file, newPageNo, newPage);
            if (status == OK)
            {
                newPage->init(newPageNo);

                // update header with the first page info
                hdrPage->firstPage = newPageNo;
                hdrPage->lastPage = newPageNo;
                hdrPage->pageCnt = 1;

                status = bufMgr->unPinPage(file, newPageNo, true); // unpin the data page
                if (status != OK)
                {
                    db.closeFile(file);
                    return status;
                }
            }
            // unpin the header page
            status = bufMgr->unPinPage(file, tempPageNo, true);
            if (status != OK)
            {
                db.closeFile(file);
                return status;
            }
        }
        // close the file
        status = db.closeFile(file);
    }
    else
    {
        db.closeFile(file);
        status = FILEEXISTS;
    }
    return status;
}

// routine to destroy a heapfile
const Status destroyHeapFile(const string fileName)
{
    return (db.destroyFile(fileName));
}

/**
 * This constructor attempts to open the file with the given name, read its header page, and the first data page.
 * It sets up the internal state of the heap file object, including current page and record pointers.
 *
 * @param fileName The name of the file to open and read from.
 * @param returnStatus Reference to a Status variable where the status of the file operation is returned.
 *                     This will be set to OK if all operations succeed, or an error status if any operation fails.
 *
 * If the file cannot be opened or read correctly, an error message is printed to the standard error stream,
 *       and the function returns early with the appropriate error status in returnStatus.
 */
HeapFile::HeapFile(const string &fileName, Status &returnStatus)
{
    Status status;
    Page *pagePtr;

    cout << "opening file " << fileName << endl;

    // open the file and read in the header page and the first data page
    if ((status = db.openFile(fileName, filePtr)) == OK)
    {
        if ((status = filePtr->getFirstPage(headerPageNo)) == OK)
        {
            if ((status = bufMgr->readPage(filePtr, headerPageNo, pagePtr)) == OK)
            {
                headerPage = (FileHdrPage *)pagePtr;
                hdrDirtyFlag = false;
                curPageNo = headerPage->firstPage;
                if ((status = bufMgr->readPage(filePtr, curPageNo, curPage)) == OK)
                {
                    curDirtyFlag = false;
                    curRec = NULLRID;
                    returnStatus = OK; // all done
                }
                else
                {
                    returnStatus = status;
                }
            }
            else
            {
                returnStatus = status;
            }
        }
        else
        {
            returnStatus = status;
        }
    }
    else
    {
        cerr << "open of heap file failed\n";
        returnStatus = status;
        return;
    }
}

// the destructor closes the file
HeapFile::~HeapFile()
{
    Status status;
    cout << "invoking heapfile destructor on file " << headerPage->fileName << endl;

    // see if there is a pinned data page. If so, unpin it
    if (curPage != NULL)
    {
        status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
        curPage = NULL;
        curPageNo = 0;
        curDirtyFlag = false;
        if (status != OK)
            cerr << "error in unpin of date page\n";
    }

    // unpin the header page
    status = bufMgr->unPinPage(filePtr, headerPageNo, hdrDirtyFlag);
    if (status != OK)
        cerr << "error in unpin of header page\n";

    // status = bufMgr->flushFile(filePtr);  // make sure all pages of the file are flushed to disk
    // if (status != OK) cerr << "error in flushFile call\n";
    // before close the file
    status = db.closeFile(filePtr);
    if (status != OK)
    {
        cerr << "error in closefile call\n";
        Error e;
        e.print(status);
    }
}

// Return number of records in heap file

const int HeapFile::getRecCnt() const
{
    return headerPage->recCnt;
}

/**
 * Retrieves an arbitrary record from a file.
 * If record is not on the currently pinned page, the current page
 * is unpinned and the required page is read into the buffer pool
 * and pinned.
 *
 * Input:   const RID &rid: rid of record to find
 *          Record &rec:    record variable to hold found record
 *
 * Output:  returns a pointer to the record via the rec parameter
 *          returns OK if record found
 *          returns first error otherwise
 */
const Status HeapFile::getRecord(const RID &rid, Record &rec)
{
    Status status;

    // cout << "getRecord. record (" << rid.pageNo << "." << rid.slotNo << ")" << endl;

    // check if the desired record is on the currently pinned page
    if (curPage != NULL && rid.pageNo == curPageNo)
    {
        status = curPage->getRecord(rid, rec);
        if (status != OK)
            return status;

        // set curRec (rid of last record returned) to rec (record we are returning)
        curRec = rid;
        return OK;
    }
    else
    { // record not on pinned page
        // unpin currently pinned page
        status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
        if (status != OK)
            return status;

        // use the pageNo field of the rid to read the page into the buffer
        // bookkeeping!
        curPage = NULL;
        curPageNo = rid.pageNo;
        curDirtyFlag = false;

        // read next page into buffer
        status = bufMgr->readPage(filePtr, curPageNo, curPage);
        if (status != OK)
            return status;

        curPageNo = rid.pageNo;
        curDirtyFlag = false;

        // get record from next page
        status = curPage->getRecord(rid, rec);
        if (status != OK)
            return status;

        curRec = rid;
        return OK;
    }
}

HeapFileScan::HeapFileScan(const string &name,
                           Status &status) : HeapFile(name, status)
{
    filter = NULL;
}

const Status HeapFileScan::startScan(const int offset_,
                                     const int length_,
                                     const Datatype type_,
                                     const char *filter_,
                                     const Operator op_)
{
    if (!filter_)
    { // no filtering requested
        filter = NULL;
        return OK;
    }

    if ((offset_ < 0 || length_ < 1) ||
        (type_ != STRING && type_ != INTEGER && type_ != FLOAT) ||
        (type_ == INTEGER && length_ != sizeof(int) || type_ == FLOAT && length_ != sizeof(float)) ||
        (op_ != LT && op_ != LTE && op_ != EQ && op_ != GTE && op_ != GT && op_ != NE))
    {
        return BADSCANPARM;
    }

    offset = offset_;
    length = length_;
    type = type_;
    filter = filter_;
    op = op_;

    return OK;
}

const Status HeapFileScan::endScan()
{
    Status status;
    // generally must unpin last page of the scan
    if (curPage != NULL)
    {
        status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
        curPage = NULL;
        curPageNo = 0;
        curDirtyFlag = false;
        return status;
    }
    return OK;
}

HeapFileScan::~HeapFileScan()
{
    endScan();
}

const Status HeapFileScan::markScan()
{
    // make a snapshot of the state of the scan
    markedPageNo = curPageNo;
    markedRec = curRec;
    return OK;
}

const Status HeapFileScan::resetScan()
{
    Status status;
    if (markedPageNo != curPageNo)
    {
        if (curPage != NULL)
        {
            status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
            if (status != OK)
                return status;
        }
        // restore curPageNo and curRec values
        curPageNo = markedPageNo;
        curRec = markedRec;
        // then read the page
        status = bufMgr->readPage(filePtr, curPageNo, curPage);
        if (status != OK)
            return status;
        curDirtyFlag = false; // it will be clean
    }
    else
        curRec = markedRec;
    return OK;
}

/**
 * Helps to get the next page and do all relevant bookkeeping when unpinning
 *
 * Input:   File *&filePtr:     file to unpin and read from
 *          Page *&curPage:     current page reference to get next page from
 *          int &curPageNo:     current page number to unpin curPage
 *          bool &curDirtyFlag: current dirty flag to unpin curPage
 *
 * Output:  reads the next page into curPage
 *          returns OK if valid output
 *          returns with first error otherwise
 */
const Status nextPageHelper(File *&filePtr, Page *&curPage, int &curPageNo, bool &curDirtyFlag)
{
    Status status = OK;
    int nextPageNo;

    // get page no of next page
    curPage->getNextPage(nextPageNo);
    if (nextPageNo <= 0)
    {
        status = FILEEOF;
    }
    else
    {
        // unpin curPage, pin nextPage
        status = bufMgr->unPinPage(filePtr, curPageNo, curDirtyFlag);
        if (status != OK)
            return status;

        status = bufMgr->readPage(filePtr, nextPageNo, curPage);
        if (status != OK)
            return status;

        curPageNo = nextPageNo;
    }
    return status;
}

/**
 * Scans the file one page at a time looking at all the records until the RID of the next record
 * matches the predicate.
 *
 * Input:   RID &outRid: RID variable for our match to be stored
 * Output:  RID &outRid returns matched RID.
 *          returns OK if no errors ocurred
 *          returns error code of first error occurred
 */
const Status HeapFileScan::scanNext(RID &outRid)
{
    Status status = OK;
    RID nextRid;
    // RID tmpRid;
    // int nextPageNo;
    Record rec;
    bool found = false;

    // if curPage is null, read next page into buffer
    if (curPage == NULL)
    {
        status = bufMgr->readPage(filePtr, curPageNo, curPage);
        if (status != OK)
            return status;
    }

    // check if curRec has been init
    if (curRec.pageNo == NULLRID.pageNo || curRec.slotNo == NULLRID.slotNo)
    {
        // find first nonempty page
        while (true)
        {
            status = curPage->firstRecord(curRec);
            if (status != NORECORDS)
            {
                break;
            }

            status = nextPageHelper(filePtr, curPage, curPageNo, curDirtyFlag);
            if (status != OK)
            {
                break;
            }
        }
        if (status != OK)
            return status;

        // get records from new page
        status = getRecord(rec);
        if (status != OK)
            return status;

        // check if match found
        found = matchRec(rec);
        if (found)
        {
            outRid = curRec;
        }
    }

    // iterate until we find match
    while (!found)
    {
        // get RID of next record
        status = curPage->nextRecord(curRec, nextRid);

        // check if end of page
        if (status == ENDOFPAGE)
        {
            // go to next page
            status = nextPageHelper(filePtr, curPage, curPageNo, curDirtyFlag);
            if (status != OK)
            {
                break;
            }

            // find next page with records
            while (true)
            {
                status = curPage->firstRecord(curRec);
                if (status != NORECORDS)
                {
                    break;
                }

                status = nextPageHelper(filePtr, curPage, curPageNo, curDirtyFlag);
                if (status != OK)
                {
                    break;
                }
            }
            if (status != OK)
            {
                break;
            }

            // get records from new page
            status = getRecord(rec);
            if (status != OK)
            {
                break;
            }

            // check if match found
            found = matchRec(rec);
            if (found)
            {
                outRid = curRec;
            }
        }
        else
        { // not the end of the page
            // check records on page
            curRec = nextRid;
            status = getRecord(rec);
            if (status != OK)
            {
                break;
            }

            // check if match found
            found = matchRec(rec);
            if (found)
            {
                outRid = curRec;
            }
        }
    }
    return status;
}

// returns pointer to the current record.  page is left pinned
// and the scan logic is required to unpin the page

const Status HeapFileScan::getRecord(Record &rec)
{
    return curPage->getRecord(curRec, rec);
}

// delete record from file.
const Status HeapFileScan::deleteRecord()
{
    Status status;

    // delete the "current" record from the page
    status = curPage->deleteRecord(curRec);
    curDirtyFlag = true;

    // reduce count of number of records in the file
    headerPage->recCnt--;
    hdrDirtyFlag = true;
    return status;
}

// mark current page of scan dirty
const Status HeapFileScan::markDirty()
{
    curDirtyFlag = true;
    return OK;
}

const bool HeapFileScan::matchRec(const Record &rec) const
{
    // no filtering requested
    if (!filter)
        return true;

    // see if offset + length is beyond end of record
    // maybe this should be an error???
    if ((offset + length - 1) >= rec.length)
        return false;

    float diff = 0; // < 0 if attr < fltr
    switch (type)
    {

    case INTEGER:
        int iattr, ifltr; // word-alignment problem possible
        memcpy(&iattr,
               (char *)rec.data + offset,
               length);
        memcpy(&ifltr,
               filter,
               length);
        diff = iattr - ifltr;
        break;

    case FLOAT:
        float fattr, ffltr; // word-alignment problem possible
        memcpy(&fattr,
               (char *)rec.data + offset,
               length);
        memcpy(&ffltr,
               filter,
               length);
        diff = fattr - ffltr;
        break;

    case STRING:
        diff = strncmp((char *)rec.data + offset,
                       filter,
                       length);
        break;
    }

    switch (op)
    {
    case LT:
        if (diff < 0.0)
            return true;
        break;
    case LTE:
        if (diff <= 0.0)
            return true;
        break;
    case EQ:
        if (diff == 0.0)
            return true;
        break;
    case GTE:
        if (diff >= 0.0)
            return true;
        break;
    case GT:
        if (diff > 0.0)
            return true;
        break;
    case NE:
        if (diff != 0.0)
            return true;
        break;
    }

    return false;
}

InsertFileScan::InsertFileScan(const string &name,
                               Status &status) : HeapFile(name, status)
{
    // Do nothing. Heapfile constructor will bread the header page and the first
    //  data page of the file into the buffer pool
}

InsertFileScan::~InsertFileScan()
{
    Status status;
    // unpin last page of the scan
    if (curPage != NULL)
    {
        status = bufMgr->unPinPage(filePtr, curPageNo, true);
        curPage = NULL;
        curPageNo = 0;
        if (status != OK)
            cerr << "error in unpin of data page\n";
    }
}

/**
 *  Inserts the record described by rec into the file returning the RID of the inserted record in outRid.


    TIPS: check if curPage is NULL. If so, make the last page the current page and read it into the buffer. Call
    curPage->insertRecord to insert the record. If successful, remember to DO THE BOOKKEEPING. That is, you have
    to update data fields such as recCnt, hdrDirtyFlag, curDirtyFlag, etc.


    If can't insert into the current page, then create a new page, initialize it properly, modify the header page
    content properly, link up the new page appropriately, make the current page to be the newly allocated page,
    then try to insert the record. Don't forget bookkeeping that must be done after successfully inserting the
    record.


    Record rec: the record to be inserted
    RID& outRid: the RID of the inserted record
 */
const Status InsertFileScan::insertRecord(const Record &rec, RID &outRid)
{
    Page *newPage;
    int newPageNo;
    int nextPageNo;
    Status status, unpinstatus;
    RID rid;

    // check for very large records
    if ((unsigned int)rec.length > PAGESIZE - DPFIXED)
    {
        // will never fit on a page, so don't even bother looking
        return INVALIDRECLEN;
    }

    FileHdrPage *hdrPage = (FileHdrPage *)headerPage;

    if (curPage == NULL)
    {
        status = bufMgr->readPage(filePtr, headerPage->lastPage, curPage);
        if (status != OK)
            return status;
        curPageNo = hdrPage->lastPage;
    }
    // try to insert rec
    status = curPage->insertRecord(rec, rid);
    // do bookkeeping if inserted
    if (status == OK)
    {
        // update headerpage info, hdrDirtyFlag, curDirtyFlag and curRec
        hdrPage->recCnt++;
        hdrDirtyFlag = true;
        curDirtyFlag = true;
        curRec = rid;
        outRid = rid;
    }
    else if (status == NOSPACE)
    {
        // create new page, initialize properly, and modify header page
        // also make new page currently allocated page, then insert rec

        // allocating new page
        status = bufMgr->allocPage(filePtr, newPageNo, newPage);
        if (status != OK)
            return status;

        // updating header
        newPage->init(newPageNo);

        status = curPage->getNextPage(nextPageNo);
        if (status != OK)
            return status;

        curPage->setNextPage(newPageNo);

        // unpin the current page
        unpinstatus = bufMgr->unPinPage(filePtr, curPageNo, true);
        if (unpinstatus != OK)
            return unpinstatus;

        // pin the new page
        curPage = newPage;
        curPageNo = newPageNo;

        // insert the record
        status = curPage->insertRecord(rec, rid);
        if (status != OK)
            return status;

        hdrPage->recCnt++;
        hdrPage->pageCnt++;
        hdrDirtyFlag = true;
        curDirtyFlag = true;
        curRec = rid;
        outRid = rid;
    }
    return status;
}
