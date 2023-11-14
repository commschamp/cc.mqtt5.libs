//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <boost/program_options.hpp>

#include <cstdint>
#include <string>

namespace cc_mqtt5_client_app
{

class ProgramOptions
{
public:
    using OptDesc = boost::program_options::options_description;
    static constexpr std::uint16_t DefaultPort = 1883U;

    enum ConnectionType
    {
        ConnectionType_Tcp,
        ConnectionType_ValuesLimit
    };

    void addCommon();
    void addNetwork(std::uint16_t port = DefaultPort);

    void printHelp();

    bool parseArgs(int argc, const char* argv[]);

    bool helpRequested() const;
    bool verbose() const;
    std::string clientId() const;

    ConnectionType connectionType() const;
    std::string networkAddress() const;
    std::uint16_t networkPort() const;


private:
    boost::program_options::variables_map m_vm;
    OptDesc m_desc;
};

} // namespace cc_mqtt5_client_app
