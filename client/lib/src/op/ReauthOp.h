//
// Copyright 2023 - 2026 (C). Alex Robenko. All rights reserved.
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

class ReauthOp final : public Op
{
    using Base = Op;
public:
    explicit ReauthOp(ClientImpl& client);

    CC_Mqtt5ErrorCode configAuth(const CC_Mqtt5AuthConfig& config);
    CC_Mqtt5ErrorCode addUserProp(const CC_Mqtt5UserProp& prop);
    CC_Mqtt5ErrorCode send(CC_Mqtt5ReauthCompleteCb cb, void* cbData);
    CC_Mqtt5ErrorCode cancel();

    using Base::handle;
    virtual void handle(AuthMsg& msg) override;

protected:
    virtual Type typeImpl() const override;
    virtual void terminateOpImpl(CC_Mqtt5AsyncOpStatus status) override;

private:
    using AuthMethodStorageType = ConnectMsg::Field_properties::ValueType::value_type::Field_authMethod::Field_value::ValueType;

    void completeOpInternal(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5AuthInfo* response = nullptr);
    void opTimeoutInternal();
    void restartTimer();

    static void opTimeoutCb(void* data);

    AuthMsg m_authMsg;
    TimerMgr::Timer m_timer;
    CC_Mqtt5ReauthCompleteCb m_cb = nullptr;
    void* m_cbData = nullptr;
    CC_Mqtt5AuthCb m_authCb = nullptr;
    void* m_authCbData = nullptr;

    static_assert(ExtConfig::ReauthOpTimers == 1U);
};

} // namespace op

} // namespace cc_mqtt5_client
