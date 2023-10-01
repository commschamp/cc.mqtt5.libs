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
    m_recvTimer(client.timerMgr().allocTimer()),
    m_info(CC_Mqtt5MessageInfo())
{
    COMMS_ASSERT(m_recvTimer.isValid());
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
            protocolErrorTermination();
            return;
        }

        topicPtr = &recvTopicAliases[topicAlias];
    } while (false);

    // TODO: populate info;
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

    if (propsHandler.m_reasonStr != nullptr) {
        auto& reasonStr = propsHandler.m_reasonStr->field_value().value();
        if (!reasonStr.empty()) {
            m_info.m_reasonStr = reasonStr.c_str();    
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
        pubackMsg.field_reasonCode().value() = PubackMsg::Field_reasonCode::ValueType::Success;
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

    if (propsHandler.m_reasonStr != nullptr) {
        auto& reasonStr = propsHandler.m_reasonStr->field_value().value();
        if (!reasonStr.empty()) {
            comms::util::assign(m_reasonStr, reasonStr.begin(), reasonStr.end());
            m_info.m_reasonStr = m_reasonStr.c_str();
        }
    }      

    PubrecMsg pubrecMsg;
    pubrecMsg.field_packetId().value() = msg.field_packetId().field().value();
    pubrecMsg.field_reasonCode().value() = PubackMsg::Field_reasonCode::ValueType::Success;
    sendMessage(pubrecMsg);
    restartRecvTimer();
}

Op::Type RecvOp::typeImpl() const
{
    return Type_Recv;
}

void RecvOp::restartRecvTimer()
{
    auto& state = client().state();
    m_recvTimer.wait(state.m_responseTimeoutMs, &RecvOp::recvTimeoutCb, this);
}

void RecvOp::recvTimoutInternal()
{
    // When there is no response from broker, just terminate the reception.
    // The retry will be initiated by the broker.
    COMMS_ASSERT(!m_recvTimer.isActive());
    opComplete();
}

void RecvOp::recvTimeoutCb(void* data)
{
    asRecvOp(data)->recvTimoutInternal();
}

void RecvOp::reportMsgInfoAndComplete()
{
    client().reportMsgInfo(m_info);
    opComplete();
}

} // namespace op

} // namespace cc_mqtt5_client