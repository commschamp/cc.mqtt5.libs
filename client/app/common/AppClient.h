//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "ProgramOptions.h"
#include "Session.h"

#include "client.h"

#include <boost/asio.hpp>

#include <functional>
#include <iosfwd>

namespace cc_mqtt5_client_app
{

class AppClient
{
    struct ClientDeleter
    {
        void operator()(CC_Mqtt5Client* ptr)
        {
            ::cc_mqtt5_client_free(ptr);
        }
    };

public:
    bool start(int argc, const char* argv[]);    

protected:
    explicit AppClient(boost::asio::io_context& io, int& result);
    ~AppClient() = default;

    CC_Mqtt5ClientHandle client()
    {
        return m_client.get();
    }

    boost::asio::io_context& io()
    {
        return m_io;
    }

    ProgramOptions& opts()
    {
        return m_opts;
    }

    using ConnectCompleteCb = std::function<void (CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response)>;
    bool asyncConnect(
        CC_Mqtt5ConnectBasicConfig* basic,
        CC_Mqtt5ConnectWillConfig* will,
        CC_Mqtt5ConnectExtraConfig* extra,
        ConnectCompleteCb&& cb
    );

    static std::ostream& logError();

    void doTerminate(int result = 1);
    void doComplete();

    virtual bool startImpl();
    virtual void brokerConnectedImpl();
    virtual void brokerDisconnectedImpl(const CC_Mqtt5DisconnectInfo* info);

    static std::vector<std::uint8_t> parseBinaryData(const std::string& val);
    static std::string toString(CC_Mqtt5ErrorCode val);
    static std::string toString(CC_Mqtt5AsyncOpStatus val);
    static void print(const CC_Mqtt5DisconnectInfo& info);
    static void print(const CC_Mqtt5ConnectResponse& response);
    static void print(const CC_Mqtt5PublishResponse& response);

private:
    using ClientPtr = std::unique_ptr<CC_Mqtt5Client, ClientDeleter>;
    using Timer = boost::asio::steady_timer;
    using Clock = Timer::clock_type;
    using Timestamp = Timer::time_point;

    void nextTickProgramInternal(unsigned duration);
    unsigned cancelNextTickWaitInternal();
    void sendDataInternal(const unsigned char* buf, unsigned bufLen);
    bool createSession();

    static void sendDataCb(void* data, const unsigned char* buf, unsigned bufLen);
    static void brokerDisconnectedCb(void* data, const CC_Mqtt5DisconnectInfo* info);
    static void messageReceivedCb(void* data, const CC_Mqtt5MessageInfo* info);
    static void logMessageCb(void* data, const char* msg);
    static void nextTickProgramCb(void* data, unsigned duration);
    static unsigned cancelNextTickWaitCb(void* data);
    static void connectCompleteCb(void* data, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response);

    boost::asio::io_context& m_io;
    int& m_result;
    Timer m_timer;
    Timestamp m_lastWaitProgram;
    ProgramOptions m_opts;
    ClientPtr m_client;
    SessionPtr m_session;
    ConnectCompleteCb m_connectCompleteCb;
};

} // namespace cc_mqtt5_client_app
