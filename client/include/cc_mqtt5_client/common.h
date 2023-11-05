//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

/// @file
/// @brief Common definition for MQTT-SN clients.

#pragma once

#ifdef __cplusplus
extern "C" {
#include <stdbool.h>
#endif // #ifdef __cplusplus

/// @brief Major verion of the library
#define CC_MQTT5_CLIENT_MAJOR_VERSION 0U

/// @brief Minor verion of the library
#define CC_MQTT5_CLIENT_MINOR_VERSION 1U

/// @brief Patch level of the library
#define CC_MQTT5_CLIENT_PATCH_VERSION 0U

/// @brief Macro to create numeric version as single unsigned number
#define CC_MQTT5_CLIENT_MAKE_VERSION(major_, minor_, patch_) \
    ((static_cast<unsigned>(major_) << 24) | \
     (static_cast<unsigned>(minor_) << 8) | \
     (static_cast<unsigned>(patch_)))

/// @brief Version of the library as single numeric value
#define CC_MQTT5_CLIENT_VERSION CC_MQTT5_CLIENT_MAKE_VERSION(CC_MQTT5_CLIENT_MAJOR_VERSION, CC_MQTT5_CLIENT_MINOR_VERSION, CC_MQTT5_CLIENT_PATCH_VERSION)

/// @brief Special value for "Session Expiry Interval" property to specify that session never expires.
/// @see @ref CC_Mqtt5ConnectExtraConfig::m_sessionExpiryInterval
/// @see @ref CC_Mqtt5ConnectResponse::m_sessionExpiryInterval
#define CC_MQTT5_SESSION_NEVER_EXPIRES 0xffffffff

/// @brief MQTT5 protocol limit to topic alias.
/// @details The application is expected not to allocate more than 65535 topic aliases.
#define CC_MQTT5_MAX_TOPIC_ALIASES_LIMIT 0xffff

/// @brief Quality of Service
typedef enum
{
    CC_Mqtt5QoS_AtMostOnceDelivery = 0, ///< QoS=0. At most once delivery.
    CC_Mqtt5QoS_AtLeastOnceDelivery = 1, ///< QoS=1. At least once delivery.
    CC_Mqtt5QoS_ExactlyOnceDelivery = 2, ///< QoS=2. Exactly once delivery.
    CC_Mqtt5QoS_ValuesLimit ///< Limit for the values
} CC_Mqtt5QoS;

/// @brief Error code returned by various API functions.
typedef enum
{
    CC_Mqtt5ErrorCode_Success, ///< The requested function executed successfully.
    CC_Mqtt5ErrorCode_InternalError, ///< Internal library error, please submit bug report    
    CC_Mqtt5ErrorCode_NotIntitialized, ///< The allocated client hasn't been initialized.
    CC_Mqtt5ErrorCode_Busy, ///< The client library is in the middle of previous operation(s), cannot start a new one.
    CC_Mqtt5ErrorCode_NotConnected, ///< The client library is not connected to the broker. Returned by operations that require connection to the broker.
    CC_Mqtt5ErrorCode_AlreadyConnected, ///< The client library is already connected to the broker, cannot perform connection operation.
    CC_Mqtt5ErrorCode_BadParam, ///< Bad parameter is passed to the function.
    CC_Mqtt5ErrorCode_InsufficientConfig, ///< The required configuration hasn't been performed.
    CC_Mqtt5ErrorCode_OutOfMemory, ///< Memory allocation failed.
    CC_Mqtt5ErrorCode_BufferOverflow, ///< Output buffer is too short
    CC_Mqtt5ErrorCode_NotSupported, ///< Feature is not supported
    CC_Mqtt5ErrorCode_RetryLater, ///< Retry in next event loop iteration.
    CC_Mqtt5ErrorCode_Terminating, ///< The client is in "terminating" state, (re)init is required.
    CC_Mqtt5ErrorCode_NetworkDisconnected, ///< When network is disconnected issueing new ops is not accepted
    CC_Mqtt5ErrorCode_NotAuthenticated, ///< The client not authenticated.
    CC_Mqtt5ErrorCode_ValuesLimit ///< Limit for the values
} CC_Mqtt5ErrorCode;

/// @brief Payload Format Indicator values as defined by the MQTT5 protocol
typedef enum
{
    CC_Mqtt5PayloadFormat_Unspecified = 0, ///< Unspecified format
    CC_Mqtt5PayloadFormat_Utf8 = 1, ///< UTF-8 string
    CC_Mqtt5PayloadFormat_ValuesLimit ///< Limit of the values
} CC_Mqtt5PayloadFormat;

/// @brief Status of the asynchronous operation.
typedef enum
{
    CC_Mqtt5AsyncOpStatus_Complete, ///< The requested operation has been completed, refer to reported extra details for information.
    CC_Mqtt5AsyncOpStatus_InternalError, ///< Internal library error, please submit bug report    
    CC_Mqtt5AsyncOpStatus_Timeout, ///< The required response from broker hasn't been received in time
    CC_Mqtt5AsyncOpStatus_ProtocolError, ///< The broker's response doesn't comply with MQTT5 specification.
    CC_Mqtt5AsyncOpStatus_Aborted, ///< The operation has been aborted before completion due to client's side operation.
    CC_Mqtt5AsyncOpStatus_BrokerDisconnected, ///< The operation has been aborted before completion due to broker's disconnection.
    CC_Mqtt5AsyncOpStatus_OutOfMemory, ///< The client library wasn't able to allocate necessary memory.
    CC_Mqtt5AsyncOpStatus_BadParam, ///< Bad value has been returned from the relevant callback.
    CC_Mqtt5AsyncOpStatus_ValuesLimit ///< Limit for the values
} CC_Mqtt5AsyncOpStatus;

/// @brief Error code returned by the @ref CC_Mqtt5AuthCb callback.
typedef enum
{
    CC_Mqtt5AuthErrorCode_Continue, ///< Continue the authentication process
    CC_Mqtt5AuthErrorCode_Disconnect, ///< Stop the authentication, send DISCONNECT to broker
    CC_Mqtt5AuthErrorCode_ValuesLimit ///< Limit for the values
} CC_Mqtt5AuthErrorCode;

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

/// @brief "Retain Handling" option as defined by the MQTT5 specification.
/// @details It is used during the "subscribe" operation topic configuration.
typedef enum
{
    CC_Mqtt5RetainHandling_Send = 0, ///< Send retained messages at the time of the subscribe
    CC_Mqtt5RetainHandling_SendIfDoesNotExist = 1, ///< Send retained messages at subscribe only if the subscription does not currently exist
    CC_Mqtt5AuthErrorCode_DoNotSend = 2, ///< Do not send retained messages at the time of the subscribe
    CC_Mqtt5RetainHandling_ValuesLimit ///< Limit for the values
} CC_Mqtt5RetainHandling;

/// @brief Preference of whether to use numeric "Topic Alias" instead of string topic during publish operation.
typedef enum
{
    CC_Mqtt5TopicAliasPreference_UseAliasIfAvailable, ///< Use topic alias if such is available
    CC_Mqtt5TopicAliasPreference_ForceAliasOnly, ///< Force sending topic alias, requires topic alias to be allocated.
    CC_Mqtt5TopicAliasPreference_ForceTopicOnly, ///< Force sending topic string even if topic alias is available.
    CC_Mqtt5TopicAliasPreference_ForceTopicWithAlias, ///< Force sending both topic string and its numeric alias.
    CC_Mqtt5TopicAliasPreference_ValuesLimit ///< Limit for the values
} CC_Mqtt5TopicAliasPreference;

/// @brief Declaration of the hidden structure used to define @ref CC_Mqtt5ClientHandle
struct CC_Mqtt5Client;

/// @brief Handle used to access client specific data structures.
/// @details Returned by cc_mqtt5_client_new() function.
typedef CC_Mqtt5Client* CC_Mqtt5ClientHandle;

/// @brief Declaration of the hidden structure used to define @ref CC_Mqtt5ConnectHandle
struct CC_Mqtt5Connect;

/// @brief Handle for "connect" operation.
/// @details Returned by cc_mqtt5_client_connect_prepare() function.
typedef CC_Mqtt5Connect* CC_Mqtt5ConnectHandle;

/// @brief Declaration of the hidden structure used to define @ref CC_Mqtt5DisconnectHandle
struct CC_Mqtt5Disconnect;

/// @brief Handle for "disconnect" operation.
/// @details Returned by cc_mqtt5_client_disconnect_prepare() function.
typedef CC_Mqtt5Disconnect* CC_Mqtt5DisconnectHandle;

/// @brief Declaration of the hidden structure used to define @ref CC_Mqtt5SubscribeHandle
struct CC_Mqtt5Subscribe;

/// @brief Handle for "subscribe" operation.
/// @details Returned by cc_mqtt5_client_subscribe_prepare() function.
typedef CC_Mqtt5Subscribe* CC_Mqtt5SubscribeHandle;

/// @brief Declaration of the hidden structure used to define @ref CC_Mqtt5UnsubscribeHandle
struct CC_Mqtt5Unsubscribe;

/// @brief Handle for "unsubscribe" operation.
/// @details Returned by cc_mqtt5_client_unsubscribe_prepare() function.
typedef CC_Mqtt5Unsubscribe* CC_Mqtt5UnsubscribeHandle;

/// @brief Declaration of the hidden structure used to define @ref CC_Mqtt5PublishHandle
struct CC_Mqtt5Publish;

/// @brief Handle for "publish" operation.
/// @details Returned by cc_mqtt5_client_publish_prepare() function.
typedef CC_Mqtt5Publish* CC_Mqtt5PublishHandle;

/// @brief Declaration of the hidden structure used to define @ref CC_Mqtt5ReauthHandle
struct CC_Mqtt5Reauth;

/// @brief Handle for "reauth" operation.
/// @details Returned by cc_mqtt5_client_reauth_prepare() function.
typedef CC_Mqtt5Reauth* CC_Mqtt5ReauthHandle;

/// @brief Wraping structre of the single "User Property".
typedef struct
{
    const char* m_key; ///< Key string
    const char* m_value; ///< Value string
} CC_Mqtt5UserProp;

typedef struct
{
    const char* m_clientId;
    const char* m_username;
    const unsigned char* m_password;
    unsigned m_passwordLen;
    unsigned m_keepAlive;
    bool m_cleanStart;
} CC_Mqtt5ConnectBasicConfig;

typedef struct
{
    const char* m_topic;
    const unsigned char* m_data;
    unsigned m_dataLen;
    const char* m_contentType;
    const char* m_responseTopic;
    const unsigned char* m_correlationData;
    unsigned m_correlationDataLen;
    unsigned m_delayInterval;
    unsigned m_messageExpiryInterval;
    CC_Mqtt5QoS m_qos;
    CC_Mqtt5PayloadFormat m_format;
    bool m_retain;
} CC_Mqtt5ConnectWillConfig;

/// @brief Extra properties configuration of the "connect" operation.
typedef struct
{
    unsigned m_sessionExpiryInterval; //!< "Session Expiry Interval" property.
    unsigned m_receiveMaximum;
    unsigned m_maxPacketSize;
    unsigned m_topicAliasMaximum;
    bool m_requestResponseInfo;
    bool m_requestProblemInfo;
} CC_Mqtt5ConnectExtraConfig;

/// @brief Response information from broker to "connect" request
typedef struct 
{
    CC_Mqtt5ReasonCode m_reasonCode;
    const char* m_assignedClientId;
    const char* m_responseInfo;
    const char* m_reasonStr;
    const char* m_serverRef;
    const unsigned char* m_authData;
    unsigned m_authDataLen;
    const CC_Mqtt5UserProp* m_userProps;
    unsigned m_userPropsCount;
    unsigned m_sessionExpiryInterval; //!< "Session Expiry Interval" property.
    unsigned m_highQosSendLimit;
    unsigned m_maxPacketSize;
    unsigned m_topicAliasMax;
    CC_Mqtt5QoS m_maxQos;
    bool m_sessionPresent;
    bool m_retainAvailable;
    bool m_wildcardSubAvailable;
    bool m_subIdsAvailable;
    bool m_sharedSubsAvailable;
} CC_Mqtt5ConnectResponse;

typedef struct
{
    const unsigned char* m_authData;
    unsigned m_authDataLen;
    const char* m_reasonStr;
    const CC_Mqtt5UserProp* m_userProps;
    unsigned m_userPropsCount;    
} CC_Mqtt5AuthInfo;

typedef struct
{
    CC_Mqtt5ReasonCode m_reasonCode;
    const char* m_reasonStr;
    const char* m_serverRef;
    const CC_Mqtt5UserProp* m_userProps;
    unsigned m_userPropsCount;      
} CC_Mqtt5DisconnectInfo;

typedef struct
{
    CC_Mqtt5ReasonCode m_reasonCode;
    const char* m_reasonStr;
    unsigned* m_expiryInterval;
} CC_Mqtt5DisconnectConfig;

typedef struct
{
    const char* m_topic;
    CC_Mqtt5QoS m_maxQos;
    CC_Mqtt5RetainHandling m_retainHandling;
    bool m_noLocal; 
    bool m_retainAsPublished;
} CC_Mqtt5SubscribeTopicConfig;

typedef struct
{
    unsigned m_subId;
} CC_Mqtt5SubscribeExtraConfig;


typedef struct 
{
    const CC_Mqtt5ReasonCode* m_reasonCodes;
    unsigned m_reasonCodesCount;
    const char* m_reasonStr;
    const CC_Mqtt5UserProp* m_userProps;
    unsigned m_userPropsCount;
} CC_Mqtt5SubscribeResponse;

typedef struct
{
    const char* m_topic;
} CC_Mqtt5UnsubscribeTopicConfig;

typedef struct 
{
    const CC_Mqtt5ReasonCode* m_reasonCodes;
    unsigned m_reasonCodesCount;
    const char* m_reasonStr;
    const CC_Mqtt5UserProp* m_userProps;
    unsigned m_userPropsCount;
} CC_Mqtt5UnsubscribeResponse;

/// @brief Received message information
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

typedef struct
{
    const char* m_topic;
    const unsigned char* m_data;
    unsigned m_dataLen;    
    CC_Mqtt5QoS m_qos;
    CC_Mqtt5TopicAliasPreference m_topicAliasPref;
    bool m_retain;
} CC_Mqtt5PublishBasicConfig;

typedef struct
{
    const char* m_contentType;
    const char* m_responseTopic;
    const unsigned char* m_correlationData;
    unsigned m_correlationDataLen;
    unsigned m_messageExpiryInterval;
    CC_Mqtt5PayloadFormat m_format;
} CC_Mqtt5PublishExtraConfig;

typedef struct 
{
    CC_Mqtt5ReasonCode m_reasonCode;
    const char* m_reasonStr;
    const CC_Mqtt5UserProp* m_userProps;
    unsigned m_userPropsCount;
} CC_Mqtt5PublishResponse;

/// @brief Callback used to request time measurement.
/// @details The callback is set using
///     cc_mqtt5_client_set_next_tick_program_callback() function.
/// @param[in] data Pointer to user data object, passed as last parameter to
///     cc_mqtt5_client_set_next_tick_program_callback() function.
/// @param[in] duration Time duration in @b milliseconds. After the requested
///     time expires, the cc_mqtt5_client_tick() function is expected to be invoked.
typedef void (*CC_Mqtt5NextTickProgramCb)(void* data, unsigned duration);

/// @brief Callback used to request termination of existing time measurement.
/// @details The callback is set using
///     cc_mqtt5_client_set_cancel_next_tick_wait_callback() function.
/// @param[in] data Pointer to user data object, passed as last parameter to
///     cc_mqtt5_client_set_cancel_next_tick_wait_callback() function.
/// @return Number of elapsed milliseconds since last time measurement request.
typedef unsigned (*CC_Mqtt5CancelNextTickWaitCb)(void* data);

/// @brief Callback used to request to send data to the broker.
/// @details The callback is set using
///     cc_mqtt5_client_set_send_output_data_callback() function. The reported
///     data resides in internal data structures of the client library, and
///     it can be updated right after the callback function returns. It means
///     the data may need to be copied into some other buffer which will be
///     held intact until the send over I/O link operation is complete.
/// @param[in] data Pointer to user data object, passed as last parameter to
///     cc_mqtt5_client_set_send_output_data_callback() function.
/// @param[in] buf Pointer to the buffer containing data to send
/// @param[in] bufLen Number of bytes in the buffer
/// @post The buffer data can be deallocated / overwritten after the callback function returns.
typedef void (*CC_Mqtt5SendOutputDataCb)(void* data, const unsigned char* buf, unsigned bufLen);

/// @brief Callback used to report unsolicited disconnection of the broker.
/// @param[in] data Pointer to user data object, passed as the last parameter to
///     the request call.
/// @param[in] info Extra disconnect information when reported by the broker. Can be NULL. 
/// @post The data members of the reported info can NOT be accessed after the function returns.
typedef void (*CC_Mqtt5BrokerDisconnectReportCb)(void* data, const CC_Mqtt5DisconnectInfo* info);

/// @brief Callback used to report new message received of the broker.
/// @param[in] data Pointer to user data object, passed as the last parameter to
///     the request call.
/// @param[in] info Message information. Will NOT be NULL.
/// @post The data members of the reported info can NOT be accessed after the function returns.
typedef void (*CC_Mqtt5MessageReceivedReportCb)(void* data, const CC_Mqtt5MessageInfo* info);

/// @brief Callback used to report discovered errors.
/// @param[in] data Pointer to user data object, passed as the last parameter to
///     the request call.
/// @param[in] msg Error log message.
typedef void (*CC_Mqtt5ErrorLogCb)(void* data, const char* msg);

typedef void (*CC_Mqtt5ConnectCompleteCb)(void* data, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response);

/// @brief Callback used to report incoming authentication data.
/// @param[in] data Pointer to user data object passed during callback registration.
/// @param[in] authInfoIn Pointer to authentication data received from broker.
/// @param[out] authInfoOut Pointer to authentication data to be sent to the broker. Expected to be filled by the callback.
typedef CC_Mqtt5AuthErrorCode (*CC_Mqtt5AuthCb)(void* data, const CC_Mqtt5AuthInfo* authInfoIn, CC_Mqtt5AuthInfo* authInfoOut);

typedef void (*CC_Mqtt5SubscribeCompleteCb)(void* data, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response);

typedef void (*CC_Mqtt5UnsubscribeCompleteCb)(void* data, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5UnsubscribeResponse* response);

typedef void (*CC_Mqtt5PublishCompleteCb)(void* data, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response);

typedef void (*CC_Mqtt5ReauthCompleteCb)(void* data, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5AuthInfo* response);


typedef struct
{
    const char* m_authMethod;
    const unsigned char* m_authData;
    unsigned m_authDataLen;
    CC_Mqtt5AuthCb m_authCb;
    void* m_authCbData;
} CC_Mqtt5AuthConfig;

#ifdef __cplusplus
}
#endif
