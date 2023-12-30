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

    explicit IntegrationTestCommonBase(boost::asio::io_context& io, const std::string& m_clientId = std::string());

    bool integrationTestStart();

    auto integrationTestClient()
    {
        return m_client.get();
    }

    decltype(auto) io()
    {
        return m_io;
    }

    std::ostream& integrationTestErrorLog();
    std::ostream& integrationTestInfoLog();
    void integrationTestPrintConnectResponse(const CC_Mqtt5ConnectResponse& response);

    CC_Mqtt5ErrorCode integrationTestSendConnect(CC_Mqtt5ConnectHandle handle);
    CC_Mqtt5ErrorCode integrationTestSendSubscribe(CC_Mqtt5SubscribeHandle handle);
    CC_Mqtt5ErrorCode integrationTestSendPublish(CC_Mqtt5PublishHandle handle);

    bool integrationTestStartBasicConnect(bool cleanStart = true);
    bool integrationTestStartBasicSubscribe(const char* topic);
    bool integrationTestStartBasicPublish(const char* topic, const char* data, CC_Mqtt5QoS qos);
    bool integrationTestStartBasicDisconnect(CC_Mqtt5ReasonCode reasonCode = CC_Mqtt5ReasonCode_NormalDisconnection);

    bool integrationTestVerifyConnectSuccessful(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response);
    bool integrationTestVerifySubscribeSuccessful(CC_Mqtt5SubscribeHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response, unsigned reasonCodesCount = 1U);
    bool integrationTestVerifyPublishSuccessful(CC_Mqtt5PublishHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response);

    const std::string& integrationTestClientId()
    {
        return m_clientId;
    }


protected:
    virtual unsigned integrationTestGetTimeoutSecImpl();
    virtual void integrationTestTimeoutImpl();
    virtual void integrationTestBrokerDisconnectedImpl(const CC_Mqtt5DisconnectInfo* info);    
    virtual void integrationTestMessageReceivedImpl(const CC_Mqtt5MessageInfo* info);    
    virtual void integrationTestConnectCompleteImpl(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response);
    virtual void integrationTestSubscribeCompleteImpl(CC_Mqtt5SubscribeHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response);
    virtual void integrationTestPublishCompleteImpl(CC_Mqtt5PublishHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response);

    void stopIoPosted();

private:
    void integrationTestDoReadInternal();
    void integrationTestDoTestTimeoutInternal();
    void integrationTestBrokerDisconnectedInternal(const CC_Mqtt5DisconnectInfo* info);    
    void integrationTestMessageReceivedInternal(const CC_Mqtt5MessageInfo* info);
    void integrationTestConnectCompleteInternal(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response);
    void integrationTestSubscribeCompleteInternal(CC_Mqtt5SubscribeHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response);
    void integrationTestPublishCompleteInternal(CC_Mqtt5PublishHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response);

    static void integrationTestTickProgramCb(void* data, unsigned ms);
    static unsigned integrationTestCancelTickWaitCb(void* data);
    static void integrationTestSendDataCb(void* data, const unsigned char* buf, unsigned bufLen);
    static void integrationTestBrokerDisconnectedCb(void* data, const CC_Mqtt5DisconnectInfo* info);
    static void integrationTestMessageReceivedCb(void* data, const CC_Mqtt5MessageInfo* info);
    static void integrationTestConnectCompleteCb(void* data, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response);
    static void integrationTestSubscribeCompleteCb(void* data, CC_Mqtt5SubscribeHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response);
    static void integrationTestPublishCompleteCb(void* data, CC_Mqtt5PublishHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response);

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
    std::string m_clientId;
};