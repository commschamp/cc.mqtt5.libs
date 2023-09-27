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
    m_timer(io),
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
    return true;
}

void IntegrationTestCommonBase::integrationTestBrokerDisconnectedImpl(const CC_Mqtt5DisconnectInfo* info)
{
    std::cout << "INFO: Disconnected ";
    if (info == nullptr) {
        std::cout << "by the client" << std::endl;
        return;
    }

    std::cout << "by the proker with reason: " << info->m_reasonCode << std::endl;
}

void IntegrationTestCommonBase::integrationTestTickProgramCb(void* data, unsigned ms)
{
    auto& timer = asObj(data)->m_timer;
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
    auto& timer = asObj(data)->m_timer;
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
    asObj(data)->integrationTestBrokerDisconnectedImpl(info);
}