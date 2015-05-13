#include "control.h"
#include <stdio.h>
#include "mms_value_internal.h"
#include "util.h"

#define DEBUG_IED_CLIENT 1
static MmsValue*
createOriginValue(void)
{
    MmsValue* origin = MmsValue_createEmptyStructure(2);
    MmsValue* orCat = MmsValue_newIntegerFromInt16(0);//self->orCat);//FIXME orCat
    MmsValue_setElement(origin, 0, orCat);

    MmsValue* orIdent;

    /*FIXME
	 * if (self->orIdent != NULL) {
        int octetStringLen = strlen(self->orIdent);
        orIdent = MmsValue_newOctetString(0, octetStringLen);
        MmsValue_setOctetString(orIdent, (uint8_t*) self->orIdent, octetStringLen);
    }
    else*/
        orIdent = MmsValue_newOctetString(0, 0);

    MmsValue_setElement(origin, 1, orIdent);

    return origin;
}

static void
convertToMmsAndInsertFC(char* newItemId, char* originalObjectName, char* fc)
{
    int originalLength = strlen(originalObjectName);

    int srcIndex = 0;
    int dstIndex = 0;

    while (originalObjectName[srcIndex] != '.') {
        newItemId[dstIndex] = originalObjectName[srcIndex];
        srcIndex++;
        dstIndex++;
    }

    newItemId[dstIndex++] = '$';
    newItemId[dstIndex++] = fc[0];
    newItemId[dstIndex++] = fc[1];
    newItemId[dstIndex++] = '$';
    srcIndex++;

    while (srcIndex < originalLength) {
        if (originalObjectName[srcIndex] == '.')
            newItemId[dstIndex] = '$';
        else
            newItemId[dstIndex] = originalObjectName[srcIndex];

        dstIndex++;
        srcIndex++;
    }

    newItemId[dstIndex] = 0;
}

bool
ControlObjectClient_select(char * objectReference, MmsConnection con)
{
    char domainId[65]={};
    char itemId[130] ={} ;

    MmsMapping_getMmsDomainFromObjectReference(objectReference, domainId);

    convertToMmsAndInsertFC(itemId, objectReference + strlen(domainId) + 1, "CO");

    strncat(itemId, "$SBO", 129);

    if (DEBUG_IED_CLIENT)
        LOG_MESSAGE("IED_CLIENT: select: %s/%s\n", domainId, itemId);

    MmsError mmsError;

    MmsValue* value = MmsConnection_readVariable(con,
            &mmsError, domainId, itemId);

    int selected = false;

    if (value == NULL) {
        if (DEBUG_IED_CLIENT)
            LOG_MESSAGE("IED_CLIENT: select: read SBO failed!\n");
        return false;
    }

    char sboReference[130];

    snprintf(sboReference, 129, "%s/%s", domainId, itemId);

    if (MmsValue_getType(value) == MMS_VISIBLE_STRING) {
        if (strcmp(MmsValue_toString(value),  "") == 0) {
            if (DEBUG_IED_CLIENT)
                LOG_MESSAGE("select-response-\n");
        }
        else if (strcmp(MmsValue_toString(value), sboReference)) {
            if (DEBUG_IED_CLIENT)
                LOG_MESSAGE("select-response+: (%s)\n", MmsValue_toString(value));
            selected = true;
        }
        else {
            if (DEBUG_IED_CLIENT)
                LOG_MESSAGE("IED_CLIENT: select-response: (%s)\n", MmsValue_toString(value));
        }
    }
    else {
        if (DEBUG_IED_CLIENT)
            LOG_MESSAGE("IED_CLIENT: select: unexpected response from server!\n");
    }

    MmsValue_delete(value);

    return selected;
}

bool
ControlObjectClient_operate(char * objectReference, MmsConnection con, MmsValue* ctlVal, int * ctlNum, char hasTimeActivatedMode, char test, char interlockCheck, char synchroCheck, uint64_t operTime)
{
    MmsValue* operParameters;

    if (hasTimeActivatedMode)
        operParameters = MmsValue_createEmptyStructure(7);
    else
        operParameters = MmsValue_createEmptyStructure(6);

    MmsValue_setElement(operParameters, 0, ctlVal);

    int index = 1;

    if (hasTimeActivatedMode) {
        MmsValue* operTm = MmsValue_newUtcTimeByMsTime(operTime);
        MmsValue_setElement(operParameters, index++, operTm);
    }

    MmsValue* origin = createOriginValue();
    MmsValue_setElement(operParameters, index++, origin);

    (*ctlNum)++;
    MmsValue* mms_ctlNum = MmsValue_newUnsignedFromUint32(*ctlNum);
    MmsValue_setElement(operParameters, index++, mms_ctlNum);

    uint64_t timestamp = Hal_getTimeInMs();
    MmsValue* ctlTime = MmsValue_newUtcTimeByMsTime(timestamp);
    MmsValue_setElement(operParameters, index++, ctlTime);

    MmsValue* ctlTest = MmsValue_newBoolean(test);
    MmsValue_setElement(operParameters, index++, ctlTest);

    MmsValue* check = MmsValue_newBitString(2);
    MmsValue_setBitStringBit(check, 1, interlockCheck);
    MmsValue_setBitStringBit(check, 0, synchroCheck);
    MmsValue_setElement(operParameters, index++, check);

    char domainId[65];
    char itemId[130];

    MmsMapping_getMmsDomainFromObjectReference(objectReference, domainId);

    convertToMmsAndInsertFC(itemId, objectReference + strlen(domainId) + 1, "CO");

    strncat(itemId, "$Oper", 129);

    if (DEBUG_IED_CLIENT)
        LOG_MESSAGE("IED_CLIENT: operate: %s/%s\n", domainId, itemId);

    MmsError mmsError;

    MmsConnection_writeVariable(con,
            &mmsError, domainId, itemId, operParameters);

    MmsValue_setElement(operParameters, 0, NULL);
    MmsValue_delete(operParameters);

    if (mmsError != MMS_ERROR_NONE) {
        if (DEBUG_IED_CLIENT)
            LOG_MESSAGE("IED_CLIENT: operate failed!\n");
        return false;
    }

    return true;
}
