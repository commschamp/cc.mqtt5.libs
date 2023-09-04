//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "op/Op.h"

#include "cc_mqtt5_client/common.h"

namespace cc_mqtt5_client
{

namespace op
{

class ConnectOp final : public Op
{
    using Base = Op;
public:
    explicit ConnectOp(Client& client) : Base(client) {}


    // TODO
    CC_Mqtt5ErrorCode apply() { return CC_Mqtt5ErrorCode_Success; }

protected:
    virtual Type typeImpl() const override;    
};

} // namespace op


} // namespace cc_mqtt5_client
