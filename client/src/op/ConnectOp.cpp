//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/ConnectOp.h"
#include "Client.h"

#include "comms/util/assign.h"

#include <algorithm>
#include <limits>

namespace cc_mqtt5_client
{

namespace op
{

Op::Type ConnectOp::typeImpl() const
{
    return Type_Connect;
}

CC_Mqtt5ErrorCode ConnectOp::configBasic(const CC_Mqtt5ConnectBasicConfig& config)
{
    if ((config.m_passwordLen > 0U) && (config.m_password == nullptr)) {
        return CC_Mqtt5ErrorCode_BadParam;
    }

    if ((config.m_passwordLen > 0U) && (config.m_username == nullptr)) {
        return CC_Mqtt5ErrorCode_BadParam;
    }

    if (config.m_clientId != nullptr) {
        m_connectMsg.field_clientId().value() = config.m_clientId;    
    }
    else {
        m_connectMsg.field_clientId().value().clear();
    }

    bool hasUsername = (config.m_username != nullptr);
    m_connectMsg.field_flags().field_high().setBitValue_userNameFlag(hasUsername);
    if (hasUsername) {
        m_connectMsg.field_userName().field().value() = config.m_username;
    }

    bool hasPassword = (config.m_passwordLen > 0U);
    m_connectMsg.field_flags().field_high().setBitValue_passwordFlag(hasPassword);
    if (hasPassword) {
        comms::util::assign(m_connectMsg.field_password().field().value(), config.m_password, config.m_password + config.m_passwordLen);
    }

    m_connectMsg.field_flags().field_low().setBitValue_cleanStart(config.m_cleanStart);    

    static constexpr auto MaxKeepAlive = 
        std::numeric_limits<ConnectMsg::Field_keepAlive::ValueType>::max();
    m_connectMsg.field_keepAlive().setValue(std::min(config.m_keepAlive, static_cast<decltype(config.m_keepAlive)>(MaxKeepAlive)));
    m_connectMsg.doRefresh();
    return CC_Mqtt5ErrorCode_Success;
}

CC_Mqtt5ErrorCode ConnectOp::send() 
{
    // TODO: callback
    return client().sendMessage(m_connectMsg); 
}

} // namespace op

} // namespace cc_mqtt5_client
