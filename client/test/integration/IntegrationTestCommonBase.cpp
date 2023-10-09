#include "IntegrationTestCommonBase.h"

#include <chrono>
#include <iostream>

namespace 
{

const std::string DefaultHost("127.0.0.1");
const std::string DefaultPort("1883");    

IntegrationTestCommonBase* asObj(void* data)
{
    return reinterpret_cast<IntegrationTestCommonBase*>(data);
}

} // namespace 


IntegrationTestCommonBase::IntegrationTestCommonBase(boost::asio::io_context& io) :
    m_io(io),
    m_socket(io),
    m_tickTimer(io),
    m_timeoutTimer(io),
    m_client(::cc_mqtt5_client_new()),
    m_host(DefaultHost),
    m_port(DefaultPort)
{
}

bool IntegrationTestCommonBase::integrationTestStart()
{
    if (!m_client) {
        std::cerr << "ERROR: client is not allocated" << std::endl;
        return false;
    }

    ::cc_mqtt5_client_set_next_tick_program_callback(m_client.get(), &IntegrationTestCommonBase::integrationTestTickProgramCb, this);
    ::cc_mqtt5_client_set_cancel_next_tick_wait_callback(m_client.get(), &IntegrationTestCommonBase::integrationTestCancelTickWaitCb, this);
    ::cc_mqtt5_client_set_send_output_data_callback(m_client.get(), &IntegrationTestCommonBase::integrationTestSendDataCb, this);
    ::cc_mqtt5_client_set_broker_disconnect_report_callback(m_client.get(), &IntegrationTestCommonBase::integrationTestBrokerDisconnectedCb, this);
    ::cc_mqtt5_client_set_message_received_report_callback(m_client.get(), &IntegrationTestCommonBase::integrationTestMessageReceivedCb, this);

    auto ec = ::cc_mqtt5_client_init(m_client.get());
    if (ec != CC_Mqtt5ErrorCode_Success) {
        std::cerr << "ERROR: Failed to init: " << ec << std::endl;
    }

    boost::system::error_code ioEc;
    boost::asio::ip::tcp::resolver resolver(m_io);
    boost::asio::connect(m_socket, resolver.resolve(m_host, m_port), ioEc);    
    if (ioEc) {
        std::cerr << "ERROR: Failed to connect to " << m_host << ":" << m_port << " with error: " << ioEc.message() << std::endl;
        return false;
    }

    assert(m_socket.is_open());
    std::cout << "INFO: Connected to broker" << std::endl;
    integrationTestDoReadInternal();
    integrationTestDoTestTimeoutInternal();
    return true;
}

CC_Mqtt5ErrorCode IntegrationTestCommonBase::integrationTestSendConnect(CC_Mqtt5ConnectHandle handle)
{
    return ::cc_mqtt5_client_connect_send(handle, &IntegrationTestCommonBase::integrationTestConnectCompleteCb, this);
}

CC_Mqtt5ErrorCode IntegrationTestCommonBase::integrationTestSendSubscribe(CC_Mqtt5SubscribeHandle handle)
{
    return ::cc_mqtt5_client_subscribe_send(handle, &IntegrationTestCommonBase::integrationTestSubscribeCompleteCb, this);
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
                std::cerr << "ERROR: Failed to read: " << ec.message() << std::endl;
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
            if (useVector) {
                m_inData.erase(m_inData.begin(), m_inData.begin() + consumed);
            }
            else {
                auto begIter = buf + consumed;
                auto endIter = begIter + (bufLen - consumed);
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

void IntegrationTestCommonBase::integrationTestBrokerDisconnectedInternal(const CC_Mqtt5DisconnectInfo* info)
{
    std::cout << "INFO: Disconnected ";
    if (info == nullptr) {
        std::cout << "by the client" << std::endl;
        return;
    }

    std::cout << "by the broker with reason: " << info->m_reasonCode << std::endl;
    integrationTestBrokerDisconnectedImpl(info);
}

void IntegrationTestCommonBase::integrationTestMessageReceivedInternal(const CC_Mqtt5MessageInfo* info)
{
    // TODO:
    static_cast<void>(info);
}

void IntegrationTestCommonBase::integrationTestConnectCompleteInternal(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response)
{
    std::cout << "INFO: Connect complete with status=" << status;
    if (response != nullptr) {
        std::cout << " and reasonCode=" << response->m_reasonCode;
    }
    std::cout << std::endl;
    integrationTestConnectCompleteImpl(status, response);
}

void IntegrationTestCommonBase::integrationTestSubscribeCompleteInternal(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response)
{
    std::cout << "INFO: Subscribe complete with status=" << status << std::endl;
    integrationTestSubscribeCompleteImpl(status, response);
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
        std::cerr << "ERROR: socket is not open, cannot send" << std::endl;
        return;
    }

    //std::cout << "DEBUG: Sending " << bufLen << " bytes." << std::endl;

    auto written = 0U;
    while (written < bufLen) {
        boost::system::error_code ec;
        written += boost::asio::write(socket, boost::asio::buffer(buf, bufLen), ec);
        if (ec) {
            std::cerr << "ERROR: Failed to write with error: " << ec.message() << std::endl;
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
