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

#define CC_MQTT5_SESSION_NEVER_EXPIRES 0xffffffff
#define CC_MQTT5_MAX_TOPIC_ALIASES_LIMIT 0xffff

/// @brief Quality of Service
typedef enum
{
    CC_Mqtt5QoS_AtMostOnceDelivery, ///< QoS=0. At most once delivery.
    CC_Mqtt5QoS_AtLeastOnceDelivery, ///< QoS=1. At least once delivery.
    CC_Mqtt5QoS_ExactlyOnceDelivery, ///< QoS=2. Exactly once delivery.
    CC_Mqtt5QoS_ValuesLimit ///< Limit for the values
} CC_Mqtt5QoS;

/// @brief Error code returned by various API functions.
typedef enum
{
    CC_Mqtt5ErrorCode_Success, ///< The requested operation was successfully started.
    CC_Mqtt5ErrorCode_InternalError, ///< Internal library error, please submit bug report    
    CC_Mqtt5ErrorCode_NotIntitialized, ///< The allocated client hasn't been initialized.
    CC_Mqtt5ErrorCode_Busy, ///< The client library is in the middle of previous operation(s), cannot start a new one.
    CC_Mqtt5ErrorCode_NotConnected, ///< The client library is not connected to the broker. Returned by operations that require connection to the broker.
    CC_Mqtt5ErrorCode_BadParam, ///< Bad parameter is passed to the function.
    CC_Mqtt5ErrorCode_OutOfMemory, ///< Memory allocation failed.
    CC_Mqtt5ErrorCode_BufferOverflow, ///< Output buffer is too short
    CC_Mqtt5ErrorCode_NotSupported, ///< Feature is not supported
    CC_Mqtt5ErrorCode_ValuesLimit ///< Limit for the values
} CC_Mqtt5ErrorCode;

/// @brief Payload format indicator
typedef enum
{
    CC_Mqtt5PayloadFormat_Unspecified = 0, ///< Unspecified format
    CC_Mqtt5PayloadFormat_Utf8 = 1, ///< UTF-8 string
    CC_Mqtt5PayloadFormat_ValuesLimit ///< Limit of the values
} CC_Mqtt5PayloadFormat;

typedef enum
{
    CC_Mqtt5AsyncOpStatus_Complete,
    CC_Mqtt5AsyncOpStatus_InternalError,
    CC_Mqtt5AsyncOpStatus_Timeout,
    CC_Mqtt5AsyncOpStatus_ProtocolError,
    CC_Mqtt5AsyncOpStatus_Aborted,
    CC_Mqtt5AsyncOpStatus_OutOfMemory,
    CC_Mqtt5AsyncOpStatus_BadParam,
    CC_Mqtt5AsyncOpStatus_ValuesLimit
} CC_Mqtt5AsyncOpStatus;


typedef enum
{
    CC_Mqtt5AuthErrorCode_Continue,
    CC_Mqtt5AuthErrorCode_Disconnect,
    CC_Mqtt5AuthErrorCode_ValuesLimit
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

/// @brief Handle used to access client specific data structures.
/// @details Returned by cc_mqtt5_client_new() function.
typedef struct { void* m_ptr; } CC_Mqtt5ClientHandle;

/// @brief Handle for connection operation.
/// @details Returned by cc_mqtt5_client_connect_prepare() function.
typedef struct { void* m_ptr; } CC_Mqtt5ConnectHandle;

typedef struct
{
    const char* m_key;
    const char* m_value;
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
    unsigned m_expiryInterval;
    CC_Mqtt5QoS m_qos;
    CC_Mqtt5PayloadFormat m_format;
    bool m_retain;
} CC_Mqtt5ConnectWillConfig;

typedef struct
{
    unsigned m_expiryInterval;
    unsigned m_receiveMaximum;
    unsigned m_maxPacketSize;
    unsigned m_topicAliasMaximum;
    bool m_requestResposnseInfo;
    bool m_requestProblemInfo;
} CC_Mqtt5ConnectExtraConfig;

typedef struct 
{
    CC_Mqtt5ReasonCode m_reasonCode;
    const char* m_assignedClientId;
    const char* m_responseInfo;
    const char* m_reasonStr;
    const char* m_serverRef;
    const char* m_authMethod;
    const unsigned char* m_authData;
    unsigned m_authDataLen;
    const CC_Mqtt5UserProp* m_userProps;
    unsigned m_userPropsCount;
    unsigned m_expiryInterval;
    unsigned m_highQosPubLimit;
    unsigned m_maxPacketSize;
    unsigned m_topicAliasMax;
    CC_Mqtt5QoS m_maxQos;
    bool m_sessionPresent;
    bool m_retainAvailable;
    bool m_wildcardSubAvailable;
    bool m_subIdsAvailalbe;
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
/// @param[in] bufLen Number of bytes to send
typedef void (*CC_Mqtt5SendOutputDataCb)(void* data, const unsigned char* buf, unsigned bufLen);

/// @brief Callback used to report unsolicited disconnection of the broker.
/// @param[in] data Pointer to user data object, passed as the last parameter to
///     the request call.
typedef void (*CC_Mqtt5BrokerDisconnectReportCb)(void* data);

typedef void (*CC_Mqtt5ConnectCompleteCb)(void* data, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response);

typedef CC_Mqtt5AuthErrorCode (*CC_Mqtt5AuthCb)(void* data, const CC_Mqtt5AuthInfo* authInfoIn, CC_Mqtt5AuthInfo* authInfoOut);

typedef struct
{
    const char* m_authMethod;
    const unsigned char* m_authData;
    unsigned m_authDataLen;
    CC_Mqtt5AuthCb m_authCb;
    void* m_authCbData;
} CC_Mqtt5ConnectAuthConfig;

#ifdef __cplusplus
}
#endif
