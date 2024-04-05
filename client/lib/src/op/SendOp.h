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
    ~SendOp();

    using Base::handle;
    virtual void handle(PubackMsg& msg) override;
    virtual void handle(PubrecMsg& msg) override;
    virtual void handle(PubcompMsg& msg) override;

    CC_Mqtt5PublishHandle toHandle()
    {
        return reinterpret_cast<CC_Mqtt5PublishHandle>(this);
    }

    static CC_Mqtt5PublishHandle asHandle(SendOp* obj)
    {
        return reinterpret_cast<CC_Mqtt5PublishHandle>(obj);
    }    

    static SendOp* fromHandle(CC_Mqtt5PublishHandle handle)
    {
        return reinterpret_cast<SendOp*>(handle);
    }    

    unsigned packetId() const
    {
        return m_pubMsg.field_packetId().field().value();
    }

    CC_Mqtt5ErrorCode configBasic(const CC_Mqtt5PublishBasicConfig& config);
    CC_Mqtt5ErrorCode configExtra(const CC_Mqtt5PublishExtraConfig& config);
    CC_Mqtt5ErrorCode addUserProp(const CC_Mqtt5UserProp& prop);
    CC_Mqtt5ErrorCode setResendAttempts(unsigned attempts);
    unsigned getResendAttempts() const;
    CC_Mqtt5ErrorCode send(CC_Mqtt5PublishCompleteCb cb, void* cbData);
    CC_Mqtt5ErrorCode cancel();
    void postReconnectionResend();
    bool resume();
    bool isPaused() const
    {
        return m_paused;
    }

    bool isSent() const
    {
        return m_sent;
    }

protected:
    virtual Type typeImpl() const override;    
    virtual void terminateOpImpl(CC_Mqtt5AsyncOpStatus status) override;
    virtual void networkConnectivityChangedImpl() override;

private:
    void restartResponseTimer();
    void responseTimeoutInternal();
    void resendDupMsg();
    void completeWithCb(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response = nullptr);
    void confirmRegisteredAlias();
    CC_Mqtt5ErrorCode doSendInternal();
    bool canSend() const;
    void opCompleteInternal();

    static void recvTimeoutCb(void* data);

    TimerMgr::Timer m_responseTimer;  
    PublishMsg m_pubMsg;
    CC_Mqtt5PublishCompleteCb m_cb = nullptr;
    void* m_cbData = nullptr;    
    unsigned m_totalSendAttempts = DefaultSendAttempts;
    unsigned m_sendAttempts = 0U;
    CC_Mqtt5ReasonCode m_reasonCode = CC_Mqtt5ReasonCode_Success;
    bool m_sent = false;
    bool m_acked = false;
    bool m_registeredAlias = false;
    bool m_topicConfigured = false;
    bool m_paused = false;

    static constexpr unsigned DefaultSendAttempts = 2U;
    static_assert(ExtConfig::SendOpTimers == 1U);
};

} // namespace op


} // namespace cc_mqtt5_client
