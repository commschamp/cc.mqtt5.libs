//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ProgramOptions.h"

#include <algorithm>
#include <iostream>

namespace po = boost::program_options;

namespace cc_mqtt5_client_afl_fuzz
{

ProgramOptions::ProgramOptions()
{
    po::options_description commonOpts("Common Options");
    commonOpts.add_options()
        ("help,h", "Display help message")
        ("verbose,v", "Verbose output")
        ("log-file,f", po::value<std::string>(), "Output log file")
    ;  

    po::options_description connectOpts("Connect Options");
    connectOpts.add_options()
        ("client-id,i", po::value<std::string>()->default_value(std::string()), "Client ID")
        ("receive-max", po::value<unsigned>()->default_value(0), "\"Receive Maximum\" property. 0 means not set.")
        ("max-packet-size", po::value<unsigned>()->default_value(0), "\"Maximum Packet Size\" property. 0 means no limit.")
        ("topic-alias-max", po::value<unsigned>()->default_value(0), "\"Topic Alias Maximum\" property.")
        ("req-response-info", "Set \"Request Response Information\" property flag.")
        ("req-problem-info", "Set \"Request Problem Information\" property flag.")
        ("auth-method", po::value<std::string>()->default_value(std::string()), 
            "Enforce exchange of the custom authentication with re-authentication testing")
    ;  

    po::options_description subOpts("Subscribe Options");
    subOpts.add_options()
        ("sub-topic,t", po::value<StringsList>(), "Subscribe topic filters. Can be used multiple times. "
            "Multiple topics can also be comma separated in single occurrance, to be packed into single subscribe message. "
            "If not specified, single subscribe to \"#\" is assumed")
    ;        

    m_desc.add(commonOpts);
    m_desc.add(connectOpts);  
    m_desc.add(subOpts);
}

bool ProgramOptions::parseArgs(int argc, const char* argv[])
{
    po::store(po::parse_command_line(argc, argv, m_desc), m_vm);
    po::notify(m_vm);  

    return true;
}

void ProgramOptions::printHelp()
{
    std::cout << m_desc << std::endl;
}

bool ProgramOptions::helpRequested() const
{
    return m_vm.count("help") > 0U;
}

bool ProgramOptions::verbose() const
{
    return m_vm.count("verbose") > 0U;
}

bool ProgramOptions::hasLogFile() const
{
    return m_vm.count("log-file") > 0U;
}

std::string ProgramOptions::logFile() const
{
    return m_vm["log-file"].as<std::string>();
}

std::string ProgramOptions::clientId() const
{
    return m_vm["client-id"].as<std::string>();
}

unsigned ProgramOptions::receiveMax() const
{
    return m_vm["receive-max"].as<unsigned>();
}

unsigned ProgramOptions::maxPacketSize() const
{
    return m_vm["max-packet-size"].as<unsigned>();
}

unsigned ProgramOptions::topicAliasMax() const
{
    return m_vm["topic-alias-max"].as<unsigned>();
}

bool ProgramOptions::reqResponseInfo() const
{
    return m_vm.count("req-response-info") > 0U;
}

bool ProgramOptions::reqProblemInfo() const
{
    return m_vm.count("req-problem-info") > 0U;
}

std::string ProgramOptions::authMethod() const
{
    return m_vm["auth-method"].as<std::string>();
}

std::vector<ProgramOptions::StringsList> ProgramOptions::subTopics() const
{
    auto values = stringListOpts("sub-topic");
    if (values.empty()) {
        values.push_back("#");
    }
    
    std::vector<StringsList> result;
    result.reserve(values.size());
    std::transform(
        values.begin(), values.end(), std::back_inserter(result), 
        [](auto& str)
        {
            StringsList topics;
            std::size_t pos = 0U;
            while (pos < str.size()) {
                auto sepPos = str.find(',', pos);
                if (sepPos == std::string::npos) {
                    topics.push_back(str.substr(pos));
                    break;
                }

                topics.push_back(str.substr(pos, sepPos - pos));
                pos = sepPos + 1U;
            }

            return topics;
        });

    return result;
}

ProgramOptions::StringsList ProgramOptions::stringListOpts(const std::string& name) const
{
    StringsList result;
    if (m_vm.count(name) > 0) {
        result = m_vm[name].as<StringsList>();
    }

    return result;    
}

} // namespace cc_mqtt5_client_afl_fuzz
