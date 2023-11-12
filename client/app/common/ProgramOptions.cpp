//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ProgramOptions.h"

namespace po = boost::program_options;

namespace cc_mqtt5_client_app
{

void ProgramOptions::addCommon(boost::program_options::options_description& desc)
{
    po::options_description opts("Common Options");
    opts.add_options()
        ("help,h", "Display help message")
        ("client-id", po::value<std::string>(), "Client ID")
    ;    

    desc.add(opts);
}

bool ProgramOptions::parseArgs(int argc, const char* argv[], const boost::program_options::options_description& desc)
{
    po::store(po::parse_command_line(argc, argv, desc), m_vm);
    po::notify(m_vm);  

    // TODO: check all set  

    return true;
}

bool ProgramOptions::helpRequested() const
{
    return m_vm.count("help") > 0U;
}

} // namespace cc_mqtt5_client_app
