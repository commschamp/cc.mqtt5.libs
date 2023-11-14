//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ProgramOptions.h"

#include <iostream>

namespace po = boost::program_options;

namespace cc_mqtt5_client_app
{

void ProgramOptions::addCommon()
{
    po::options_description opts("Common Options");
    opts.add_options()
        ("help,h", "Display help message")
        ("verbose,v", "Verbose output")
        ("client-id,i", po::value<std::string>()->default_value(std::string()), "Client ID")
    ;    

    m_desc.add(opts);
}

void ProgramOptions::addNetwork(std::uint16_t port)
{
    po::options_description opts("Network Options");
    opts.add_options()
        ("host,t", po::value<std::string>()->default_value("127.0.0.1"), "Broker address to connect to")
        ("port,p", po::value<std::uint16_t>()->default_value(port), "Network port")
    ;    

    m_desc.add(opts);
}

void ProgramOptions::printHelp()
{
    std::cout << m_desc << std::endl;
}

bool ProgramOptions::parseArgs(int argc, const char* argv[])
{
    po::store(po::parse_command_line(argc, argv, m_desc), m_vm);
    po::notify(m_vm);  

    // TODO: check all set  

    return true;
}

bool ProgramOptions::helpRequested() const
{
    return m_vm.count("help") > 0U;
}

bool ProgramOptions::verbose() const
{
    return m_vm.count("verbose") > 0U;
}

std::string ProgramOptions::clientId() const
{
    return m_vm["client-id"].as<std::string>();
}

ProgramOptions::ConnectionType ProgramOptions::connectionType() const
{
    // Hardcoded for now
    return ConnectionType_Tcp;
}

std::string ProgramOptions::networkAddress() const
{
    return m_vm["host"].as<std::string>();
}

std::uint16_t ProgramOptions::networkPort() const
{
    return m_vm["port"].as<std::uint16_t>();
}

} // namespace cc_mqtt5_client_app
