#include "IntegrationTestCommonBase.h"

#include <boost/asio.hpp>

#include <iostream>
#include <stdexcept>

namespace 
{

std::ostream& errorLog(const std::string& name)
{
    return (std::cerr << "ERROR: " << name << ": ");
}    

std::ostream& infoLog(const std::string& name)
{
    return (std::cout << "INFO: " << name << ": ");
}    

void printConnectInfo(const std::string& name, const CC_Mqtt5ConnectResponse* response)
{
    infoLog(name) << "m_highQosPubLimit=" << response->m_highQosPubLimit << '\n';
    infoLog(name) << "m_topicAliasMax=" << response->m_topicAliasMax << '\n';
    infoLog(name) << "maxQos=" << response->m_maxQos << '\n';
    infoLog(name) << "sessionPresent=" << response->m_sessionPresent << '\n';
    infoLog(name) << "retainAvailable=" << response->m_retainAvailable << '\n';
    infoLog(name) << "wildcardSubAvailable=" << response->m_wildcardSubAvailable << '\n';
    infoLog(name) << "subIdsAvailable=" << response->m_subIdsAvailable << '\n';
    infoLog(name) << "sharedSubsAvailable=" << response->m_sharedSubsAvailable << '\n';
}

} // namespace 


class IntegrationTestBasicPubSub_Client1: public IntegrationTestCommonBase
{
    using Base = IntegrationTestCommonBase;
public:
    IntegrationTestBasicPubSub_Client1(boost::asio::io_context& io, int& exitCode) :
        Base(io),
        m_exitCode(exitCode)
    {
    }

    bool start()
    {
        if (!integrationTestStart()) {
            errorLog(m_name) << "Failed to start." << std::endl;
            return false;
        }

        auto connectConfig = CC_Mqtt5ConnectBasicConfig();
        ::cc_mqtt5_client_connect_init_config_basic(&connectConfig);
        connectConfig.m_clientId = m_name.c_str();
        connectConfig.m_cleanStart = true;

        auto* client = integrationTestClient();
        if (client == nullptr) {
            errorLog(m_name) << "Invalid client" << std::endl;
            return false;
        }

        auto* connect = ::cc_mqtt5_client_connect_prepare(client, nullptr);
        if (connect == nullptr) {
            errorLog(m_name) << "Failed to prepare connect" << std::endl;
            return false;
        }

        auto ec = ::cc_mqtt5_client_connect_config_basic(connect, &connectConfig);
        if (ec != CC_Mqtt5ErrorCode_Success) {
            errorLog(m_name) << "Failed to configure connect" << std::endl;
            return false;
        }
        
        ec = integrationTestSendConnect(connect);
        if (ec != CC_Mqtt5ErrorCode_Success) {
            errorLog(m_name) << "Failed to send connect" << std::endl;
            return false;
        } 

        return true;       
    }

protected:
    virtual void integrationTestBrokerDisconnectedImpl([[maybe_unused]] const CC_Mqtt5DisconnectInfo* info) override
    {
        errorLog(m_name) << "Unexpected disconnection from broker" << std::endl;
        failTestInternal();
    }  

    virtual void integrationTestConnectCompleteImpl(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response) override
    {
        if (status != CC_Mqtt5AsyncOpStatus_Complete) {
            errorLog(m_name) << "Unexpected connection status: " << status << std::endl;
            failTestInternal();
            return;
        }

        if (response == nullptr) {
            errorLog(m_name) << "connection response is not provided" << std::endl;
            failTestInternal();
            return;            
        }

        if (response->m_reasonCode != CC_Mqtt5ReasonCode_Success) {
            failTestInternal();
            return; 
        }

        printConnectInfo(m_name, response);
        doSubscribe();
    }

    virtual void integrationTestSubscribeCompleteImpl(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response) override
    {
        if (status != CC_Mqtt5AsyncOpStatus_Complete) {
            errorLog(m_name) << "Unexpected subscribe status: " << status << std::endl;
            failTestInternal();
            return;
        }

        if (response == nullptr) {
            errorLog(m_name) << "Subscription response is not provided" << std::endl;
            failTestInternal();
            return;            
        }

        if (response->m_reasonCodesCount != 1U) {
            errorLog(m_name) << "Unexpected amount of susbscription reason codes: " << response->m_reasonCodesCount << std::endl;
            failTestInternal();
            return; 
        }

        if (response->m_reasonCodes[0] >= CC_Mqtt5ReasonCode_UnspecifiedError) {
            errorLog(m_name) << "Unexpected subscription reason code: " << response->m_reasonCodes[0] << std::endl;
            failTestInternal();
            return; 
        }

        infoLog(m_name) << "Subscription complete." << std::endl;
        io().stop(); // TODO: remove
    }    

private:
    void failTestInternal()
    {
        assert(0);
        m_exitCode = -1;
        io().stop();        
    }

    void doSubscribe()
    {
        auto config = CC_Mqtt5SubscribeTopicConfig();
        ::cc_mqtt5_client_subscribe_init_config_topic(&config);
        config.m_topic = "#";
        auto* client = integrationTestClient();
        auto subscribe = ::cc_mqtt5_client_subscribe_prepare(client, nullptr);
        if (subscribe == nullptr) {
            errorLog(m_name) << "Failed to prepare subscribe" << std::endl;
            failTestInternal();
            return;
        }

        auto ec = ::cc_mqtt5_client_subscribe_config_topic(subscribe, &config);
        if (ec != CC_Mqtt5ErrorCode_Success) {
            errorLog(m_name) << "Failed to configure subscribe topic" << std::endl;
            failTestInternal();
            return;
        }

        ec = integrationTestSendSubscribe(subscribe);
        if (ec != CC_Mqtt5ErrorCode_Success) {
            errorLog(m_name) << "Failed to configure subscribe topic" << std::endl;
            failTestInternal();
            return;
        }        
    }

    int& m_exitCode; 
    std::string m_name = "client1";
};


int main([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[])
{
    int exitCode = 0;
    try {
        boost::asio::io_context io;
        IntegrationTestBasicPubSub_Client1 client1(io, exitCode);

        if (!client1.start()) {
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