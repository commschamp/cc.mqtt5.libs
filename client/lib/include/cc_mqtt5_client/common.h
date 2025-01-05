//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

/// @file
/// @brief Common definition for MQTT v5 clients.

#pragma once

#ifdef __cplusplus
extern "C" {
#else // #ifdef __cplusplus      
#include <stdbool.h>
#endif // #ifdef __cplusplus

/// @brief Major verion of the library
/// @ingroup global
#define CC_MQTT5_CLIENT_MAJOR_VERSION 1U

/// @brief Minor verion of the library
/// @ingroup global
#define CC_MQTT5_CLIENT_MINOR_VERSION 0U

/// @brief Patch level of the library
/// @ingroup global
#define CC_MQTT5_CLIENT_PATCH_VERSION 3U

/// @brief Macro to create numeric version as single unsigned number
/// @ingroup global
#define CC_MQTT5_CLIENT_MAKE_VERSION(major_, minor_, patch_) \
    ((static_cast<unsigned>(major_) << 24) | \
     (static_cast<unsigned>(minor_) << 8) | \
     (static_cast<unsigned>(patch_)))

/// @brief Version of the library as single numeric value
/// @ingroup global
#define CC_MQTT5_CLIENT_VERSION CC_MQTT5_CLIENT_MAKE_VERSION(CC_MQTT5_CLIENT_MAJOR_VERSION, CC_MQTT5_CLIENT_MINOR_VERSION, CC_MQTT5_CLIENT_PATCH_VERSION)

/// @brief Special value for "Session Expiry Interval" property to specify that session never expires.
/// @see @ref CC_Mqtt5ConnectExtraConfig::m_sessionExpiryInterval
/// @see @ref CC_Mqtt5ConnectResponse::m_sessionExpiryInterval
/// @ingroup global
#define CC_MQTT5_SESSION_NEVER_EXPIRES 0xffffffff

/// @brief MQTT5 protocol limit to topic alias.
/// @details The application is expected not to allocate more than 65535 topic aliases.
/// @ingroup global
#define CC_MQTT5_MAX_TOPIC_ALIASES_LIMIT 0xffff

/// @brief Quality of Service
/// @ingroup global
typedef enum
{
    CC_Mqtt5QoS_AtMostOnceDelivery = 0, ///< QoS=0. At most once delivery.
    CC_Mqtt5QoS_AtLeastOnceDelivery = 1, ///< QoS=1. At least once delivery.
    CC_Mqtt5QoS_ExactlyOnceDelivery = 2, ///< QoS=2. Exactly once delivery.
    CC_Mqtt5QoS_ValuesLimit ///< Limit for the values
} CC_Mqtt5QoS;

/// @brief Error code returned by various API functions.
/// @ingroup global
typedef enum
{
    CC_Mqtt5ErrorCode_Success = 0, ///< The requested function executed successfully.
    CC_Mqtt5ErrorCode_InternalError = 1, ///< Internal library error, please submit bug report    
    CC_Mqtt5ErrorCode_NotIntitialized = 2, ///< The allocated client hasn't been initialized.
    CC_Mqtt5ErrorCode_Busy = 3, ///< The client library is in the middle of previous operation(s), cannot start a new one.
    CC_Mqtt5ErrorCode_NotConnected = 4, ///< The client library is not connected to the broker. Returned by operations that require connection to the broker.
    CC_Mqtt5ErrorCode_AlreadyConnected = 5, ///< The client library is already connected to the broker, cannot perform connection operation.
    CC_Mqtt5ErrorCode_BadParam = 6, ///< Bad parameter is passed to the function.
    CC_Mqtt5ErrorCode_InsufficientConfig = 7, ///< The required configuration hasn't been performed.
    CC_Mqtt5ErrorCode_OutOfMemory = 8, ///< Memory allocation failed.
    CC_Mqtt5ErrorCode_BufferOverflow = 9, ///< Output buffer is too short
    CC_Mqtt5ErrorCode_NotSupported = 10, ///< Feature is not supported
    CC_Mqtt5ErrorCode_RetryLater = 11, ///< Retry in next event loop iteration.
    CC_Mqtt5ErrorCode_Disconnecting = 12, ///< The client is in "disconnecting" state, (re)connect is required in the next iteration loop.
    CC_Mqtt5ErrorCode_NetworkDisconnected = 13, ///< When network is disconnected issueing new ops is not accepted
    CC_Mqtt5ErrorCode_NotAuthenticated = 14, ///< The client not authenticated.
    CC_Mqtt5ErrorCode_PreparationLocked = 15, ///< Another operation is being prepared, cannot create a new one without performing "send" or "cancel".
    CC_Mqtt5ErrorCode_ValuesLimit ///< Limit for the values
} CC_Mqtt5ErrorCode;

/// @brief Payload Format Indicator values as defined by the MQTT v5 protocol
/// @ingroup global
typedef enum
{
    CC_Mqtt5PayloadFormat_Unspecified = 0, ///< Unspecified format
    CC_Mqtt5PayloadFormat_Utf8 = 1, ///< UTF-8 string
    CC_Mqtt5PayloadFormat_ValuesLimit ///< Limit of the values
} CC_Mqtt5PayloadFormat;

/// @brief Status of the asynchronous operation.
/// @ingroup global
typedef enum
{
    CC_Mqtt5AsyncOpStatus_Complete = 0, ///< The requested operation has been completed, refer to reported extra details for information.
    CC_Mqtt5AsyncOpStatus_InternalError = 1, ///< Internal library error, please submit bug report    
    CC_Mqtt5AsyncOpStatus_Timeout = 2, ///< The required response from broker hasn't been received in time
    CC_Mqtt5AsyncOpStatus_ProtocolError = 3, ///< The broker's response doesn't comply with MQTT5 specification.
    CC_Mqtt5AsyncOpStatus_Aborted = 4, ///< The operation has been aborted before completion due to client's side operation.
    CC_Mqtt5AsyncOpStatus_BrokerDisconnected = 5, ///< The operation has been aborted before completion due to broker's disconnection.
    CC_Mqtt5AsyncOpStatus_OutOfMemory = 6, ///< The client library wasn't able to allocate necessary memory.
    CC_Mqtt5AsyncOpStatus_BadParam = 7, ///< Bad value has been returned from the relevant callback.
    CC_Mqtt5AsyncOpStatus_ValuesLimit ///< Limit for the values
} CC_Mqtt5AsyncOpStatus;

/// @brief Error code returned by the @ref CC_Mqtt5AuthCb callback.
/// @ingroup global
typedef enum
{
    CC_Mqtt5AuthErrorCode_Continue = 0, ///< Continue the authentication process
    CC_Mqtt5AuthErrorCode_Disconnect = 1, ///< Stop the authentication, send DISCONNECT to broker
    CC_Mqtt5AuthErrorCode_ValuesLimit = 2 ///< Limit for the values
} CC_Mqtt5AuthErrorCode;

/// @brief "Reason Code" as defined in MQTT v5 specification
/// @ingroup global
typedef enum
{
    CC_Mqtt5ReasonCode_Success = 0, ///< value @b Success. 
    CC_Mqtt5ReasonCode_NormalDisconnection = 0, ///< value <b>Normal Disconnection</b>. 
    CC_Mqtt5ReasonCode_GrantedQos0 = 0, ///< value <b>Granted QoS0</b>. 
    CC_Mqtt5ReasonCode_GrantedQos1 = 1, ///< value <b>Granted QoS1</b>. 
    CC_Mqtt5ReasonCode_GrantedQos2 = 2, ///< value <b>Granted QoS2</b>. 
    CC_Mqtt5ReasonCode_DisconnectWithWill = 4, ///< value <b>Disconnect w/ Will</b>. 
    CC_Mqtt5ReasonCode_NoMatchingSubscribers = 16, ///< value <b>No Matching Subscribers</b>. 
    CC_Mqtt5ReasonCode_NoSubscriptionExisted = 17, ///< value <b>No Subscription Existed</b>. 
    CC_Mqtt5ReasonCode_ContinueAuth = 24, ///< value <b>Continue authentication</b>. 
    CC_Mqtt5ReasonCode_ReAuth = 25, ///< value <b>Re-authenticate</b>. 
    CC_Mqtt5ReasonCode_UnspecifiedError = 128, ///< value <b>Unspecified error</b>. 
    CC_Mqtt5ReasonCode_MalformedPacket = 129, ///< value <b>Malformed Packet</b>. 
    CC_Mqtt5ReasonCode_ProtocolError = 130, ///< value <b>Protocol Error</b>. 
    CC_Mqtt5ReasonCode_ImplSpecificError = 131, ///< value <b>Impl. Specific Error</b>. 
    CC_Mqtt5ReasonCode_UnsupportedVersion = 132, ///< value <b>Unsupported Version</b>. 
    CC_Mqtt5ReasonCode_ClientIdInvalid = 133, ///< value <b>Client ID Invalid</b>. 
    CC_Mqtt5ReasonCode_BadUserPassword = 134, ///< value <b>Bad Username/Password</b>. 
    CC_Mqtt5ReasonCode_NotAuthorized = 135, ///< value <b>Not authorized</b>. 
    CC_Mqtt5ReasonCode_ServerUnavailable = 136, ///< value <b>Server unavailable</b>. 
    CC_Mqtt5ReasonCode_ServerBusy = 137, ///< value <b>Server busy</b>. 
    CC_Mqtt5ReasonCode_Banned = 138, ///< value @b Banned. 
    CC_Mqtt5ReasonCode_ServerShuttingDown = 139, ///< value <b>Server shutting down</b>. 
    CC_Mqtt5ReasonCode_BadAuthMethod = 140, ///< value <b>Bad auth method</b>. 
    CC_Mqtt5ReasonCode_KeepAliveTimeout = 141, ///< value <b>Keep Alive timeout</b>. 
    CC_Mqtt5ReasonCode_SessionTakenOver = 142, ///< value <b>Session taken over</b>. 
    CC_Mqtt5ReasonCode_TopicFilterInvalid = 143, ///< value <b>Topic Filter invalid</b>. 
    CC_Mqtt5ReasonCode_TopicNameInvalid = 144, ///< value <b>Topic Name invalid</b>. 
    CC_Mqtt5ReasonCode_PacketIdInUse = 145, ///< value <b>Packet ID in use</b>. 
    CC_Mqtt5ReasonCode_PacketIdNotFound = 146, ///< value <b>Packet ID not found</b>. 
    CC_Mqtt5ReasonCode_ReceiveMaxExceeded = 147, ///< value <b>Receive Max exceeded</b>. 
    CC_Mqtt5ReasonCode_TopicAliasInvalid = 148, ///< value <b>Topic Alias invalid</b>. 
    CC_Mqtt5ReasonCode_PacketTooLarge = 149, ///< value <b>Packet too large</b>. 
    CC_Mqtt5ReasonCode_MsgRateTooHigh = 150, ///< value <b>Message rate too high</b>. 
    CC_Mqtt5ReasonCode_QuotaExceeded = 151, ///< value <b>Quota exceeded</b>. 
    CC_Mqtt5ReasonCode_AdministrativeAction = 152, ///< value <b>Administrative action</b>. 
    CC_Mqtt5ReasonCode_PayloadFormatInvalid = 153, ///< value <b>Payload format invalid</b>. 
    CC_Mqtt5ReasonCode_RetainNotSupported = 154, ///< value <b>Retain not supported</b>. 
    CC_Mqtt5ReasonCode_QosNotSupported = 155, ///< value <b>QoS not supported</b>. 
    CC_Mqtt5ReasonCode_UseAnotherServer = 156, ///< value <b>Use another server</b>. 
    CC_Mqtt5ReasonCode_ServerMoved = 157, ///< value <b>Server moved</b>. 
    CC_Mqtt5ReasonCode_SharedSubNotSuppored = 158, ///< value <b>Shared Sub not supported</b>. 
    CC_Mqtt5ReasonCode_ConnectionRateExceeded = 159, ///< value <b>Connection rate exceeded</b>. 
    CC_Mqtt5ReasonCode_MaxConnectTime = 160, ///< value <b>Maximum connect time</b>. 
    CC_Mqtt5ReasonCode_SubIdsNotSupported = 161, ///< value <b>Sub IDs not supported</b>. 
    CC_Mqtt5ReasonCode_WildcardSubsNotSupported = 162, ///< value <b>Wildcard Subs not supported</b>. 
} CC_Mqtt5ReasonCode;

/// @brief "Retain Handling" option as defined by the MQTT v5 specification.
/// @details It is used during the "subscribe" operation topic configuration.
/// @ingroup subscribe
typedef enum
{
    CC_Mqtt5RetainHandling_Send = 0, ///< Send retained messages at the time of the subscribe
    CC_Mqtt5RetainHandling_SendIfDoesNotExist = 1, ///< Send retained messages at subscribe only if the subscription does not currently exist
    CC_Mqtt5RetainHandling_DoNotSend = 2, ///< Do not send retained messages at the time of the subscribe
    CC_Mqtt5RetainHandling_ValuesLimit ///< Limit for the values
} CC_Mqtt5RetainHandling;

/// @brief Preference of whether to use numeric "Topic Alias" instead of string topic during publish operation.
/// @ingroup publish
typedef enum
{
    CC_Mqtt5TopicAliasPreference_UseAliasIfAvailable = 0, ///< Use topic alias if such is available
    CC_Mqtt5TopicAliasPreference_ForceAliasOnly = 1, ///< Force sending topic alias, requires topic alias to be allocated.
    CC_Mqtt5TopicAliasPreference_ForceTopicOnly = 2, ///< Force sending topic string even if topic alias is available.
    CC_Mqtt5TopicAliasPreference_ForceTopicWithAlias = 3, ///< Force sending both topic string and its numeric alias.
    CC_Mqtt5TopicAliasPreference_ValuesLimit ///< Limit for the values
} CC_Mqtt5TopicAliasPreference;

/// @brief Publish ordering configuration
/// @ingroup publish
typedef enum
{
    CC_Mqtt5PublishOrdering_SameQos, ///< Preserve strict order only between same QoS messages.
    CC_Mqtt5PublishOrdering_Full, ///< Preserve strict order between @b all messages.
    CC_Mqtt5PublishOrdering_ValuesLimit ///< Limit for the values
} CC_Mqtt5PublishOrdering;

/// @brief Reason for reporting unsolicited broker disconnection
/// @ingroup global
typedef enum
{
    CC_Mqtt5BrokerDisconnectReason_DisconnectMsg = 0, ///< The broker sent @b DISCONNECT message.
    CC_Mqtt5BrokerDisconnectReason_InternalError = 1, ///< The library encountered internal error and there is a need to close network connection
    CC_Mqtt5BrokerDisconnectReason_NoBrokerResponse = 2, ///< No messages from the broker and no response to @b PINGREQ
    CC_Mqtt5BrokerDisconnectReason_ProtocolError = 3, ///< Protocol error was detected.
    CC_Mqtt5BrokerDisconnectReason_ValuesLimit ///< Limit for the values
} CC_Mqtt5BrokerDisconnectReason;

/// @brief Declaration of the hidden structure used to define @ref CC_Mqtt5ClientHandle
/// @ingroup client
struct CC_Mqtt5Client;

/// @brief Handle used to access client specific data structures.
/// @details Returned by cc_mqtt5_client_alloc() function.
/// @ingroup client
typedef struct CC_Mqtt5Client* CC_Mqtt5ClientHandle;

/// @brief Declaration of the hidden structure used to define @ref CC_Mqtt5ConnectHandle
/// @ingroup connect
struct CC_Mqtt5Connect;

/// @brief Handle for "connect" operation.
/// @details Returned by cc_mqtt5_client_connect_prepare() function.
/// @ingroup connect
typedef struct CC_Mqtt5Connect* CC_Mqtt5ConnectHandle;

/// @brief Declaration of the hidden structure used to define @ref CC_Mqtt5DisconnectHandle
/// @ingroup disconnect
struct CC_Mqtt5Disconnect;

/// @brief Handle for "disconnect" operation.
/// @details Returned by cc_mqtt5_client_disconnect_prepare() function.
/// @ingroup disconnect
typedef struct CC_Mqtt5Disconnect* CC_Mqtt5DisconnectHandle;

/// @brief Declaration of the hidden structure used to define @ref CC_Mqtt5SubscribeHandle
/// @ingroup subscribe
struct CC_Mqtt5Subscribe;

/// @brief Handle for "subscribe" operation.
/// @details Returned by cc_mqtt5_client_subscribe_prepare() function.
/// @ingroup subscribe
typedef struct CC_Mqtt5Subscribe* CC_Mqtt5SubscribeHandle;

/// @brief Declaration of the hidden structure used to define @ref CC_Mqtt5UnsubscribeHandle
/// @ingroup unsubscribe
struct CC_Mqtt5Unsubscribe;

/// @brief Handle for "unsubscribe" operation.
/// @details Returned by cc_mqtt5_client_unsubscribe_prepare() function.
/// @ingroup unsubscribe
typedef struct CC_Mqtt5Unsubscribe* CC_Mqtt5UnsubscribeHandle;

/// @brief Declaration of the hidden structure used to define @ref CC_Mqtt5PublishHandle
/// @ingroup publish
struct CC_Mqtt5Publish;

/// @brief Handle for "publish" operation.
/// @details Returned by cc_mqtt5_client_publish_prepare() function.
/// @ingroup publish
typedef struct CC_Mqtt5Publish* CC_Mqtt5PublishHandle;

/// @brief Declaration of the hidden structure used to define @ref CC_Mqtt5ReauthHandle
/// @ingroup reauth
struct CC_Mqtt5Reauth;

/// @brief Handle for "reauth" operation.
/// @details Returned by cc_mqtt5_client_reauth_prepare() function.
/// @ingroup reauth
typedef struct CC_Mqtt5Reauth* CC_Mqtt5ReauthHandle;

/// @brief Wraping structre of the single "User Property".
/// @see @b cc_mqtt5_client_init_user_prop()
/// @ingroup global
typedef struct
{
    const char* m_key; ///< Key string, mustn't be NULL
    const char* m_value; ///< Value string, can be NULL
} CC_Mqtt5UserProp;

/// @brief Configuration structure to be passed to the @b cc_mqtt5_client_connect_config_basic().
/// @see @b cc_mqtt5_client_connect_init_config_basic()
/// @ingroup connect
typedef struct
{
    const char* m_clientId; ///< Zero terminated Client ID string, can be NULL. When NULL means empty "Client ID". Defaults to NULL.
    const char* m_username; ///< Zero terminated Username string, can be NULL. When NULL means no username value. Defaults to NULL.
    const unsigned char* m_password; ///< Pointer to password buffer, can be NULL, defaults to NULL.
    unsigned m_passwordLen; ///< Number of password bytes. When 0 means no password value. Defaults to 0.
    unsigned m_keepAlive; ///< Keep alive seconds configuration, defaults to 60.
    bool m_cleanStart; ///< Clean start flag, defaults to false.
} CC_Mqtt5ConnectBasicConfig;

/// @brief Configuration structure to be passed to the @b cc_mqtt5_client_connect_config_will().
/// @see @b cc_mqtt5_client_connect_init_config_will()
/// @ingroup connect
typedef struct
{
    const char* m_topic; ///< Will topic string, must NOT be NULL or empty. Defaults to NULL.
    const unsigned char* m_data; ///< Will message data, can be NULL. Defaults to NULL.
    unsigned m_dataLen; ///< Number of will data bytes. When 0 means no data. Defaults to 0.
    const char* m_contentType; ///< "Content Type" will property, not added when NULL. Defaults to NULL.
    const char* m_responseTopic; ///< "Response Topic" will property, not added when NULL. Defaults to NULL.
    const unsigned char* m_correlationData; ///< "Correlation Data" will property, can be NULL. Defaults to NULL.
    unsigned m_correlationDataLen; ///< Number of bytes in the "Correlation Data" buffer. Not added when 0. Defaults to 0.
    unsigned m_delayInterval; ///< "Will Delay Interval" will property, not added when 0. Defaults to 0.
    unsigned m_messageExpiryInterval; ///< "Message Expiry Interval" will property, not added when 0. Defaults to 0.
    CC_Mqtt5QoS m_qos; ///< QoS value of the will message, defaults to CC_Mqtt5QoS_AtMostOnceDelivery.
    CC_Mqtt5PayloadFormat m_format; ///< "Payload Format Indicator" will property, defaults to CC_Mqtt5PayloadFormat_Unspecified, not added when CC_Mqtt5PayloadFormat_Unspecified.
    bool m_retain; ///< "Retain" flag, defaults to false.
} CC_Mqtt5ConnectWillConfig;

/// @brief Extra properties configuration of the "connect" operation.
/// @see @b cc_mqtt5_client_connect_init_config_extra()
/// @ingroup connect
typedef struct
{
    unsigned m_sessionExpiryInterval; ///< "Session Expiry Interval" property, defaults to 0, not added when 0.
    unsigned m_receiveMaximum; ///< "Receive Maximum" property - allowed amount of incomplete Qos1 and Qos2 publishes.
                               ///< When equals to @b 0 (default) no property is added, which is perceived as 65,535 on the broker side.
    unsigned m_maxPacketSize; ///< "Maximum Packet Size" property, defaults to 0, not added when 0, which means "no limit".
    unsigned m_topicAliasMaximum; ///< "Topic Alias Maximum" property, defaults to 0, not added when 0.
    bool m_requestResponseInfo; ///< "Request Response Information" property, defaults to @b false, not added when @b false.
    bool m_requestProblemInfo; ///< "Request Problem Information" property, defaults to @b false, not added when @b false.
} CC_Mqtt5ConnectExtraConfig;

/// @brief Response information from broker to "connect" request
/// @ingroup connect
typedef struct 
{
    CC_Mqtt5ReasonCode m_reasonCode; ///< "Reason Code" reported by the broker
    const char* m_assignedClientId; ///< "Assigned Client Identifier" property, NULL if not reported
    const char* m_responseInfo; ///< "Response Information" property, NULL if not reported
    const char* m_reasonStr; ///< "Reason String" property, NULL if not reported
    const char* m_serverRef; ///< "Server Reference" property, NULL if not reported
    const unsigned char* m_authData; ///< Final "Authentication Data" property buffer, NULL if not reported
    unsigned m_authDataLen; ///< Amount of bytes in the "Authentication Data" property buffer
    const CC_Mqtt5UserProp* m_userProps; ///< Pointer to array containing "User Properties", NULL if none
    unsigned m_userPropsCount; ///< Amount of "User Properties" in the array
    unsigned m_sessionExpiryInterval; ///< "Session Expiry Interval" property, 0 if not reported.
    unsigned m_highQosSendLimit; ///< "Receive Maximum" property.
    unsigned m_maxPacketSize; ///< "Maximum Packet Size" property, 0 if not reported.
    unsigned m_topicAliasMax; ///< "Topic Alias Maximum" property, 0 if not reported.
    CC_Mqtt5QoS m_maxQos; ///< "Maximum QoS" property, 
    bool m_sessionPresent; ///< "Session Present" indication.
    bool m_retainAvailable; ///< "Retain Available" indication.
    bool m_wildcardSubAvailable; ///< "Wildcard Subscription Available" indication.
    bool m_subIdsAvailable; ///< "Subscription Identifiers Available" indication.
    bool m_sharedSubsAvailable; ///< "Shared Subscription Available" indication.
} CC_Mqtt5ConnectResponse;

/// @brief  Authentication handshake information
/// @ingroup global
/// @see @b cc_mqtt5_client_connect_init_auth_info()
typedef struct
{
    const unsigned char* m_authData; ///< Pointer to the authentication data buffer, can be NULL
    unsigned m_authDataLen; ///< Amount of bytes in the authentication data buffer, defaults to 0, not added when 0.
    const char* m_reasonStr; ///< "Reason String" property, defaults to NULL, not added when NULL.
    const CC_Mqtt5UserProp* m_userProps; ///< Pointer to "User Properties" array, can be NULL
    unsigned m_userPropsCount; ///< Amount of elements in the "User Properties" array, defaults to 0, not added when 0.
} CC_Mqtt5AuthInfo;

/// @brief Broker disconnection information
/// @ingroup global
typedef struct
{
    CC_Mqtt5ReasonCode m_reasonCode; ///< "Reason Code" reported by the broker
    const char* m_reasonStr; ///< "Reason String" property, NULL if not reported
    const char* m_serverRef; ///< "Server Reference" property, NULL if not reported
    const CC_Mqtt5UserProp* m_userProps; ///< Pointer to "User Properties" array, can be NULL
    unsigned m_userPropsCount; ///< Amount of elements in the "User Properties" array, defaults to 0, not added when 0.  
} CC_Mqtt5DisconnectInfo;

/// @brief Configuration structure of the "disconnect" operation.
/// @ingroup disconnect
/// @see @b cc_mqtt5_client_disconnect_init_config()
typedef struct
{
    CC_Mqtt5ReasonCode m_reasonCode; ///< "Reason Code" to report to the broker
    const char* m_reasonStr; ///< "Reason String" property, defaults to NULL, not added when NULL.
    unsigned* m_sessionExpiryInterval; ///< Pointer to "Session Expiry Interval" property value, defaults to NULL, not added when NULL.
} CC_Mqtt5DisconnectConfig;

/// @brief Topic filter configuration structure of the "subscribe" operation.
/// @ingroup subscribe
/// @see @b cc_mqtt5_client_subscribe_init_config_topic()
typedef struct
{
    const char* m_topic; ///< "Topic Filter" string, mustn't be NULL
    CC_Mqtt5QoS m_maxQos; ///< "Maximum QoS" value, defaults to @ref CC_Mqtt5QoS_ExactlyOnceDelivery.
    CC_Mqtt5RetainHandling m_retainHandling; ///< "Retain Handling" subscription option, defaults to @ref CC_Mqtt5RetainHandling_Send.
    bool m_noLocal; ///< "No Local" subscription option, defaults to @b false.
    bool m_retainAsPublished; ///< "Retain As Published" subscription option, defaults to @b false.
} CC_Mqtt5SubscribeTopicConfig;

/// @brief Extra subscription properties configuration structure.
/// @ingroup subscribe
/// @see @b cc_mqtt5_client_subscribe_init_config_extra()
typedef struct
{
    unsigned m_subId; ///< "Subscription Identifier" property, defaults to invalid value of @b 0.
} CC_Mqtt5SubscribeExtraConfig;

/// @brief Response information from broker to "subscribe" request
/// @ingroup subscribe
typedef struct 
{
    const CC_Mqtt5ReasonCode* m_reasonCodes; ///< Pointer to array contianing per-topic subscription reason codes.
    unsigned m_reasonCodesCount; ///< Amount of reason codes in the array.
    const char* m_reasonStr; ///< "Reason String" property, can be NULL
    const CC_Mqtt5UserProp* m_userProps; ///< Pointer to "User Properties" array, can be NULL
    unsigned m_userPropsCount; ///< Amount of elements in the "User Properties" array.
} CC_Mqtt5SubscribeResponse;

/// @brief Topic filter configuration structure of the "unsubscribe" operation.
/// @see @b cc_mqtt5_client_unsubscribe_init_config_topic()
/// @ingroup unsubscribe
typedef struct
{
    const char* m_topic; ///< "Topic Filter" string, mustn't be NULL
} CC_Mqtt5UnsubscribeTopicConfig;

/// @brief Response information from broker to "unsubscribe" request
/// @ingroup unsubscribe
typedef struct 
{
    const CC_Mqtt5ReasonCode* m_reasonCodes; ///< Pointer to array contianing per-topic unsubscription reason codes.
    unsigned m_reasonCodesCount; ///< Amount of reason codes in the array.
    const char* m_reasonStr; ///< "Reason String" property, can be NULL
    const CC_Mqtt5UserProp* m_userProps; ///< Pointer to "User Properties" array, can be NULL
    unsigned m_userPropsCount; ///< Amount of elements in the "User Properties" array.
} CC_Mqtt5UnsubscribeResponse;

/// @brief Received message information
/// @ingroup global
typedef struct
{
    const char* m_topic; ///< Topic used to publish the message
    const unsigned char* m_data; ///< Pointer to the temporary buffer containin message data
    unsigned m_dataLen; ///< Amount of data bytes 
    const char* m_responseTopic; ///< "Response Topic" property when provided, NULL if not.
    const unsigned char* m_correlationData; ///< Pointer to the "Correlation Data" property value when provided, NULL if not.
    unsigned m_correlationDataLen; ///< Amount of "Correlation Data" bytes;
    const CC_Mqtt5UserProp* m_userProps; ///< Pointer to the "User Property" properties array when provided, NULL if not.
    unsigned m_userPropsCount; ///< Amount of "User Property" properties
    const char* m_contentType; ///< "Content Type" property if provided, NULL if not.
    const unsigned* m_subIds; ///< Pointer to array containing "Subscription Identifier" properties list when provided, NULL if not.
    unsigned m_subIdsCount; ///< Amount of "Subscription Identifiers" in array.
    unsigned m_messageExpiryInterval; ///< "Message Expiry Interval" property, defaults to 0 when not reported.
    CC_Mqtt5QoS m_qos; ///< QoS value used by the broker to report the message.
    CC_Mqtt5PayloadFormat m_format; ///< "Payload Format Indicator" property, defaults to @ref CC_Mqtt5PayloadFormat_Unspecified when not reported.
    bool m_retained; ///< Indication of whether the received message was "retained".
} CC_Mqtt5MessageInfo;

/// @brief Configuration structure to be passed to the @b cc_mqtt5_client_publish_config_basic().
/// @see @b cc_mqtt5_client_publish_init_config_basic()
/// @ingroup publish
typedef struct
{
    const char* m_topic; ///< Publish topic, cannot be NULL.
    const unsigned char* m_data; ///< Pointer to publish data buffer, defaults to NULL.
    unsigned m_dataLen; ///< Amount of bytes in the publish data buffer, defaults to 0.
    CC_Mqtt5QoS m_qos; ///< Publish QoS value, defaults to @ref CC_Mqtt5QoS_AtMostOnceDelivery.
    CC_Mqtt5TopicAliasPreference m_topicAliasPref; ///< Topic alias usage preference, defaults to @ref CC_Mqtt5TopicAliasPreference_UseAliasIfAvailable.
    bool m_retain; ///< "Retain" flag, defaults to false.
} CC_Mqtt5PublishBasicConfig;

/// @brief Configuration structure to be passed to the @b cc_mqtt5_client_publish_config_extra().
/// @see @b cc_mqtt5_client_publish_init_config_extra()
/// @ingroup publish
typedef struct
{
    const char* m_contentType; ///< "Content Type" property, defaults to NULL, not added when NULL.
    const char* m_responseTopic; ///< "Response Topic" property, defaults to NULL, not added when NULL.
    const unsigned char* m_correlationData; ///< "Correlation Data" property, can be NULL.
    unsigned m_correlationDataLen; ///< Length of the "Correlation Data", not added when 0.
    unsigned m_messageExpiryInterval; ///< "Message Expiry Interval" property, defaults to 0, not added when 0.
    CC_Mqtt5PayloadFormat m_format; ///< "Payload Format Indicator" property, defaults to CC_Mqtt5PayloadFormat_Unspecified, not added when CC_Mqtt5PayloadFormat_Unspecified.
} CC_Mqtt5PublishExtraConfig;

/// @brief Response information from broker to "publish" request
/// @ingroup publish
typedef struct 
{
    CC_Mqtt5ReasonCode m_reasonCode; ///< "Reason Code" of the operation
    const char* m_reasonStr; ///< "Reason String" property, can be NULL.
    const CC_Mqtt5UserProp* m_userProps; ///< Pointer to array of "User Properties", can be NULL
    unsigned m_userPropsCount; ///< Number of elements in "User Properties" array.
} CC_Mqtt5PublishResponse;

/// @brief Callback used to request time measurement.
/// @details The callback is set using
///     cc_mqtt5_client_set_next_tick_program_callback() function.
/// @param[in] data Pointer to user data object, passed as last parameter to
///     cc_mqtt5_client_set_next_tick_program_callback() function.
/// @param[in] duration Time duration in @b milliseconds. After the requested
///     time expires, the cc_mqtt5_client_tick() function is expected to be invoked.
/// @ingroup client
typedef void (*CC_Mqtt5NextTickProgramCb)(void* data, unsigned duration);

/// @brief Callback used to request termination of existing time measurement.
/// @details The callback is set using
///     cc_mqtt5_client_set_cancel_next_tick_wait_callback() function.
/// @param[in] data Pointer to user data object, passed as last parameter to
///     cc_mqtt5_client_set_cancel_next_tick_wait_callback() function.
/// @return Number of elapsed milliseconds since last time measurement request.
/// @ingroup client
typedef unsigned (*CC_Mqtt5CancelNextTickWaitCb)(void* data);

/// @brief Callback used to request to send data to the broker.
/// @details The callback is set using
///     cc_mqtt5_client_set_send_output_data_callback() function. The reported
///     data resides in internal data structures of the client library, which
///     can be updated / deleted right after the callback function returns. It means
///     the data may need to be copied into some other buffer which will be
///     held intact until the send over I/O link operation is complete.
/// @param[in] data Pointer to user data object, passed as last parameter to
///     cc_mqtt5_client_set_send_output_data_callback() function.
/// @param[in] buf Pointer to the buffer containing data to send
/// @param[in] bufLen Number of bytes in the buffer
/// @post The buffer data can be deallocated / overwritten after the callback function returns.
/// @ingroup client
typedef void (*CC_Mqtt5SendOutputDataCb)(void* data, const unsigned char* buf, unsigned bufLen);

/// @brief Callback used to report unsolicited disconnection of the broker.
/// @details When invoked the "info" is present <b>if and only if</b> the 
///     broker disconnection report is due to the reception of the @b DISCONNECT
///     message from the broker.
/// @param[in] data Pointer to user data object, passed as the last parameter to
///     the request call.
/// @param[in] reason Reson for reporting unsolicited broker disconnection.
/// @param[in] info Extra disconnect information when reported by the broker. Can be NULL. 
///     Not null, <b>if and only if</b> the @b reason is @ref CC_Mqtt5BrokerDisconnectReason_DisconnectMsg.
/// @post The data members of the reported info can NOT be accessed after the function returns.
/// @ingroup client
typedef void (*CC_Mqtt5BrokerDisconnectReportCb)(void* data, CC_Mqtt5BrokerDisconnectReason reason, const CC_Mqtt5DisconnectInfo* info);

/// @brief Callback used to report new message received of the broker.
/// @param[in] data Pointer to user data object, passed as the last parameter to
///     the request call.
/// @param[in] info Message information. Will NOT be NULL.
/// @post The data members of the reported info can NOT be accessed after the function returns.
/// @ingroup client
typedef void (*CC_Mqtt5MessageReceivedReportCb)(void* data, const CC_Mqtt5MessageInfo* info);

/// @brief Callback used to report discovered errors.
/// @param[in] data Pointer to user data object, passed as the last parameter to
///     the request call.
/// @param[in] msg Error log message.
/// @ingroup client
typedef void (*CC_Mqtt5ErrorLogCb)(void* data, const char* msg);

/// @brief Callback used to report completion of the "connect" operation.
/// @param[in] data Pointer to user data object passed as last parameter to the
///     @b cc_mqtt5_client_connect_send().
/// @param[in] status Status of the "connect" operation.
/// @param[in] response Response information from the broker. Not-NULL is reported <b>if and onfly if</b>
///     the "status" is equal to @ref CC_Mqtt5AsyncOpStatus_Complete.
/// @post The data members of the reported response can NOT be accessed after the function returns.
/// @ingroup connect
typedef void (*CC_Mqtt5ConnectCompleteCb)(void* data, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response);

/// @brief Callback used to report incoming authentication data.
/// @param[in] data Pointer to user data object passed during callback registration.
/// @param[in] authInfoIn Pointer to authentication data received from broker.
/// @param[out] authInfoOut Pointer to authentication data to be sent to the broker. Expected to be filled by the callback.
typedef CC_Mqtt5AuthErrorCode (*CC_Mqtt5AuthCb)(void* data, const CC_Mqtt5AuthInfo* authInfoIn, CC_Mqtt5AuthInfo* authInfoOut);

/// @brief Callback used to report completion of the "subscribe" operation.
/// @param[in] data Pointer to user data object passed as last parameter to the
///     @b cc_mqtt5_client_subscribe_send().
/// @param[in] handle Handle returned by @b cc_mqtt5_client_subscribe_prepare() function. When the 
///     callback is invoked the handle is already invalid and cannot be used in any relevant 
///     function invocation, but it allows end application to identify the original "subscribe" operation
///     and use the same callback function in parallel requests.
/// @param[in] status Status of the "subscribe" operation.
/// @param[in] response Response information from the broker. Not-NULL is reported <b>if and onfly if</b>
///     the "status" is equal to @ref CC_Mqtt5AsyncOpStatus_Complete.
/// @post The data members of the reported response can NOT be accessed after the function returns.
/// @ingroup subscribe
typedef void (*CC_Mqtt5SubscribeCompleteCb)(void* data, CC_Mqtt5SubscribeHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response);

/// @brief Callback used to report completion of the "unsubscribe" operation.
/// @param[in] data Pointer to user data object passed as last parameter to the
///     @b cc_mqtt5_client_unsubscribe_send().
/// @param[in] handle Handle returned by @b cc_mqtt5_client_unsubscribe_prepare() function. When the 
///     callback is invoked the handle is already invalid and cannot be used in any relevant 
///     function invocation, but it allows end application to identify the original "unsubscribe" operation
///     and use the same callback function in parallel requests.
/// @param[in] status Status of the "unsubscribe" operation.
/// @param[in] response Response information from the broker. Not-NULL is reported <b>if and onfly if</b>
///     the "status" is equal to @ref CC_Mqtt5AsyncOpStatus_Complete.
/// @post The data members of the reported response can NOT be accessed after the function returns.
/// @ingroup unsubscribe
typedef void (*CC_Mqtt5UnsubscribeCompleteCb)(void* data, CC_Mqtt5UnsubscribeHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5UnsubscribeResponse* response);

/// @brief Callback used to report completion of the "publish" operation.
/// @param[in] data Pointer to user data object passed as last parameter to the
///     @b cc_mqtt5_client_publish_send().
/// @param[in] handle Handle returned by @b cc_mqtt5_client_publish_prepare() function. When the 
///     callback is invoked the handle is already invalid and cannot be used in any relevant 
///     function invocation, but it allows end application to identify the original "publish" operation
///     and use the same callback function in parallel requests.
/// @param[in] status Status of the "publish" operation.
/// @param[in] response Response information from the broker. Not-NULL is reported <b>if and onfly if</b>
///     the "status" is equal to @ref CC_Mqtt5AsyncOpStatus_Complete.
/// @post The data members of the reported response can NOT be accessed after the function returns.
/// @ingroup publish
typedef void (*CC_Mqtt5PublishCompleteCb)(void* data, CC_Mqtt5PublishHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response);

/// @brief Callback used to report completion of the "reauth" operation.
/// @param[in] data Pointer to user data object passed as last parameter to the
///     @b cc_mqtt5_client_reauth_send().
/// @param[in] status Status of the "reauth" operation.
/// @param[in] response Response information from the broker. Not-NULL is reported <b>if and onfly if</b>
///     the "status" is equal to @ref CC_Mqtt5AsyncOpStatus_Complete.
/// @post The data members of the reported response can NOT be accessed after the function returns.
/// @ingroup reauth
typedef void (*CC_Mqtt5ReauthCompleteCb)(void* data, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5AuthInfo* response);

/// @brief Authentication Configuration.
/// @see @b cc_mqtt5_client_connect_init_config_auth()
/// @ingroup global
typedef struct
{
    const char* m_authMethod; ///< Authentication method string, must NOT be NULL.
    const unsigned char* m_authData; ///< Pointer to the authentication data buffer, can be NULL
    unsigned m_authDataLen; ///< Amount of bytes in the authentication data buffer.
    CC_Mqtt5AuthCb m_authCb; ///< Callback to be invoked during the authentication handshake
    void* m_authCbData; ///< Pointer to user data object, passed as first parameter to the "m_authCb" callback.

} CC_Mqtt5AuthConfig;

#ifdef __cplusplus
}
#endif
