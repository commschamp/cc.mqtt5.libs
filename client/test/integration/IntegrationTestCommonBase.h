#pragma once

#include "client.h"

#include <boost/asio.hpp>

#include <array>
#include <chrono>
#include <vector>

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

    auto integrationTestClient()
    {
        return m_client.get();
    }

    decltype(auto) io()
    {
        return m_io;
    }

    CC_Mqtt5ErrorCode integrationTestSendConnect(CC_Mqtt5ConnectHandle handle);
    CC_Mqtt5ErrorCode integrationTestSendSubscribe(CC_Mqtt5SubscribeHandle handle);

protected:
    virtual unsigned integrationTestGetTimeoutSecImpl();
    virtual void integrationTestTimeoutImpl();
    virtual void integrationTestBrokerDisconnectedImpl(const CC_Mqtt5DisconnectInfo* info);    
    virtual void integrationTestConnectCompleteImpl(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response);
    virtual void integrationTestSubscribeCompleteImpl(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response);

private:
    void integrationTestDoReadInternal();
    void integrationTestDoTestTimeoutInternal();
    void integrationTestBrokerDisconnectedInternal(const CC_Mqtt5DisconnectInfo* info);    
    void integrationTestMessageReceivedInternal(const CC_Mqtt5MessageInfo* info);
    void integrationTestConnectCompleteInternal(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response);
    void integrationTestSubscribeCompleteInternal(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response);

    static void integrationTestTickProgramCb(void* data, unsigned ms);
    static unsigned integrationTestCancelTickWaitCb(void* data);
    static void integrationTestSendDataCb(void* data, const unsigned char* buf, unsigned bufLen);
    static void integrationTestBrokerDisconnectedCb(void* data, const CC_Mqtt5DisconnectInfo* info);
    static void integrationTestMessageReceivedCb(void* data, const CC_Mqtt5MessageInfo* info);
    static void integrationTestConnectCompleteCb(void* data, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response);
    static void integrationTestSubscribeCompleteCb(void* data, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response);

    boost::asio::io_context& m_io;
    Socket m_socket;
    Timer m_tickTimer;
    Timer m_timeoutTimer;
    ClientPtr m_client;
    Timestamp m_tickReqTs;
    std::string m_host;
    std::string m_port;
    std::array<std::uint8_t, 1024> m_inBuf;
    std::vector<std::uint8_t> m_inData;
};