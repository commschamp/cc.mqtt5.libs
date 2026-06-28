//
// Copyright 2026 - 2026 (C). Alex Robenko. All rights reserved.
//
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "PubStressClient.h"
#include "PubStressProgramOptions.h"

#include <boost/asio.hpp>

#include <vector>

namespace cc_mqtt5_client_app
{

class PubStressMgr
{
public:
    PubStressMgr(boost::asio::io_context& io, PubStressProgramOptions& opts, int& result);

    bool start(int argc, const char* argv[]);

private:
    boost::asio::io_context& m_io;
    PubStressProgramOptions& m_opts;
    int& m_result;
    std::vector<PubStressClientPtr> m_clients;
};

} // namespace cc_mqtt5_client_app