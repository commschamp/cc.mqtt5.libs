#include "IntegrationTestCommonBase.h"

#include <chrono>
#include <cstring>
#include <iostream>

namespace 
{

const std::string DefaultHost("127.0.0.1");
const std::string DefaultPort("1883");    

IntegrationTestCommonBase* asObj(void* data)
{
    return reinterpret_cast<IntegrationTestCommonBase*>(data);
}

std::ostream& errorLog(const std::string& name)
{
    std::cerr << "ERROR: ";
    if (!name.empty()) {
        std::cerr << name << ": ";
    }

    return std::cerr;
}    

std::ostream& infoLog(const std::string& name)
{
    std::cout << "INFO: ";
    if (!name.empty()) {
        std::cout << name << ": ";
    }

    return std::cout;
}    

} // namespace 


IntegrationTestCommonBase::IntegrationTestCommonBase(boost::asio::io_context& io, const std::string& clientId) :
    m_io(io),
    m_socket(io),
    m_tickTimer(io),
    m_timeoutTimer(io),
    m_client(::cc_mqtt5_client_alloc()),
    m_host(DefaultHost),
    m_port(DefaultPort),
    m_clientId(clientId)
{
}

bool IntegrationTestCommonBase::integrationTestStart()
{
    if (!m_client) {
        errorLog(m_clientId) << "Client is not allocated" << std::endl;
        return false;
    }

    ::cc_mqtt5_client_set_next_tick_program_callback(m_client.get(), &IntegrationTestCommonBase::integrationTestTickProgramCb, this);
    ::cc_mqtt5_client_set_cancel_next_tick_wait_callback(m_client.get(), &IntegrationTestCommonBase::integrationTestCancelTickWaitCb, this);
    ::cc_mqtt5_client_set_send_output_data_callback(m_client.get(), &IntegrationTestCommonBase::integrationTestSendDataCb, this);
    ::cc_mqtt5_client_set_broker_disconnect_report_callback(m_client.get(), &IntegrationTestCommonBase::integrationTestBrokerDisconnectedCb, this);
    ::cc_mqtt5_client_set_message_received_report_callback(m_client.get(), &IntegrationTestCommonBase::integrationTestMessageReceivedCb, this);

    auto ec = ::cc_mqtt5_client_init(m_client.get());
    if (ec != CC_Mqtt5ErrorCode_Success) {
        errorLog(m_clientId) << "Failed to init: " << ec << std::endl;
    }

    boost::system::error_code ioEc;
    boost::asio::ip::tcp::resolver resolver(m_io);
    boost::asio::connect(m_socket, resolver.resolve(m_host, m_port), ioEc);    
    if (ioEc) {
        errorLog(m_clientId) << "Failed to connect to " << m_host << ":" << m_port << " with error: " << ioEc.message() << std::endl;
        return false;
    }

    assert(m_socket.is_open());
    infoLog(m_clientId) << "Connected to broker" << std::endl;
    integrationTestDoReadInternal();
    integrationTestDoTestTimeoutInternal();
    return true;
}

std::ostream& IntegrationTestCommonBase::integrationTestErrorLog()
{
    return errorLog(m_clientId);
}

std::ostream& IntegrationTestCommonBase::integrationTestInfoLog()
{
    return infoLog(m_clientId);
}

void IntegrationTestCommonBase::integrationTestPrintConnectResponse(const CC_Mqtt5ConnectResponse& response)
{
    integrationTestInfoLog() << "m_highQosSendLimit=" << response.m_highQosSendLimit << '\n';
    integrationTestInfoLog() << "m_topicAliasMax=" << response.m_topicAliasMax << '\n';
    integrationTestInfoLog() << "maxQos=" << response.m_maxQos << '\n';
    integrationTestInfoLog() << "sessionPresent=" << response.m_sessionPresent << '\n';
    integrationTestInfoLog() << "retainAvailable=" << response.m_retainAvailable << '\n';
    integrationTestInfoLog() << "wildcardSubAvailable=" << response.m_wildcardSubAvailable << '\n';
    integrationTestInfoLog() << "subIdsAvailable=" << response.m_subIdsAvailable << '\n';
    integrationTestInfoLog() << "sharedSubsAvailable=" << response.m_sharedSubsAvailable << '\n';
}

CC_Mqtt5ErrorCode IntegrationTestCommonBase::integrationTestSendConnect(CC_Mqtt5ConnectHandle handle)
{
    return ::cc_mqtt5_client_connect_send(handle, &IntegrationTestCommonBase::integrationTestConnectCompleteCb, this);
}

CC_Mqtt5ErrorCode IntegrationTestCommonBase::integrationTestSendSubscribe(CC_Mqtt5SubscribeHandle handle)
{
    return ::cc_mqtt5_client_subscribe_send(handle, &IntegrationTestCommonBase::integrationTestSubscribeCompleteCb, this);
}

CC_Mqtt5ErrorCode IntegrationTestCommonBase::integrationTestSendPublish(CC_Mqtt5PublishHandle handle)
{
    return ::cc_mqtt5_client_publish_send(handle, &IntegrationTestCommonBase::integrationTestPublishCompleteCb, this);
}

bool IntegrationTestCommonBase::integrationTestStartBasicConnect(bool cleanStart)
{
    auto connectConfig = CC_Mqtt5ConnectBasicConfig();
    ::cc_mqtt5_client_connect_init_config_basic(&connectConfig);
    connectConfig.m_clientId = m_clientId.c_str();
    connectConfig.m_cleanStart = cleanStart;

    auto* client = integrationTestClient();
    if (client == nullptr) {
        errorLog(m_clientId) << "Invalid client" << std::endl;
        return false;
    }

    auto* connect = ::cc_mqtt5_client_connect_prepare(client, nullptr);
    if (connect == nullptr) {
        errorLog(m_clientId) << "Failed to prepare connect" << std::endl;
        return false;
    }

    auto ec = ::cc_mqtt5_client_connect_config_basic(connect, &connectConfig);
    if (ec != CC_Mqtt5ErrorCode_Success) {
        errorLog(m_clientId) << "Failed to configure connect" << std::endl;
        return false;
    }
    
    ec = integrationTestSendConnect(connect);
    if (ec != CC_Mqtt5ErrorCode_Success) {
        errorLog(m_clientId) << "Failed to send connect" << std::endl;
        return false;
    } 

    integrationTestInfoLog() << "Sent connect request" << std::endl;
    return true;
}

bool IntegrationTestCommonBase::integrationTestStartBasicSubscribe(const char* topic)
{
    auto config = CC_Mqtt5SubscribeTopicConfig();
    ::cc_mqtt5_client_subscribe_init_config_topic(&config);
    config.m_topic = topic;
    config.m_noLocal = true;
    auto* client = integrationTestClient();
    auto subscribe = ::cc_mqtt5_client_subscribe_prepare(client, nullptr);
    if (subscribe == nullptr) {
        integrationTestErrorLog() << "Failed to prepare subscribe" << std::endl;
        return false;
    }

    auto ec = ::cc_mqtt5_client_subscribe_config_topic(subscribe, &config);
    if (ec != CC_Mqtt5ErrorCode_Success) {
        integrationTestErrorLog() << "Failed to configure subscribe topic" << std::endl;
        return false;
    }

    ec = integrationTestSendSubscribe(subscribe);
    if (ec != CC_Mqtt5ErrorCode_Success) {
        integrationTestErrorLog() << "Failed to send subscribe" << std::endl;
        return false;
    }     

    integrationTestInfoLog() << "Sent subscribe to " << topic << std::endl;
    return true;
}

bool IntegrationTestCommonBase::integrationTestStartBasicPublish(const char* topic, const char* data, CC_Mqtt5QoS qos)
{
    auto config = CC_Mqtt5PublishBasicConfig();
    ::cc_mqtt5_client_publish_init_config_basic(&config);
    config.m_topic = topic;
    config.m_data = reinterpret_cast<const std::uint8_t*>(data);
    config.m_dataLen = static_cast<unsigned>(std::strlen(data));
    config.m_qos = qos;

    auto* client = integrationTestClient();
    auto publish = ::cc_mqtt5_client_publish_prepare(client, nullptr);
    if (publish == nullptr) {
        integrationTestErrorLog() << "Failed to prepare publish" << std::endl;
        return false;
    }

    auto ec = ::cc_mqtt5_client_publish_config_basic(publish, &config);
    if (ec != CC_Mqtt5ErrorCode_Success) {
        integrationTestErrorLog() << "Failed to configure publish" << std::endl;
        return false;
    }

    ec = integrationTestSendPublish(publish);
    if (ec != CC_Mqtt5ErrorCode_Success) {
        integrationTestErrorLog() << "Failed to send publish." << std::endl;
        return false;
    }     

    integrationTestInfoLog() << "Sent publish of " << topic << std::endl;
    return true;
}

bool IntegrationTestCommonBase::integrationTestStartBasicDisconnect(CC_Mqtt5ReasonCode reasonCode)
{
    auto config = CC_Mqtt5DisconnectConfig();
    ::cc_mqtt5_client_disconnect_init_config(&config);
    config.m_reasonCode = reasonCode;

    auto* client = integrationTestClient();
    auto disconnect = ::cc_mqtt5_client_disconnect_prepare(client, nullptr);
    if (disconnect == nullptr) {
        integrationTestErrorLog() << "Failed to prepare disconnect" << std::endl;
        return false;
    }

    auto ec = cc_mqtt5_client_disconnect_config(disconnect, &config);
    if (ec != CC_Mqtt5ErrorCode_Success) {
        integrationTestErrorLog() << "Failed to configure disconnect" << std::endl;
        return false;             
    }

    ec = ::cc_mqtt5_client_disconnect_send(disconnect);
    if (ec != CC_Mqtt5ErrorCode_Success) {
        integrationTestErrorLog() << "Failed to send disconnect" << std::endl;
        return false;             
    }

    return true;
}

bool IntegrationTestCommonBase::integrationTestVerifyConnectSuccessful(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response)
{
    if (status != CC_Mqtt5AsyncOpStatus_Complete) {
        integrationTestErrorLog() << "Unexpected connection status: " << status << std::endl;
        return false;
    }

    if (response == nullptr) {
        integrationTestErrorLog() << "Connection response is not provided" << std::endl;
        return false;            
    }

    if (response->m_reasonCode >= CC_Mqtt5ReasonCode_UnspecifiedError) {
        integrationTestErrorLog() << "Unexpected connection reason code: " << response->m_reasonCode << std::endl;
        return false; 
    }

    integrationTestInfoLog() << "Connection successful" << std::endl;
    return true;
}

bool IntegrationTestCommonBase::integrationTestVerifySubscribeSuccessful(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response, unsigned reasonCodesCount)
{
    if (status != CC_Mqtt5AsyncOpStatus_Complete) {
        integrationTestErrorLog() << "Unexpected subscribe status: " << status << std::endl;
        return false;
    }

    if (response == nullptr) {
        integrationTestErrorLog() << "Subscription response is not provided" << std::endl;
        return false;            
    }

    if (response->m_reasonCodesCount != reasonCodesCount) {
        integrationTestErrorLog() << "Unexpected amount of susbscription reason codes: " << response->m_reasonCodesCount << std::endl;
        return false; 
    }

    for (auto idx = 0U; idx < response->m_reasonCodesCount; ++idx) {
        if (response->m_reasonCodes[idx] >= CC_Mqtt5ReasonCode_UnspecifiedError) {
            integrationTestErrorLog() << "Unexpected subscription reason code idx=" << idx << ": " << response->m_reasonCodes[0] << std::endl;
            return false; 
        }        
    }

    integrationTestInfoLog() << "Subscription successful" << std::endl;
    return true;
}

bool IntegrationTestCommonBase::integrationTestVerifyPublishSuccessful(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response)
{
    if (status != CC_Mqtt5AsyncOpStatus_Complete) {
        integrationTestErrorLog() << "Unexpected publish status: " << status << std::endl;
        return false;
    }

    if (response == nullptr) {
        return true;            
    }
    if (response->m_reasonCode >= CC_Mqtt5ReasonCode_UnspecifiedError) {
        integrationTestErrorLog() << "Unexpected publish reason code: " << response->m_reasonCode << std::endl;
        return false; 
    }

    return true;
}

void IntegrationTestCommonBase::integrationTestDoReadInternal()
{
    assert(m_socket.is_open());
    m_socket.async_read_some(
        boost::asio::buffer(m_inBuf),
        [this](const boost::system::error_code& ec, std::size_t bytesCount)
        {
            if (ec == boost::asio::error::operation_aborted) {
                return;
            }

            if (ec) {
                integrationTestErrorLog() << "Failed to read: " << ec.message() << std::endl;
                return;
            }

            // std::cout << "DEBUG: Received " << bytesCount << " bytes" << std::endl;
            auto* buf = &m_inBuf[0];
            auto bufLen = static_cast<unsigned>(bytesCount);
            bool useVector = !m_inData.empty();
            if (useVector) {
                m_inData.insert(m_inData.end(), buf, buf + bufLen);
                buf = &m_inData[0];
                bufLen = static_cast<decltype(bufLen)>(m_inData.size());
            }

            auto consumed = ::cc_mqtt5_client_process_data(m_client.get(), buf, bufLen);
            assert(consumed <= bufLen);
            if (useVector) {
                m_inData.erase(m_inData.begin(), m_inData.begin() + consumed);
            }
            else {
                auto begIter = buf + consumed;
                auto endIter = buf + bufLen;
                m_inData.assign(begIter, endIter);
            }

            integrationTestDoReadInternal();
        });
}

void IntegrationTestCommonBase::integrationTestDoTestTimeoutInternal()
{
    m_timeoutTimer.expires_after(std::chrono::seconds(integrationTestGetTimeoutSecImpl()));
    m_timeoutTimer.async_wait(
        [this](const boost::system::error_code& ec)
        {
            if (ec == boost::asio::error::operation_aborted) {
                return;
            }

            integrationTestTimeoutImpl();
        });
}

unsigned IntegrationTestCommonBase::integrationTestGetTimeoutSecImpl()
{
    return 5U;
}

void IntegrationTestCommonBase::integrationTestTimeoutImpl()
{
    m_io.stop();
    exit(-1);
}

void IntegrationTestCommonBase::integrationTestBrokerDisconnectedImpl([[maybe_unused]] const CC_Mqtt5DisconnectInfo* info)
{
}

void IntegrationTestCommonBase::integrationTestMessageReceivedImpl([[maybe_unused]] const CC_Mqtt5MessageInfo* info)
{
}

void IntegrationTestCommonBase::integrationTestConnectCompleteImpl(
    [[maybe_unused]] CC_Mqtt5AsyncOpStatus status, 
    [[maybe_unused]] const CC_Mqtt5ConnectResponse* response)
{
}

void IntegrationTestCommonBase::integrationTestSubscribeCompleteImpl(
    [[maybe_unused]] CC_Mqtt5AsyncOpStatus status, 
    [[maybe_unused]] const CC_Mqtt5SubscribeResponse* response)
{
}

void IntegrationTestCommonBase::integrationTestPublishCompleteImpl(
    [[maybe_unused]] CC_Mqtt5AsyncOpStatus status, 
    [[maybe_unused]] const CC_Mqtt5PublishResponse* response)
{
}

void IntegrationTestCommonBase::integrationTestBrokerDisconnectedInternal(const CC_Mqtt5DisconnectInfo* info)
{
    auto& out = integrationTestInfoLog() << "Disconnected ";
    if (info == nullptr) {
        out << "by the client" << std::endl;
        return;
    }

    out << "by the broker with reason: " << info->m_reasonCode << std::endl;
    integrationTestBrokerDisconnectedImpl(info);
}

void IntegrationTestCommonBase::integrationTestMessageReceivedInternal(const CC_Mqtt5MessageInfo* info)
{
    assert(info != nullptr);
    integrationTestInfoLog() << "Received message with topic: " << info->m_topic << std::endl;
    integrationTestMessageReceivedImpl(info);
}

void IntegrationTestCommonBase::integrationTestConnectCompleteInternal(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response)
{
    auto& out = integrationTestInfoLog() << "Connect complete with status=" << status;
    if (response != nullptr) {
        out << " and reasonCode=" << response->m_reasonCode;
    }
    out << std::endl;
    integrationTestConnectCompleteImpl(status, response);
}

void IntegrationTestCommonBase::integrationTestSubscribeCompleteInternal(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response)
{
    integrationTestInfoLog() << "Subscribe complete with status=" << status << std::endl;
    integrationTestSubscribeCompleteImpl(status, response);
}

void IntegrationTestCommonBase::integrationTestPublishCompleteInternal(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response)
{
    integrationTestInfoLog() << "Publish complete with status=" << status << std::endl;
    integrationTestPublishCompleteImpl(status, response);
}

void IntegrationTestCommonBase::integrationTestTickProgramCb(void* data, unsigned ms)
{
    auto& timer = asObj(data)->m_tickTimer;
    asObj(data)->m_tickReqTs = TimestampClock::now();
    timer.expires_after(std::chrono::milliseconds(ms));
    timer.async_wait(
        [data, ms](const boost::system::error_code& ec)
        {
            if (ec == boost::asio::error::operation_aborted) {
                return;
            }

            auto& client = asObj(data)->m_client;
            ::cc_mqtt5_client_tick(client.get(), ms);
        });
}

unsigned IntegrationTestCommonBase::integrationTestCancelTickWaitCb(void* data)
{
    auto& ts = asObj(data)->m_tickReqTs;
    assert(ts != Timestamp());
    auto now = TimestampClock::now();
    auto elapsed = static_cast<unsigned>(std::chrono::duration_cast<std::chrono::milliseconds>(now - ts).count());
    ts = Timestamp();
    boost::system::error_code ec;
    auto& timer = asObj(data)->m_tickTimer;
    timer.cancel(ec);
    return elapsed;
}

void IntegrationTestCommonBase::integrationTestSendDataCb(void* data, const unsigned char* buf, unsigned bufLen)
{
    auto& socket = asObj(data)->m_socket;
    if (!socket.is_open()) {
        errorLog(asObj(data)->m_clientId) << "Socket is not open, cannot send" << std::endl;
        return;
    }

    //std::cout << "DEBUG: Sending " << bufLen << " bytes." << std::endl;

    std::size_t written = 0U;
    while (written < bufLen) {
        boost::system::error_code ec;
        written += boost::asio::write(socket, boost::asio::buffer(buf, bufLen), ec);
        if (ec) {
            errorLog(asObj(data)->m_clientId) << "Failed to write with error: " << ec.message() << std::endl;
            break;
        }
    }
}

void IntegrationTestCommonBase::integrationTestBrokerDisconnectedCb(void* data, const CC_Mqtt5DisconnectInfo* info)
{
    asObj(data)->integrationTestBrokerDisconnectedInternal(info);
}

void IntegrationTestCommonBase::integrationTestMessageReceivedCb(void* data, const CC_Mqtt5MessageInfo* info)
{
    asObj(data)->integrationTestMessageReceivedInternal(info);
}

void IntegrationTestCommonBase::integrationTestConnectCompleteCb(void* data, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response)
{
    asObj(data)->integrationTestConnectCompleteInternal(status, response);
}

void IntegrationTestCommonBase::integrationTestSubscribeCompleteCb(void* data, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response)
{
    asObj(data)->integrationTestSubscribeCompleteInternal(status, response);
}

void IntegrationTestCommonBase::integrationTestPublishCompleteCb(void* data, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response)
{
    asObj(data)->integrationTestPublishCompleteInternal(status, response);
}

