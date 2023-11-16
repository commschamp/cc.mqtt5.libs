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
    ;    

    m_desc.add(opts);
}

void ProgramOptions::addConnect()
{
    po::options_description opts("Connect Options");
    opts.add_options()
        ("client-id,i", po::value<std::string>()->default_value(std::string()), "Client ID")
    ;    

    m_desc.add(opts);
}

void ProgramOptions::addNetwork(std::uint16_t port)
{
    po::options_description opts("Network Options");
    opts.add_options()
        ("broker,b", po::value<std::string>()->default_value("127.0.0.1"), "Broker address to connect to")
        ("port,p", po::value<std::uint16_t>()->default_value(port), "Network port")
    ;    

    m_desc.add(opts);
}

void ProgramOptions::addPublish()
{
    po::options_description opts("Publish Options");
    opts.add_options()
        ("pub-topic,t", po::value<std::string>()->default_value(std::string()), "Publish topic")
        ("pub-msg,m", po::value<std::string>()->default_value(std::string()), "Publish message, use \"\\x\" prefix to specify single byte, "
            "such as \"\\x01\\xb9\\xaf\".")
        ("pub-qos,q", po::value<unsigned>()->default_value(0U), "Publish QoS: 0, 1, or 2")
        ("pub-retain", po::value<bool>()->default_value(false), "Retain the publish message");
    ;    

    m_desc.add(opts);
}

void ProgramOptions::addSubscribe()
{
    po::options_description opts("Subscribe Options");
    opts.add_options()
        ("sub-topic,t", po::value<StringsList>(), "Subscribe topic filter. Can be used multiple times.")
        ("sub-qos,q", po::value<UnsignedsList>(), "Subscribe max QoS: 0, 1, or 2. Defaults to 2. Can be used multiple times "
            "(for each topic filter correspondingly).")
        ("sub-binary", "Force binary output of the received message data")
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

ProgramOptions::ConnectionType ProgramOptions::connectionType() const
{
    // Hardcoded for now
    return ConnectionType_Tcp;
}

std::string ProgramOptions::clientId() const
{
    return m_vm["client-id"].as<std::string>();
}

std::string ProgramOptions::networkAddress() const
{
    return m_vm["broker"].as<std::string>();
}

std::uint16_t ProgramOptions::networkPort() const
{
    return m_vm["port"].as<std::uint16_t>();
}

std::string ProgramOptions::pubTopic() const
{
    return m_vm["pub-topic"].as<std::string>();
}

std::string ProgramOptions::pubMessage() const
{
    return m_vm["pub-msg"].as<std::string>();
}

unsigned ProgramOptions::pubQos() const
{
    return m_vm["pub-qos"].as<unsigned>();
}

bool ProgramOptions::pubRetain() const
{
    return m_vm["pub-retain"].as<bool>();
}

ProgramOptions::StringsList ProgramOptions::subTopics() const
{
    StringsList result;
    auto* id = "sub-topic";
    if (m_vm.count(id) > 0) {
        result = m_vm[id].as<StringsList>();
    }

    return result;
}

ProgramOptions::UnsignedsList ProgramOptions::subQoses() const
{
    UnsignedsList result;
    auto* id = "sub-qos";
    if (m_vm.count(id) > 0) {
        result = m_vm[id].as<UnsignedsList>();
    }

    return result;    
}

bool ProgramOptions::subBinary() const
{
    return m_vm.count("sub-binary") > 0U;
}

} // namespace cc_mqtt5_client_app
