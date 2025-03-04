#include "catalog.h"
#include "query.h"

/*
 * Inserts a record into the specified relation.
 *
 * Returns:
 *  OK on success
 *  an error code otherwise
 */

const Status QU_Insert(const string &relation, const int attrCnt, const attrInfo attrList[])
{
    Status resultStatus;
    Record newRecord;
    RID recordId;
    int numRelationAttrs;
    AttrDesc *attributeDescriptors;

    // Retrieve relation info
    if ((resultStatus = attrCat->getRelInfo(relation, numRelationAttrs, attributeDescriptors)) == OK)
    {
        // Ensure the attribute count matches
        if (numRelationAttrs == attrCnt)
        {
            int totalLength = 0;
            // Calculate the total length of the record
            for (int i = 0; i < attrCnt; i++)
            {
                totalLength += attributeDescriptors[i].attrLen;
            }

            // Prepare the insert file scan
            InsertFileScan fileScan(relation, resultStatus);
            if (resultStatus == OK)
            {
                newRecord.length = totalLength;
                newRecord.data = (char *)malloc(totalLength);

                // Map attributes to record data
                for (int i = 0; i < numRelationAttrs; i++)
                {
                    for (int j = 0; j < attrCnt; j++)
                    {
                        if (strcmp(attrList[j].attrName, attributeDescriptors[i].attrName) == 0)
                        {
                            char *attributeValue = (char *)attrList[j].attrValue;

                            // Handle data types and convert appropriately
                            if (attrList[j].attrType == INTEGER)
                            {
                                int integerValue = atoi((char *)attrList[j].attrValue);
                                attributeValue = (char *)&integerValue;
                            }
                            else if (attrList[j].attrType == FLOAT)
                            {
                                float floatValue = atof((char *)attrList[j].attrValue);
                                attributeValue = (char *)&floatValue;
                            }

                            // Copy the attribute value into the record at the correct offset
                            memcpy((char *)newRecord.data + attributeDescriptors[i].attrOffset, attributeValue, attributeDescriptors[i].attrLen);
                            break;
                        }
                    }
                }

                // Insert the record
                fileScan.insertRecord(newRecord, recordId);
                return OK; // Insertion succeeded
            }
            else
            {
                return resultStatus; // Error during insert scan
            }
        }
        else
        {
            return ATTRTYPEMISMATCH; // Mismatch between attribute counts
        }
    }
    else
    {
        return resultStatus; // Error retrieving relation info
    }
}
