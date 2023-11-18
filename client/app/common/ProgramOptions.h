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
#include <vector>

namespace cc_mqtt5_client_app
{

class ProgramOptions
{
public:
    using OptDesc = boost::program_options::options_description;
    using StringsList = std::vector<std::string>;
    using UnsignedsList = std::vector<unsigned>;

    static constexpr std::uint16_t DefaultPort = 1883U;

    enum ConnectionType
    {
        ConnectionType_Tcp,
        ConnectionType_ValuesLimit
    };

    void addCommon();
    void addConnect();
    void addNetwork(std::uint16_t port = DefaultPort);
    void addPublish();
    void addSubscribe();

    void printHelp();

    bool parseArgs(int argc, const char* argv[]);

    // Common options
    bool helpRequested() const;
    bool verbose() const;
    ConnectionType connectionType() const;

    // Connect Options
    std::string clientId() const;
    std::string username() const;
    std::string password() const;
    unsigned keepAlive() const;
    unsigned sessionExpiry() const;
    unsigned receiveMax() const;
    unsigned maxPacketSize() const;
    unsigned topicAliasMax() const;
    bool reqResponseInfo() const;
    bool reqProblemInfo() const;
    StringsList connectUserProps() const;
    std::string willTopic() const;
    std::string willMessage() const;
    unsigned willQos() const;
    bool willRetain() const;
    std::string willContentType() const;
    std::string willResponseTopic() const;
    std::string willCorrelationData() const;
    unsigned willDelay() const;
    unsigned willMessageExpiry() const;
    unsigned willMessageFormat() const;
    StringsList willUserProps() const;

    // Network Options
    std::string networkAddress() const;
    std::uint16_t networkPort() const;

    // Publish Options
    std::string pubTopic() const;
    std::string pubMessage() const;
    unsigned pubQos() const;
    bool pubRetain() const;
    std::string pubContentType() const;
    std::string pubResponseTopic() const;
    std::string pubCorrelationData() const;
    unsigned pubMessageExpiry() const;
    unsigned pubMessageFormat() const;
    StringsList pubUserProps() const;

    // Subscribe Options
    StringsList subTopics() const;
    UnsignedsList subQoses() const;
    bool subBinary() const;
    unsigned subId() const;
    StringsList subUserProps() const;

private:
    StringsList stringListOpts(const std::string& name) const;

    boost::program_options::variables_map m_vm;
    OptDesc m_desc;
};

} // namespace cc_mqtt5_client_app
