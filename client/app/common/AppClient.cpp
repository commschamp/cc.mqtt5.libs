//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "AppClient.h"

#include <cassert>
#include <iostream>

namespace cc_mqtt5_client_app
{

namespace 
{

AppClient* asThis(void* data)
{
    return reinterpret_cast<AppClient*>(data);
}

} // namespace 
    
bool AppClient::start(int argc, const char* argv[])
{
    if (!m_opts.parseArgs(argc, argv)) {
        logError() << "Failed to parse arguments." << std::endl;
        return false;
    }

    if (m_opts.helpRequested()) {
        std::cout << "Usage: " << argv[0] << " [options...]" << '\n';
        m_opts.printHelp();
        io().stop();
        return true;
    }

    auto ec = ::cc_mqtt5_client_init(m_client.get());
    if (ec != CC_Mqtt5ErrorCode_Success) {
        logError() << "Failed to initialize client object." << std::endl;
        return false;
    }

    m_session = Session::create(m_io, m_opts);
    if (!m_session) {
        logError() << "Failed to create network connection session." << std::endl;
        return false;
    }

    m_session->setDataReportCb(
        [this](const std::uint8_t* buf, std::size_t bufLen) -> unsigned
        {
            assert(m_client);
            return ::cc_mqtt5_client_process_data(m_client.get(), buf, static_cast<unsigned>(bufLen));
        });

    // TODO: session callbacks

    if (!m_session->start()) {
        logError() << "Failed to connect to the broker." << std::endl;
        return false;
    }

    // TODO:

    return true;
}    

AppClient::AppClient(boost::asio::io_context& io) : 
    m_io(io),
    m_timer(io),
    m_client(::cc_mqtt5_client_alloc())
{
    assert(m_client);
    ::cc_mqtt5_client_set_send_output_data_callback(m_client.get(), &AppClient::sendDataCb, this);
    ::cc_mqtt5_client_set_broker_disconnect_report_callback(m_client.get(), &AppClient::brokerDisconnectedCb, this);
    ::cc_mqtt5_client_set_message_received_report_callback(m_client.get(), &AppClient::messageReceivedCb, this);
    ::cc_mqtt5_client_set_error_log_callback(m_client.get(), &AppClient::logMessageCb, this);
    ::cc_mqtt5_client_set_next_tick_program_callback(m_client.get(), &AppClient::nextTickProgramCb, this);
    ::cc_mqtt5_client_set_cancel_next_tick_wait_callback(m_client.get(), &AppClient::cancelNextTickWaitCb, this);
}

bool AppClient::asyncConnect(
    CC_Mqtt5ConnectBasicConfig* basic,
    CC_Mqtt5ConnectWillConfig* will,
    CC_Mqtt5ConnectExtraConfig* extra,
    ConnectCompleteCb&& cb)
{
    if (!cb) {
        logError() << "Connection complete callback is not provided" << std::endl;
        return false;
    }

    auto ec = CC_Mqtt5ErrorCode_Success;
    auto connect = ::cc_mqtt5_client_connect_prepare(m_client.get(), &ec);
    if (!connect) {
        logError() << "Failed to allocate client with ec=" << ec << std::endl;
        return false;
    }
    
    if (basic != nullptr) {
        ec = ::cc_mqtt5_client_connect_config_basic(connect, basic);
        if (ec != CC_Mqtt5ErrorCode_Success) {
            logError() << "Failed to apply basic connect configuration with ec=" << ec << std::endl;
            return false;
        }
    }

    if (will != nullptr) {
        ec = ::cc_mqtt5_client_connect_config_will(connect, will);
        if (ec != CC_Mqtt5ErrorCode_Success) {
            logError() << "Failed to apply will connect configuration with ec=" << ec << std::endl;
            return false;
        }        
    }

    if (extra != nullptr) {
        ec = ::cc_mqtt5_client_connect_config_extra(connect, extra);
        if (ec != CC_Mqtt5ErrorCode_Success) {
            logError() << "Failed to apply extra connect configuration with ec=" << ec << std::endl;
            return false;
        }        
    }    

    ec = ::cc_mqtt5_client_connect_send(connect, &AppClient::connectCompleteCb, this);
    if (ec != CC_Mqtt5ErrorCode_Success) {
        logError() << "Failed to send connect request with ec=" << ec << std::endl;
        return false;
    }    

    m_connectCompleteCb = std::move(cb);    
    return true;
}

std::ostream& AppClient::logError()
{
    return std::cerr << "ERROR: ";
}

void AppClient::nextTickProgramInternal(unsigned duration)
{
    m_lastWaitProgram = Clock::now();
    m_timer.expires_after(std::chrono::milliseconds(duration));
    m_timer.async_wait(
        [this, duration](const boost::system::error_code& ec)
        {
            if (ec == boost::asio::error::operation_aborted) {
                return;
            }

            if (ec) {
                logError() << "Timer error: " << ec.message();
                m_io.stop();
                return;
            }

            ::cc_mqtt5_client_tick(m_client.get(), duration);
        }
    );
}

unsigned AppClient::cancelNextTickWaitInternal()
{
    boost::system::error_code ec;
    m_timer.cancel(ec);
    auto now = Clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastWaitProgram).count();
    return static_cast<unsigned>(diff);
}

void AppClient::sendDataCb(void* data, const unsigned char* buf, unsigned bufLen)
{
    // TODO
    static_cast<void>(data);
    static_cast<void>(buf);
    static_cast<void>(bufLen);
}

void AppClient::brokerDisconnectedCb(void* data, const CC_Mqtt5DisconnectInfo* info)
{
    // TODO:
    static_cast<void>(data);
    static_cast<void>(info);
}

void AppClient::messageReceivedCb(void* data, const CC_Mqtt5MessageInfo* info)
{
    // TODO:
    static_cast<void>(data);
    static_cast<void>(info);    
}

void AppClient::logMessageCb(void* data, const char* msg)
{
    // TODO:
    static_cast<void>(data);
    static_cast<void>(msg);    
}

void AppClient::nextTickProgramCb(void* data, unsigned duration)
{
    asThis(data)->nextTickProgramInternal(duration);
}

unsigned AppClient::cancelNextTickWaitCb(void* data)
{
    return asThis(data)->cancelNextTickWaitInternal();
}

void AppClient::connectCompleteCb(void* data, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response)
{
    asThis(data)->m_connectCompleteCb(status, response);
}

} // namespace cc_mqtt5_client_app
