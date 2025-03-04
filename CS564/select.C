#include "catalog.h"
#include "query.h"

// forward declaration
const Status ScanSelect(const string &result,
                        const int projCnt,
                        const AttrDesc projNames[],
                        const AttrDesc *attrDesc,
                        const Operator op,
                        const char *filter,
                        const int reclen);

/*
 * Selects records from the specified relation.
 *
 * Returns:
 *      OK on success
 *      an error code otherwise
 */

const Status QU_Select(const string &result,
                       const int projCnt,
                       const attrInfo projNames[],
                       const attrInfo *attr,
                       const Operator op,
                       const char *attrValue)
{
    // Qu_Select sets up things and then calls ScanSelect to do the actual work
    cout << "Doing QU_Select " << endl;

    // project list defined by projCnt & projNames

    Status status;
    AttrDesc attrDesc;
    AttrDesc *attrDescPtr = NULL;
    AttrDesc projDescs[projCnt];
    int recLength = 0;
    const attrInfo *projName;
    AttrDesc *projDesc;

    if (attr != NULL)
    {
        // init attrDesc, terminate on failure ???
        status = attrCat->getInfo(attr->relName, attr->attrName, attrDesc);
        if (status != OK)
            return status;

        attrDescPtr = &attrDesc;
    }

    // iterate through projection attributes
    for (int i = 0; i < projCnt; i++)
    {
        projName = &projNames[i];
        projDesc = &projDescs[i];

        // get proj attribute descriptor
        status = attrCat->getInfo(projName->relName, projName->attrName, *projDesc);
        if (status == OK)
        {
            // increment recLength by proj attribute length
            recLength += projDesc->attrLen;
        }
        else
        {
            break;
        }
    }

    if (status == OK)
    {
        // make sure to give ScanSelect proper input!
        status = ScanSelect(result, projCnt, projDescs, attrDescPtr, op, attrValue, recLength);
    }
    else
    {
        return status;
    }
}

/*
 * ScanSelect performs a selection operation on a table using a filter condition,
 *      then projects specific columns from the selected rows into a result table.
 *
 * INPUT:       result          : table to store output
 *                      projCnt         : number of attributes
 *                      projNames       : array of ArrayDesc/attributes
 *                      attrDesc        : attr for selection
 *                      op                      : operator used to compare filter
 *                      filter          : *attrValue from QU_SELECT
 *                      reclen          : length of output tuple
 *
 * OUTPUT:      result          : table to store output
 *                      RETURNS OK on success
 *                      and error otherwise.
 *
 */
const Status ScanSelect(const string &result,
#include "stdio.h"
#include "stdlib.h"
                        const int projCnt,
                        const AttrDesc projNames[],
                        const AttrDesc *attrDesc,
                        const Operator op,
                        const char *filter,
                        const int reclen)
{
    cout << "Doing HeapFileScan Selection using ScanSelect()" << endl;

    // declare variables
    Status status;

    int scanAttrCount;
    AttrDesc *scanAttrDescs;

    int filterInt;
    float filterFloat;

    int scanAttrOffset;
    int scanAttrLength;
    Datatype scanAttrType;
    AttrDesc *scanAttrDesc;

    const AttrDesc *projName;
    char projData[reclen];
    int projDataOffset;

    Record resultRec = {projData, reclen};
    RID resultRID;

    // temporary record for output table
    RID scanRID;
    Record scanRec;

    // open "result" as an InsertFileScan object
    InsertFileScan resultRel(result, status);
    if (status == OK)
    {
        // get attribute descriptors of table to scan
        status = attrCat->getRelInfo(projNames[0].relName, scanAttrCount, scanAttrDescs);
        if (status == OK)
        {
            // open current table as a HeapFileScan object
            HeapFileScan scanRel(projNames[0].relName, status);
            if (status == OK)
            {
                if (attrDesc != NULL)
                {
                    // check attrType: INTEGER, FLOAT, STRING
                    scanAttrType = static_cast<Datatype>(attrDesc->attrType);
                    if (scanAttrType == INTEGER)
                    {
                        // convert attrValue/filter from char* to an integer
                        filterInt = atoi(filter);
                        filter = (char *)&filterInt;
                    }
                    else if (scanAttrType == FLOAT)
                    {
                        // convert attrValue/filter from char* to a float
                        filterFloat = atof(filter);
                        filter = (char *)&filterFloat;
                    }
                    scanAttrOffset = attrDesc->attrOffset;
                    scanAttrLength = attrDesc->attrLen;
                }

                // scan the current table
                status = scanRel.startScan(scanAttrOffset, scanAttrLength, scanAttrType, filter, op);
                if (status == OK)
                {
                    while (true)
                    {
                        // get next record
                        status = scanRel.scanNext(scanRID);
                        if (status != OK)
                            break;
                        status = scanRel.getRecord(scanRec);
                        if (status != OK)
                            break;

                        projDataOffset = 0;

                        // iterate through projection attributes
                        for (projName = projNames; projName < &projNames[projCnt]; projName++)
                        {
                            // iterate through scanned record attributes
                            for (scanAttrDesc = scanAttrDescs; scanAttrDesc < &scanAttrDescs[scanAttrCount]; scanAttrDesc++)
                            {
                                // check if current matches temp
                                if (strcmp(scanAttrDesc->attrName, projName->attrName) == 0)
                                {
                                    // copy curr stuff over to the temporary record
                                    memcpy(projData + projDataOffset, static_cast<char *>(scanRec.data) + scanAttrDesc->attrOffset, scanAttrDesc->attrLen);
                                    projDataOffset += scanAttrDesc->attrLen;
                                    break;
                                }
                            }
                        }
                        // insert into the output table
                        status = resultRel.insertRecord(resultRec, resultRID);
                        if (status != OK)
                            break;
                    }
                }
            }
        }
    }
    else
    {
        return status;
    }
}
