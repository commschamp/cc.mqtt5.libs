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

    auto& clientIdStr = m_connectMsg.field_clientId().value();
    if (config.m_clientId != nullptr) {
        clientIdStr = config.m_clientId;    
    }
    else {
        clientIdStr.clear();
    }

    if (maxStringLen() < clientIdStr.size()) {
        errorLog("Client ID is too long");
        clientIdStr.clear();
        return CC_Mqtt5ErrorCode_BadParam;        
    }

    if ((m_connectMsg.field_clientId().value().empty()) && (!config.m_cleanStart)) {
        errorLog("Clean start flag needs to be set for empty client id");
        return CC_Mqtt5ErrorCode_BadParam;
    }

    bool hasUsername = (config.m_username != nullptr);
    m_connectMsg.field_flags().field_high().setBitValue_userNameFlag(hasUsername);
    if (hasUsername) {
        m_connectMsg.field_userName().field().value() = config.m_username;
    }

    auto& usernameStr = m_connectMsg.field_userName().field().value();
    if (maxStringLen() < usernameStr.size()) {
        errorLog("Username is too long");
        usernameStr.clear();
        return CC_Mqtt5ErrorCode_BadParam;        
    }

    bool hasPassword = (config.m_passwordLen > 0U);
    m_connectMsg.field_flags().field_high().setBitValue_passwordFlag(hasPassword);

    auto& passwordStr = m_connectMsg.field_password().field().value();
    if (hasPassword) {
        comms::util::assign(passwordStr, config.m_password, config.m_password + config.m_passwordLen);
    }

    if (maxStringLen() < passwordStr.size()) {
        errorLog("Password is too long");
        passwordStr.clear();
        return CC_Mqtt5ErrorCode_BadParam;        
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
        errorLog("Invalid will QoS value in configuration.");
        return CC_Mqtt5ErrorCode_BadParam;
    }    

    auto& willTopicStr = m_connectMsg.field_willTopic().field().value();
    willTopicStr = config.m_topic;
    if (maxStringLen() < willTopicStr.size()) {
        errorLog("Will topic is too long.");
        willTopicStr.clear();
        return CC_Mqtt5ErrorCode_BadParam;        
    }

    auto& willData = m_connectMsg.field_willMessage().field().value();
    if (config.m_dataLen > 0U) {
        comms::util::assign(willData, config.m_data, config.m_data + config.m_dataLen);
    }

    if (maxStringLen() < willData.size()) {
        errorLog("Will data is too long.");
        willData.clear();
        return CC_Mqtt5ErrorCode_BadParam;                
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
            discardLastProp(propsField);
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
            discardLastProp(propsField);
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

        if (maxStringLen() < valueField.value().size()) {
            errorLog("Content type value is too long");
            discardLastProp(propsField);
            return CC_Mqtt5ErrorCode_BadParam;
        }
    }

    if (config.m_responseTopic != nullptr) {
        if (!verifyPubTopic(config.m_responseTopic, true)) {
            errorLog("Bad will response topic format in CONNECT.");
            return CC_Mqtt5ErrorCode_BadParam;
        }

        if (!canAddProp(propsField)) {
            errorLog("Cannot add will property, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }        

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_responseTopic();
        auto& valueField = propBundle.field_value();
        valueField.value() = config.m_responseTopic;    

        if (maxStringLen() < valueField.value().size()) {
            errorLog("Will response topic value is too long");
            discardLastProp(propsField);
            return CC_Mqtt5ErrorCode_BadParam;
        }            
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

        if (maxStringLen() < valueField.value().size()) {
            errorLog("Will correlation data value is too long");
            discardLastProp(propsField);
            return CC_Mqtt5ErrorCode_BadParam;
        }          
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

        m_highQosRecvLimit = config.m_receiveMaximum;
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

        m_maxRecvPacketSize = config.m_maxPacketSize;
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
        client().sessionState().m_recvTopicAliases.reserve(config.m_topicAliasMaximum);
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
        m_requestProblemInfo = config.m_requestProblemInfo;
    }    

    return CC_Mqtt5ErrorCode_Success;
}

CC_Mqtt5ErrorCode ConnectOp::configAuth(const CC_Mqtt5AuthConfig& config)
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

        if (maxStringLen() < valueField.value().size()) {
            errorLog("Auth method value is too big.");
            discardLastProp(propsField);
            return CC_Mqtt5ErrorCode_BadParam;
        }

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

        if (maxStringLen() < valueField.value().size()) {
            errorLog("Auth data value is too big.");
            discardLastProp(propsField);
            return CC_Mqtt5ErrorCode_BadParam;
        }        
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
    response.m_highQosSendLimit = std::numeric_limits<std::uint16_t>::max();
    response.m_maxQos = CC_Mqtt5QoS_ExactlyOnceDelivery;
    response.m_retainAvailable = true;
    response.m_wildcardSubAvailable = true;
    response.m_subIdsAvailable = true;
    response.m_sharedSubsAvailable = true;

    if constexpr (Config::SendMaxLimit > 0U) {
        response.m_highQosSendLimit = std::min(response.m_highQosSendLimit, Config::SendMaxLimit);
    }

    if constexpr (Config::MaxOutputPacketSize > 0U) {
        response.m_maxPacketSize = Config::MaxOutputPacketSize;
    }    

    auto completeOpOnExit = 
        comms::util::makeScopeGuard(
            [this, &status, &response]()
            {
                auto* responsePtr = &response;
                if (status != CC_Mqtt5AsyncOpStatus_Complete) {
                    responsePtr = nullptr;
                }                
                completeOpInternal(status, responsePtr);
            });

    comms::cast_assign(response.m_reasonCode) = msg.field_reasonCode().value();
    response.m_sessionPresent = msg.field_flags().getBitValue_sp();

    if (response.m_sessionPresent && m_connectMsg.field_flags().field_low().getBitValue_cleanStart()) {
        errorLog("Session present when clean session is requested");
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
        response.m_highQosSendLimit = propsHandler.m_receiveMax->field_value().value();

        if constexpr (Config::SendMaxLimit > 0U) {
            response.m_highQosSendLimit = std::min(response.m_highQosSendLimit, Config::SendMaxLimit);
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
        state.m_highQosSendLimit = 0U;
        state.m_subIdsAvailable = false;
        return;
    }

    auto& reuseState = client().reuseState();
    if (!response.m_sessionPresent) {
        reuseState = ReuseState();
    }    

    state.m_sendTopicAliases.resize(std::min(state.m_sendTopicAliases.size(), std::size_t(response.m_topicAliasMax)));

    state.m_keepAliveMs = keepAlive * 1000U;
    state.m_highQosSendLimit = response.m_highQosSendLimit;
    state.m_highQosRecvLimit = m_highQosRecvLimit;
    state.m_authMethod = std::move(m_authMethod);
    state.m_sessionExpiryIntervalMs = response.m_sessionExpiryInterval * 1000U;
    state.m_connectSessionExpiryInterval = m_sessionExpiryInterval;
    state.m_maxRecvPacketSize = m_maxRecvPacketSize;
    state.m_maxSendPacketSize = response.m_maxPacketSize;
    state.m_pubMaxQos = response.m_maxQos;
    state.m_wildcardSubAvailable = response.m_wildcardSubAvailable;
    state.m_subIdsAvailable = response.m_subIdsAvailable;
    state.m_retainAvailable = response.m_retainAvailable;
    state.m_sharedSubsAvailable = response.m_sharedSubsAvailable;
    state.m_firstConnect = false;
    state.m_problemInfoAllowed = m_requestProblemInfo;

    if constexpr (Config::SendMaxLimit > 0U) {
        state.m_highQosSendLimit = std::min(state.m_highQosSendLimit, Config::SendMaxLimit);
    }

    client().notifyConnected();
}

void ConnectOp::handle(DisconnectMsg& msg)
{

    auto info = CC_Mqtt5DisconnectInfo();
    
    if (msg.field_reasonCode().doesExist()) {
        comms::cast_assign(info.m_reasonCode) = msg.field_reasonCode().field().value();
    }

    if (msg.field_propertiesList().doesExist()) {
        PropsHandler propsHandler;
        for (auto& p : msg.field_propertiesList().field().value()) {
            p.currentFieldExec(propsHandler);
        } 

        if (propsHandler.m_reasonStr != nullptr) {
            info.m_reasonStr = propsHandler.m_reasonStr->field_value().value().c_str();
        }     

        if (propsHandler.m_serverRef != nullptr) {
            info.m_serverRef = propsHandler.m_serverRef->field_value().value().c_str();
        }

        if (!propsHandler.m_userProps.empty()) {
            UserPropsList userProps;
            fillUserProps(propsHandler, userProps);
            info.m_userProps = &userProps[0];
            comms::cast_assign(info.m_userPropsCount) = userProps.size();
        }        
    }

    auto& cl = client();
    completeOpInternal(CC_Mqtt5AsyncOpStatus_BrokerDisconnected);
    // No members access after this point, the op will be deleted    

    cl.notifyDisconnected(true, CC_Mqtt5AsyncOpStatus_BrokerDisconnected, &info);
}

void ConnectOp::handle(AuthMsg& msg)
{
    m_timer.cancel();

    auto protocolErrorCompletion = 
        [this](DisconnectReason reason = DisconnectReason::ProtocolError)
        {
            sendDisconnectWithReason(reason);
            completeOpInternal(CC_Mqtt5AsyncOpStatus_ProtocolError);
        };

    if (!msg.doValid()) {
        errorLog("Invalid flags received in AUTH message");
        protocolErrorCompletion(DisconnectReason::MalformedPacket);
        // No members access after this point, the op will be deleted
        return;
    }        

    if ((m_authMethod.empty()) || 
        (msg.field_reasonCode().isMissing()) ||
        (msg.field_reasonCode().field().value() != AuthMsg::Field_reasonCode::Field::ValueType::ContinueAuth)) {
        errorLog("Invalid reason code received received in AUTH message");
        protocolErrorCompletion();
        // No members access after this point, the op will be deleted
        return;
    }

    UserPropsList userProps; // Will be referenced in inInfo
    auto inInfo = CC_Mqtt5AuthInfo();

    if (msg.field_propertiesList().doesExist()) {

        PropsHandler propsHandler;
        for (auto& p : msg.field_propertiesList().field().value()) {
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

        if (propsHandler.m_authData != nullptr) {
            auto& vec = propsHandler.m_authData->field_value().value();
            comms::cast_assign(inInfo.m_authDataLen) = vec.size();
            if (inInfo.m_authDataLen > 0U) {
                inInfo.m_authData = &vec[0];
            }
        }

        if (propsHandler.m_reasonStr != nullptr) {
            if (!m_requestProblemInfo) {
                errorLog("Received reason string in AUTH when \"problem information\" was disabled in CONNECT.");
                protocolErrorCompletion();
                return;
            }

            inInfo.m_reasonStr = propsHandler.m_reasonStr->field_value().value().c_str();
        }

        if (!propsHandler.m_userProps.empty()) {
            if (!m_requestProblemInfo) {
                errorLog("Received user properties in AUTH when \"problem information\" was disabled in CONNECT.");
                protocolErrorCompletion();
                return;
            }

            fillUserProps(propsHandler, userProps);
            inInfo.m_userProps = &userProps[0];
            comms::cast_assign(inInfo.m_userPropsCount) = userProps.size();
        }
    }

    auto outInfo = CC_Mqtt5AuthInfo();
    auto authEc = m_authCb(m_authCbData, &inInfo, &outInfo);
    if (authEc == CC_Mqtt5AuthErrorCode_Disconnect) {
        sendDisconnectWithReason(DisconnectReason::NotAuthorized);
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
    respMsg.field_reasonCode().setExists();
    respMsg.field_reasonCode().field().setValue(AuthMsg::Field_reasonCode::Field::ValueType::ContinueAuth);

    respMsg.field_propertiesList().setExists();
    auto& propsField = respMsg.field_propertiesList().field();
    {
        if (!canAddProp(propsField)) {
            errorLog("Cannot add connect auth property, reached available limit.");
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
            errorLog("Cannot add connect auth property, reached available limit.");
            return;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_authData();
        auto& valueField = propBundle.field_value();        
        comms::util::assign(valueField.value(), outInfo.m_authData, outInfo.m_authData + outInfo.m_authDataLen); 

        if (maxStringLen() < valueField.value().size()) {
            errorLog("Auth data value is too long");
            discardLastProp(propsField);
            termStatus = CC_Mqtt5AsyncOpStatus_BadParam;
            return;            
        }        
    }    

    if (outInfo.m_reasonStr != nullptr) {
        if (!canAddProp(propsField)) {
            errorLog("Cannot add connect auth property, reached available limit.");
            return;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_reasonStr();
        auto& valueField = propBundle.field_value();  
        valueField.value() = outInfo.m_reasonStr;           

        if (maxStringLen() < valueField.value().size()) {
            errorLog("Reason string in CONNECT auth info too long");
            discardLastProp(propsField);
            termStatus = CC_Mqtt5AsyncOpStatus_BadParam;
            return;               
        }         
    }

    if (outInfo.m_userPropsCount > 0U) {
        if (outInfo.m_userProps == nullptr) {
            termStatus = CC_Mqtt5AsyncOpStatus_BadParam;
            return;
        }

        for (auto idx = 0U; idx < outInfo.m_userPropsCount; ++idx) {
            auto& prop = outInfo.m_userProps[idx];
            auto ec = addUserPropToList(propsField, prop);
            if (ec == CC_Mqtt5ErrorCode_Success) {
                continue;
            }

            static const std::pair<CC_Mqtt5ErrorCode, CC_Mqtt5AsyncOpStatus> Map[] = {
                {CC_Mqtt5ErrorCode_BadParam, CC_Mqtt5AsyncOpStatus_BadParam},
                {CC_Mqtt5ErrorCode_OutOfMemory, CC_Mqtt5AsyncOpStatus_OutOfMemory},
                {CC_Mqtt5ErrorCode_NotSupported, CC_Mqtt5AsyncOpStatus_BadParam},
            };

            auto errorIter = 
                std::find_if(
                    std::begin(Map), std::end(Map),
                    [ec](auto& info)
                    {
                        return ec == info.first;
                    });

            if (errorIter != std::end(Map)) {
                termStatus = errorIter->second;
                return;
            }

            COMMS_ASSERT(false); // Should not happen
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
    auto cb = m_cb;
    auto* cbData = m_cbData;
    opComplete(); // mustn't access data members after destruction
    if (cb != nullptr) {
        cb(cbData, status, response);    
    }
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
