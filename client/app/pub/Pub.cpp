//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Pub.h"

#include <iostream>

namespace po = boost::program_options;

namespace cc_mqtt5_client_app
{

Pub::Pub(boost::asio::io_context& io) : 
    Base(io)
{
}    

bool Pub::start(int argc, const char* argv[])
{
    po::options_description desc;
    m_opts.addCommon(desc);
    if (!m_opts.parseArgs(argc, argv, desc)) {
        return false;
    }

    if (m_opts.helpRequested()) {
        std::cout << desc << std::endl;
        io().stop();
        return true;
    }

    static_cast<void>(argc);
    static_cast<void>(argv);    
    return true;
}

} // namespace cc_mqtt5_client_app
