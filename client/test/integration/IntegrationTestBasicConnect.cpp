#include "IntegrationTestCommonBase.h"

#include <boost/asio.hpp>

#include <iostream>
#include <stdexcept>

class IntegrationTestBasicConnect : public IntegrationTestCommonBase
{
    using Base = IntegrationTestCommonBase;
public:
    IntegrationTestBasicConnect(boost::asio::io_context& io, int& exitCode) :
        Base(io),
        m_exitCode(exitCode)
    {
    }

protected:
    virtual void integrationTestBrokerDisconnectedImpl([[maybe_unused]] const CC_Mqtt5DisconnectInfo* info) override
    {
        std::cerr << "ERROR: Unexpected disconnection from broker" << std::endl;
        failTestInternal();
    }  

    virtual void integrationTestConnectCompleteImpl(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response) override
    {
        if (status != CC_Mqtt5AsyncOpStatus_Complete) {
            std::cerr << "ERROR: Unexpected connection status: " << status << std::endl;
            failTestInternal();
            return;
        }

        if (response == nullptr) {
            std::cerr << "ERROR: connection response is not provided" << std::endl;
            failTestInternal();
            return;            
        }

        if (response->m_reasonCode != CC_Mqtt5ReasonCode_Success) {
            failTestInternal();
            return; 
        }

        integrationTestPrintConnectResponse(*response);

        auto disconnect = ::cc_mqtt5_client_disconnect_prepare(integrationTestClient(), nullptr);
        if (disconnect == nullptr) {
            std::cerr << "ERROR: Failed to allocate disconnect message" << std::endl;
            failTestInternal();
            return;                
        }

        auto config = CC_Mqtt5DisconnectConfig();
        ::cc_mqtt5_client_disconnect_init_config(&config);
        auto ec = cc_mqtt5_client_disconnect_config(disconnect, &config);
        if (ec != CC_Mqtt5ErrorCode_Success) {
            std::cerr << "ERROR: Failed to configure disconnect" << std::endl;
            failTestInternal();
            return;             
        }

        ec = ::cc_mqtt5_client_disconnect_send(disconnect);
        if (ec != CC_Mqtt5ErrorCode_Success) {
            std::cerr << "ERROR: Failed to send disconnect" << std::endl;
            failTestInternal();
            return;             
        }

        io().stop();
    }


private:
    void failTestInternal()
    {
        assert(0);
        m_exitCode = -1;
        io().stop();        
    }

    int& m_exitCode;    
};


int main([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[])
{
    int exitCode = 0;
    try {
        boost::asio::io_context io;
        IntegrationTestBasicConnect test(io, exitCode);

        if (!test.integrationTestStart()) {
            std::cerr << "ERROR: Failed to start" << std::endl;
            return -1;
        }

        auto connectConfig = CC_Mqtt5ConnectBasicConfig();
        ::cc_mqtt5_client_connect_init_config_basic(&connectConfig);
        connectConfig.m_clientId = "IntegrationTestBasicConnect";
        connectConfig.m_cleanStart = true;

        auto* client = test.integrationTestClient();
        if (client == nullptr) {
            std::cerr << "ERROR: Invalid client" << std::endl;
            return -1;
        }

        auto* connect = ::cc_mqtt5_client_connect_prepare(client, nullptr);
        if (connect == nullptr) {
            std::cerr << "ERROR: Failed to prepare connect" << std::endl;
            return -1;
        }

        auto ec = ::cc_mqtt5_client_connect_config_basic(connect, &connectConfig);
        if (ec != CC_Mqtt5ErrorCode_Success) {
            std::cerr << "ERROR: Failed to configure connect" << std::endl;
            return -1;
        }
        
        ec = test.integrationTestSendConnect(connect);
        if (ec != CC_Mqtt5ErrorCode_Success) {
            std::cerr << "ERROR: Failed to send connect" << std::endl;
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