//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "AppClient.h"

#include <cassert>
#include <iostream>

namespace cc_mqtt5_client_app
{

namespace 
{

AppClient* asThis(void* data)
{
    return reinterpret_cast<AppClient*>(data);
}

} // namespace 
    

AppClient::AppClient(boost::asio::io_context& io) : 
    m_io(io),
    m_client(::cc_mqtt5_client_alloc())
{
    static_cast<void>(m_io);
    assert(m_client);
}

bool AppClient::asyncConnect(
    CC_Mqtt5ConnectBasicConfig* basic,
    CC_Mqtt5ConnectWillConfig* will,
    CC_Mqtt5ConnectExtraConfig* extra,
    ConnectCompleteCb&& cb)
{
    if (!cb) {
        logError() << "Connection complete callback is not provided" << std::endl;
        return false;
    }

    auto ec = CC_Mqtt5ErrorCode_Success;
    auto connect = ::cc_mqtt5_client_connect_prepare(m_client.get(), &ec);
    if (!connect) {
        logError() << "Failed to allocate client with ec=" << ec << std::endl;
        return false;
    }
    
    if (basic != nullptr) {
        ec = ::cc_mqtt5_client_connect_config_basic(connect, basic);
        if (ec != CC_Mqtt5ErrorCode_Success) {
            logError() << "Failed to apply basic connect configuration with ec=" << ec << std::endl;
            return false;
        }
    }

    if (will != nullptr) {
        ec = ::cc_mqtt5_client_connect_config_will(connect, will);
        if (ec != CC_Mqtt5ErrorCode_Success) {
            logError() << "Failed to apply will connect configuration with ec=" << ec << std::endl;
            return false;
        }        
    }

    if (extra != nullptr) {
        ec = ::cc_mqtt5_client_connect_config_extra(connect, extra);
        if (ec != CC_Mqtt5ErrorCode_Success) {
            logError() << "Failed to apply extra connect configuration with ec=" << ec << std::endl;
            return false;
        }        
    }    

    ec = ::cc_mqtt5_client_connect_send(connect, &AppClient::connectCompleteCb, this);
    if (ec != CC_Mqtt5ErrorCode_Success) {
        logError() << "Failed to send connect request with ec=" << ec << std::endl;
        return false;
    }    

    m_connectCompleteCb = std::move(cb);    
    return true;
}

std::ostream& AppClient::logError()
{
    return std::cerr << "ERROR: ";
}

void AppClient::connectCompleteCb(void* data, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response)
{
    asThis(data)->m_connectCompleteCb(status, response);
}

} // namespace cc_mqtt5_client_app
