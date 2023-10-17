//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/ConnectOp.h"
#include "ClientImpl.h"

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

} // namespace 
    

ConnectOp::ConnectOp(ClientImpl& client) : 
    Base(client),
    m_timer(client.timerMgr().allocTimer())
{
}   


CC_Mqtt5ErrorCode ConnectOp::configBasic(const CC_Mqtt5ConnectBasicConfig& config)
{
    if ((config.m_passwordLen > 0U) && (config.m_password == nullptr)) {
        errorLog("Bad password parameter in connect configuration.");
        return CC_Mqtt5ErrorCode_BadParam;
    }

    if ((!config.m_cleanStart) && (client().sessionState().m_firstConnect)) {
        errorLog("Clean start flag needs to be set on the first connection attempt");
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
        errorLog("Keep alive value is too high in connect configuration.");
        return CC_Mqtt5ErrorCode_BadParam;
    }
    
    comms::units::setSeconds(m_connectMsg.field_keepAlive(), config.m_keepAlive);
    m_connectMsg.doRefresh();
    return CC_Mqtt5ErrorCode_Success;
}

CC_Mqtt5ErrorCode ConnectOp::configWill(const CC_Mqtt5ConnectWillConfig& config)
{
    if ((config.m_dataLen > 0U) && (config.m_data == nullptr)) {
        errorLog("Bad data parameter in will configuration.");
        return CC_Mqtt5ErrorCode_BadParam;
    }

    if ((config.m_topic == nullptr) || (config.m_topic[0] == '\0')) {
        errorLog("Will topic is not provided.");
        return CC_Mqtt5ErrorCode_BadParam;
    }

    if ((config.m_qos < CC_Mqtt5QoS_AtMostOnceDelivery) || (config.m_qos > CC_Mqtt5QoS_ExactlyOnceDelivery)) {
        errorLog("Invalid qill QoS value in configuration.");
        return CC_Mqtt5ErrorCode_BadParam;
    }    

    m_connectMsg.field_willTopic().field().value() = config.m_topic;
    if (config.m_dataLen > 0U) {
        comms::util::assign(m_connectMsg.field_willMessage().field().value(), config.m_data, config.m_data + config.m_dataLen);
    }

    m_connectMsg.field_flags().field_willQos().setValue(config.m_qos);
    m_connectMsg.field_flags().field_high().setBitValue_willRetain(config.m_retain);
    m_connectMsg.field_flags().field_low().setBitValue_willFlag(true);

    auto& propsField = m_connectMsg.field_willProperties().field();

    if (config.m_format != CC_Mqtt5PayloadFormat_Unspecified) {
        if (!canAddProp(propsField)) {
            errorLog("Cannot add will property, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_payloadFormatIndicator();
        propBundle.field_value().setValue(config.m_format);
    }

    if (config.m_delayInterval > 0U) {
        if (!canAddProp(propsField)) {
            errorLog("Cannot add will property, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_willDelayInterval();
        auto& valueField = propBundle.field_value();

        using ValueField = std::decay_t<decltype(valueField)>;
        static constexpr auto MaxValue = std::numeric_limits<ValueField::ValueType>::max();

        if (MaxValue < config.m_delayInterval) {
            errorLog("Will delay interval is too high.");
            return CC_Mqtt5ErrorCode_BadParam;
        }

        comms::units::setSeconds(valueField, config.m_delayInterval);
    }

    if (config.m_messageExpiryInterval > 0U) {
        if (!canAddProp(propsField)) {
            errorLog("Cannot add will property, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_messageExpiryInterval();
        auto& valueField = propBundle.field_value();

        using ValueField = std::decay_t<decltype(valueField)>;
        static constexpr auto MaxValue = std::numeric_limits<ValueField::ValueType>::max();

        if (MaxValue < config.m_messageExpiryInterval) {
            errorLog("Message expiry interval is too high");
            return CC_Mqtt5ErrorCode_BadParam;
        }

        comms::units::setSeconds(valueField, config.m_messageExpiryInterval);
    }

    if (config.m_contentType != nullptr) {
        if (!canAddProp(propsField)) {
            errorLog("Cannot add will property, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_contentType();
        auto& valueField = propBundle.field_value();
        valueField.value() = config.m_contentType;
    }

    if (config.m_responseTopic != nullptr) {
        if (!canAddProp(propsField)) {
            errorLog("Cannot add will property, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }        
        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_responseTopic();
        auto& valueField = propBundle.field_value();
        valueField.value() = config.m_responseTopic;        
    }

    if ((config.m_correlationDataLen > 0U) && (config.m_correlationData == nullptr)) {
        errorLog("Bad correlation data parameter in will configuration.");
        return CC_Mqtt5ErrorCode_BadParam;
    }

    if (config.m_correlationData != nullptr) {
        if (!canAddProp(propsField)) {
            errorLog("Cannot add will property, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_correlationData();
        auto& valueField = propBundle.field_value();        

        comms::util::assign(valueField.value(), config.m_correlationData, config.m_correlationData + config.m_correlationDataLen);
    }

    m_connectMsg.doRefresh();
    return CC_Mqtt5ErrorCode_Success;
}

CC_Mqtt5ErrorCode ConnectOp::configExtra(const CC_Mqtt5ConnectExtraConfig& config)
{
    auto& propsField = m_connectMsg.field_properties();
    if (config.m_sessionExpiryInterval > 0U) {
        if (!canAddProp(propsField)) {
            errorLog("Cannot add connect property, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_sessionExpiryInterval();
        auto& valueField = propBundle.field_value();        
        comms::units::setSeconds(valueField, config.m_sessionExpiryInterval);                

        m_sessionExpiryInterval = config.m_sessionExpiryInterval;
    }

    if (config.m_receiveMaximum > 0U) {
        if (!canAddProp(propsField)) {
            errorLog("Cannot add connect property, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_receiveMax();
        auto& valueField = propBundle.field_value();          
        valueField.setValue(config.m_receiveMaximum);
    }

    if (config.m_maxPacketSize > 0U) {
        static const std::size_t MinValue = 
            ProtFrame::Layer_idAndFlags::Field::maxLength() + 
            ProtFrame::Layer_size::Field::maxLength();

        if (config.m_maxPacketSize < MinValue) {
            errorLog("The \"Max Packet Size\" configuration value is too small.");
            return CC_Mqtt5ErrorCode_BadParam;
        }

        if (!canAddProp(propsField)) {
            errorLog("Cannot add connect property, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_maxPacketSize();
        auto& valueField = propBundle.field_value();          
        valueField.setValue(config.m_maxPacketSize);

        m_maxPacketSize = config.m_maxPacketSize;
    }

    if (config.m_topicAliasMaximum > 0U) {
        if constexpr (!ExtConfig::HasTopicAliases) {
            errorLog("Cannot add topic alias maximum property, reature is not supported.");
            return CC_Mqtt5ErrorCode_BadParam;
        }

        if (config.m_topicAliasMaximum > CC_MQTT5_MAX_TOPIC_ALIASES_LIMIT) {
            errorLog("Topic alias maximum value is too high.");
            return CC_Mqtt5ErrorCode_BadParam;
        }

        if (!canAddProp(propsField)) {
            errorLog("Cannot add connect property, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        using ValueType = ConnectMsg::Field_properties::ValueType::value_type::Field_topicAliasMax::Field_value::ValueType;
        static constexpr auto MaxSupportedVal = std::numeric_limits<ValueType>::max();
        static_assert(MaxSupportedVal <= CC_MQTT5_MAX_TOPIC_ALIASES_LIMIT);

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_topicAliasMax();
        auto& valueField = propBundle.field_value();          
        valueField.setValue(config.m_topicAliasMaximum);        
        client().sessionState().m_maxRecvTopicAlias = config.m_topicAliasMaximum;
        client().reuseState().m_recvTopicAliases.reserve(config.m_topicAliasMaximum);
    }

    if (config.m_requestResponseInfo) {
        if (!canAddProp(propsField)) {
            errorLog("Cannot add connect property, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_requestResponseInfo();
        auto& valueField = propBundle.field_value();          
        valueField.setValue(config.m_requestResponseInfo);   
    }

    if (config.m_requestProblemInfo) {
        if (!canAddProp(propsField)) {
            errorLog("Cannot add connect property, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addProp(propsField);
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
        errorLog("Required authentication configuration is not provided.");
        return CC_Mqtt5ErrorCode_BadParam;
    }

    m_authCb = config.m_authCb;
    m_authCbData = config.m_authCbData;
    
    auto& propsField = m_connectMsg.field_properties();

    if (config.m_authMethod != nullptr) {
        if (!canAddProp(propsField)) {
            errorLog("Cannot add connect auth property, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_authMethod();
        auto& valueField = propBundle.field_value();        
        valueField.value() = config.m_authMethod;

        m_authMethod = config.m_authMethod;
    }

    if (config.m_authDataLen > 0U) {
        if (config.m_authData == nullptr) {
            errorLog("Bad authentication data parameter.");
            return CC_Mqtt5ErrorCode_BadParam;
        }

        if (!canAddProp(propsField)) {
            errorLog("Cannot add connect auth property, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_authData();
        auto& valueField = propBundle.field_value();        
        comms::util::assign(valueField.value(), config.m_authData, config.m_authData + config.m_authDataLen); 
    }

    return CC_Mqtt5ErrorCode_Success;
}

CC_Mqtt5ErrorCode ConnectOp::addUserProp(const CC_Mqtt5UserProp& prop)
{
    auto& propsField = m_connectMsg.field_properties();
    return addUserPropToList(propsField, prop);
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
        errorLog("Connect completion callback is not provided.");
        return CC_Mqtt5ErrorCode_BadParam;
    }

    if (!m_timer.isValid()) {
        errorLog("The library cannot allocate required number of timers.");
        return CC_Mqtt5ErrorCode_InternalError;
    }    

    if ((!m_connectMsg.field_flags().field_low().getBitValue_cleanStart()) && (client().sessionState().m_firstConnect)) {
        errorLog("Clean start flag needs to be set on the first connection attempt, perform configuration first.");
        return CC_Mqtt5ErrorCode_InsufficientConfig;
    }    

    m_cb = cb;
    m_cbData = cbData;
    
    auto result = client().sendMessage(m_connectMsg); 
    if (result != CC_Mqtt5ErrorCode_Success) {
        return result;
    }

    completeOnError.release(); // don't complete op yet
    auto guard = client().apiEnter();
    restartTimer();
    return CC_Mqtt5ErrorCode_Success;
}

void ConnectOp::handle(ConnackMsg& msg)
{
    m_timer.cancel();

    auto status = CC_Mqtt5AsyncOpStatus_ProtocolError;
    UserPropsList userProps; // Will be referenced in response
    auto response = CC_Mqtt5ConnectResponse();
    response.m_sessionExpiryInterval = m_sessionExpiryInterval;
    response.m_highQosPubLimit = std::numeric_limits<std::uint16_t>::max();
    response.m_maxQos = CC_Mqtt5QoS_ExactlyOnceDelivery;
    response.m_retainAvailable = true;
    response.m_wildcardSubAvailable = true;
    response.m_subIdsAvailable = true;
    response.m_sharedSubsAvailable = true;

    if constexpr (Config::SendMaxLimit > 0U) {
        response.m_highQosPubLimit = std::min(response.m_highQosPubLimit, Config::SendMaxLimit);
    }

    if constexpr (Config::MaxOutputPacketSize > 0U) {
        response.m_maxPacketSize = Config::MaxOutputPacketSize;
    }    

    auto completeOpOnExit = 
        comms::util::makeScopeGuard(
            [this, &status, &response]()
            {
                completeOpInternal(status, &response);
            });

    comms::cast_assign(response.m_reasonCode) = msg.field_reasonCode().value();
    response.m_sessionPresent = msg.field_flags().getBitValue_sp();

    if (response.m_sessionPresent && m_connectMsg.field_flags().field_low().getBitValue_cleanStart()) {
        errorLog("Session present when clean session is requested");
        sendDisconnectWithReason(DisconnectReason::ProtocolError);
        return;
    }

    PropsHandler propsHandler;
    for (auto& p : msg.field_propertiesList().value()) {
        p.currentFieldExec(propsHandler);
    }

    if (propsHandler.isProtocolError()) {
        errorLog("Protocol error in CONNACK properties");
        sendDisconnectWithReason(DisconnectReason::ProtocolError);
        return;
    }    

    // Auth method needs to be the same
    if ((propsHandler.m_authMethod != nullptr) && (m_authMethod != propsHandler.m_authMethod->field_value().value())) {
        errorLog("Invalid authentication method in CONNACK.");
        sendDisconnectWithReason(DisconnectReason::ProtocolError);
        return;
    }      
    
    // Auth method needs to be the same
    if ((propsHandler.m_authMethod == nullptr) && (!m_authMethod.empty())) {
        errorLog("No authentication method in CONNACK.");
        sendDisconnectWithReason(DisconnectReason::ProtocolError);
        return;
    }
    
    if (propsHandler.m_assignedClientId != nullptr) {
        response.m_assignedClientId = propsHandler.m_assignedClientId->field_value().value().c_str();
    }
    else if ((response.m_reasonCode < CC_Mqtt5ReasonCode_UnspecifiedError) && (m_connectMsg.field_clientId().value().empty())) {
        errorLog("Client ID hasn't been assigned by the broker");
        sendDisconnectWithReason(DisconnectReason::ProtocolError);
        return;
    }

    if (propsHandler.m_responseInfo != nullptr) {
        response.m_responseInfo = propsHandler.m_responseInfo->field_value().value().c_str();
    }    

    if (propsHandler.m_reasonStr != nullptr) {
        response.m_reasonStr = propsHandler.m_reasonStr->field_value().value().c_str();
    }

    if (propsHandler.m_serverRef != nullptr) {
        response.m_serverRef = propsHandler.m_serverRef->field_value().value().c_str();
    }

    if (propsHandler.m_authData != nullptr) {
        auto& vec = propsHandler.m_authData->field_value().value();
        comms::cast_assign(response.m_authDataLen) = vec.size();
        if (response.m_authDataLen > 0U) {
            response.m_authData = &vec[0];
        }
    }

    if (!propsHandler.m_userProps.empty()) {
        fillUserProps(propsHandler, userProps);
        response.m_userProps = &userProps[0];
        comms::cast_assign(response.m_userPropsCount) = userProps.size();
    }

    if (propsHandler.m_sessionExpiryInterval != nullptr) {
        response.m_sessionExpiryInterval = 
            comms::units::getSeconds<decltype(response.m_sessionExpiryInterval)>(propsHandler.m_sessionExpiryInterval->field_value());
    }

    if (propsHandler.m_receiveMax != nullptr) {
        response.m_highQosPubLimit = propsHandler.m_receiveMax->field_value().value();

        if constexpr (Config::SendMaxLimit > 0U) {
            response.m_highQosPubLimit = std::min(response.m_highQosPubLimit, Config::SendMaxLimit);
        }
    }

    if (propsHandler.m_maxPacketSize != nullptr) {
        response.m_maxPacketSize = propsHandler.m_maxPacketSize->field_value().value();
        
        if constexpr (Config::MaxOutputPacketSize > 0U) {
            response.m_maxPacketSize = std::min(response.m_maxPacketSize, Config::MaxOutputPacketSize);
        }          
    }

    if (propsHandler.m_topicAliasMax != nullptr) {
        if constexpr (Config::HasTopicAliases) {
            response.m_topicAliasMax = propsHandler.m_topicAliasMax->field_value().value();

            if constexpr (Config::TopicAliasesLimit > 0U) {
                response.m_topicAliasMax = std::min(response.m_topicAliasMax, Config::TopicAliasesLimit);
            }   

            client().sessionState().m_maxSendTopicAlias = response.m_topicAliasMax;
        }
    }

    if (propsHandler.m_maxQos != nullptr) {
        comms::cast_assign(response.m_maxQos) = propsHandler.m_maxQos->field_value().value();
    }

    if (propsHandler.m_retainAvailable != nullptr) {
        response.m_retainAvailable = 
            (propsHandler.m_retainAvailable->field_value().value() == PropsHandler::RetainAvailable::Field_value::ValueType::Enabled);
    }

    if (propsHandler.m_wildcardSubAvail != nullptr) {
        response.m_wildcardSubAvailable = 
            (propsHandler.m_wildcardSubAvail->field_value().value() == PropsHandler::WildcardSubAvail::Field_value::ValueType::Enabled);
    }

    if (propsHandler.m_subIdAvail != nullptr) {
        response.m_subIdsAvailable = 
            (propsHandler.m_subIdAvail->field_value().value() == PropsHandler::SubIdAvail::Field_value::ValueType::Enabled);
    }    

    if (propsHandler.m_sharedSubAvail != nullptr) {
        response.m_sharedSubsAvailable = 
            (propsHandler.m_sharedSubAvail->field_value().value() == PropsHandler::SharedSubAvail::Field_value::ValueType::Enabled);        
    }

    auto keepAlive = m_connectMsg.field_keepAlive().value();
    if (propsHandler.m_serverKeepAlive != nullptr) {
        keepAlive = propsHandler.m_serverKeepAlive->field_value().value();
    }

    status = CC_Mqtt5AsyncOpStatus_Complete; // Reported in op completion callback
    bool connected = (response.m_reasonCode == CC_Mqtt5ReasonCode_Success);

    auto& state = client().sessionState();
    if (!connected) {
        state.m_keepAliveMs = 0U;
        state.m_sendLimit = 0U;
        state.m_subIdsAvailable = false;
        return;
    }

    auto& reuseState = client().reuseState();
    if (!response.m_sessionPresent) {
        reuseState = ReuseState();
    }    

    reuseState.m_sendTopicAliases.resize(std::min(reuseState.m_sendTopicAliases.size(), std::size_t(response.m_topicAliasMax)));

    state.m_keepAliveMs = keepAlive * 1000U;
    state.m_sendLimit = response.m_highQosPubLimit + 1U;
    state.m_sessionExpiryIntervalMs = response.m_sessionExpiryInterval * 1000U;
    state.m_maxPacketSize = m_maxPacketSize;
    state.m_subIdsAvailable = response.m_subIdsAvailable;
    state.m_firstConnect = false;

    if constexpr (Config::SendMaxLimit > 0U) {
        state.m_sendLimit = std::min(state.m_sendLimit, Config::SendMaxLimit);
    }

    client().notifyConnected();
}

void ConnectOp::handle(AuthMsg& msg)
{
    m_timer.cancel();

    auto protocolErrorCompletion = 
        [this]()
        {
            sendDisconnectWithReason(DisconnectReason::ProtocolError);
            completeOpInternal(CC_Mqtt5AsyncOpStatus_ProtocolError);
        };

    if ((m_authMethod.empty()) || 
        (msg.field_reasonCode().value() != AuthMsg::Field_reasonCode::ValueType::ContinueAuth)) {
        protocolErrorCompletion();
        // No members access after this point, the op will be deleted
        return;
    }

    PropsHandler propsHandler;
    for (auto& p : msg.field_propertiesList().value()) {
        p.currentFieldExec(propsHandler);
    }

    if (propsHandler.isProtocolError()) {
        protocolErrorCompletion();
        // No members access after this point, the op will be deleted
        return;
    }

    if ((propsHandler.m_authMethod == nullptr) || 
        (m_authMethod != propsHandler.m_authMethod->field_value().value().c_str())) {
        protocolErrorCompletion();
        // No members access after this point, the op will be deleted
        return;        
    }

    UserPropsList userProps; // Will be referenced in inInfo
    auto inInfo = CC_Mqtt5AuthInfo();

    if (propsHandler.m_authData != nullptr) {
        auto& vec = propsHandler.m_authData->field_value().value();
        comms::cast_assign(inInfo.m_authDataLen) = vec.size();
        if (inInfo.m_authDataLen > 0U) {
            inInfo.m_authData = &vec[0];
        }
    }

    if (propsHandler.m_reasonStr != nullptr) {
        inInfo.m_reasonStr = propsHandler.m_reasonStr->field_value().value().c_str();
    }

    if (!propsHandler.m_userProps.empty()) {
        fillUserProps(propsHandler, userProps);
        inInfo.m_userProps = &userProps[0];
        comms::cast_assign(inInfo.m_userPropsCount) = userProps.size();
    }

    auto outInfo = CC_Mqtt5AuthInfo();
    auto authEc = m_authCb(m_authCbData, &inInfo, &outInfo);
    if (authEc != CC_Mqtt5AuthErrorCode_Continue) {
        COMMS_ASSERT(authEc == CC_Mqtt5AuthErrorCode_Disconnect);
        sendDisconnectWithReason(DisconnectReason::UnspecifiedError);
        completeOpInternal(CC_Mqtt5AsyncOpStatus_Aborted);
        // No members access after this point, the op will be deleted
        return;
    }

    auto termStatus = CC_Mqtt5AsyncOpStatus_OutOfMemory;
    auto termConnectOnExit = 
        comms::util::makeScopeGuard(
            [this, &termStatus]()
            {
                sendDisconnectWithReason(DisconnectReason::UnspecifiedError);
                completeOpInternal(termStatus);
            });

    AuthMsg respMsg;
    respMsg.field_reasonCode().setValue(AuthMsg::Field_reasonCode::ValueType::ContinueAuth);

    auto& propsField = respMsg.field_propertiesList();

    {
        if (!canAddProp(propsField)) {
            return;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_authMethod();
        auto& valueField = propBundle.field_value();        
        valueField.value() = m_authMethod.c_str();
    }

    if (outInfo.m_authDataLen > 0U) {
        if (outInfo.m_authData == nullptr) {
            termStatus = CC_Mqtt5AsyncOpStatus_BadParam;
            return;
        }

        if (!canAddProp(propsField)) {
            return;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_authData();
        auto& valueField = propBundle.field_value();        
        comms::util::assign(valueField.value(), outInfo.m_authData, outInfo.m_authData + outInfo.m_authDataLen); 
    }    

    if (outInfo.m_reasonStr != nullptr) {
        if (!canAddProp(propsField)) {
            return;
        }

        auto& propVar = addProp(propsField);
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

            if (!canAddProp(propsField)) {
                return;
            }

            auto& propVar = addProp(propsField);
            auto& propBundle = propVar.initField_userProperty();            
            auto& valueField = propBundle.field_value();
            valueField.field_first().value() = prop.m_key;

            if (prop.m_value != nullptr) {
                valueField.field_second().value() = prop.m_value;
            }
        }
    }

    termConnectOnExit.release();
    restartTimer();
    client().sendMessage(respMsg);
}

Op::Type ConnectOp::typeImpl() const
{
    return Type_Connect;
}

void ConnectOp::terminateOpImpl(CC_Mqtt5AsyncOpStatus status)
{
    completeOpInternal(status);
}

void ConnectOp::networkConnectivityChangedImpl()
{
    if (client().sessionState().m_networkDisconnected) {
        completeOpInternal(CC_Mqtt5AsyncOpStatus_BrokerDisconnected);
        return;
    }
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
    m_timer.wait(getResponseTimeout(), &ConnectOp::opTimeoutCb, this);
}

void ConnectOp::opTimeoutCb(void* data)
{
    asConnectOp(data)->opTimeoutInternal();
}

} // namespace op

} // namespace cc_mqtt5_client
