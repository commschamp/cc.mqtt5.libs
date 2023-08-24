//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Client.h"

namespace cc_mqtt5_client
{

CC_Mqtt5ErrorCode Client::init()
{
    // TODO: more callbacks
    if (m_sendOutputDataFn == nullptr) {
        return CC_Mqtt5ErrorCode_BadParam;
    }

    // TODO: abort pending ops
    m_state = State();
    m_state.m_initialized = true;
    return CC_Mqtt5ErrorCode_Success;
}

} // namespace cc_mqtt5_client
