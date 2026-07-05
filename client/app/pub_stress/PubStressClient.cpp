//
// Copyright 2026 - 2026 (C). Alex Robenko. All rights reserved.
//
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "PubStressClient.h"

#include <chrono>
#include <iostream>

namespace cc_mqtt5_client_app
{

namespace
{

PubStressClient* asThis(void* data)
{
    return reinterpret_cast<PubStressClient*>(data);
}

} // namespace

PubStressClient::PubStressClient(boost::asio::io_context& io, PubStressProgramOptions& opts, int& result, unsigned threadId, unsigned clientId) :
    Base(io, opts, result),
    m_timer(io),
    m_threadId(threadId),
    m_clientId(clientId),
    m_verbose(opts.verbose())
{
}

void PubStressClient::brokerConnectedImpl()
{
    auto topics = opts().subTopics();
    auto qoses = opts().subQoses();
    bool noRetained = opts().subNoRetained();

    auto ec = CC_Mqtt5ErrorCode_InternalError;
    auto* subscribe = ::cc_mqtt5_client_subscribe_prepare(client(), &ec);
    if (subscribe == nullptr) {
        logError() << "Failed to allocate subscribe message: " << toString(ec) << std::endl;
        doTerminate();
        return;
    }

    for (auto idx = 0U; idx < topics.size(); ++idx) {
        auto topic = topics[idx];
        auto topicSuffix = std::to_string(m_threadId) + '/' + std::to_string(m_clientId);
        if (topic.empty()) {
            topic = topicSuffix;
        }
        else if (topic.back() != '#') {
            topic += '/' + topicSuffix;
        }

        if (m_verbose) {
            logInfo() << "Subscription topic: " << topic << std::endl;
        }

        auto topicConfig = CC_Mqtt5SubscribeTopicConfig();
        ::cc_mqtt5_client_subscribe_init_config_topic(&topicConfig);
        topicConfig.m_topic = topic.c_str();

        if (idx < qoses.size()) {
            topicConfig.m_maxQos = static_cast<decltype(topicConfig.m_maxQos)>(qoses[idx]);
        }

        if (noRetained) {
            topicConfig.m_retainHandling = CC_Mqtt5RetainHandling_DoNotSend;
        }

        ec = ::cc_mqtt5_client_subscribe_config_topic(subscribe, &topicConfig);
        if (ec != CC_Mqtt5ErrorCode_Success) {
            logError() << "Failed to configure topic \"" << topics[idx] << "\": " << toString(ec) << std::endl;
            doTerminate();
            return;
        }
    }

    auto subId = opts().subId();
    if (subId > 0U) {
        auto extraConfig = CC_Mqtt5SubscribeExtraConfig();
        ::cc_mqtt5_client_subscribe_init_config_extra(&extraConfig);
        extraConfig.m_subId = subId;

        ec = ::cc_mqtt5_client_subscribe_config_extra(subscribe, &extraConfig);
        if (ec != CC_Mqtt5ErrorCode_Success) {
            logError() << "Failed to configure extra subscribe properties: " << toString(ec) << std::endl;
            doTerminate();
            return;
        }
    }

    auto props = parseUserProps(opts().subUserProps());
    for (auto& p : props) {
        auto info = CC_Mqtt5UserProp();
        info.m_key = p.m_key.c_str();
        info.m_value = p.m_value.c_str();

        ec = ::cc_mqtt5_client_subscribe_add_user_prop(subscribe, &info);
        if (ec != CC_Mqtt5ErrorCode_Success) {
            logError() << "Failed to add subscribe user property: " << toString(ec) << std::endl;
            doTerminate();
            return;
        }
    }

    ec = ::cc_mqtt5_client_subscribe_send(subscribe, &PubStressClient::subscribeCompleteCb, this);
    if (ec != CC_Mqtt5ErrorCode_Success) {
        logError() << "Failed to send SUBSCRIBE message: " << toString(ec) << std::endl;
        doTerminate();
        return;
    }
}

void PubStressClient::messageReceivedImpl(const CC_Mqtt5MessageInfo* info)
{
    assert(info != nullptr);
    assert(info->m_topic != nullptr);

    if (m_verbose) {
        print(*info);
    }
}

std::string PubStressClient::clientIdImpl() const
{
    return Base::clientIdImpl() + '_' + std::to_string(m_threadId) + '_' + std::to_string(m_clientId);
}

std::string PubStressClient::logPrefixImpl() const
{
    return '(' + std::to_string(m_threadId) + '/' + std::to_string(m_clientId) + ") ";
}

void PubStressClient::subscribeCompleteInternal([[maybe_unused]] CC_Mqtt5SubscribeHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response)
{
    if (status != CC_Mqtt5AsyncOpStatus_Complete) {
        logError() << "Subscribe failed: " << toString(status) << std::endl;
        doTerminate();
        return;
    }

    assert(response != nullptr);
    for (auto idx = 0U; idx < response->m_reasonCodesCount; ++idx) {
        auto reasonCode = response->m_reasonCodes[idx];
        if (reasonCode >= CC_Mqtt5ReasonCode_UnspecifiedError) {
            auto topics = opts().subTopics();
            assert(idx < topics.size());
            std::cerr << "Failed to subscribe with reason code: " << toString(reasonCode) << std::endl;
        }
    }

    if (m_verbose) {
        print(*response);
    }

    // Listening to the messages
    doPublishInternal();
}

void PubStressClient::publishCompleteInternal([[maybe_unused]] CC_Mqtt5PublishHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response)
{
    if (status != CC_Mqtt5AsyncOpStatus_Complete) {
        logError() << "Publish failed: " << toString(status) << std::endl;
        doTerminate();
        return;
    }

    if ((response == nullptr) || (response->m_reasonCode < CC_Mqtt5ReasonCode_UnspecifiedError)) {
        if (m_verbose) {
            logInfo() << "Publish successful" << std::endl;
        }
    }
    else {
        logError() << "Publish failed" << std::endl;
    }

    if ((response != nullptr) && (m_verbose)) {
        print(*response);
    }
}

void PubStressClient::doPublishInternal()
{
    m_timer.expires_after(std::chrono::milliseconds(static_cast<const PubStressProgramOptions&>(opts()).wait()));
    m_timer.async_wait(
        [this](const boost::system::error_code& ec)
        {
            if (ec == boost::asio::error::operation_aborted) {
                return;
            }

            publishMsgInternal();
            doPublishInternal();
        }
    );
}

void PubStressClient::publishMsgInternal()
{
    auto topic = opts().pubTopic();
    if (!topic.empty()) {
        topic += '/';
    }
    topic += std::to_string(m_threadId) + '/' + std::to_string(m_clientId);

    if (m_verbose) {
        logInfo() << "Publishing to topic: " << topic << std::endl;
    }

    auto data = parseBinaryData(opts().pubMessage());
    auto contentType = opts().pubContentType();
    auto responseTopic = opts().pubResponseTopic();
    if (!responseTopic.empty()) {
        responseTopic += '/' + std::to_string(m_threadId) + '/' + std::to_string(m_clientId);
    }

    auto correlationData = parseBinaryData(opts().pubCorrelationData());

    auto basicConfig = CC_Mqtt5PublishBasicConfig();
    ::cc_mqtt5_client_publish_init_config_basic(&basicConfig);
    basicConfig.m_topic = topic.c_str();
    if (!data.empty()) {
        basicConfig.m_data = &data[0];
    }
    basicConfig.m_dataLen = static_cast<decltype(basicConfig.m_dataLen)>(data.size());
    basicConfig.m_qos = static_cast<decltype(basicConfig.m_qos)>(opts().pubQos());
    basicConfig.m_retain = opts().pubRetain();

    auto ec = CC_Mqtt5ErrorCode_InternalError;
    auto* publish = ::cc_mqtt5_client_publish_prepare(client(), &ec);
    if (publish == nullptr) {
        logError() << "Failed to allocate publish message: " << toString(ec) << std::endl;
        doTerminate();
        return;
    }

    ec = ::cc_mqtt5_client_publish_config_basic(publish, &basicConfig);
    if (ec != CC_Mqtt5ErrorCode_Success) {
        logError() << "Failed to perform basic publish configuration: " << toString(ec) << std::endl;
        doTerminate();
        return;
    }

    auto extraConfig = CC_Mqtt5PublishExtraConfig();
    ::cc_mqtt5_client_publish_init_config_extra(&extraConfig);
    if (!contentType.empty()) {
        extraConfig.m_contentType = contentType.c_str();
    }

    if (!responseTopic.empty()) {
        extraConfig.m_responseTopic = responseTopic.c_str();
    }

    if (!correlationData.empty()) {
        extraConfig.m_correlationData = &correlationData[0];
        extraConfig.m_correlationDataLen = static_cast<decltype(extraConfig.m_correlationDataLen)>(correlationData.size());
    }

    extraConfig.m_messageExpiryInterval = opts().pubMessageExpiry();
    extraConfig.m_format = static_cast<decltype(extraConfig.m_format)>(opts().pubMessageFormat());

    ec = ::cc_mqtt5_client_publish_config_extra(publish, &extraConfig);
    if (ec != CC_Mqtt5ErrorCode_Success) {
        logError() << "Failed to perform extra publish configuration: " << toString(ec) << std::endl;
        doTerminate();
        return;
    }

    auto props = parseUserProps(opts().pubUserProps());
    for (auto& p : props) {
        auto info = CC_Mqtt5UserProp();
        info.m_key = p.m_key.c_str();
        info.m_value = p.m_value.c_str();

        ec = ::cc_mqtt5_client_publish_add_user_prop(publish, &info);
        if (ec != CC_Mqtt5ErrorCode_Success) {
            logError() << "Failed to add publish user property: " << toString(ec) << std::endl;
            doTerminate();
            return;
        }
    }

    ec = ::cc_mqtt5_client_publish_send(publish, &PubStressClient::publishCompleteCb, this);
    if (ec != CC_Mqtt5ErrorCode_Success) {
        logError() << "Failed to send PUBLISH message: " << toString(ec) << std::endl;
        doTerminate();
        return;
    }
}

void PubStressClient::subscribeCompleteCb(
    void* data,
    CC_Mqtt5SubscribeHandle handle,
    CC_Mqtt5AsyncOpStatus status,
    const CC_Mqtt5SubscribeResponse* response)
{
    asThis(data)->subscribeCompleteInternal(handle, status, response);
}

void PubStressClient::publishCompleteCb(
    void* data,
    CC_Mqtt5PublishHandle handle,
    CC_Mqtt5AsyncOpStatus status,
    const CC_Mqtt5PublishResponse* response)
{
    asThis(data)->publishCompleteInternal(handle, status, response);
}

} // namespace cc_mqtt5_client_app
