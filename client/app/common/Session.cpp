//
// Copyright 2023 - 2025 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Session.h"

#include "TcpSession.h"

#include <iostream>
#include <type_traits>

namespace cc_mqtt5_client_app
{

Session::Ptr Session::create(boost::asio::io_context& io, const ProgramOptions& opts)
{
    using CreateFunc = Ptr (*)(boost::asio::io_context&, const ProgramOptions&);
    static const CreateFunc Map[] = {
        /* ConnectionType_Tcp */ &TcpSession::create
    };
    static constexpr std::size_t MapSize = std::extent<decltype(Map)>::value;
    static_assert(MapSize == ProgramOptions::ConnectionType_ValuesLimit);

    auto idx = static_cast<unsigned>(opts.connectionType());
    if (MapSize <= idx) {
        return Ptr();
    }

    auto func = Map[idx];
    return func(io, opts);
}

Session::Session(boost::asio::io_context& io, const ProgramOptions& opts) :
    m_io(io),
    m_opts(opts)
{
}

std::ostream& Session::logError()
{
    return std::cerr << "ERROR: ";
}

unsigned Session::reportData(const std::uint8_t* buf, std::size_t bufLen)
{
    assert(m_dataReportCb);
    return m_dataReportCb(buf, bufLen);
}

void Session::reportNetworkDisconnected()
{
    if (m_networkDisconnected) {
        return;
    }

    m_networkDisconnected = true;
    
    assert(m_networkDisconnectedReportCb);
    m_networkDisconnectedReportCb();
}

} // namespace cc_mqtt5_client_app
