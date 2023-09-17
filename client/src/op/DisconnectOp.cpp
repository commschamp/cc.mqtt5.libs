//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/DisconnectOp.h"
#include "Client.h"

#include "comms/units.h"

namespace cc_mqtt5_client
{

namespace op
{

DisconnectOp::DisconnectOp(Client& client) : 
    Base(client)
{
}    

CC_Mqtt5ErrorCode DisconnectOp::configBasic(const CC_Mqtt5DisconnectConfig& config)
{
    // Don't update existance of the optional fields right away, do it during send.
    comms::cast_assign(m_disconnectMsg.field_reasonCode().field().value()) = config.m_reasonCode;

    if (config.m_expiryInterval != nullptr) {
        if (!canAddProp()) {
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addDisconnectMsgProp();
        auto& propBundle = propVar.initField_sessionExpiryInterval();
        auto& valueField = propBundle.field_value();        
        comms::units::setSeconds(valueField, *config.m_expiryInterval);                
    }

    if (config.m_reasonStr != nullptr) {
        if (!canAddProp()) {
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addDisconnectMsgProp();
        auto& propBundle = propVar.initField_reasonStr();
        auto& valueField = propBundle.field_value();        
        valueField.value() = config.m_reasonStr;
    }

    return CC_Mqtt5ErrorCode_Success;
}

CC_Mqtt5ErrorCode DisconnectOp::addUserProp(const CC_Mqtt5UserProp& prop)
{
    if constexpr (ExtConfig::HasUserProps) {
        if (prop.m_key == nullptr) {
            return CC_Mqtt5ErrorCode_BadParam;
        }

        if (!canAddProp()) {
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addDisconnectMsgProp();    
        auto& propBundle = propVar.initField_userProperty();
        auto& valueField = propBundle.field_value();
        valueField.field_first().value() = prop.m_key;

        if (prop.m_value != nullptr) {
            valueField.field_second().value() = prop.m_value;
        }

        return CC_Mqtt5ErrorCode_Success;
    }
    else {
        return CC_Mqtt5ErrorCode_NotSupported;
    }
}

CC_Mqtt5ErrorCode DisconnectOp::send()
{
    if ((m_disconnectMsg.field_reasonCode().field().value() != DisconnectMsg::Field_reasonCode::Field::ValueType::Success) || 
        (!m_disconnectMsg.field_propertiesList().field().value().empty())) {
        m_disconnectMsg.field_reasonCode().setExists();
        m_disconnectMsg.field_propertiesList().setExists();
    }

    auto& clientObj = client();
    clientObj.sendMessage(m_disconnectMsg);
    opComplete(); // No members access after this point, the op will be deleted
    clientObj.notifyDisconnected(false);
    return CC_Mqtt5ErrorCode_Success;
}

Op::Type DisconnectOp::typeImpl() const
{
    return Type_Disconnect;
}

bool DisconnectOp::canAddProp() const
{
    auto& vec = m_disconnectMsg.field_propertiesList().field().value();
    return vec.size() < vec.capacity();
}

DisconnectMsg::Field_propertiesList::Field::ValueType::reference DisconnectOp::addDisconnectMsgProp()
{
    auto& vec = m_disconnectMsg.field_propertiesList().field().value();
    vec.resize(vec.size() + 1U);
    return vec.back();
}

} // namespace op

} // namespace cc_mqtt5_client