/**
 * @file ua_services.h
 *
 * @brief Defines the method signatures for all the standard defined services.
 */

#ifndef UA_SERVICES_H_
#define UA_SERVICES_H_

#include "opcua.h"
#include "ua_application.h"
#include "ua_transport_binary_secure.h"

/**
 * @defgroup services Services
 *
 * @brief This module describes all the services used to communicate in in OPC UA.
 *
 * @{
 */

/**
 * @name Discovery Service Set
 *
 * This Service Set defines Services used to discover the Endpoints implemented
 * by a Server and to read the security configuration for those Endpoints.
 *
 * @{
 */
// Service_FindServers

/**
 * @brief This Service returns the Endpoints supported by a Server and all of
 * the configuration information required to establish a SecureChannel and a
 * Session.
 */
UA_Int32 Service_GetEndpoints(SL_Channel *channel, const UA_GetEndpointsRequest* request, UA_GetEndpointsResponse *response);
// Service_RegisterServer
/** @} */

/**
 * @name SecureChannel Service Set
 *
 * This Service Set defines Services used to open a communication channel that
 * ensures the confidentiality and Integrity of all Messages exchanged with the
 * Server.
 *
 * @{
 */

/**
 * @brief This Service is used to open or renew a SecureChannel that can be used
 * to ensure Confidentiality and Integrity for Message exchange during a
 * Session.
 */
UA_Int32 Service_OpenSecureChannel(SL_Channel *channel, const UA_OpenSecureChannelRequest* request, UA_OpenSecureChannelResponse* response);

/**
 * @brief This Service is used to terminate a SecureChannel.
 */
UA_Int32 Service_CloseSecureChannel(SL_Channel *channel, const UA_CloseSecureChannelRequest *request, UA_CloseSecureChannelResponse *response);
/** @} */

/**
 * @name Session Service Set
 *
 * This Service Set defines Services for an application layer connection
 * establishment in the context of a Session.
 *
 * @{
 */

/**
 * @brief This Service is used by an OPC UA Client to create a Session and the
 * Server returns two values which uniquely identify the Session. The first
 * value is the sessionId which is used to identify the Session in the audit
 * logs and in the Server’s address space. The second is the authenticationToken
 * which is used to associate an incoming request with a Session.
 */
UA_Int32 Service_CreateSession(SL_Channel *channel, const UA_CreateSessionRequest *request, UA_CreateSessionResponse *response);

/**
 * @brief This Service is used by the Client to submit its SoftwareCertificates
 * to the Server for validation and to specify the identity of the user
 * associated with the Session. This Service request shall be issued by the
 * Client before it issues any other Service request after CreateSession.
 * Failure to do so shall cause the Server to close the Session.
 */
UA_Int32 Service_ActivateSession(SL_Channel *channel, const UA_ActivateSessionRequest *request, UA_ActivateSessionResponse *response);

/**
 * @brief This Service is used to terminate a Session.
 */
UA_Int32 Service_CloseSession(SL_Channel *channel, const UA_CloseSessionRequest *request, UA_CloseSessionResponse *response);
// Service_Cancel
/** @} */

/**
 * @name NodeManagement Service Set
 *
 * This Service Set defines Services to add and delete AddressSpace Nodes and References between
 * them. All added Nodes continue to exist in the AddressSpace even if the Client that created them
 * disconnects from the Server.
 *
 * @{
 */
// Service_AddNodes
// Service_AddReferences
// Service_DeleteNodes
// Service_DeleteReferences
/** @} */

/**
 * @name View Service Set
 *
 * Clients use the browse Services of the View Service Set to navigate through
 * the AddressSpace or through a View which is a subset of the AddressSpace.
 *
 * @{
 */

/**
 * @brief This Service is used to discover the References of a specified Node.
 * The browse can be further limited by the use of a View. This Browse Service
 * also supports a primitive filtering capability.
 */ 
UA_Int32 Service_Browse(SL_Channel *channel, const UA_BrowseRequest *request, UA_BrowseResponse *response);
// Service_BrowseNext
// Service_TranslateBrowsePathsToNodeIds
// Service_RegisterNodes
// Service_UnregisterNodes
/** @} */

/* Part 4: 5.9 Query Service Set */
/**
 * @name Query Service Set
 *
 * This Service Set is used to issue a Query to a Server. OPC UA Query is
 * generic in that it provides an underlying storage mechanism independent Query
 * capability that can be used to access a wide variety of OPC UA data stores
 * and information management systems. OPC UA Query permits a Client to access
 * data maintained by a Server without any knowledge of the logical schema used
 * for internal storage of the data. Knowledge of the AddressSpace is
 * sufficient.
 *
 * @{
 */
// Service_QueryFirst
// Service_QueryNext
/** @} */

/* Part 4: 5.10 Attribute Service Set */
/**
 * @name Attribute Service Set
 *
 * This Service Set provides Services to access Attributes that are part of
 * Nodes.
 *
 * @{
 */

/**
 * @brief This Service is used to read one or more Attributes of one or more
 * Nodes. For constructed Attribute values whose elements are indexed, such as
 * an array, this Service allows Clients to read the entire set of indexed
 * values as a composite, to read individual elements or to read ranges of
 * elements of the composite.
 */
UA_Int32 Service_Read(SL_Channel *channel, const UA_ReadRequest *request, UA_ReadResponse *response);
// Service_HistoryRead
// Service_Write
// Service_HistoryUpdate
/** @} */

/**
 * @name Method Service Set
 *
 * The Method Service Set defines the means to invoke methods. A method shall be
a component of an Object.
 *
 * @{
 */
// Service_Call
/** @} */

/**
 * @name MonitoredItem Service Set
 *
 * Clients define MonitoredItems to subscribe to data and Events. Each
 * MonitoredItem identifies the item to be monitored and the Subscription to use
 * to send Notifications. The item to be monitored may be any Node Attribute.
 *
 * @{
 */

/**
 * @brief This Service is used to create and add one or more MonitoredItems to a
 * Subscription. A MonitoredItem is deleted automatically by the Server when the
 * Subscription is deleted. Deleting a MonitoredItem causes its entire set of
 * triggered item links to be deleted, but has no effect on the MonitoredItems
 * referenced by the triggered items.
 */
UA_Int32 Service_CreateMonitoredItems(SL_Channel *channel, const UA_CreateMonitoredItemsRequest *request, UA_CreateMonitoredItemsResponse *response);
// Service_ModifyMonitoredItems
// Service_SetMonitoringMode
// Service_SetTriggering
// Service_DeleteMonitoredItems
/** @} */

/**
 * @name Subscription Service Set
 *
 * Subscriptions are used to report Notifications to the Client.
 *
 * @{
 */
// Service_CreateSubscription
// Service_ModifySubscription
// Service_SetPublishingMode
// Service_Publish
// Service_Republish
// Service_TransferSubscription
// Service_DeleteSubscription
/** @} */

/** @} */ // end of group

#endif
