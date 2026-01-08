#include "IntegrationTestCommonBase.h"

#include <boost/asio.hpp>

#include <iostream>
#include <stdexcept>

namespace
{

struct PubInfo
{
    std::string m_topic;
    std::string m_data;
    CC_Mqtt5QoS m_qos;
};

const PubInfo PubInfoMap[] = {
    {"cc_mqtt5_client/pubsub_topic1", "11111", CC_Mqtt5QoS_AtMostOnceDelivery},
    {"cc_mqtt5_client/pubsub_topic2", "22222", CC_Mqtt5QoS_AtLeastOnceDelivery},
    {"cc_mqtt5_client/pubsub_topic3", "33333", CC_Mqtt5QoS_ExactlyOnceDelivery},
};
std::size_t PubInfoMapSize = std::extent<decltype(PubInfoMap)>::value;

} // namespace

class IntegrationTestBasicPubSub_Client1: public IntegrationTestCommonBase
{
    using Base = IntegrationTestCommonBase;
public:
    IntegrationTestBasicPubSub_Client1(boost::asio::io_context& io, int& exitCode, unsigned& opCount) :
        Base(io, "IntegrationTestBasicPubSub_Client1"),
        m_exitCode(exitCode),
        m_opCount(opCount)
    {
    }

    ~IntegrationTestBasicPubSub_Client1()
    {
        std::cout << __FUNCTION__ << std::endl;
    }

    bool start()
    {
        if (!integrationTestStart()) {
            return false;
        }

        if (!integrationTestStartBasicConnect()) {
            return false;
        }

        ++m_opCount;

        return true;
    }

protected:
    virtual void integrationTestBrokerDisconnectedImpl([[maybe_unused]] const CC_Mqtt5DisconnectInfo* info) override
    {
        integrationTestErrorLog() << "Unexpected disconnection from broker" << std::endl;
        failTestInternal();
    }

    virtual void integrationTestMessageReceivedImpl(const CC_Mqtt5MessageInfo* info) override
    {
        // Echo all incoming messages
        std::string data(reinterpret_cast<const char*>(info->m_data), info->m_dataLen);
        ++m_opCount;
        if (!integrationTestStartBasicPublish(info->m_topic, data.c_str(), info->m_qos)) {
            failTestInternal();
            return;
        }
    }

    virtual void integrationTestConnectCompleteImpl(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response) override
    {
        assert(m_opCount > 0U);
        --m_opCount;
        if (!integrationTestVerifyConnectSuccessful(status, response)) {
            failTestInternal();
            return;
        }

        integrationTestPrintConnectResponse(*response);

        if (!integrationTestStartBasicSubscribe("#")) {
            failTestInternal();
            return;
        }

        ++m_opCount;
    }

    virtual void integrationTestSubscribeCompleteImpl(
        CC_Mqtt5SubscribeHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response) override
    {
        assert(m_opCount > 0U);
        --m_opCount;
        if (!integrationTestVerifySubscribeSuccessful(handle, status, response)) {
            failTestInternal();
            return;
        }

        // Wait for message
    }

    virtual void integrationTestPublishCompleteImpl(CC_Mqtt5PublishHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response) override
    {
        assert(m_opCount > 0U);
        --m_opCount;

        if (!integrationTestVerifyPublishSuccessful(handle, status, response)) {
            failTestInternal();
            return;
        }
    }

private:
    void failTestInternal()
    {
        assert(0);
        m_exitCode = -1;
        io().stop();
    }

    int& m_exitCode;
    unsigned& m_opCount;
};

class IntegrationTestBasicPubSub_Client2: public IntegrationTestCommonBase
{
    using Base = IntegrationTestCommonBase;
public:
    IntegrationTestBasicPubSub_Client2(boost::asio::io_context& io, int& exitCode, unsigned& opCount) :
        Base(io, "IntegrationTestBasicPubSub_Client2"),
        m_exitCode(exitCode),
        m_opCount(opCount)
    {
    }

    ~IntegrationTestBasicPubSub_Client2()
    {
        std::cout << __FUNCTION__ << std::endl;
    }

    bool start()
    {
        if (!integrationTestStart()) {
            return false;
        }

        if (!integrationTestStartBasicConnect()) {
            return false;
        }

        ++m_opCount;
        return true;
    }

protected:
    virtual void integrationTestBrokerDisconnectedImpl([[maybe_unused]] const CC_Mqtt5DisconnectInfo* info) override
    {
        integrationTestErrorLog() << "Unexpected disconnection from broker" << std::endl;
        failTestInternal();
    }

    virtual void integrationTestMessageReceivedImpl(const CC_Mqtt5MessageInfo* info) override
    {
        assert(m_pubCount < PubInfoMapSize);
        auto& pubInfo = PubInfoMap[m_pubCount];
        if (info->m_topic != pubInfo.m_topic) {
            integrationTestErrorLog() << "Unexpected topic: " << info->m_topic << "!=" << pubInfo.m_topic << std::endl;
            failTestInternal();
            return;
        }

        std::string data(reinterpret_cast<const char*>(info->m_data), info->m_dataLen);
        if (data != pubInfo.m_data) {
            integrationTestErrorLog() << "Unexpected data: " << info->m_data << "!=" << pubInfo.m_data << std::endl;
            failTestInternal();
            return;
        }

        if (info->m_qos != pubInfo.m_qos) {
            integrationTestErrorLog() << "Unexpected qos: " << info->m_qos << "!=" << pubInfo.m_qos << std::endl;
            failTestInternal();
            return;
        }

        integrationTestInfoLog() << "Publish " << m_pubCount << " is echoed back" << std::endl;
        ++m_pubCount;
        doNextPublish();
    }

    virtual void integrationTestConnectCompleteImpl(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response) override
    {
        assert(m_opCount > 0U);
        --m_opCount;

        if (!integrationTestVerifyConnectSuccessful(status, response)) {
            failTestInternal();
            return;
        }

        integrationTestPrintConnectResponse(*response);

        if (!integrationTestStartBasicSubscribe("#")) {
            failTestInternal();
            return;
        }

        ++m_opCount;
    }

    virtual void integrationTestSubscribeCompleteImpl(CC_Mqtt5SubscribeHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response) override
    {
        assert(m_opCount > 0U);
        --m_opCount;

        if (!integrationTestVerifySubscribeSuccessful(handle, status, response)) {
            failTestInternal();
            return;
        }

        doNextPublish();
    }

    virtual void integrationTestPublishCompleteImpl(CC_Mqtt5PublishHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response) override
    {
        assert(m_opCount > 0U);
        --m_opCount;

        if (!integrationTestVerifyPublishSuccessful(handle, status, response)) {
            failTestInternal();
            return;
        }
    }

private:
    void failTestInternal()
    {
        assert(0);
        m_exitCode = -1;
        io().stop();
    }

    void doNextPublish()
    {
        if (PubInfoMapSize <= m_pubCount) {
            integrationTestInfoLog() << "All publishes are complete" << std::endl;
            stopTestWhenReady();
            return;
        }

        auto& info = PubInfoMap[m_pubCount];
        ++m_opCount;
        if (!integrationTestStartBasicPublish(info.m_topic.c_str(), info.m_data.c_str(), info.m_qos)) {
            failTestInternal();
            return;
        }
    }

    void stopTestWhenReady()
    {
        if (m_opCount == 0U) {
            io().stop();
            return;
        }

        boost::asio::post(
            io(),
            [this]()
            {
                stopTestWhenReady();
            });
    }

    int& m_exitCode;
    unsigned& m_opCount;
    unsigned m_pubCount = 0U;
};

int main([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[])
{
    int exitCode = 0;
    unsigned opCount = 0U;
    try {
        boost::asio::io_context io;

        IntegrationTestBasicPubSub_Client1 client1(io, exitCode, opCount);
        IntegrationTestBasicPubSub_Client2 client2(io, exitCode, opCount);

        if ((!client1.start()) ||
            (!client2.start())) {
            return -1;
        }

        io.run();
    }
    catch (const std::exception& e) {
        std::cerr << "ERROR: Unexpected exception: " << e.what() << std::endl;
        return -1;
    }

    return exitCode;
}
