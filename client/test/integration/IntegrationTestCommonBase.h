#pragma once

#include "client.h"

#include <boost/asio.hpp>

#include <chrono>

class IntegrationTestCommonBase
{
    struct ClientDeleter
    {
        void operator()(CC_Mqtt5Client* ptr)
        {
            ::cc_mqtt5_client_free(ptr);
        }
    }; 

public:
    using ClientPtr = std::unique_ptr<CC_Mqtt5Client, ClientDeleter>;
    using Socket = boost::asio::ip::tcp::socket;
    using Timer = boost::asio::steady_timer;
    using TimestampClock = std::chrono::steady_clock;
    using Timestamp = std::chrono::time_point<TimestampClock>;

    explicit IntegrationTestCommonBase(boost::asio::io_context& io);

    bool integrationTestStart();

    auto client()
    {
        return m_client.get();
    }

protected:
    virtual void integrationTestBrokerDisconnectedImpl(const CC_Mqtt5DisconnectInfo* info);    

private:
    static void integrationTestTickProgramCb(void* data, unsigned ms);
    static unsigned integrationTestCancelTickWaitCb(void* data);
    static void integrationTestSendDataCb(void* data, const unsigned char* buf, unsigned bufLen);
    static void integrationTestBrokerDisconnectedCb(void* data, const CC_Mqtt5DisconnectInfo* info);

    boost::asio::io_context& m_io;
    Socket m_socket;
    Timer m_timer;
    ClientPtr m_client;
    Timestamp m_tickReqTs;
    std::string m_host;
    std::string m_port;
};