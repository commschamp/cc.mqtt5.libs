//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
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
    opts().addConnect();
    opts().addNetwork();
    opts().addPublish();
}    

void Pub::brokerConnectedImpl()
{
    auto topic = opts().pubTopic();
    auto data = parseBinaryData(opts().pubMessage());

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
        logError() << "Failed to allocate publish message with ec=" << toString(ec) << std::endl;
        doTerminate();
        return;
    }

    ec = ::cc_mqtt5_client_publish_config_basic(publish, &basicConfig);
    if (ec != CC_Mqtt5ErrorCode_Success) {
        logError() << "Failed to perform basic publish configuration with ec=" << toString(ec) << std::endl;
        doTerminate();
        return;
    }

    ec = ::cc_mqtt5_client_publish_send(publish, &Pub::publishCompleteCb, this);
    if (ec != CC_Mqtt5ErrorCode_Success) {
        logError() << "Failed to send PUBLISH message with ec=" << toString(ec) << std::endl;
        doTerminate();
        return;
    }    
}

void Pub::publishCompleteInternal(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response)
{
    if (status != CC_Mqtt5AsyncOpStatus_Complete) {
        logError() << "Publish failed with status=" << toString(status) << std::endl;
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

void Pub::publishCompleteCb(void* data, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response)
{
    asThis(data)->publishCompleteInternal(status, response);
}


} // namespace cc_mqtt5_client_app
