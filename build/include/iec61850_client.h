/*
 *  iec61850_client.h
 *
 *  Copyright 2013 Michael Zillgith
 *
 *  This file is part of libIEC61850.
 *
 *  libIEC61850 is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  libIEC61850 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with libIEC61850.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  See COPYING file for the complete license text.
 */

#ifndef IEC61850_CLIENT_H_
#define IEC61850_CLIENT_H_

#include "libiec61850_platform_includes.h"
#include "iec61850_common.h"
#include "goose_subscriber.h"
#include "mms_value.h"
#include "mms_client_connection.h"
#include "linked_list.h"

/**
 * \addtogroup  client_api_group
 */
/**@{*/

#define TRG_OPT_DATA_CHANGED 1
#define TRG_OPT_QUALITY_CHANGED 2
#define TRG_OPT_DATA_UPDATE 4
#define TRG_OPT_INTEGRITY 8
#define TRG_OPT_GI 16

typedef struct sIedConnection* IedConnection;
typedef struct sClientDataSet* ClientDataSet;
typedef struct sClientReport* ClientReport;

typedef enum {
    IED_ERROR_OK,
    IED_ERROR_NOT_CONNECTED,
    IED_ERROR_TIMEOUT,
    IED_ERROR_ACCESS_DENIED,
    IED_ERROR_ENABLE_REPORT_FAILED_DATASET_MISMATCH,
    IED_ERROR_UNKNOWN,
    IED_ERROR_OBJECT_REFERENCE_INVALID,
} IedClientError;

IedConnection
IedConnection_create();

void
IedConnection_connect(IedConnection self, IedClientError* error, char* hostname, int tcpPort);

void
IedConnection_close(IedConnection self);

/**
 * \brief get a handle to the underlying MmsConnection
 *
 * Get access to the underlying MmsConnection used by this IedConnection.
 * This can be used to set/change specific MmsConnection parameters.
 *
 * \param self
 *
 * \return the MmsConnection instance used by this IedConnection.
 */
MmsConnection
IedConnection_getMmsConnection(IedConnection self);

void
IedConnection_destroy(IedConnection self);

/**
 * \brief get data set values from a server device
 *
 * \param connection the connection object
 * \param error the error code if an error occurs
 * \param dataSetReference object reference of the data set
 * \param dataSet a data set instance where to store the retrieved values or NULL if a new instance
 *        shall be created.
 *
 * \return data set instance with retrieved values of NULL if an error occured.
 */
ClientDataSet
IedConnection_getDataSet(IedConnection self, IedClientError* error, char* objectReference, ClientDataSet dataSet);

typedef void (*ReportCallbackFunction) (void* parameter, ClientReport report);

/**
 * \brief enable a report control block (RCB)
 *
 * \param connection the connection object
 * \param error the error code if an error occurs
 * \param rcbReference object reference of the report control block
 * \param dataSet a data set instance where to store the retrieved values the data set has to match with the
 *        data set of the RCB
 * \param triggerOptions the options for report triggering. If set to 0 the configured trigger options of
 *        the RCB will not be changed
 * \param callback user provided callback function to be invoked when a report is received.
 * \param callbackParameter user provided parameter that will be passed to the callback function
 *
 */
void
IedConnection_enableReporting(IedConnection self, IedClientError* error, char* rcbReference, ClientDataSet dataSet,
        int triggerOptions, ReportCallbackFunction callback, void* callbackParameter);

void
IedConnection_disableReporting(IedConnection self, IedClientError* error, char* rcbReference);

MmsValue*
IedConnection_readObject(IedConnection self, IedClientError* error, char* objectReference, FunctionalConstraint fc);

void
IedConnection_writeObject(IedConnection self, IedClientError* error, char* objectReference, FunctionalConstraint fc,
        MmsValue* value);

//void
//IedConnection_controlObject(IedConnection self, IedClientError* error, char* objectReference, MmsValue* ctlVal);

void
IedConnection_getDeviceModelFromServer(IedConnection self, IedClientError* error);

LinkedList /*<char*>*/
IedConnection_getLogicalDeviceList(IedConnection self, IedClientError* error);

LinkedList /*<char*>*/
IedConnection_getLogicalDeviceDirectory(IedConnection self, IedClientError* error, char* logicalDeviceName);

typedef enum {
    ACSI_CLASS_DATA_OBJECT,
    ACSI_CLASS_DATA_SET,
    ACSI_CLASS_BRCB,
    ACSI_CLASS_URCB,
    ACSI_CLASS_LCB,
    ACSI_CLASS_LOG,
    ACSI_CLASS_SGCB,
    ACSI_CLASS_GoCB,
    ACSI_CLASS_GsCB,
    ACSI_CLASS_MSVCB,
    ACSI_CLASS_USVCB
} ACSIClass;

LinkedList /*<char*>*/
IedConnection_getLogicalNodeVariables(IedConnection self, IedClientError* error,
		char* logicalNodeReference);

LinkedList /*<char*>*/
IedConnection_getLogicalNodeDirectory(IedConnection self, IedClientError* error,
		char* logicalNodeReference, ACSIClass acseClass);

LinkedList
IedConnection_getDataDirectory(IedConnection self, IedClientError* error,
		char* dataReference);

/**
 * \brief addSubscription service according to IEC 61400-25 (NOT YET IMPLEMENTED)
 *
 * Create a new data set and enable reporting for that data set.
 *
 * \param connection the connection object
 * \param error the error code if an error occurs
 * \param fcda the data set members of the newly created data set
 */
ClientDataSet
IedConnection_addSubscription(IedConnection self, IedClientError* error, char* fcda[]);

/**
 * \brief Create a GOOSE subscriber for the specified GooseControlBlock of the server
 *        (NOT YET IMPLEMENTED)
 *
 * Calling this function with a valid reference for a Goose Control Block (GCB) at the
 * server will create a client data set according to the data set specified in the
 * GCB.
 */
GooseSubscriber
IedConnection_createGooseSubscriber(IedConnection self, char* gooseCBReference);

ClientDataSet
ClientDataSet_create(char* dataSetReference);

void
ClientDataSet_setDataSetValues(ClientDataSet self, MmsValue* dataSetValues);

MmsValue*
ClientDataSet_getDataSetValues(ClientDataSet self);

char*
ClientDataSet_getReference(ClientDataSet self);

int
ClientDataSet_getDataSetSize(ClientDataSet self);

/**@}*/

#endif /* IEC61850_CLIENT_H_ */
