#include "catalog.h"
#include "query.h"

/*
 * Deletes records from a specified relation.
 * This function will delete all tuples in relation satisfying the predicate specified by attrName, op,
 * and the constant attrValue. type denotes the type of the attribute. You can locate all the qualifying
 * tuples using a filtered HeapFileScan.
 *
 * Parameters:
 *  relation: the relation to delete from
 *  attrName: the attribute to delete on
 *  op: the comparison operator (attrname and op together form the condition)
 *  type: the type of the attribute
 *  attrValue: the value of the attribute to delete on
 *
 * Returns:
 *  OK on success
 *  an error code otherwise
 */

const Status QU_Delete(const string &relation,
                       const string &attrName,
                       const Operator op,
                       const Datatype type,
                       const char *attrValue)
{
    // part 6
    // 1. check if the relation exists in the catalog
    // 2. determine type of the attribute to delete on
    // 3. get the attribute number of the attribute to delete on
    // 4. create a heapfilescan object for the relation
    // 5. start the scan
    // 6. loop through the records in the relation
    // 7. check if the record satisfies the predicate
    // 8. if it does, delete the record
    // 9. end the scan
    // 10. return OK

    Status status;
    AttrDesc attrDesc;
    int attrOffset;
    // char *filter;

    // check if the relation exists in the catalog
    HeapFileScan *scan = new HeapFileScan(relation, status);
    if (status != OK)
        return status;

    // determine type of the attribute to delete on
    // if (attrName.length() > MAXNAME) return ATTRTOOLONG;

    // if the attribute name is empty, start the scan with no filtering
    if (attrName.length() == 0)
    {
        scan->startScan(0, 0, STRING, NULL, op);
        // loop through the records in the relation and delete them
        RID rid;
        while (scan->scanNext(rid) == OK)
        {
            // check if the record satisfies the predicate
            // if it does, delete the record
            status = scan->deleteRecord();
            if (status != OK)
                return status;
        }
        return OK;
    }
    else
    { // there is a filter
        // get the attribute number of the attribute to delete on
        status = attrCat->getInfo(relation, attrName, attrDesc);
        if (status != OK)
            return status;

        // get the attribute offset
        attrOffset = attrDesc.attrOffset;
    }

    // check type of the attribute and set the filter
    if (type == FLOAT)
    {
        *(float *)attrValue = atof(attrValue);
    }
    else if (type == INTEGER)
    {
        *(int *)attrValue = atoi(attrValue);
    }
    else if (type == STRING)
    {
        // do nothing
    }

    // start scanning
    status = scan->startScan(attrOffset, attrDesc.attrLen, type, attrValue, op);

    // loop through the records in the relation
    RID rid;
    Record rec;
    while (scan->scanNext(rid) == OK)
    {
        scan->getRecord(rec);
        // print(cout, rid);
        //  if it does, delete the record
        status = scan->deleteRecord();
        // if (status != OK) return status;
    }

    // end the scan
    status = scan->endScan();

    return OK;
}
