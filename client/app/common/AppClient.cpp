//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "AppClient.h"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <map>
#include <type_traits>

namespace cc_mqtt5_client_app
{

namespace 
{

AppClient* asThis(void* data)
{
    return reinterpret_cast<AppClient*>(data);
}

std::string toString(CC_Mqtt5ReasonCode val)
{
    static const std::map<CC_Mqtt5ReasonCode, std::string> Map = {
        {CC_Mqtt5ReasonCode_Success, "Success / Normal Disconnection / Granted QoS0"},
        {CC_Mqtt5ReasonCode_GrantedQos1, "Granted QoS1"},
        {CC_Mqtt5ReasonCode_GrantedQos2, "Granted QoS2"},
        {CC_Mqtt5ReasonCode_DisconnectWithWill, "Disconnect With Will"},
        {CC_Mqtt5ReasonCode_NoMatchingSubscribers, "No Matching Subscribers"},
        {CC_Mqtt5ReasonCode_NoSubscriptionExisted, "No Subscription Existed"},
        {CC_Mqtt5ReasonCode_ContinueAuth, "Continue Authentication"},
        {CC_Mqtt5ReasonCode_ReAuth, "Re-Authenticate"},
        {CC_Mqtt5ReasonCode_UnspecifiedError, "Unspecified Error"},
        {CC_Mqtt5ReasonCode_MalformedPacket, "Malformed Packet"},
        {CC_Mqtt5ReasonCode_ProtocolError, "Protocol Error"},
        {CC_Mqtt5ReasonCode_ImplSpecificError, "Implementation Specific Error"},
        {CC_Mqtt5ReasonCode_UnsupportedVersion, "Unsupported Version"},
        {CC_Mqtt5ReasonCode_ClientIdInvalid, "Client ID Invalid"},
        {CC_Mqtt5ReasonCode_BadUserPassword, "Bad Username / Password"},
        {CC_Mqtt5ReasonCode_NotAuthorized, "Not Authorized"},
        {CC_Mqtt5ReasonCode_ServerUnavailable, "Server is Unavailable"},
        {CC_Mqtt5ReasonCode_ServerBusy, "Server is Busy"},
        {CC_Mqtt5ReasonCode_Banned, "Banned"},
        {CC_Mqtt5ReasonCode_ServerShuttingDown, "Server is Shutting Down"},
        {CC_Mqtt5ReasonCode_BadAuthMethod, "Bad Authentication Method"},
        {CC_Mqtt5ReasonCode_KeepAliveTimeout, "Keep Alive Timeout"},
        {CC_Mqtt5ReasonCode_SessionTakenOver, "Session is Taken Over"},
        {CC_Mqtt5ReasonCode_TopicFilterInvalid, "Topic Filter is Invalid"},
        {CC_Mqtt5ReasonCode_TopicNameInvalid, "Topic Name is Invalid"},
        {CC_Mqtt5ReasonCode_PacketIdInUse, "Packet ID is in Use"},
        {CC_Mqtt5ReasonCode_PacketIdNotFound, "Packet ID is Not Found"},
        {CC_Mqtt5ReasonCode_ReceiveMaxExceeded, "Receive Maximum is Exceeded"},
        {CC_Mqtt5ReasonCode_TopicAliasInvalid, "Topic Alias is Invalid"},
        {CC_Mqtt5ReasonCode_PacketTooLarge, "Packet is Too Large"},
        {CC_Mqtt5ReasonCode_MsgRateTooHigh, "Message Rate is Too High"},
        {CC_Mqtt5ReasonCode_QuotaExceeded, "Quota is Exceeded"},
        {CC_Mqtt5ReasonCode_AdministrativeAction, "Administrative Action"},
        {CC_Mqtt5ReasonCode_PayloadFormatInvalid, "Payload Format is Invalid"},
        {CC_Mqtt5ReasonCode_RetainNotSupported, "Retain is Not Supported"},
        {CC_Mqtt5ReasonCode_QosNotSupported, "QoS is Not Supported"},
        {CC_Mqtt5ReasonCode_UseAnotherServer, "Use Another Server"},
        {CC_Mqtt5ReasonCode_ServerMoved, "Server Moved"},
        {CC_Mqtt5ReasonCode_SharedSubNotSuppored, "Shared Subscriptions are Not Supported"},
        {CC_Mqtt5ReasonCode_ConnectionRateExceeded, "Connection Ratio Exceeded"},
        {CC_Mqtt5ReasonCode_MaxConnectTime, "Maximum Connection Time is Exceeded"},
        {CC_Mqtt5ReasonCode_SubIdsNotSupported, "Subscription IDs are Not Supported"},
        {CC_Mqtt5ReasonCode_WildcardSubsNotSupported, "Wildcard Subscriptions are Not Supported"},
    };

    auto iter = Map.find(val);
    if (iter == Map.end()) {
        return std::to_string(val);
    }

    return iter->second + " (" + std::to_string(val) + ')';
}

std::string toString(CC_Mqtt5QoS val)
{
    static const std::string Map[] = {
        /* CC_Mqtt5QoS_AtMostOnceDelivery */ "QoS0 - At Most Once Delivery",
        /* CC_Mqtt5QoS_AtLeastOnceDelivery */ "QoS1 - At Least Once Delivery",
        /* CC_Mqtt5QoS_ExactlyOnceDelivery */ "QoS2 - Exactly Once Delivery",
    };
    static constexpr std::size_t MapSize = std::extent<decltype(Map)>::value;
    static_assert(MapSize == CC_Mqtt5QoS_ValuesLimit);

    auto idx = static_cast<unsigned>(val);
    if (MapSize <= idx) {
        assert(false); // Should not happen
        return std::to_string(val);
    }

    return Map[idx] + " (" + std::to_string(val) + ')';
}

void printReasonCode(CC_Mqtt5ReasonCode val)
{
    std::cout << "\tReason Code: " << toString(val) << '\n';
}

void printQos(const char* prefix, CC_Mqtt5QoS val)
{
    std::cout << "\t" << prefix << ": " << toString(val) << '\n';
}

void printString(const char* prefix, const char* val)
{
    if (val != nullptr) {
        std::cout << '\t' << prefix << ": " << val << '\n';
    }
}

void printUnsigned(const char* prefix, unsigned val, unsigned ignoreValue = 0U, bool forcePrint = false)
{
    if (forcePrint || (val != ignoreValue)) {
        std::cout << '\t' << prefix << ": " << val << '\n';
    }
}

void printBool(const char* prefix, bool val)
{
    std::cout << '\t' << prefix << ": " << std::boolalpha << val << '\n';
}

void printReasonString(const char* val)
{
    printString("Reason String", val);
}

void printResponseInfo(const char* val)
{
    printString("Response Information", val);
}

void printUserProperties(const CC_Mqtt5UserProp* props, unsigned propsCount)
{
    for (auto idx = 0U; idx < propsCount; ++idx) {
        auto& p = props[idx];
        std::cout << "\tUser Property: " << p.m_key << " = " << p.m_value << '\n';
    }    
}

void printData(const char* prefix, const unsigned char* data, unsigned dataLen)
{
    if (dataLen == 0U) {
        return;
    }

    std::cout << '\t' << prefix << ": " << std::hex;

    for (auto idx = 0U; idx < dataLen; ++idx) {
        std::cout << std::setw(2) << std::setfill('0') << static_cast<unsigned>(data[idx]) << ' ';
    }
    std::cout << '\n';
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

    if (!createSession()) {
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

void AppClient::doTerminate()
{
    m_session.reset();
    m_io.stop();
}

bool AppClient::startImpl()
{
    auto clientId = m_opts.clientId();

    auto basicConfig = CC_Mqtt5ConnectBasicConfig();
    ::cc_mqtt5_client_connect_init_config_basic(&basicConfig);
    basicConfig.m_cleanStart = true;

    if (!clientId.empty()) {
        basicConfig.m_clientId = clientId.c_str();
    }

    auto willConfig = CC_Mqtt5ConnectWillConfig();
    CC_Mqtt5ConnectWillConfig* willConfigPtr = nullptr;

    ::cc_mqtt5_client_connect_init_config_will(&willConfig);

    auto extraConfig = CC_Mqtt5ConnectExtraConfig();
    CC_Mqtt5ConnectExtraConfig* extraConfigPtr = nullptr;

    ::cc_mqtt5_client_connect_init_config_extra(&extraConfig);

    bool result = asyncConnect(
        &basicConfig, willConfigPtr, extraConfigPtr,
        [this](CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response)
        {
            do {
                if (status != CC_Mqtt5AsyncOpStatus_Complete) {
                    logError() << "Connection operation was not properly completed: status=" << status << std::endl;
                    break;
                }

                assert(response != nullptr);
                if (m_opts.verbose()) {
                    print(*response);
                }
                
                if (CC_Mqtt5ReasonCode_UnspecifiedError <= response->m_reasonCode) {
                    logError() << "Connection attempt was rejected" << std::endl;
                    break;
                }

                brokerConnectedImpl();
                return;
            } while (false);

            doTerminate();
        });

    return result;
}

void AppClient::brokerConnectedImpl()
{
    assert(false); // Expected to be overriden
}

void AppClient::brokerDisconnectedImpl(const CC_Mqtt5DisconnectInfo* info)
{
    logError() << "Broker disconnected." << std::endl;
    doTerminate();
    if ((info == nullptr) || (!m_opts.verbose())) {
        return;
    }

    print(*info);
}

void AppClient::print(const CC_Mqtt5DisconnectInfo& info)
{
    std::cout << "Disconnect Info:\n";
    printReasonCode(info.m_reasonCode);
    printReasonString(info.m_reasonStr);
    printString("Server Reference", info.m_serverRef);
    printUserProperties(info.m_userProps, info.m_userPropsCount);
    std::cout << std::endl;
}

void AppClient::print(const CC_Mqtt5ConnectResponse& response)
{
    std::cout << "Connection Response:\n";
    printReasonCode(response.m_reasonCode);
    printString("Assinged Client ID" , response.m_assignedClientId);
    printResponseInfo(response.m_responseInfo);
    printReasonString(response.m_reasonStr);
    printString("Server Reference", response.m_serverRef);
    printData("Authentication Data", response.m_authData, response.m_authDataLen);
    printUserProperties(response.m_userProps, response.m_userPropsCount);
    printUnsigned("Session Expiry Interval", response.m_sessionExpiryInterval);
    printUnsigned("Receive Maximum", response.m_highQosSendLimit);
    printUnsigned("Maximum Packet Size", response.m_maxPacketSize);
    printUnsigned("Topic Alias Maximum", response.m_topicAliasMax);
    printQos("Maximum QoS", response.m_maxQos);
    printBool("Session Present", response.m_sessionPresent);
    printBool("Retain Available", response.m_retainAvailable);
    printBool("Wildcard Subscriptions Available", response.m_wildcardSubAvailable);
    printBool("Subscription IDs Available", response.m_subIdsAvailable);
    printBool("Shared Subscription Available", response.m_sharedSubsAvailable);
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
                doTerminate();
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

void AppClient::sendDataInternal(const unsigned char* buf, unsigned bufLen)
{
    assert(m_session);
    m_session->sendData(buf, bufLen);
}

bool AppClient::createSession()
{
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

    m_session->setNetworkDisconnectedReportCb(
        [this](bool disconnected)
        {
            assert(m_client);
            ::cc_mqtt5_client_notify_network_disconnected(m_client.get(), disconnected);
        }
    );

    if (!m_session->start()) {
        logError() << "Failed to connect to the broker." << std::endl;
        return false;
    }

    return startImpl();
}

void AppClient::sendDataCb(void* data, const unsigned char* buf, unsigned bufLen)
{
    asThis(data)->sendDataInternal(buf, bufLen);
}

void AppClient::brokerDisconnectedCb(void* data, const CC_Mqtt5DisconnectInfo* info)
{
    asThis(data)->brokerDisconnectedImpl(info);
}

void AppClient::messageReceivedCb(void* data, const CC_Mqtt5MessageInfo* info)
{
    // TODO:
    static_cast<void>(data);
    static_cast<void>(info);    
}

void AppClient::logMessageCb([[maybe_unused]] void* data, const char* msg)
{
    logError() << msg << std::endl;
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
