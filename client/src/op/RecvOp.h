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

#include "TimerMgr.h"

namespace cc_mqtt5_client
{

namespace op
{

class RecvOp final : public Op
{
    using Base = Op;
public:
    explicit RecvOp(ClientImpl& client);

    using Base::handle;
    void handle(PublishMsg& msg) override;

protected:
    virtual Type typeImpl() const override;    

private:
    void restartRecvTimer();
    void recvTimoutInternal();
    void populateMsgInfo(const PublishMsg& msg, CC_Mqtt5MessageInfo& info);
    void reportMsgInfo(const CC_Mqtt5MessageInfo& info);

    static void recvTimeoutCb(void* data);

    TimerMgr::Timer m_recvTimer;  
    static_assert(ExtConfig::RecvOpTimers == 1U);
    PublishMsg m_pubMsg;
};

} // namespace op


} // namespace cc_mqtt5_client
