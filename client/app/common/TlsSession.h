//
// Copyright 2023 - 2025 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#ifdef CC_MQTT5_CLIENT_APP_HAS_OPENSSL

#include "Session.h"

#include <boost/asio/ssl.hpp>

#include <array>
#include <cstdint>
#include <vector>

namespace cc_mqtt5_client_app
{

class TlsSession final : public Session
{
    using Base = Session;
public:
    static Ptr create(boost::asio::io_context& io, const ProgramOptions& opts);

protected:
    virtual bool startImpl() override;
    virtual void sendDataImpl(const std::uint8_t* buf, std::size_t bufLen) override;

private:
    using SslContext = boost::asio::ssl::context;
    using Socket = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;
    using InDataBuf = std::array<std::uint8_t, 4096>;
    using DataBuf = std::vector<std::uint8_t>;

    TlsSession(boost::asio::io_context& io, const ProgramOptions& opts);
    void doRead();
    bool configureSslContext();
    bool configureSslCaCert();
    bool configureSslKey();
    bool configureSslCert();

    std::unique_ptr<SslContext> m_ctx;
    std::unique_ptr<Socket> m_socket;
    InDataBuf m_inData;
    DataBuf m_buf;
};

} // namespace cc_mqtt5_client_app

#endif // #ifdef CC_MQTT5_CLIENT_APP_HAS_OPENSSL
