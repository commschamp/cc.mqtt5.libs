//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/ConnectOp.h"
#include "Client.h"

#include "comms/util/assign.h"
#include "comms/util/ScopeGuard.h"
#include "comms/units.h"

#include <algorithm>
#include <limits>

namespace cc_mqtt5_client
{

namespace op
{

namespace 
{

inline ConnectOp* asConnectOp(void* data)
{
    return reinterpret_cast<ConnectOp*>(data);
}

template <typename TField>
bool canAddPropToField(const TField& field)
{
    auto& vec = field.value();
    return vec.size() < vec.capacity();
}

} // namespace 
    

ConnectOp::ConnectOp(Client& client) : 
    Base(client),
    m_timer(client.timerMgr().allocTimer())
{
}    

void ConnectOp::handle(AuthMsg& msg)
{
    m_timer.cancel();
    auto restartTimerOnExit = 
        comms::util::makeScopeGuard(
            [this]()
            {
                restartTimer();
            });

    if (msg.field_reasonCode().value() != AuthMsg::Field_reasonCode::ValueType::ContinueAuth) {
        sendDisconnectWithReason(DisconnectMsg::Field_reasonCode::Field::ValueType::ProtocolError);
        restartTimerOnExit.release();
        completeOpInternal(CC_Mqtt5AsyncOpStatus_ProtocolError);
        return;
    }

    static_cast<void>(msg);
    // TODO: dispatch props

    CC_Mqtt5AuthInfo inInfo;

    // TODO: populate inInfo
    CC_Mqtt5AuthInfo outInfo;
    auto authEc = m_authCb(m_authCbData, &inInfo, &outInfo);
    if (authEc != CC_Mqtt5AuthErrorCode_Continue) {
        COMMS_ASSERT(authEc == CC_Mqtt5AuthErrorCode_Disconnect);
        sendDisconnectWithReason(DisconnectMsg::Field_reasonCode::Field::ValueType::UnspecifiedError);
        // TODO: report network disconnect request
        restartTimerOnExit.release();
        completeOpInternal(CC_Mqtt5AsyncOpStatus_Aborted);
        return;
    }

    auto termStatus = CC_Mqtt5AsyncOpStatus_OutOfMemory;
    auto termConnectOnExit = 
        comms::util::makeScopeGuard(
            [this, &termStatus]()
            {
                sendDisconnectWithReason(DisconnectMsg::Field_reasonCode::Field::ValueType::UnspecifiedError);
                completeOpInternal(termStatus);
            });

    AuthMsg respMsg;
    respMsg.field_reasonCode().setValue(AuthMsg::Field_reasonCode::ValueType::ContinueAuth);

    auto canAddAuthProps = 
        [&respMsg]()
        {
            return canAddPropToField(respMsg.field_propertiesList());
        };

    auto addAuthProp = 
        [&respMsg]() -> decltype(auto)
        {
            auto& vec = respMsg.field_propertiesList().value();
            vec.resize(vec.size() + 1U);
            return vec.back();
        };        

    if (outInfo.m_authMethod != nullptr) {
        if (!canAddAuthProps()) {
            return;
        }

        auto& propVar = addAuthProp();
        auto& propBundle = propVar.initField_authMethod();
        auto& valueField = propBundle.field_value();        
        valueField.value() = outInfo.m_authMethod;
    }

    if (outInfo.m_authDataLen > 0U) {
        if (outInfo.m_authData == nullptr) {
            termStatus = CC_Mqtt5AsyncOpStatus_BadParam;
            return;
        }

        if (!canAddAuthProps()) {
            return;
        }

        auto& propVar = addAuthProp();
        auto& propBundle = propVar.initField_authData();
        auto& valueField = propBundle.field_value();        
        comms::util::assign(valueField.value(), outInfo.m_authData, outInfo.m_authData + outInfo.m_authDataLen); 
    }    

    if (outInfo.m_reasonStr != nullptr) {
        if (!canAddAuthProps()) {
            return;
        }

        auto& propVar = addAuthProp();
        auto& propBundle = propVar.initField_reasonStr();
        auto& valueField = propBundle.field_value();  
        valueField.value() = outInfo.m_reasonStr;           
    }

    if (outInfo.m_userPropsCount > 0U) {
        if (outInfo.m_userProps == nullptr) {
            termStatus = CC_Mqtt5AsyncOpStatus_BadParam;
            return;
        }

        for (auto idx = 0U; idx < outInfo.m_userPropsCount; ++idx) {
            auto& prop = outInfo.m_userProps[idx];

            if (prop.m_key == nullptr) {
                termStatus = CC_Mqtt5AsyncOpStatus_BadParam;
                return;
            }

            if (!canAddAuthProps()) {
                return;
            }

            auto& propVar = addAuthProp();
            auto& propBundle = propVar.initField_userProperty();            
            auto& valueField = propBundle.field_value();
            valueField.field_first().value() = prop.m_key;

            if (prop.m_value != nullptr) {
                valueField.field_second().value() = prop.m_value;
            }
        }
    }

    termConnectOnExit.release();
    client().sendMessage(respMsg);
}

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

    auto canAddWillProp = 
        [this]()
        {
            auto& vec = m_connectMsg.field_willProperties().field().value();
            return vec.size() < vec.capacity();
        };

    auto addWillProp = 
        [this]() -> decltype(auto)
        {
            auto& vec = m_connectMsg.field_willProperties().field().value();
            vec.resize(vec.size() + 1U);
            return vec.back();
        };

    if (config.m_format != CC_Mqtt5PayloadFormat_Unspecified) {
        if (!canAddWillProp()) {
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addWillProp();
        auto& propBundle = propVar.initField_payloadFormatIndicator();
        propBundle.field_value().setValue(config.m_format);
    }

    if (config.m_delayInterval > 0U) {
        if (!canAddWillProp()) {
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addWillProp();
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
        if (!canAddWillProp()) {
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addWillProp();
        auto& propBundle = propVar.initField_messageExpiryInterval();
        auto& valueField = propBundle.field_value();

        using ValueField = std::decay_t<decltype(valueField)>;
        static constexpr auto MaxValue = std::numeric_limits<ValueField::ValueType>::max();

        if (MaxValue < config.m_expiryInterval) {
            return CC_Mqtt5ErrorCode_BadParam;
        }

        comms::units::setSeconds(valueField, config.m_expiryInterval);
    }

    if (config.m_contentType != nullptr) {
        if (!canAddWillProp()) {
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addWillProp();
        auto& propBundle = propVar.initField_contentType();
        auto& valueField = propBundle.field_value();
        valueField.value() = config.m_contentType;
    }

    if (config.m_responseTopic != nullptr) {
        if (!canAddWillProp()) {
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }        
        auto& propVar = addWillProp();
        auto& propBundle = propVar.initField_responseTopic();
        auto& valueField = propBundle.field_value();
        valueField.value() = config.m_responseTopic;        
    }

    if ((config.m_correlationDataLen > 0U) && (config.m_correlationData == nullptr)) {
        return CC_Mqtt5ErrorCode_BadParam;
    }

    if (config.m_correlationData != nullptr) {
        if (!canAddWillProp()) {
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addWillProp();
        auto& propBundle = propVar.initField_correlationData();
        auto& valueField = propBundle.field_value();        

        comms::util::assign(valueField.value(), config.m_correlationData, config.m_correlationData + config.m_correlationDataLen);
    }

    m_connectMsg.doRefresh();
    return CC_Mqtt5ErrorCode_Success;
}

CC_Mqtt5ErrorCode ConnectOp::configExtra(const CC_Mqtt5ConnectExtraConfig& config)
{
    if (config.m_expiryInterval > 0U) {
        if (!canAddProp()) {
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addConnectMsgProp();
        auto& propBundle = propVar.initField_messageExpiryInterval();
        auto& valueField = propBundle.field_value();        
        comms::units::setSeconds(valueField, config.m_expiryInterval);                
    }

    if (config.m_receiveMaximum > 0U) {
        if (!canAddProp()) {
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addConnectMsgProp();
        auto& propBundle = propVar.initField_receiveMax();
        auto& valueField = propBundle.field_value();          
        valueField.setValue(config.m_receiveMaximum);
    }

    if (config.m_maxPacketSize > 0U) {
        if (!canAddProp()) {
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addConnectMsgProp();
        auto& propBundle = propVar.initField_maxPacketSize();
        auto& valueField = propBundle.field_value();          
        valueField.setValue(config.m_maxPacketSize);
    }

    if (config.m_topicAliasMaximum > 0U) {
        if constexpr (!ExtConfig::HasTopicAliases) {
            return CC_Mqtt5ErrorCode_BadParam;
        }

        if (config.m_topicAliasMaximum > CC_MQTT5_MAX_TOPIC_ALIASES_LIMIT) {
            return CC_Mqtt5ErrorCode_BadParam;
        }

        if (!canAddProp()) {
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        using ValueType = ConnectMsg::Field_properties::ValueType::value_type::Field_topicAliasMax::Field_value::ValueType;
        static constexpr auto MaxSupportedVal = std::numeric_limits<ValueType>::max();
        static_assert(MaxSupportedVal <= CC_MQTT5_MAX_TOPIC_ALIASES_LIMIT);

        auto& propVar = addConnectMsgProp();
        auto& propBundle = propVar.initField_topicAliasMax();
        auto& valueField = propBundle.field_value();          
        valueField.setValue(config.m_topicAliasMaximum);        
    }

    if (config.m_requestResposnseInfo) {
        if (!canAddProp()) {
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addConnectMsgProp();
        auto& propBundle = propVar.initField_requestResponseInfo();
        auto& valueField = propBundle.field_value();          
        valueField.setValue(config.m_requestResposnseInfo);   
    }

    if (config.m_requestProblemInfo) {
        if (!canAddProp()) {
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addConnectMsgProp();
        auto& propBundle = propVar.initField_requestProblemInfo();
        auto& valueField = propBundle.field_value();          
        valueField.setValue(config.m_requestProblemInfo);   
    }    

    return CC_Mqtt5ErrorCode_Success;
}

CC_Mqtt5ErrorCode ConnectOp::configAuth(const CC_Mqtt5ConnectAuthConfig& config)
{
    if ((config.m_authCb == nullptr) ||
        (config.m_authMethod == nullptr)) {
        return CC_Mqtt5ErrorCode_BadParam;
    }

    m_authCb = config.m_authCb;
    m_authCbData = config.m_authCbData;

    if (config.m_authMethod != nullptr) {
        if (!canAddProp()) {
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addConnectMsgProp();
        auto& propBundle = propVar.initField_authMethod();
        auto& valueField = propBundle.field_value();        
        valueField.value() = config.m_authMethod;
    }

    if (config.m_authDataLen > 0U) {
        if (config.m_authData == nullptr) {
            return CC_Mqtt5ErrorCode_BadParam;
        }

        if (!canAddProp()) {
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addConnectMsgProp();
        auto& propBundle = propVar.initField_authData();
        auto& valueField = propBundle.field_value();        
        comms::util::assign(valueField.value(), config.m_authData, config.m_authData + config.m_authDataLen); 
    }

    return CC_Mqtt5ErrorCode_Success;
}

CC_Mqtt5ErrorCode ConnectOp::addUserProp(const CC_Mqtt5UserProp& prop)
{
    if constexpr (ExtConfig::HasUserProps) {
        if (prop.m_key == nullptr) {
            return CC_Mqtt5ErrorCode_BadParam;
        }

        if (!canAddProp()) {
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addConnectMsgProp();    
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

CC_Mqtt5ErrorCode ConnectOp::send(CC_Mqtt5ConnectCompleteCb cb, void* cbData) 
{
    auto completeOnError = 
        comms::util::makeScopeGuard(
            [this]()
            {
                opComplete();
            });

    if (cb == nullptr) {
        return CC_Mqtt5ErrorCode_BadParam;
    }

    m_cb = cb;
    m_cbData = cbData;
    
    auto result = client().sendMessage(m_connectMsg); 
    if (result != CC_Mqtt5ErrorCode_Success) {
        return result;
    }

    if (!m_timer.isValid()) {
        return CC_Mqtt5ErrorCode_OutOfMemory;
    }

    completeOnError.release(); // don't complete op yet
    restartTimer();
    return CC_Mqtt5ErrorCode_Success;
}

void ConnectOp::completeOpInternal(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response)
{
    COMMS_ASSERT(m_cb != nullptr);
    auto cb = m_cb;
    auto* cbData = m_cbData;
    opComplete(); // mustn't access data members after destruction
    cb(cbData, status, response);    
}

void ConnectOp::opTimeoutInternal()
{
    completeOpInternal(CC_Mqtt5AsyncOpStatus_Timeout);
}

void ConnectOp::restartTimer()
{
    m_timer.wait(getOpTimeout(), &ConnectOp::opTimeoutCb, this);
}

bool ConnectOp::canAddProp() const
{
    return canAddPropToField(m_connectMsg.field_properties());
}

ConnectMsg::Field_properties::ValueType::reference ConnectOp::addConnectMsgProp()
{
    auto& vec = m_connectMsg.field_properties().value();
    vec.resize(vec.size() + 1U);
    return vec.back();    
}

void ConnectOp::opTimeoutCb(void* data)
{
    asConnectOp(data)->opTimeoutInternal();
}

} // namespace op

} // namespace cc_mqtt5_client
