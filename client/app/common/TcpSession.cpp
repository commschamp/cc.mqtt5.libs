//
// Copyright 2023 - 2026 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "TcpSession.h"

#include <cassert>

namespace cc_mqtt5_client_app
{

Session::Ptr TcpSession::create(boost::asio::io_context& io, const ProgramOptions& opts)
{
    return Ptr(new TcpSession(io, opts));
}

bool TcpSession::startImpl()
{
    boost::asio::ip::tcp::resolver resolver(io());
    boost::system::error_code ec;
    auto endpoints = resolver.resolve(opts().networkAddress(), std::to_string(opts().networkPort()), ec);
    if (ec) {
        logError() << "Failed to resolve address: " << ec.message() << std::endl;
        return false;
    }

    boost::asio::connect(m_socket, endpoints, ec);
    if (ec) {
        logError() << "Failed to connect: " << ec.message() << std::endl;
        return false;
    }

    doRead();
    return true;
}

void TcpSession::sendDataImpl(const std::uint8_t* buf, std::size_t bufLen)
{
    boost::system::error_code ec;
    auto written = boost::asio::write(m_socket, boost::asio::buffer(buf, bufLen), ec);
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

void TcpSession::doRead()
{
    m_socket.async_read_some(
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

} // namespace cc_mqtt5_client_app
