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

class SubscribeOp final : public Op
{
    using Base = Op;
public:
    explicit SubscribeOp(ClientImpl& client);
    ~SubscribeOp();

    CC_Mqtt5ErrorCode configTopic(const CC_Mqtt5SubscribeTopicConfig& config);
    CC_Mqtt5ErrorCode configExtra(const CC_Mqtt5SubscribeExtraConfig& config);
    CC_Mqtt5ErrorCode addUserProp(const CC_Mqtt5UserProp& prop);
    CC_Mqtt5ErrorCode send(CC_Mqtt5SubscribeCompleteCb cb, void* cbData);
    CC_Mqtt5ErrorCode cancel();

    CC_Mqtt5SubscribeHandle toHandle()
    {
        return reinterpret_cast<CC_Mqtt5SubscribeHandle>(this);
    }

    static CC_Mqtt5SubscribeHandle asHandle(SubscribeOp* obj)
    {
        return reinterpret_cast<CC_Mqtt5SubscribeHandle>(obj);
    }    

    static SubscribeOp* fromHandle(CC_Mqtt5SubscribeHandle handle)
    {
        return reinterpret_cast<SubscribeOp*>(handle);
    }

    using Base::handle;
    virtual void handle(SubackMsg& msg) override;

protected:
    virtual Type typeImpl() const override;    
    virtual void terminateOpImpl(CC_Mqtt5AsyncOpStatus status) override;
    virtual void networkConnectivityChangedImpl() override;

private:
    void completeOpInternal(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response = nullptr);
    void opTimeoutInternal();
    void restartTimer();

    static void opTimeoutCb(void* data);

    SubscribeMsg m_subMsg;
    TimerMgr::Timer m_timer;
    CC_Mqtt5SubscribeCompleteCb m_cb = nullptr;
    void* m_cbData = nullptr;

    static_assert(ExtConfig::SubscribeOpTimers == 1U);
};

} // namespace op


} // namespace cc_mqtt5_client
