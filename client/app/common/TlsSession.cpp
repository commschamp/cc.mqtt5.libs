//
// Copyright 2023 - 2025 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifdef CC_MQTT5_CLIENT_APP_HAS_OPENSSL

#include "TlsSession.h"

#include <algorithm>
#include <cassert>

namespace cc_mqtt5_client_app
{

Session::Ptr TlsSession::create(boost::asio::io_context& io, const ProgramOptions& opts)
{
    return Ptr(new TlsSession(io, opts));
}

bool TlsSession::startImpl()
{
    boost::asio::ip::tcp::resolver resolver(io());
    boost::system::error_code ec;
    auto endpoints = resolver.resolve(opts().networkAddress(), std::to_string(opts().networkPort()), ec);
    if (ec) {
        logError() << "Failed to resolve address: " << ec.message() << std::endl;
        return false;
    }

    if (!configureSslContext()) {
        return false;
    }

    assert(m_ctx);
    m_socket = std::make_unique<Socket>(io(), *m_ctx);

    boost::asio::connect(m_socket->lowest_layer(), endpoints, ec);
    if (ec) {
        logError() << "Failed to connect: " << ec.message() << std::endl;
        return false;
    }   
    
    m_socket->handshake(Socket::handshake_type::client, ec);
    if (ec) {
        logError() << "Failed to perform SSL/TLS handshake: " << ec.message() << std::endl;
        return false;
    }      

    doRead();
    return true;
}

void TlsSession::sendDataImpl(const std::uint8_t* buf, std::size_t bufLen)
{
    boost::system::error_code ec;
    auto written = boost::asio::write(*m_socket, boost::asio::buffer(buf, bufLen), ec);
    do {

        if (ec) {
            if (ec == boost::asio::error::operation_aborted) {
                return;
            }

            logError() << "Failed to write data: " << ec.message() << std::endl;
            break;
        }

        if (written != bufLen) {
            logError() << "Not all data has been written." << std::endl;
            break;
        }

        return;
    } while (false);

    reportNetworkDisconnected();
}

TlsSession::TlsSession(boost::asio::io_context& io, const ProgramOptions& opts) : 
    Base(io, opts)
{
}

void TlsSession::doRead()
{
    m_socket->async_read_some(
        boost::asio::buffer(m_inData),
        [this](const boost::system::error_code& ec, std::size_t bytesCount)
        {
            if (ec == boost::asio::error::operation_aborted) {
                return;
            }

            if (ec) {
                logError() << "Failed to read data: " << ec.message() << std::endl;
                reportNetworkDisconnected();
                return;
            }

            auto buf = &m_inData[0];
            auto bufLen = bytesCount;
            if (!m_buf.empty()) {
                m_buf.reserve(m_buf.size() + bytesCount);
                m_buf.insert(m_buf.end(), m_inData.begin(), m_inData.begin() + bytesCount);
                buf = &m_buf[0];
                bufLen = m_buf.size();
            }

            auto consumed = reportData(buf, bufLen);
            do {
                if (bufLen <= consumed) {
                    m_buf.clear();
                    break;
                }

                if (!m_buf.empty()) {
                    assert(buf == &m_buf[0]);
                    m_buf.erase(m_buf.begin(), m_buf.begin() + consumed);
                    break;
                }

                assert(buf == &m_inData[0]);
                m_buf.reserve(m_buf.size() + (bytesCount - consumed));
                m_buf.insert(m_buf.end(), m_inData.begin() + consumed, m_inData.begin() + bytesCount);
            } while (false);

            doRead();
        }
    );
}

bool TlsSession::configureSslContext()
{
    m_ctx = std::make_unique<SslContext>(SslContext::sslv23_client);

    return 
        configureSslCaCert() &&
        configureSslKey() &&
        configureSslCert();
}

bool TlsSession::configureSslCaCert()
{
    assert(m_ctx);
    auto tlsCa = opts().tlsCa();
    auto verifyMode = boost::asio::ssl::verify_none;

    boost::system::error_code ec;
    if (!tlsCa.empty()) {
        m_ctx->load_verify_file(tlsCa, ec);
        if (ec) {
            logError() << "Failed to parce CA file: " << ec.message() << std::endl;
            return false;            
        }

        verifyMode = boost::asio::ssl::verify_peer;
    }

    m_ctx->set_verify_mode(verifyMode, ec);
    if (ec) {
        logError() << "Failed to update verify mode: " << ec.message() << std::endl;
        return false;            
    }    

    return true;
}

bool TlsSession::configureSslKey()
{
    auto key = opts().tlsPrivateKey();
    if (key.empty()) {
        return true;
    }

    assert(m_ctx);

    boost::system::error_code ec;
    auto pass = opts().tlsPrivateKeyPass();
    if (!pass.empty()) {
        m_ctx->set_password_callback(
            [pass](std::size_t maxLen, [[maybe_unused]] auto purpose)
            {
                auto ret = pass;
                ret.resize(std::min(pass.size(), maxLen));
                return ret;
            },
            ec);

        if (ec) {
            logError() << "Failed to set password callback: " << ec.message() << std::endl;
            return false;            
        }        
    }

    m_ctx->use_private_key_file(key, SslContext::pem, ec);
    if (ec) {
        logError() << "Failed to update private key: " << ec.message() << std::endl;
        return false;            
    }


    return true;
}

bool TlsSession::configureSslCert()
{
    auto cert = opts().tlsCert();
    if (cert.empty()) {
        return true;
    }

    assert(m_ctx);
    boost::system::error_code ec;
    m_ctx->use_certificate_file(cert, SslContext::pem, ec);
    if (ec) {
        logError() << "Failed to update certificate: " << ec.message() << std::endl;
        return false;            
    }

    return true;
}


} // namespace cc_mqtt5_client_app

#endif // #ifdef CC_MQTT5_CLIENT_APP_HAS_OPENSSL
