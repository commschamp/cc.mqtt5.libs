//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "op/Op.h"
#include "ProtocolDefs.h"
#include "TimerMgr.h"

namespace cc_mqtt5_client
{

namespace op
{

class UnsubscribeOp final : public Op
{
    using Base = Op;
public:
    explicit UnsubscribeOp(ClientImpl& client);
    ~UnsubscribeOp();

    CC_Mqtt5ErrorCode configTopic(const CC_Mqtt5UnsubscribeTopicConfig& config);
    CC_Mqtt5ErrorCode addUserProp(const CC_Mqtt5UserProp& prop);
    CC_Mqtt5ErrorCode send(CC_Mqtt5UnsubscribeCompleteCb cb, void* cbData);

    using Base::handle;
    virtual void handle(UnsubackMsg& msg) override;

protected:
    virtual Type typeImpl() const override;    
    virtual void terminateOpImpl(CC_Mqtt5AsyncOpStatus status) override;

private:
    void completeOpInternal(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5UnsubscribeResponse* response = nullptr);
    void opTimeoutInternal();
    void restartTimer();

    static void opTimeoutCb(void* data);

    UnsubscribeMsg m_unsubMsg;
    TimerMgr::Timer m_timer;
    CC_Mqtt5UnsubscribeCompleteCb m_cb = nullptr;
    void* m_cbData = nullptr;

    static_assert(ExtConfig::UnsubscribeOpTimers == 1U);
};

} // namespace op


} // namespace cc_mqtt5_client
