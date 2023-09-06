//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/ConnectOp.h"
#include "Client.h"

#include "comms/util/assign.h"
#include "comms/units.h"

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

    if (MaxKeepAlive < config.m_keepAlive) {
        return CC_Mqtt5ErrorCode_BadParam;
    }
    
    //comms::units::setSeconds(m_connectMsg.field_keepAlive(), config.m_keepAlive);
    m_connectMsg.field_keepAlive().setValue(config.m_keepAlive);
    m_connectMsg.doRefresh();
    return CC_Mqtt5ErrorCode_Success;
}

CC_Mqtt5ErrorCode ConnectOp::configWill(const CC_Mqtt5ConnectWillConfig& config)
{
    if ((config.m_dataLen > 0U) && (config.m_data == nullptr)) {
        return CC_Mqtt5ErrorCode_BadParam;
    }

    if (config.m_topic == nullptr) {
        return CC_Mqtt5ErrorCode_BadParam;
    }

    m_connectMsg.field_willTopic().field().value() = config.m_topic;
    if (config.m_dataLen > 0U) {
        comms::util::assign(m_connectMsg.field_willMessage().field().value(), config.m_data, config.m_data + config.m_dataLen);
    }

    m_connectMsg.field_flags().field_willQos().setValue(config.m_qos);
    m_connectMsg.field_flags().field_high().setBitValue_willRetain(config.m_retain);
    m_connectMsg.field_flags().field_low().setBitValue_willFlag(true);

    auto addProp = 
        [this]() -> decltype(auto)
        {
            auto& vec = m_connectMsg.field_willProperties().field().value();
            vec.resize(vec.size() + 1U);
            return vec.back();
        };

    if (config.m_format != CC_Mqtt5PayloadFromat_Unspecified) {
        auto& propVar = addProp();
        auto& propBundle = propVar.initField_payloadFormatIndicator();
        propBundle.field_value().setValue(config.m_format);
    }

    if (config.m_delayInterval > 0U) {
        auto& propVar = addProp();
        auto& propBundle = propVar.initField_willDelayInterval();
        auto& valueField = propBundle.field_value();

        using ValueField = std::decay_t<decltype(valueField)>;
        static constexpr auto MaxValue = std::numeric_limits<ValueField::ValueType>::max();

        if (MaxValue < config.m_delayInterval) {
            return CC_Mqtt5ErrorCode_BadParam;
        }

        comms::units::setSeconds(valueField, config.m_delayInterval);
    }

    if (config.m_expiryInterval > 0U) {
        auto& propVar = addProp();
        auto& propBundle = propVar.initField_messageExpiryInterval();
        auto& valueField = propBundle.field_value();

        using ValueField = std::decay_t<decltype(valueField)>;
        static constexpr auto MaxValue = std::numeric_limits<ValueField::ValueType>::max();

        if (MaxValue < config.m_expiryInterval) {
            return CC_Mqtt5ErrorCode_BadParam;
        }

        comms::units::setSeconds(valueField, config.m_expiryInterval);
    }

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
