//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/RecvOp.h"
#include "ClientImpl.h"

namespace cc_mqtt5_client
{

namespace op
{

namespace 
{

inline RecvOp* asRecvOp(void* data)
{
    return reinterpret_cast<RecvOp*>(data);
}

} // namespace     

RecvOp::RecvOp(ClientImpl& client) : 
    Base(client),
    m_responseTimer(client.timerMgr().allocTimer()),
    m_info(CC_Mqtt5MessageInfo())
{
    COMMS_ASSERT(m_responseTimer.isValid());
}    

void RecvOp::handle(PublishMsg& msg)
{
    PropsHandler propsHandler;
    for (auto& p : msg.field_propertiesList().value()) {
        p.currentFieldExec(propsHandler);
    }

    if (propsHandler.isProtocolError()) {
        protocolErrorTermination();
        return;
    }     

    auto& topic = msg.field_topic().value();
    if (topic.empty() && (propsHandler.m_topicAlias == nullptr)) {
        protocolErrorTermination();
        return;        
    }

    using Qos = PublishMsg::TransportField_flags::Field_qos::ValueType;
    auto qos = msg.transportField_flags().field_qos().value();

    if (qos > Qos::ExactlyOnceDelivery) {
        protocolErrorTermination();
        return;         
    }

    auto* topicPtr = &topic;
    do {
        if (propsHandler.m_topicAlias == nullptr) {
            break;
        }

        auto topicAlias = propsHandler.m_topicAlias->field_value().value();
        if ((topicAlias == 0U) ||
            (client().state().m_maxRecvTopicAlias < topicAlias)) {
            errorLog("Broker used invalid topic alias.");
            terminationWithReason(DiconnectReason::TopicAliasInvalid);
            return;
        }

        auto& recvTopicAliases = client().state().m_recvTopicAliases;

        if (!topic.empty()) {
            recvTopicAliases.resize(std::max(recvTopicAliases.size(), std::size_t(topicAlias + 1U)));
            recvTopicAliases[topicAlias] = topic;
            break;
        }   

        if ((recvTopicAliases.size() <= topicAlias) ||
            (recvTopicAliases[topicAlias].empty())) {
            errorLog("Broker used unknown topic alias.");
            protocolErrorTermination();
            return;
        }

        topicPtr = &recvTopicAliases[topicAlias];
    } while (false);

    COMMS_ASSERT(topicPtr != nullptr);
    m_info.m_topic = topicPtr->c_str();
    auto& data = msg.field_payload().value();
    comms::cast_assign(m_info.m_dataLen) = data.size();
    if (!data.empty()) {
        m_info.m_data = &data[0];
    }

    if (propsHandler.m_responseTopic != nullptr) {
        m_info.m_responseTopic = propsHandler.m_responseTopic->field_value().value().c_str();
    }

    if (propsHandler.m_correlationData != nullptr) {
        auto& correlationData = propsHandler.m_correlationData->field_value().value();
        comms::cast_assign(m_info.m_correlationDataLen) = correlationData.size();
        if (!correlationData.empty()) {
            m_info.m_correlationData = &correlationData[0];
        }
    }

    if ((!propsHandler.m_userProps.empty()) && (qos < Qos::ExactlyOnceDelivery)) {
        fillUserProps(propsHandler, m_userProps);
        comms::cast_assign(m_info.m_userPropsCount) = m_userProps.size();
        m_info.m_userProps = &m_userProps[0];
    }    

    if (propsHandler.m_contentType != nullptr) {
        auto& contentType = propsHandler.m_contentType->field_value().value();
        if (!contentType.empty()) {
            m_info.m_contentType = contentType.c_str();    
        }        
    }

    if (!propsHandler.m_subscriptionIds.empty()) {
        m_subIds.reserve(propsHandler.m_subscriptionIds.size());
        for (auto* id : propsHandler.m_subscriptionIds) {
            m_subIds.push_back(id->field_value().value());
        }

        m_info.m_subIds = &m_subIds[0];
        comms::cast_assign(m_info.m_subIdsCount) = m_subIds.size();
    }

    comms::cast_assign(m_info.m_qos) = qos;
    
    if (propsHandler.m_payloadFormatIndicator != nullptr) {
        comms::cast_assign(m_info.m_format) = propsHandler.m_payloadFormatIndicator->field_value().value();
    }

    m_info.m_retained = msg.transportField_flags().field_retain().getBitValue_bit();

    if (qos == Qos::AtMostOnceDelivery) {
        reportMsgInfoAndComplete();
        return;
    }

    if (!msg.field_packetId().doesExist()) {
        [[maybe_unused]] static constexpr bool ProtocolDecodingError = false;
        COMMS_ASSERT(ProtocolDecodingError);
        terminationWithReason(DiconnectReason::UnspecifiedError);
        return;
    }    

    if (qos == Qos::AtLeastOnceDelivery) {
        PubackMsg pubackMsg;
        pubackMsg.field_packetId().value() = msg.field_packetId().field().value();
        sendMessage(pubackMsg);
        reportMsgInfoAndComplete();
        return;
    }    

    comms::util::assign(m_topicStr, topicPtr->begin(), topicPtr->end());
    m_info.m_topic = m_topicStr.c_str();

    comms::util::assign(m_data, data.begin(), data.end());
    if (!m_data.empty()) {
        m_info.m_data = &m_data[0];
    }

    if (propsHandler.m_responseTopic != nullptr) {
        auto& responseTopic = propsHandler.m_responseTopic->field_value().value();
        comms::util::assign(m_responseTopic, responseTopic.begin(), responseTopic.end());
        m_info.m_responseTopic = m_responseTopic.c_str();
    }    

    if (propsHandler.m_correlationData != nullptr) {
        auto& correlationData = propsHandler.m_correlationData->field_value().value();
        if (!correlationData.empty()) {
            comms::util::assign(m_correlationData, correlationData.begin(), correlationData.end());
            m_info.m_correlationData = &m_correlationData[0];
        }
    }    

    if (!propsHandler.m_userProps.empty()) {
        m_userPropsCpy.resize(propsHandler.m_userProps.size());
        m_userProps.resize(propsHandler.m_userProps.size());
        for (auto idx = 0U; idx < propsHandler.m_userProps.size(); ++idx) {
            auto* srcFieldPtr = propsHandler.m_userProps[idx];
            auto& srcKey = srcFieldPtr->field_value().field_first().value();
            auto& srcValue = srcFieldPtr->field_value().field_second().value();
            auto& tgtStorage = m_userPropsCpy[idx];

            comms::util::assign(tgtStorage.m_key, srcKey.begin(), srcKey.end());
            comms::util::assign(tgtStorage.m_value, srcValue.begin(), srcValue.end());

            m_userProps[idx].m_key = tgtStorage.m_key.c_str();
            m_userProps[idx].m_value = tgtStorage.m_value.c_str();
        }
    }     

    if (propsHandler.m_contentType != nullptr) {
        auto& contentType = propsHandler.m_contentType->field_value().value();
        if (!contentType.empty()) {
            comms::util::assign(m_contentType, contentType.begin(), contentType.end());
            m_info.m_contentType = m_contentType.c_str();
        }
    }    

    m_packetId = msg.field_packetId().field().value();
    PubrecMsg pubrecMsg;
    pubrecMsg.field_packetId().setValue(m_packetId);
    sendMessage(pubrecMsg);
    restartResponseTimer();
}

void RecvOp::handle(PubrelMsg& msg)
{
    if (msg.field_packetId().value() != m_packetId) {
        return;
    }

    m_responseTimer.cancel();

    if (!msg.doValid()) {
        protocolErrorTermination();
        return;
    }

    if ((msg.field_reasonCode().doesExist()) && 
        (msg.field_reasonCode().field().value() != PubrelMsg::Field_reasonCode::Field::ValueType::Success)) {
        errorLog("Publish reception terminated due to error reason code in PUBREL message.");
        opComplete();
        return;
    }

    if (msg.field_propertiesList().doesExist()) {
        PropsHandler propsHandler;
        for (auto& p : msg.field_propertiesList().field().value()) {
            p.currentFieldExec(propsHandler);
        }

        if (propsHandler.m_reasonStr != nullptr) {
            auto& reasonStr = propsHandler.m_reasonStr->field_value().value();
            if (!reasonStr.empty()) {
                m_info.m_reasonStr = reasonStr.c_str();    
            }        
        }    

        if (!propsHandler.m_userProps.empty()) {
            fillUserProps(propsHandler, m_userProps);
            comms::cast_assign(m_info.m_userPropsCount) = m_userProps.size();
            m_info.m_userProps = &m_userProps[0];
        }     
    }   

    PubcompMsg pubcompMsg;
    pubcompMsg.field_packetId().setValue(m_packetId);
    sendMessage(pubcompMsg);
    reportMsgInfoAndComplete();
}

void RecvOp::reset()
{
    m_responseTimer.cancel();
    m_topicStr.clear();
    m_data.clear();
    m_responseTopic.clear();
    m_correlationData.clear();
    m_userPropsCpy.clear();
    m_userProps.clear();
    m_contentType.clear();
    m_subIds.clear();
    m_info = CC_Mqtt5MessageInfo();
}

Op::Type RecvOp::typeImpl() const
{
    return Type_Recv;
}

void RecvOp::restartResponseTimer()
{
    auto& state = client().state();
    m_responseTimer.wait(state.m_responseTimeoutMs, &RecvOp::recvTimeoutCb, this);
}

void RecvOp::responseTimeoutInternal()
{
    // When there is no response from broker, just terminate the reception.
    // The retry will be initiated by the broker.
    COMMS_ASSERT(!m_responseTimer.isActive());
    errorLog("Timeout on PUBREL reception from broker.");
    opComplete();
}

void RecvOp::recvTimeoutCb(void* data)
{
    asRecvOp(data)->responseTimeoutInternal();
}

void RecvOp::reportMsgInfoAndComplete()
{
    client().reportMsgInfo(m_info);
    opComplete();
}

} // namespace op

} // namespace cc_mqtt5_client
