//
// Copyright 2026 - 2026 (C). Alex Robenko. All rights reserved.
//
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "PubStressMgr.h"

#include <algorithm>
#include <iostream>

namespace cc_mqtt5_client_app
{

PubStressMgr::PubStressMgr(boost::asio::io_context& io, PubStressProgramOptions& opts, int& result) :
    m_io(io),
    m_opts(opts),
    m_result(result)
{
}

bool PubStressMgr::start(int argc, const char* argv[])
{
    if (!m_opts.parseArgs(argc, argv)) {
        std::cerr << "ERROR: Failed to parse arguments." << std::endl;
        return false;
    }

    if (m_opts.helpRequested()) {
        std::cout << "Usage: " << argv[0] << " [options...]" << '\n';
        m_opts.printHelp();
        m_io.stop();
        return true;
    }

    m_clients.resize(std::max(1U, m_opts.clientCount()));
    for (auto idx = 0U; idx < m_clients.size(); ++idx) {
        auto& c = m_clients[idx];
        c = std::make_unique<PubStressClient>(m_io, m_opts, m_result, idx);
        if (!c->startSession()) {
            std::cerr << "ERROR: Failed to start execution of client " << idx << std::endl;
            return false;
        }
    }
    
    return true;
}

} // namespace cc_mqtt5_client_app