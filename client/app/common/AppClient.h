//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "client.h"

#include <boost/asio.hpp>

#include <functional>
#include <iosfwd>

namespace cc_mqtt5_client_app
{

class AppClient
{
    struct ClientDeleter
    {
        void operator()(CC_Mqtt5Client* ptr)
        {
            ::cc_mqtt5_client_free(ptr);
        }
    };

protected:
    explicit AppClient(boost::asio::io_context& io);
    ~AppClient() = default;

    CC_Mqtt5ClientHandle client()
    {
        return m_client.get();
    }

    boost::asio::io_context& io()
    {
        return m_io;
    }

    using ConnectCompleteCb = std::function<void (CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response)>;
    bool asyncConnect(
        CC_Mqtt5ConnectBasicConfig* basic,
        CC_Mqtt5ConnectWillConfig* will,
        CC_Mqtt5ConnectExtraConfig* extra,
        ConnectCompleteCb&& cb
    );

private:
    using ClientPtr = std::unique_ptr<CC_Mqtt5Client, ClientDeleter>;

    static std::ostream& logError();

    static void connectCompleteCb(void* data, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response);

    boost::asio::io_context& m_io;
    ClientPtr m_client;
    ConnectCompleteCb m_connectCompleteCb;
};

} // namespace cc_mqtt5_client_app
