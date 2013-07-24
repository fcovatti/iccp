/*
 *  server_api_v2.h
 *
 *  IEC 61850 server API for libiec61850.
 *
 *  Copyright 2013 Michael Zillgith
 *
 *	This file is part of libIEC61850.
 *
 *	libIEC61850 is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	libIEC61850 is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with libIEC61850.  If not, see <http://www.gnu.org/licenses/>.
 *
 *	See COPYING file for the complete license text.
 *
 *	Description:
 *
 *  The user of this API has to handle MMS values directly. It provides no additional
 *  abstraction layer on top of the MMS layer. Therefore it is not intended to implement
 *  a generic IEC 61850 server. But an IEC 61850 compliant MMS server can be implemented
 *  easily with this API.
 *  This API is intended to be used on embedded devices that should not be burdened with an
 *  additional data modeling and data handling layer.
 *
 *  This API provides two different usage patterns that can also be combined in a single
 *  application:
 *
 *  Option 1: The user can provide callbacks for MMS server read and write operations.
 *  Every time a client requests a value a callback will be invoked and the appropriate
 *  value has to be provided by the application.
 *
 *  Option 2: The MMS server can handle all read and write operations autonomously without
 *  invoking any application callbacks. The application can provide updated process values
 *  by pushing them periodically or on demand to the MMS server.
 *
 */

#ifndef IED_SERVER_API_V2_H_
#define IED_SERVER_API_V2_H_

/** \defgroup server_api_group IEC 61850 server API
 *  @{
 */

#include "mms_server.h"
#include "model.h"

typedef struct sIedServer* IedServer;

/**
 * \brief Create a new IedServer instance
 *
 * \param iedModel reference to the IedModel data structure to be used as IEC 61850 model of the device
 *
 * \return the newly generated IedServer instance
 */
IedServer
IedServer_create(IedModel* iedModel);

/**
 * \brief Destroy an IedServer instance and release all resources (memory, TCP sockets)
 *
 * \param self the instance of IedServer to operate on.
 */
void
IedServer_destroy(IedServer self);

/**
 * \brief Start handling client connections
 *
 * \param self the instance of IedServer to operate on.
 * \param tcpPort the TCP port the server is listening
 */
void
IedServer_start(IedServer self, int tcpPort);

/**
 * \brief Stop handling client connections
 *
 * \param self the instance of IedServer to operate on.
 */
void
IedServer_stop(IedServer self);

/**
 * \brief Check if IedServer instance is listening for client connections
 *
 * \param self the instance of IedServer to operate on.
 *
 * \return true if IedServer instance is listening for client connections
 */
bool
IedServer_isRunning(IedServer self);

/**
 * \brief Get data attribute value
 *
 * Get the MmsValue object of an MMS Named Variable that is part of the device model.
 * You should not manipulate the received object directly. Instead you should use
 * the IedServer_updateValue method.
 *
 * \param self the instance of IedServer to operate on.
 * \param node the data attribute handle
 *
 * \return MmsValue object of the MMS Named Variable or NULL if the value does not exist.
 */
MmsValue*
IedServer_getAttributeValue(IedServer self, ModelNode* node);


/**
 * \brief Update the MmsValue object of an IEC 61850 data attribute.
 *
 * The data attribute handle of type ModelNode* are imported with static_model.h.
 * You should use this function instead of directly operating on the MmsValue instance
 * that is hold by the MMS server. Otherwise the IEC 61850 server is not aware of the
 * changed value and will e.g. not generate a report.
 *
 * \param self the instance of IedServer to operate on.
 * \param node the data attribute handle
 * \param value MmsValue object used to update the value cached by the server.
 *
 * \return MmsValue object of the MMS Named Variable or NULL if the value does not exist.
 */
void
IedServer_updateAttributeValue(IedServer self, DataAttribute* node, MmsValue* value);

/**
 * \brief Inform the IEC 61850 stack that the quality of a data attribute has changed.
 *
 * The data attribute handle of type ModelNode* are imported with static_model.h.
 * This function is required to trigger reports that have been configured with
 * QUALITY CHANGE trigger condition.
 *
 * \param self the instance of IedServer to operate on.
 * \param node the data attribute handle
 */
void
IedServer_attributeQualityChanged(IedServer self, ModelNode* node);

/**
 * User provided callback function for the control model. It will be invoked when
 * a control operation happens (Oper, SBOw).
 *
 * \param parameter the parameter that was specified when setting the control handler
 * \param ctlVal the control value of the control operation.
 */
typedef bool (*ControlHandler) (void* parameter, MmsValue* ctlVal);

/**
 * \brief Set control handler for controllable data object
 *
 * This functions sets a user provided control handler for a data object. The data object
 * has to be an instance of a controllable CDC (Common Data Class) like e.g. SPC, DPC or APC.
 * The control handler is a callback function that will be called by the IEC server when a
 * client invokes a control operation on the data object.
 *
 * \param self the instance of IedServer to operate on.
 * \param node the controllable data object handle
 * \param handler a callback function of type ControlHandler
 * \param parameter a user provided parameter that is passed to the control handler.
 */
void
IedServer_setControlHandler(IedServer self, DataObject* node, ControlHandler handler, void* parameter);

/**
 * \brief Lock the MMS server data model.
 *
 * Client requests will be postponed until the lock is removed
 *
 * \param self the instance of IedServer to operate on.
 */
void
IedServer_lockDataModel(IedServer self);

/**
 * \brief Unlock the MMS server data model and process pending client requests.
 *
 * \param self the instance of IedServer to operate on.
 */
void
IedServer_unlockDataModel(IedServer self);

/**
 * \brief Enable all GOOSE control blocks.
 *
 * This will set the GoEna attribute of all configured GOOSE control blocks
 * to true. If this method is not called at the startup or reset of the server
 * then configured GOOSE control blocks keep inactive until a MMS client enables
 * them by writing to the GOOSE control block.
 *
 * \param self the instance of IedServer to operate on.
 */
void
IedServer_enableGoosePublishing(IedServer self);

/**
 * \brief callback handler to monitor client access to data attributes
 *
 * User provided callback function to observe (monitor) MMS client access to
 * IEC 61850 data attributes. The application can install the same handler
 * multiple times and distinguish data attributes by the dataAttribute parameter.
 *
 * \param the data attribute that has been written by an MMS client.
 */
typedef void (*AttributeChangedHandler) (DataAttribute* dataAttribute);

/**
 * \brief Install an observer for a data attribute.
 *
 * This instructs the server to monitor write attempts by MMS clients to specific
 * data attributes. If a successful write attempt happens the server will call
 * the provided callback function to inform the application.
 *
 * \param self the instance of IedServer to operate on.
 * \param dataAttribute the data attribute to monitor
 * \param handler the callback function that is invoked if a client has written to
 *        the monitored data attribute.
 */
void
IedServer_observeDataAttribute(IedServer self, DataAttribute* dataAttribute,
        AttributeChangedHandler handler);

/**@}*/

#endif /* IED_SERVER_API_V2_H_ */
