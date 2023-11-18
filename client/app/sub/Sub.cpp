//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Sublic
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Sub.h"

#include <iostream>

namespace cc_mqtt5_client_app
{

namespace 
{

Sub* asThis(void* data)
{
    return reinterpret_cast<Sub*>(data);
}

} // namespace 
    

Sub::Sub(boost::asio::io_context& io, int& result) : 
    Base(io, result)
{
    opts().addCommon();
    opts().addNetwork();
    opts().addConnect();
    opts().addSubscribe();
}    

void Sub::brokerConnectedImpl()
{
    auto topics = opts().subTopics();
    auto qoses = opts().subQoses();

    auto ec = CC_Mqtt5ErrorCode_InternalError;
    auto* subscribe = ::cc_mqtt5_client_subscribe_prepare(client(), &ec);
    if (subscribe == nullptr) {
        logError() << "Failed to allocate subscribe message with ec=" << toString(ec) << std::endl;
        doTerminate();
        return;
    }

    for (auto idx = 0U; idx < topics.size(); ++idx) {
        auto topicConfig = CC_Mqtt5SubscribeTopicConfig();
        ::cc_mqtt5_client_subscribe_init_config_topic(&topicConfig);        
        topicConfig.m_topic = topics[idx].c_str();

        if (idx < qoses.size()) {
            topicConfig.m_maxQos = static_cast<decltype(topicConfig.m_maxQos)>(qoses[idx]);
        }

        ec = ::cc_mqtt5_client_subscribe_config_topic(subscribe, &topicConfig);
        if (ec != CC_Mqtt5ErrorCode_Success) {
            logError() << "Failed to configure topic \"" << topics[idx] << "\" with ec=" << toString(ec) << std::endl;
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
            logError() << "Failed to configure extra subscribe properties with ec=" << toString(ec) << std::endl;
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
            logError() << "Failed to add subscribe user property with ec=" << toString(ec) << std::endl;
            doTerminate();
            return;
        }         
    }     

    ec = ::cc_mqtt5_client_subscribe_send(subscribe, &Sub::subscribeCompleteCb, this);
    if (ec != CC_Mqtt5ErrorCode_Success) {
        logError() << "Failed to send SUBSCRIBE message with ec=" << toString(ec) << std::endl;
        doTerminate();
        return;
    }        
}

void Sub::messageReceivedImpl(const CC_Mqtt5MessageInfo* info)
{
    assert(info != nullptr);
    assert(info->m_topic != nullptr);

    if (opts().verbose()) {
        print(*info);
    }   
    else {
        std::cout << info->m_topic << ": " << toString(info->m_data, info->m_dataLen, opts().subBinary()) << std::endl;
    }
}

void Sub::subscribeCompleteInternal(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response)
{
    if (status != CC_Mqtt5AsyncOpStatus_Complete) {
        logError() << "Subscribe failed with status=" << toString(status) << std::endl;
        doTerminate();
        return;
    }

    assert(response != nullptr);
    for (auto idx = 0U; idx < response->m_reasonCodesCount; ++idx) {
        auto reasonCode = response->m_reasonCodes[idx];
        if (reasonCode >= CC_Mqtt5ReasonCode_UnspecifiedError) {
            auto topics = opts().subTopics();
            assert(idx < topics.size());
            std::cerr << "Failed to subscribe to topic \"" << topics[idx] << "\" with reason code: " << toString(reasonCode) << std::endl;
        }
    }

    if (opts().verbose()) {
        print(*response);
    }    

    // Listening to the messages    
}

void Sub::subscribeCompleteCb(void* data, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response)
{
    asThis(data)->subscribeCompleteInternal(status, response);
}


} // namespace cc_mqtt5_client_app
