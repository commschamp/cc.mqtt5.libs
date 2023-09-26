//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "op/Op.h"
#include "ProtocolDefs.h"

namespace cc_mqtt5_client
{

namespace op
{

class DisconnectOp final : public Op
{
    using Base = Op;
public:
    explicit DisconnectOp(Client& client);

    CC_Mqtt5ErrorCode configBasic(const CC_Mqtt5DisconnectConfig& config);
    CC_Mqtt5ErrorCode addUserProp(const CC_Mqtt5UserProp& prop);
    CC_Mqtt5ErrorCode send();

protected:
    virtual Type typeImpl() const override;    

private:

    DisconnectMsg m_disconnectMsg;
};

} // namespace op


} // namespace cc_mqtt5_client
