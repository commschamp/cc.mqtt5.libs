//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "op/Op.h"
#include "ExtConfig.h"
#include "ProtocolDefs.h"
#include "TopicAliasDefs.h"

#include "TimerMgr.h"

namespace cc_mqtt5_client
{

namespace op
{

class SendOp final : public Op
{
    using Base = Op;
public:
    explicit SendOp(ClientImpl& client);

    using Base::handle;

    unsigned packetId() const
    {
        return m_pubMsg.field_packetId().field().value();
    }

    CC_Mqtt5ErrorCode configBasic(const CC_Mqtt5PublishBasicConfig& config);

protected:
    virtual Type typeImpl() const override;    

private:
    void restartResponseTimer();
    void responseTimeoutInternal();
    static void recvTimeoutCb(void* data);

    TimerMgr::Timer m_responseTimer;  
    PublishMsg m_pubMsg;

    static_assert(ExtConfig::SendOpTimers == 1U);
};

} // namespace op


} // namespace cc_mqtt5_client
