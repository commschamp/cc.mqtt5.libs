//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/SendOp.h"
#include "ClientImpl.h"

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

CC_Mqtt5ErrorCode SendOp::configBasic(const CC_Mqtt5PublishBasicConfig& config)
{
    if ((config.m_topic == nullptr) || (config.m_topic[0] == '\0')) {
        errorLog("Topic hasn't been provided in publish configuration");
        return CC_Mqtt5ErrorCode_BadParam;
    }

    unsigned alias = 0U;
    bool mustAssignTopic = true;
    do {
        if (config.m_topicAliasPref == CC_Mqtt5TopicAliasPreference_ForceTopicOnly) {
            break;
        }

        auto& state = client().state();
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
    } while (false);

    m_pubMsg.transportField_flags().field_qos().setValue(config.m_qos);
    
    if (mustAssignTopic) {
        m_pubMsg.field_topic().value() = config.m_topic;
    }

    if (config.m_qos > CC_Mqtt5QoS_AtMostOnceDelivery) {
        m_pubMsg.field_packetId().field().setValue(allocPacketId());
    }

    auto& propsField = m_pubMsg.field_propertiesList();
    if (alias > 0U) {
        if (!canAddProp(propsField)) {
            errorLog("Cannot add topic alias property, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_topicAlias();
        auto& valueField = propBundle.field_value();
        valueField.setValue(alias);
    }

    if (config.m_dataLen > 0U) {
        COMMS_ASSERT(config.m_data != nullptr);
        comms::util::assign(m_pubMsg.field_payload().value(), config.m_data, config.m_data + config.m_dataLen);
    }

    m_pubMsg.doRefresh(); // Update packetId presence
    return CC_Mqtt5ErrorCode_Success;
}

Op::Type SendOp::typeImpl() const
{
    return Type_Send;
}

void SendOp::restartResponseTimer()
{
    auto& state = client().state();
    m_responseTimer.wait(state.m_responseTimeoutMs, &SendOp::recvTimeoutCb, this);
}

void SendOp::responseTimeoutInternal()
{
    // TODO:
    COMMS_ASSERT(!m_responseTimer.isActive());
    // errorLog("Timeout on PUBREL reception from broker.");
    // opComplete();
}

void SendOp::recvTimeoutCb(void* data)
{
    asSendOp(data)->responseTimeoutInternal();
}

} // namespace op

} // namespace cc_mqtt5_client
