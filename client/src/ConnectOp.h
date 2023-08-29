//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "cc_mqtt5_client/common.h"

namespace cc_mqtt5_client
{

struct ConnectOp
{
    int m_dummy = 0;

    // TODO
    CC_Mqtt5ErrorCode apply() { return CC_Mqtt5ErrorCode_Success; }
};

} // namespace cc_mqtt5_client
