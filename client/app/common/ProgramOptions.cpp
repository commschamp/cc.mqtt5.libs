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
        ("client-id,i", po::value<std::string>()->default_value(std::string()), "Client ID.")
        ("username", po::value<std::string>()->default_value(std::string()), "Username.")
        ("password", po::value<std::string>()->default_value(std::string()), "Password, use \"\\x\" prefix to specify hexadecimal value of a single byte.")
        ("keep-alive", po::value<unsigned>()->default_value(60), "Keep alive period in seconds.")
        ("session-expiry", po::value<unsigned>()->default_value(0), "\"Session Expiry Interval\" property.")
        ("receive-max", po::value<unsigned>()->default_value(0), "\"Receive Maximum\" property. 0 means not set.")
        ("max-packet-size", po::value<unsigned>()->default_value(0), "\"Maximum Packet Size\" property. 0 means no limit.")
        ("topic-alias-max", po::value<unsigned>()->default_value(0), "\"Topic Alias Maximum\" property.")
        ("req-response-info", "Set \"Request Response Information\" property flag.")
        ("req-problem-info", "Set \"Request Problem Information\" property flag.")
        ("connect-user-prop", po::value<StringsList>(), "Add \"User Property\" in \"key=value\" format to CONNECT message.")
        ("will-topic", po::value<std::string>()->default_value(std::string()), "Will topic.")
        ("will-msg", po::value<std::string>()->default_value(std::string()), 
            "Will message data, use \"\\x\" prefix to specify hexadecimal value of a single byte."
            "Applicable only if will-topic is set.")
        ("will-qos", po::value<unsigned>()->default_value(0U), "Will Message QoS: 0, 1, or 2")            
        ("will-retain", "Set \"retain\" flag on the will message.")            
        ("will-content-type", po::value<std::string>()->default_value(std::string()), 
            "Will \"Content Type\" property."
            "Applicable only if will-topic is set.")
        ("will-response-topic", po::value<std::string>()->default_value(std::string()), 
            "Will \"Response Topic\" property."
            "Applicable only if will-topic is set.")    
        ("will-correlation-data", po::value<std::string>()->default_value(std::string()), 
            "Will \"Correlation Data\" property, use \"\\x\" prefix to specify hexadecimal value of a single byte."
            "Applicable only if will-topic is set.")        
        ("will-delay", po::value<unsigned>()->default_value(0), 
            "Will \"Delay Interval\" property."
            "Applicable only if will-topic is set.") 
        ("will-message-expiry", po::value<unsigned>()->default_value(0), 
            "Will \"Message Expiry Interval\" property."
            "Applicable only if will-topic is set.") 
        ("will-message-format", po::value<unsigned>()->default_value(0), 
            "Will \"Payload Format Indicator\" property."
            "Applicable only if will-topic is set.")      
        ("will-user-prop", po::value<StringsList>(), "Add \"User Property\" in \"key=value\" format to the will message.")
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
        ("pub-msg,m", po::value<std::string>()->default_value(std::string()), "Publish message, use \"\\x\" prefix to specify hexadecimal value of a single byte, "
            "such as \"\\x01\\xb9\\xaf\".")
        ("pub-qos,q", po::value<unsigned>()->default_value(0U), "Publish QoS: 0, 1, or 2")
        ("pub-retain", po::value<bool>()->default_value(false), "Retain the publish message")
        ("pub-content-type", po::value<std::string>()->default_value(std::string()), "\"Content Type\" property.")
        ("pub-response-topic", po::value<std::string>()->default_value(std::string()), "\"Response Topic\" property.")        
        ("pub-correlation-data", po::value<std::string>()->default_value(std::string()), 
            "\"Correlation Data\" property, use \"\\x\" prefix to specify hexadecimal value of a single byte.")        
        ("pub-message-expiry", po::value<unsigned>()->default_value(0), "\"Message Expiry Interval\" property.") 
        ("pub-message-format", po::value<unsigned>()->default_value(0), "\"Payload Format Indicator\" property.")      
        ("pub-user-prop", po::value<StringsList>(), "Add \"User Property\" in \"key=value\" format to the message.")
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
        ("sub-no-retained", "Disable reception of the retained messages")
        ("sub-binary", "Force binary output of the received message data")
        ("sub-id", po::value<unsigned>()->default_value(0), "\"Subscription Identifier\" property.")
        ("sub-user-prop", po::value<StringsList>(), "Add \"User Property\" in \"key=value\" format to the SUBSCRIBE request.")
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

std::string ProgramOptions::username() const
{
    return m_vm["username"].as<std::string>();
}

std::string ProgramOptions::password() const
{
    return m_vm["password"].as<std::string>();
}

unsigned ProgramOptions::keepAlive() const
{
    return m_vm["keep-alive"].as<unsigned>();
}

unsigned ProgramOptions::sessionExpiry() const
{
    return m_vm["session-expiry"].as<unsigned>();
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

ProgramOptions::StringsList ProgramOptions::connectUserProps() const
{
    return stringListOpts("connect-user-prop");
}

std::string ProgramOptions::willTopic() const
{
    return m_vm["will-topic"].as<std::string>();
}

std::string ProgramOptions::willMessage() const
{
    return m_vm["will-msg"].as<std::string>();
}

unsigned ProgramOptions::willQos() const
{
    return m_vm["will-qos"].as<unsigned>();
}

bool ProgramOptions::willRetain() const
{
    return m_vm.count("will-retain") > 0U;
}

std::string ProgramOptions::willContentType() const
{
    return m_vm["will-content-type"].as<std::string>();
}

std::string ProgramOptions::willResponseTopic() const
{
    return m_vm["will-response-topic"].as<std::string>();
}

std::string ProgramOptions::willCorrelationData() const
{
    return m_vm["will-correlation-data"].as<std::string>();
}

unsigned ProgramOptions::willDelay() const
{
    return m_vm["will-delay"].as<unsigned>();
}

unsigned ProgramOptions::willMessageExpiry() const
{
    return m_vm["will-message-expiry"].as<unsigned>();
}

unsigned ProgramOptions::willMessageFormat() const
{
    return m_vm["will-message-format"].as<unsigned>();
}

ProgramOptions::StringsList ProgramOptions::willUserProps() const
{
    return stringListOpts("will-user-prop");
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

std::string ProgramOptions::pubContentType() const
{
    return m_vm["pub-content-type"].as<std::string>();
}

std::string ProgramOptions::pubResponseTopic() const
{
    return m_vm["pub-response-topic"].as<std::string>();
}

std::string ProgramOptions::pubCorrelationData() const
{
    return m_vm["pub-correlation-data"].as<std::string>();
}

unsigned ProgramOptions::pubMessageExpiry() const
{
    return m_vm["pub-message-expiry"].as<unsigned>();
}

unsigned ProgramOptions::pubMessageFormat() const
{
    return m_vm["pub-message-format"].as<unsigned>();
}

ProgramOptions::StringsList ProgramOptions::pubUserProps() const
{
    return stringListOpts("pub-user-prop");
}

ProgramOptions::StringsList ProgramOptions::subTopics() const
{
    return stringListOpts("sub-topic");
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

bool ProgramOptions::subNoRetained() const
{
    return m_vm.count("sub-no-retained") > 0U;
}

bool ProgramOptions::subBinary() const
{
    return m_vm.count("sub-binary") > 0U;
}

unsigned ProgramOptions::subId() const
{
    return m_vm["sub-id"].as<unsigned>();
}

ProgramOptions::StringsList ProgramOptions::subUserProps() const
{
    return stringListOpts("sub-user-prop");
}

ProgramOptions::StringsList ProgramOptions::stringListOpts(const std::string& name) const
{
    StringsList result;
    if (m_vm.count(name) > 0) {
        result = m_vm[name].as<StringsList>();
    }

    return result;    
}

} // namespace cc_mqtt5_client_app
