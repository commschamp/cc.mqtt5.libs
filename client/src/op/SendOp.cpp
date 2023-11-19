//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/SendOp.h"
#include "ClientImpl.h"

#include "comms/units.h"

namespace cc_mqtt5_client
{

namespace op
{

namespace 
{

inline SendOp* asSendOp(void* data)
{
    return reinterpret_cast<SendOp*>(data);
}

} // namespace     

SendOp::SendOp(ClientImpl& client) : 
    Base(client),
    m_responseTimer(client.timerMgr().allocTimer())
{
    COMMS_ASSERT(m_responseTimer.isValid());
}    

SendOp::~SendOp()
{
    releasePacketId(m_pubMsg.field_packetId().field().value());
}

void SendOp::handle(PubackMsg& msg)
{
    if (m_pubMsg.field_packetId().field().value() != msg.field_packetId().value()) {
        return;
    }

    m_responseTimer.cancel();

    auto terminateOnExit = 
        comms::util::makeScopeGuard(
            [&cl = client()]()
            {
                terminationWithReason(cl, DisconnectReason::ProtocolError);
            }
        );      

    auto status = CC_Mqtt5AsyncOpStatus_ProtocolError;
    UserPropsList userProps; // Will be referenced in response
    auto response = CC_Mqtt5PublishResponse();

    auto completeOpOnExit = 
        comms::util::makeScopeGuard(
            [this, &status, &response]()
            {
                auto* responsePtr = &response;
                if (status != CC_Mqtt5AsyncOpStatus_Complete) {
                    responsePtr = nullptr;
                }
                reportPubComplete(status, responsePtr);
                opComplete();
            });        

    if (m_pubMsg.transportField_flags().field_qos().value() != PublishMsg::TransportField_flags::Field_qos::ValueType::AtLeastOnceDelivery) {
        errorLog("Unexpected PUBACK for Qos2 message");
        return;
    }

    if (msg.field_reasonCode().doesExist()) {
        comms::cast_assign(response.m_reasonCode) = msg.field_reasonCode().field().value();
    }

    if (m_registeredAlias && (response.m_reasonCode < CC_Mqtt5ReasonCode_UnspecifiedError)) {
        confirmRegisteredAlias();
    }

    if (msg.field_properties().doesExist()) {
        PropsHandler propsHandler;
        for (auto& p : msg.field_properties().field().value()) {
            p.currentFieldExec(propsHandler);
        }

        if (propsHandler.isProtocolError()) {
            errorLog("Invalid properties in PUBACK message");
            return;
        }    

        if (propsHandler.m_reasonStr != nullptr) {
            if (!client().sessionState().m_problemInfoAllowed) {
                errorLog("Received reason string in PUBACK when \"problem information\" was disabled in CONNECT.");
                return; 
            }

            response.m_reasonStr = propsHandler.m_reasonStr->field_value().value().c_str();
        }

        if (!propsHandler.m_userProps.empty()) {
            if (!client().sessionState().m_problemInfoAllowed) {
                errorLog("Received user properties in PUBACK when \"problem information\" was disabled in CONNECT.");
                return; 
            }

            fillUserProps(propsHandler, userProps);
            response.m_userProps = &userProps[0];
            comms::cast_assign(response.m_userPropsCount) = userProps.size();
        }        
    }

    terminateOnExit.release();
    status = CC_Mqtt5AsyncOpStatus_Complete;
}

void SendOp::handle(PubrecMsg& msg)
{
    if (m_pubMsg.field_packetId().field().value() != msg.field_packetId().value()) {
        return;
    }

    m_responseTimer.cancel();

    auto terminateOnExit = 
        comms::util::makeScopeGuard(
            [&cl = client()]()
            {
                terminationWithReason(cl, DisconnectReason::ProtocolError);
            }
        );      

    auto status = CC_Mqtt5AsyncOpStatus_ProtocolError;
    UserPropsList userProps; // Will be referenced in response
    auto response = CC_Mqtt5PublishResponse();

    auto completeOpOnExit = 
        comms::util::makeScopeGuard(
            [this, &status, &response]()
            {
                auto* responsePtr = &response;
                if (status != CC_Mqtt5AsyncOpStatus_Complete) {
                    responsePtr = nullptr;
                }
                reportPubComplete(status, responsePtr);
                opComplete();
            });    

    if (m_pubMsg.transportField_flags().field_qos().value() != PublishMsg::TransportField_flags::Field_qos::ValueType::ExactlyOnceDelivery) {
        errorLog("Unexpected PUBREC for Qos1 message");
        return;
    }

    if (m_acked) {
        errorLog("Double PUBREC message");
        return;
    }

    if (msg.field_properties().doesExist()) {
        PropsHandler propsHandler;
        for (auto& p : msg.field_properties().field().value()) {
            p.currentFieldExec(propsHandler);
        }

        if (propsHandler.isProtocolError()) {
            errorLog("Invalid properties in PUBREC message");
            return;
        }    

        if (propsHandler.m_reasonStr != nullptr) {
            if (!client().sessionState().m_problemInfoAllowed) {
                errorLog("Received reason string in PUBREC when \"problem information\" was disabled in CONNECT.");
                return; 
            }

            response.m_reasonStr = propsHandler.m_reasonStr->field_value().value().c_str();
        }

        if (!propsHandler.m_userProps.empty()) {
            if (!client().sessionState().m_problemInfoAllowed) {
                errorLog("Received user properties in PUBREC when \"problem information\" was disabled in CONNECT.");
                return; 
            }

            fillUserProps(propsHandler, userProps);
            response.m_userProps = &userProps[0];
            comms::cast_assign(response.m_userPropsCount) = userProps.size();
        }        
    }
    
    if ((msg.field_reasonCode().doesExist()) && 
        (msg.field_reasonCode().field().value() >= PubrecMsg::Field_reasonCode::Field::ValueType::UnspecifiedError)) {
        comms::cast_assign(response.m_reasonCode) = msg.field_reasonCode().field().value();
        terminateOnExit.release();
        status = CC_Mqtt5AsyncOpStatus_Complete;
        return;
    }

    if (m_registeredAlias) {
        confirmRegisteredAlias();
    }    

    m_acked = true;
    m_sendAttempts = 0U;
    PubrelMsg pubrelMsg;
    pubrelMsg.field_packetId().setValue(m_pubMsg.field_packetId().field().value());
    auto result = client().sendMessage(pubrelMsg); 
    if (result != CC_Mqtt5ErrorCode_Success) {
        errorLog("Failed to resend PUBREL message.");
        terminateOnExit.release();
        status = CC_Mqtt5AsyncOpStatus_InternalError;
        return;
    }

    m_reasonCode = response.m_reasonCode;
    terminateOnExit.release();
    completeOpOnExit.release();
    ++m_sendAttempts;
    restartResponseTimer();
}

void SendOp::handle(PubcompMsg& msg)
{
    if (m_pubMsg.field_packetId().field().value() != msg.field_packetId().value()) {
        return;
    }

    m_responseTimer.cancel();

    auto terminateOnExit = 
        comms::util::makeScopeGuard(
            [&cl = client()]()
            {
                terminationWithReason(cl, DisconnectReason::ProtocolError);
            }
        );      

    auto status = CC_Mqtt5AsyncOpStatus_ProtocolError;
    UserPropsList userProps; // Will be referenced in response
    auto response = CC_Mqtt5PublishResponse();

    auto completeOpOnExit = 
        comms::util::makeScopeGuard(
            [this, &status, &response]()
            {
                auto* responsePtr = &response;
                if (status != CC_Mqtt5AsyncOpStatus_Complete) {
                    responsePtr = nullptr;
                }
                reportPubComplete(status, responsePtr);
                opComplete();
            });  

    if (m_pubMsg.transportField_flags().field_qos().value() != PublishMsg::TransportField_flags::Field_qos::ValueType::ExactlyOnceDelivery) {
        errorLog("Unexpected PUBCOMP for Qos1 message");
        return;
    }

    if (!m_acked) {
        errorLog("Unexpected PUBCOMP without PUBREC");
        return;
    }    

    response.m_reasonCode = m_reasonCode;
    if ((msg.field_reasonCode().doesExist()) && 
        (msg.field_reasonCode().field().value() != PubcompMsg::Field_reasonCode::Field::ValueType::Success)) {
        comms::cast_assign(response.m_reasonCode) = msg.field_reasonCode().field().value();
    }    

    if (msg.field_properties().doesExist()) {
        PropsHandler propsHandler;
        for (auto& p : msg.field_properties().field().value()) {
            p.currentFieldExec(propsHandler);
        }

        if (propsHandler.isProtocolError()) {
            errorLog("Invalid properties in PUBCOMP message");
            return;
        }    

        if (propsHandler.m_reasonStr != nullptr) {
            if (!client().sessionState().m_problemInfoAllowed) {
                errorLog("Received reason string in PUBCOMP when \"problem information\" was disabled in CONNECT.");
                return; 
            }

            response.m_reasonStr = propsHandler.m_reasonStr->field_value().value().c_str();
        }

        if (!propsHandler.m_userProps.empty()) {
            if (!client().sessionState().m_problemInfoAllowed) {
                errorLog("Received user properties in PUBCOMP when \"problem information\" was disabled in CONNECT.");
                return; 
            }

            fillUserProps(propsHandler, userProps);
            response.m_userProps = &userProps[0];
            comms::cast_assign(response.m_userPropsCount) = userProps.size();
        }        
    }
    
    terminateOnExit.release();
    status = CC_Mqtt5AsyncOpStatus_Complete;
}

CC_Mqtt5ErrorCode SendOp::configBasic(const CC_Mqtt5PublishBasicConfig& config)
{
    if ((config.m_topic == nullptr) || (config.m_topic[0] == '\0')) {
        errorLog("Topic hasn't been provided in publish configuration");
        return CC_Mqtt5ErrorCode_BadParam;
    }

    if (!verifyPubTopic(config.m_topic, true)) {
        errorLog("Bad topic format in publish.");
        return CC_Mqtt5ErrorCode_BadParam;
    }

    auto& state = client().sessionState();

    if (config.m_qos > state.m_pubMaxQos) {
        errorLog("QoS value is too high in publish.");
        return CC_Mqtt5ErrorCode_BadParam;
    }

    if ((config.m_qos > CC_Mqtt5QoS_AtMostOnceDelivery) && (state.m_highQosSendLimit < client().sendsCount())) {
        errorLog("Exceeding number of allowed high QoS publishes.");
        return CC_Mqtt5ErrorCode_Busy;        
    }

    if (config.m_retain && (!state.m_retainAvailable)) {
        errorLog("Retain is not supported by the broker");
        return CC_Mqtt5ErrorCode_BadParam;
    }

    unsigned alias = 0U;
    bool mustAssignTopic = true;
    do {
        if constexpr (Config::HasTopicAliases) {
            if (config.m_topicAliasPref == CC_Mqtt5TopicAliasPreference_ForceTopicOnly) {
                break;
            }
            
            auto iter = 
                std::lower_bound(
                    state.m_sendTopicAliases.begin(), state.m_sendTopicAliases.end(), config.m_topic,
                    [](auto& info, const char* topicParam)
                    {
                        return info.m_topic < topicParam;
                    });

            bool found = ((iter != state.m_sendTopicAliases.end()) && (iter->m_topic == config.m_topic));
            if (!found) {
                if ((config.m_topicAliasPref == CC_Mqtt5TopicAliasPreference_ForceTopicWithAlias) || 
                    (config.m_topicAliasPref == CC_Mqtt5TopicAliasPreference_ForceAliasOnly)) {
                    errorLog("The topic alias for the publish hasn't been allocated");
                    return CC_Mqtt5ErrorCode_BadParam;
                }

                COMMS_ASSERT(config.m_topicAliasPref == CC_Mqtt5TopicAliasPreference_UseAliasIfAvailable);
                break;
            }

            alias = iter->m_alias;

            if (config.m_topicAliasPref == CC_Mqtt5TopicAliasPreference_ForceTopicWithAlias) {
                break;
            }

            if (config.m_topicAliasPref == CC_Mqtt5TopicAliasPreference_ForceAliasOnly) {
                mustAssignTopic = false;
                break;
            }

            if (iter->m_highQosRegRemCount == 0U) {
                // The message was acked
                mustAssignTopic = false;
                break;
            }

            if (config.m_qos > CC_Mqtt5QoS_AtMostOnceDelivery) {
                break;
            }

            if (iter->m_lowQosRegRemCount == 0U) {
                mustAssignTopic = false;
                break;
            }

            --iter->m_lowQosRegRemCount;
        }
        else {
            if ((config.m_topicAliasPref != CC_Mqtt5TopicAliasPreference_UseAliasIfAvailable) && 
                (config.m_topicAliasPref != CC_Mqtt5TopicAliasPreference_ForceTopicOnly)) {
                errorLog("Topic aliases are not supported");
                return CC_Mqtt5ErrorCode_NotSupported;
            }
        }
    } while (false);

    m_pubMsg.transportField_flags().field_retain().setBitValue_bit(config.m_retain);
    m_pubMsg.transportField_flags().field_qos().setValue(config.m_qos);
    
    if (mustAssignTopic) {
        auto& topicStr = m_pubMsg.field_topic().value();
        topicStr = config.m_topic;
        m_topicConfigured = true;

        if (maxStringLen() < topicStr.size()) {
            errorLog("Publish topic value is too long");
            return CC_Mqtt5ErrorCode_BadParam;
        }          
    }

    auto& propsField = m_pubMsg.field_properties();
    if (alias > 0U) {
        if (!canAddProp(propsField)) {
            errorLog("Cannot add topic alias property, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_topicAlias();
        auto& valueField = propBundle.field_value();
        valueField.setValue(alias);
        m_topicConfigured = true;
    }

    auto& dataVec = m_pubMsg.field_payload().value();
    if (config.m_dataLen > 0U) {
        COMMS_ASSERT(config.m_data != nullptr);
        comms::util::assign(dataVec, config.m_data, config.m_data + config.m_dataLen);
    }

    if (maxStringLen() < dataVec.size()) {
        errorLog("Publish data value is too long");
        return CC_Mqtt5ErrorCode_BadParam;
    }      

    m_registeredAlias = (alias > 0) && mustAssignTopic;
    return CC_Mqtt5ErrorCode_Success;
}

CC_Mqtt5ErrorCode SendOp::configExtra(const CC_Mqtt5PublishExtraConfig& config)
{
    auto& propsField = m_pubMsg.field_properties();

    if (config.m_contentType != nullptr) {
        if (!canAddProp(propsField)) {
            errorLog("Cannot add publish property, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_contentType();
        auto& valueField = propBundle.field_value();
        valueField.value() = config.m_contentType;       

        if (maxStringLen() < valueField.value().size()) {
            errorLog("Publish content type value is too long");
            discardLastProp(propsField);
            return CC_Mqtt5ErrorCode_BadParam;
        }         
    }

    if (config.m_responseTopic != nullptr) {
        if (!verifyPubTopic(config.m_responseTopic, true)) {
            errorLog("Bad response topic format in publish.");
            return CC_Mqtt5ErrorCode_BadParam;
        }

        if (!canAddProp(propsField)) {
            errorLog("Cannot add publish property, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }        
        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_responseTopic();
        auto& valueField = propBundle.field_value();
        valueField.value() = config.m_responseTopic;        

        if (maxStringLen() < valueField.value().size()) {
            errorLog("Publish response topic value is too long");
            discardLastProp(propsField);
            return CC_Mqtt5ErrorCode_BadParam;
        }          
    }

    if ((config.m_correlationDataLen > 0U) && (config.m_correlationData == nullptr)) {
        errorLog("Bad correlation data parameter in publish configuration.");
        return CC_Mqtt5ErrorCode_BadParam;
    }

    if (config.m_correlationData != nullptr) {
        if (!canAddProp(propsField)) {
            errorLog("Cannot add publish property, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_correlationData();
        auto& valueField = propBundle.field_value();        

        comms::util::assign(valueField.value(), config.m_correlationData, config.m_correlationData + config.m_correlationDataLen);

        if (maxStringLen() < valueField.value().size()) {
            errorLog("Publish correlation data value is too long");
            discardLastProp(propsField);
            return CC_Mqtt5ErrorCode_BadParam;
        }         
    }    

    if (config.m_messageExpiryInterval > 0U) {
        if (!canAddProp(propsField)) {
            errorLog("Cannot add publish property, reached available limit.");
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

    if (config.m_format != CC_Mqtt5PayloadFormat_Unspecified) {
        if (!canAddProp(propsField)) {
            errorLog("Cannot add will publish, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_payloadFormatIndicator();
        propBundle.field_value().setValue(config.m_format);
    }    

    return CC_Mqtt5ErrorCode_Success;
}

CC_Mqtt5ErrorCode SendOp::addUserProp(const CC_Mqtt5UserProp& prop)
{
    auto& propsField = m_pubMsg.field_properties();
    return addUserPropToList(propsField, prop);
}

CC_Mqtt5ErrorCode SendOp::setResendAttempts(unsigned attempts)
{
    if (attempts == 0U) {
        errorLog("PUBLISH resend attempts must be greater than 0");
        return CC_Mqtt5ErrorCode_BadParam;
    }

    m_totalSendAttempts = attempts;
    return CC_Mqtt5ErrorCode_Success;
}

unsigned SendOp::getResendAttempts() const
{
    return m_totalSendAttempts;
}

CC_Mqtt5ErrorCode SendOp::send(CC_Mqtt5PublishCompleteCb cb, void* cbData)
{
    auto completeOnExit = 
        comms::util::makeScopeGuard(
            [this]()
            {
                opComplete();
            });

    if (!m_responseTimer.isValid()) {
        errorLog("The library cannot allocate required number of timers.");
        return CC_Mqtt5ErrorCode_InternalError;
    }    

    if (!m_topicConfigured) {
        errorLog("Topic hasn't been properly configured, cannot publish");
        return CC_Mqtt5ErrorCode_InsufficientConfig;
    }

    m_cb = cb;
    m_cbData = cbData;

    using Qos = PublishMsg::TransportField_flags::Field_qos::ValueType;
    if (m_pubMsg.transportField_flags().field_qos().value() > Qos::AtMostOnceDelivery) {
        m_pubMsg.field_packetId().field().setValue(allocPacketId());
    }

    m_pubMsg.doRefresh(); // Update packetId presence

    m_sendAttempts = 0U;
    auto result = client().sendMessage(m_pubMsg); 
    if (result != CC_Mqtt5ErrorCode_Success) {
        return result;
    }

    ++m_sendAttempts;

    if (m_pubMsg.transportField_flags().field_qos().value() == PublishMsg::TransportField_flags::Field_qos::ValueType::AtMostOnceDelivery) {
        reportPubComplete(CC_Mqtt5AsyncOpStatus_Complete);
        return CC_Mqtt5ErrorCode_Success;
    }

    completeOnExit.release(); // don't complete op yet
    auto guard = client().apiEnter();
    restartResponseTimer();
    return CC_Mqtt5ErrorCode_Success;
}

CC_Mqtt5ErrorCode SendOp::cancel()
{
    opComplete();
    return CC_Mqtt5ErrorCode_Success;
}

Op::Type SendOp::typeImpl() const
{
    return Type_Send;
}

void SendOp::terminateOpImpl(CC_Mqtt5AsyncOpStatus status)
{
    reportPubComplete(status);
    opComplete();
}

void SendOp::networkConnectivityChangedImpl()
{
    m_responseTimer.setSuspended(client().sessionState().m_networkDisconnected);
}

void SendOp::restartResponseTimer()
{
    auto& state = client().configState();
    m_responseTimer.wait(state.m_responseTimeoutMs, &SendOp::recvTimeoutCb, this);
}

void SendOp::responseTimeoutInternal()
{
    COMMS_ASSERT(!m_responseTimer.isActive());
    errorLog("Timeout on publish acknowledgement from broker.");
    if (m_totalSendAttempts <= m_sendAttempts) {
        errorLog("Exhauses all attempts to publish message, discarding publish.");
        reportPubComplete(CC_Mqtt5AsyncOpStatus_Timeout);
        opComplete();
        return;
    }

    if (!m_acked) {
        m_pubMsg.transportField_flags().field_dup().setBitValue_bit(true);
        auto result = client().sendMessage(m_pubMsg); 
        if (result != CC_Mqtt5ErrorCode_Success) {
            errorLog("Failed to resend PUBLISH message.");
            reportPubComplete(CC_Mqtt5AsyncOpStatus_InternalError);
            opComplete();
            return;
        }

        ++m_sendAttempts;
        restartResponseTimer();
        return;
    }

    COMMS_ASSERT(m_pubMsg.transportField_flags().field_qos().value() == PublishMsg::TransportField_flags::Field_qos::ValueType::ExactlyOnceDelivery);
    PubrelMsg pubrelMsg;
    pubrelMsg.field_packetId().setValue(m_pubMsg.field_packetId().field().value());
    auto result = client().sendMessage(pubrelMsg); 
    if (result != CC_Mqtt5ErrorCode_Success) {
        errorLog("Failed to resend PUBREL message.");
        reportPubComplete(CC_Mqtt5AsyncOpStatus_InternalError);
        opComplete();
        return;
    }

    ++m_sendAttempts;
    restartResponseTimer();
}

void SendOp::reportPubComplete(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response)
{
    if (m_cb == nullptr) {
        return;
    }

    m_cb(m_cbData, status, response);
}

void SendOp::confirmRegisteredAlias()
{
    COMMS_ASSERT(!m_pubMsg.field_topic().value().empty());
    COMMS_ASSERT(m_registeredAlias);
    auto& state = client().sessionState();
    auto& topic = m_pubMsg.field_topic().value();
    auto iter = 
        std::lower_bound(
            state.m_sendTopicAliases.begin(), state.m_sendTopicAliases.end(), topic,
            [](auto& info, auto& topicParam)
            {
                return info.m_topic < topicParam;
            });    

    if (iter == state.m_sendTopicAliases.end()) {
        errorLog("Topic alias freed before it is acknowledged");
        return;
    }

    if (iter->m_highQosRegRemCount > 0U) {
        --(iter->m_highQosRegRemCount);
    }
}

void SendOp::recvTimeoutCb(void* data)
{
    asSendOp(data)->responseTimeoutInternal();
}

} // namespace op

} // namespace cc_mqtt5_client
