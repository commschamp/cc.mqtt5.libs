//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "op/Op.h"
#include "ExtConfig.h"
#include "PropsHandler.h"
#include "ProtocolDefs.h"

#include "TimerMgr.h"

namespace cc_mqtt5_client
{

namespace op
{

class ConnectOp final : public Op
{
    using Base = Op;
public:
    explicit ConnectOp(Client& client);

    CC_Mqtt5ErrorCode configBasic(const CC_Mqtt5ConnectBasicConfig& config);
    CC_Mqtt5ErrorCode configWill(const CC_Mqtt5ConnectWillConfig& config);
    CC_Mqtt5ErrorCode configExtra(const CC_Mqtt5ConnectExtraConfig& config);
    CC_Mqtt5ErrorCode configAuth(const CC_Mqtt5ConnectAuthConfig& config);
    CC_Mqtt5ErrorCode addUserProp(const CC_Mqtt5UserProp& prop);
    CC_Mqtt5ErrorCode send(CC_Mqtt5ConnectCompleteCb cb, void* cbData);

    using Base::handle;
    virtual void handle(ConnackMsg& msg) override;
    virtual void handle(AuthMsg& msg) override;

protected:
    virtual Type typeImpl() const override;    
    virtual void terminateOpImpl(CC_Mqtt5AsyncOpStatus status) override;

private:
    using AuthMethodStorageType = ConnectMsg::Field_properties::ValueType::value_type::Field_authMethod::Field_value::ValueType;

    void completeOpInternal(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response = nullptr);
    void opTimeoutInternal();
    void restartTimer();
    bool canAddProp() const;
    ConnectMsg::Field_properties::ValueType::reference addConnectMsgProp();


    static void opTimeoutCb(void* data);

    ConnectMsg m_connectMsg;  
    TimerMgr::Timer m_timer;  
    CC_Mqtt5ConnectCompleteCb m_cb = nullptr;
    void* m_cbData = nullptr;
    CC_Mqtt5AuthCb m_authCb = nullptr;
    void* m_authCbData = nullptr;
    AuthMethodStorageType m_authMethod;
    unsigned m_sessionExpiryInterval = 0U;

    static_assert(ExtConfig::ConnectOpTimers == 1U);
};

} // namespace op


} // namespace cc_mqtt5_client
