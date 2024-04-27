//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/DisconnectOp.h"
#include "ClientImpl.h"

#include "comms/units.h"

namespace cc_mqtt5_client
{

namespace op
{

DisconnectOp::DisconnectOp(ClientImpl& client) : 
    Base(client)
{
}    

CC_Mqtt5ErrorCode DisconnectOp::configBasic(const CC_Mqtt5DisconnectConfig& config)
{
    // Don't update existance of the optional fields right away, do it during send.
    comms::cast_assign(m_disconnectMsg.field_reasonCode().field().value()) = config.m_reasonCode;

    auto& propsField = m_disconnectMsg.field_properties().field();
    if (config.m_sessionExpiryInterval != nullptr) {
        if ((*config.m_sessionExpiryInterval > 0) && (client().sessionState().m_connectSessionExpiryInterval == 0U)) {
            errorLog("Non-zero expiry interval in DISCONNECT is not allowed when zero expiry interval used in CONNECT.");
            return CC_Mqtt5ErrorCode_BadParam;
        }

        if (!canAddProp(propsField)) {
            errorLog("Cannot add disconnect property, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_sessionExpiryInterval();
        auto& valueField = propBundle.field_value();        
        comms::units::setSeconds(valueField, *config.m_sessionExpiryInterval);                
    }

    if (config.m_reasonStr != nullptr) {
        if (!canAddProp(propsField)) {
            errorLog("Cannot add disconnect property, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_reasonStr();
        auto& valueField = propBundle.field_value();        
        valueField.value() = config.m_reasonStr;

        if (maxStringLen() < valueField.value().size()) {
            errorLog("Reason string value is too long");
            discardLastProp(propsField);
            return CC_Mqtt5ErrorCode_BadParam;
        }        
    }

    return CC_Mqtt5ErrorCode_Success;
}

CC_Mqtt5ErrorCode DisconnectOp::addUserProp(const CC_Mqtt5UserProp& prop)
{
    m_disconnectMsg.field_properties().setExists();
    auto& propsField = m_disconnectMsg.field_properties().field();
    return addUserPropToList(propsField, prop);
}

CC_Mqtt5ErrorCode DisconnectOp::send()
{
    client().allowNextPrepare();
    if ((m_disconnectMsg.field_reasonCode().field().value() != DisconnectReason::Success) || 
        (!m_disconnectMsg.field_properties().field().value().empty())) {
        m_disconnectMsg.field_reasonCode().setExists();
        m_disconnectMsg.field_properties().setExists();
    }

    auto guard = client().apiEnter();
    auto& clientObj = client();
    clientObj.sendMessage(m_disconnectMsg);
    opComplete(); // No members access after this point, the op will be deleted
    clientObj.brokerDisconnected();
    return CC_Mqtt5ErrorCode_Success;
}

CC_Mqtt5ErrorCode DisconnectOp::cancel()
{
    client().allowNextPrepare();
    opComplete();
    return CC_Mqtt5ErrorCode_Success;
}

Op::Type DisconnectOp::typeImpl() const
{
    return Type_Disconnect;
}


} // namespace op

} // namespace cc_mqtt5_client
