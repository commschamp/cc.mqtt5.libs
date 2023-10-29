//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/RecvOp.h"
#include "ClientImpl.h"

#include <string_view>

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

bool isTopicMatch(std::string_view filter, std::string_view topic)
{
    if ((filter.size() == 1U) && (filter[0] == '#')) {
        return true;
    }

    if (topic.empty()) {
        return filter.empty();
    }

    auto filterSepPos = filter.find_first_of("/");
    auto topicSepPos = topic.find_first_of("/");

    if ((filterSepPos == std::string_view::npos) && 
        (topicSepPos != std::string_view::npos)) {
        return false;
    }

    if (topicSepPos != std::string_view::npos) {
        COMMS_ASSERT(filterSepPos != std::string_view::npos);
        if (((filter[0] == '+') && (filterSepPos == 1U)) || 
            (filter.substr(0, filterSepPos) == topic.substr(0, topicSepPos))) {
            return isTopicMatch(filter.substr(filterSepPos + 1U), topic.substr(topicSepPos + 1U));
        }

        return false;
    }

    if (filterSepPos != std::string_view::npos) {
        COMMS_ASSERT(topicSepPos == std::string_view::npos);
        if (filter.size() <= (filterSepPos + 1U)) {
            // trailing '/' on the filter without any character after that
            return false;
        }

        if (((filter[0] == '+') && (filterSepPos == 1U)) || 
            (filter.substr(0, filterSepPos) == topic)) {
            return isTopicMatch(filter.substr(filterSepPos + 1U), std::string_view());
        }

        return false;
    }

    COMMS_ASSERT(filterSepPos == std::string_view::npos);
    COMMS_ASSERT(topicSepPos == std::string_view::npos);

    return 
        (((filter[0] == '+') && (filter.size() == 1U)) || 
         (filter == topic));
}

bool isTopicMatch(const TopicFilterStr& filter, const TopicStr& topic)
{
    COMMS_ASSERT(!filter.empty());
    COMMS_ASSERT(!topic.empty());
    return isTopicMatch(std::string_view(filter.c_str(), filter.size()), std::string_view(topic.c_str(), topic.size()));
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
    using Qos = PublishMsg::TransportField_flags::Field_qos::ValueType;
    auto qos = msg.transportField_flags().field_qos().value();

    if (qos > Qos::ExactlyOnceDelivery) {
        terminationWithReason(DisconnectReason::QosNotSupported);
        return;
    }

    auto completeNotAuthorized = 
        [this, &msg, qos]()
        {
            auto sendNotAuthorized = 
                [this, &msg](auto& outMsg)
                {
                    outMsg.field_packetId().value() = msg.field_packetId().field().value();
                    outMsg.field_reasonCode().setExists();
                    outMsg.field_reasonCode().field().value() = PubackMsg::Field_reasonCode::Field::ValueType::NotAuthorized;
                    outMsg.field_propertiesList().setExists();
                    sendMessage(outMsg);
                };    

            do {
                if (qos == Qos::AtMostOnceDelivery) {
                    break;
                }    

                if (qos == Qos::AtLeastOnceDelivery) {
                    PubackMsg pubackMsg;
                    sendNotAuthorized(pubackMsg);
                    break;
                }

                PubrecMsg pubrecMsg;
                sendNotAuthorized(pubrecMsg);
                break;
            } while (false);

            opComplete();
        };

    if (!client().sessionState().m_connected) {
        errorLog("Received PUBLISH when not CONNECTED");
        completeNotAuthorized();
        return;
    }

    auto& topic = msg.field_topic().value();
    if ((!topic.empty()) && (!verifyPubTopic(topic.c_str(), false))) {
        errorLog("Received PUBLISH with invalid topic format.");
        protocolErrorTermination();
        return;
    }

    PropsHandler propsHandler;
    for (auto& p : msg.field_propertiesList().value()) {
        p.currentFieldExec(propsHandler);
    }

    if (propsHandler.isProtocolError()) {
        errorLog("Received PUBLISH with protocol error when parsing properties.");
        protocolErrorTermination();
        return;
    }     

    if (topic.empty() && (propsHandler.m_topicAlias == nullptr)) {
        errorLog("Received PUBLISH without topic alias.");
        protocolErrorTermination();
        return;        
    }

    if (qos > Qos::ExactlyOnceDelivery) {
        errorLog("Received PUBLISH with unknown Qos value.");
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
            (client().sessionState().m_maxRecvTopicAlias < topicAlias)) {
            errorLog("Broker used invalid topic alias.");
            terminationWithReason(DisconnectReason::TopicAliasInvalid);
            return;
        }

        auto& recvTopicAliases = client().sessionState().m_recvTopicAliases;

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

    if constexpr (Config::HasSubTopicVerification) {
        if (client().configState().m_verifySubFilter) {
            auto& subFilters = client().reuseState().m_subFilters;
            auto iter = 
                std::find_if(
                    subFilters.begin(), subFilters.end(),
                    [&topicStr = *topicPtr](auto& filter)
                    {
                        return isTopicMatch(filter, topicStr);
                    });

            if (iter == subFilters.end()) {
                errorLog("Received PUBLISH on non-subscribed topic");
                completeNotAuthorized();
                return;                
            }
        }
    }  

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

    if (propsHandler.m_messageExpiryInterval != nullptr) {
        comms::cast_assign(m_info.m_messageExpiryInterval) = propsHandler.m_messageExpiryInterval->field_value().value();
    }

    m_info.m_retained = msg.transportField_flags().field_retain().getBitValue_bit();

    if (qos == Qos::AtMostOnceDelivery) {
        reportMsgInfoAndComplete();
        return;
    }

    if (!msg.field_packetId().doesExist()) {
        [[maybe_unused]] static constexpr bool ProtocolDecodingError = false;
        COMMS_ASSERT(ProtocolDecodingError);
        terminationWithReason(DisconnectReason::UnspecifiedError);
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

        comms::cast_assign(m_info.m_userPropsCount) = m_userProps.size();
        m_info.m_userProps = &m_userProps[0];
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
        errorLog("Received invalid flags in PUBREL message");
        protocolErrorTermination();
        return;
    }

    if (msg.field_propertiesList().doesExist()) {
        PropsHandler propsHandler;
        for (auto& p : msg.field_propertiesList().field().value()) {
            p.currentFieldExec(propsHandler);
        }

        if (propsHandler.m_reasonStr != nullptr) {
            if (!client().sessionState().m_problemInfoAllowed) {
                errorLog("Received reason string in PUBREL when \"problem information\" was disabled in CONNECT.");
                protocolErrorTermination();
                return; 
            }

            errorLog("PUBREL reason info:");
            errorLog(propsHandler.m_reasonStr->field_value().value().c_str());
        }    

        if (!propsHandler.m_userProps.empty()) {
            if (!client().sessionState().m_problemInfoAllowed) {
                errorLog("Received user properties in PUBREL when \"problem information\" was disabled in CONNECT.");
                protocolErrorTermination();
                return; 
            }

            fillUserProps(propsHandler, m_userProps);
            comms::cast_assign(m_info.m_userPropsCount) = m_userProps.size();
            m_info.m_userProps = &m_userProps[0];
        }     
    }   

    if ((msg.field_reasonCode().doesExist()) && 
        (msg.field_reasonCode().field().value() != PubrelMsg::Field_reasonCode::Field::ValueType::Success)) {
        errorLog("Publish reception terminated due to error reason code in PUBREL message.");
        opComplete();
        return;
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

void RecvOp::networkConnectivityChangedImpl()
{
    m_responseTimer.setSuspended(client().sessionState().m_networkDisconnected);
}

void RecvOp::restartResponseTimer()
{
    auto& state = client().configState();
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
