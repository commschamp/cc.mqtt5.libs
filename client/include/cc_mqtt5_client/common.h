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

/// @brief Quality of Service
typedef enum
{
    CC_Mqtt5QoS_AtMostOnceDelivery, ///< QoS=0. At most once delivery.
    CC_Mqtt5QoS_AtLeastOnceDelivery, ///< QoS=1. At least once delivery.
    CC_Mqtt5QoS_ExactlyOnceDelivery ///< QoS=2. Exactly once delivery.
} CC_Mqtt5QoS;

/// @brief Error code returned by various API functions.
typedef enum
{
    CC_Mqtt5ErrorCode_Success, ///< The requested operation was successfully started.
    CC_Mqtt5ErrorCode_NotIntitialized, ///< The allocated client hasn't been initialized.
    CC_Mqtt5ErrorCode_Busy, ///< The client library is in the middle of previous operation(s), cannot start a new one.
    CC_Mqtt5ErrorCode_NotConnected, ///< The client library is not connected to the broker. Returned by operations that require connection to the broker.
    CC_Mqtt5ErrorCode_BadParam, ///< Bad parameter is passed to the function.
    CC_Mqtt5ErrorCode_OutOfMemory, ///< Memory allocation failed.
} CC_Mqtt5ErrorCode;

/// @brief Handle used to access client specific data structures.
/// @details Returned by @b cc_mqtt5_client_new() function.
typedef struct { void* m_ptr; } CC_Mqtt5ClientHandle;

/// @brief Handle for connection operation.
/// @details Returned by @b cc_mqtt5_client_connect_prepare() function.
typedef struct { void* m_ptr; } CC_Mqtt5ConnectHandle;


struct CC_Mqtt5ConnectBasicConfig
{
    const char* m_clientId;
    const char* m_username;
    const unsigned char* m_password;
    unsigned m_passwordLen;
    unsigned m_keepAlive;
    bool m_cleanStart;
};

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


#ifdef __cplusplus
}
#endif
