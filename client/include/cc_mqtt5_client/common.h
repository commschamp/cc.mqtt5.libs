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

/// @brief Handle used to access client specific data structures.
/// @details Returned by @b cc_mqtt5_client_new() function.
typedef struct { void* m_ptr; } CC_Mqtt5ClientHandle;

#ifdef __cplusplus
}
#endif
