//
// Copyright 2023 - 2025 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Pub.h"

#include <iostream>

namespace cc_mqtt5_client_app
{

namespace 
{

Pub* asThis(void* data)
{
    return reinterpret_cast<Pub*>(data);
}

} // namespace 
    

Pub::Pub(boost::asio::io_context& io, int& result) : 
    Base(io, result)
{
    opts().addCommon();
    opts().addNetwork();
    opts().addConnect();
    opts().addPublish();
}    

void Pub::brokerConnectedImpl()
{
    auto topic = opts().pubTopic();
    auto data = parseBinaryData(opts().pubMessage());
    auto contentType = opts().pubContentType();
    auto responseTopic = opts().pubResponseTopic();
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

    ec = ::cc_mqtt5_client_publish_send(publish, &Pub::publishCompleteCb, this);
    if (ec != CC_Mqtt5ErrorCode_Success) {
        logError() << "Failed to send PUBLISH message: " << toString(ec) << std::endl;
        doTerminate();
        return;
    }    
}

void Pub::publishCompleteInternal([[maybe_unused]] CC_Mqtt5PublishHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response)
{
    if (status != CC_Mqtt5AsyncOpStatus_Complete) {
        logError() << "Publish failed: " << toString(status) << std::endl;
        doTerminate();
        return;
    }

    int result = 1;
    if ((response == nullptr) || (response->m_reasonCode < CC_Mqtt5ReasonCode_UnspecifiedError)) {
        std::cout << "Publish successful" << std::endl;
        result = 0;
    }
    else {
        std::cout << "Publish failed" << std::endl;
    }

    if ((response != nullptr) && (opts().verbose())) {
        print(*response);
    }

    doTerminate(result);
}

void Pub::publishCompleteCb(void* data, CC_Mqtt5PublishHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response)
{
    asThis(data)->publishCompleteInternal(handle, status, response);
}


} // namespace cc_mqtt5_client_app
