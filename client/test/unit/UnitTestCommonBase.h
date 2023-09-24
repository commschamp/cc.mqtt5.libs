#pragma once

#include "UnitTestDefs.h"
#include "UnitTestProtocolDefs.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <tuple>
#include <vector>

class UnitTestCommonBase
{
protected:
    static constexpr unsigned UnitTestDefaultOpTimeoutMs = 2000;

    using UnitTestData = std::vector<std::uint8_t>;

    struct UnitTestUserProp
    {
        std::string m_key;
        std::string m_value;

        UnitTestUserProp() = default;
        UnitTestUserProp(const UnitTestUserProp&) = default;
        UnitTestUserProp(const std::string& key, const std::string& value) : m_key(key), m_value(value) {}

        explicit UnitTestUserProp(const CC_Mqtt5UserProp& other)
        {
            *this = other;
        }

        UnitTestUserProp& operator=(const CC_Mqtt5UserProp& other)
        {
            std::tie(m_key, m_value) = std::make_tuple(other.m_key, other.m_value);
            return *this;
        }

        UnitTestUserProp& operator=(const UnitTestUserProp&) = default;

        using List = std::vector<UnitTestUserProp>;
        static void copyProps(const CC_Mqtt5UserProp* userProps, unsigned userPropsCount, List& list)
        {
            list.clear();
            if (userPropsCount > 0) {
                std::transform(userProps, userProps + userPropsCount, std::back_inserter(list),
                [](auto& prop)
                {
                    return UnitTestUserProp{prop};
                });   
            }         
        }
    };

    struct UnitTestAuthInfo
    {
        UnitTestData m_authData;
        std::string m_reasonStr;
        UnitTestUserProp::List m_userProps;

        UnitTestAuthInfo() = default;
        UnitTestAuthInfo(const UnitTestAuthInfo&) = default;
        explicit UnitTestAuthInfo(const CC_Mqtt5AuthInfo& other)
        {
            *this = other;
        }

        UnitTestAuthInfo& operator=(const UnitTestAuthInfo&) = default;
        UnitTestAuthInfo& operator=(const CC_Mqtt5AuthInfo& other)
        {
            m_authData.clear();
            if (other.m_authDataLen > 0U) {
                std::copy_n(other.m_authData, other.m_authDataLen, std::back_inserter(m_authData));
            }

            m_reasonStr.clear();
            if (other.m_reasonStr != nullptr) {
                m_reasonStr = other.m_reasonStr;
            }

            UnitTestUserProp::copyProps(other.m_userProps, other.m_userPropsCount, m_userProps);
            return *this;
        }
    };

    struct UnitTestConnectResponse
    {
        CC_Mqtt5ReasonCode m_reasonCode = CC_Mqtt5ReasonCode_UnspecifiedError;
        std::string m_assignedClientId;
        std::string m_responseInfo;
        std::string m_reasonStr;
        std::string m_serverRef;
        std::string m_authMethod;
        std::vector<std::uint8_t> m_authData;
        std::vector<UnitTestUserProp> m_userProps;
        unsigned m_sessionExpiryInterval = 0;
        unsigned m_highQosPubLimit = 0;
        unsigned m_maxPacketSize = 0;
        unsigned m_topicAliasMax = 0;
        CC_Mqtt5QoS m_maxQos = CC_Mqtt5QoS_ExactlyOnceDelivery;
        bool m_sessionPresent = false;
        bool m_retainAvailable = false;
        bool m_wildcardSubAvailable = false;
        bool m_subIdsAvailable = false;
        bool m_sharedSubsAvailable = false;

        UnitTestConnectResponse& operator=(const CC_Mqtt5ConnectResponse& response)
        {
            auto assignString = 
                [](std::string& dest, const char* source)
                {
                    dest.clear();
                    if (source != nullptr)
                    {
                        dest = source;
                    }
                };

            auto thisTie = 
                std::tie(
                    m_reasonCode, m_sessionExpiryInterval, m_highQosPubLimit, m_maxPacketSize, m_topicAliasMax,
                    m_maxQos, m_sessionPresent, m_retainAvailable, m_wildcardSubAvailable, m_subIdsAvailable, 
                    m_sharedSubsAvailable);

            auto responseTie = 
                std::forward_as_tuple(
                    response.m_reasonCode, response.m_sessionExpiryInterval, response.m_highQosPubLimit, response.m_maxPacketSize, response.m_topicAliasMax,
                    response.m_maxQos, response.m_sessionPresent, response.m_retainAvailable, response.m_wildcardSubAvailable, response.m_subIdsAvailable, 
                    response.m_sharedSubsAvailable);            

            thisTie = responseTie;

            assignString(m_assignedClientId, response.m_assignedClientId);
            assignString(m_responseInfo, response.m_responseInfo);
            assignString(m_reasonStr, response.m_reasonStr);
            assignString(m_serverRef, response.m_serverRef);
            assignString(m_authMethod, response.m_authMethod);
            
            if (response.m_authDataLen > 0) {
                m_authData.assign(response.m_authData, response.m_authData + response.m_authDataLen);
            }
            else {
                assert(response.m_authData == nullptr);
                m_authData.clear();
            }

            UnitTestUserProp::copyProps(response.m_userProps, response.m_userPropsCount, m_userProps);
            return *this;
        }
    };

    struct UnitTestConnectResponseInfo
    {
        CC_Mqtt5AsyncOpStatus m_status = CC_Mqtt5AsyncOpStatus_ValuesLimit;
        UnitTestConnectResponse m_response;
    };

    using UnitTestConnectResponseInfoPtr = std::unique_ptr<UnitTestConnectResponseInfo>;

    struct UnitTestDisconnectInfo
    {
        CC_Mqtt5ReasonCode m_reasonCode = CC_Mqtt5ReasonCode_UnspecifiedError;
        std::string m_reasonStr;
        std::string m_serverRef;
        UnitTestUserProp::List m_userProps;

        UnitTestDisconnectInfo() = default;
        UnitTestDisconnectInfo(const UnitTestDisconnectInfo&) = default;

        UnitTestDisconnectInfo(const CC_Mqtt5DisconnectInfo& other)
        {
            *this = other;
        }

        UnitTestDisconnectInfo& operator=(const UnitTestDisconnectInfo&) = default;
        UnitTestDisconnectInfo& operator=(const CC_Mqtt5DisconnectInfo& other)
        {
            std::tie(m_reasonCode, m_reasonStr, m_serverRef) = std::make_tuple(other.m_reasonCode, other.m_reasonStr, other.m_serverRef);
            UnitTestUserProp::copyProps(other.m_userProps, other.m_userPropsCount, m_userProps);
            return *this;
        }
    };    

    struct TickInfo
    {
        unsigned m_requested = 0U;
        unsigned m_elapsed = 0U;
    };    

    void unitTestSetUp()
    {
        assert(!m_client);
        m_tickReq.clear();
        m_sentData.clear();
        m_connectResp.clear();
        m_inAuthInfo.clear();
        m_outAuthInfo.clear();
        m_disconnectInfo.clear();
        m_disconnected = false;
    }

    void unitTestTearDown()
    {
        m_client.reset();
    }

    UnitTestClientPtr::pointer unitTestAllocClient()
    {
        m_client.reset(cc_mqtt5_client_new());
        cc_mqtt5_client_set_broker_disconnect_report_callback(m_client.get(), &UnitTestCommonBase::unitTestBrokerDisconnectedCb, this);
        cc_mqtt5_client_set_send_output_data_callback(m_client.get(), &UnitTestCommonBase::unitTestSendOutputDataCb, this);
        cc_mqtt5_client_set_next_tick_program_callback(m_client.get(), &UnitTestCommonBase::unitTestProgramNextTickCb, this);
        cc_mqtt5_client_set_cancel_next_tick_wait_callback(m_client.get(), &UnitTestCommonBase::unitTestCancelNextTickWaitCb, this);
        auto ec = cc_mqtt5_client_init(m_client.get());
        assert(ec == CC_Mqtt5ErrorCode_Success);
        return m_client.get();
    }

    decltype(auto) unitTestSentData()
    {
        return m_sentData;
    }

    const TickInfo* unitTestTickReq()
    {
        assert(!m_tickReq.empty());
        return &m_tickReq.front();
    }

    bool unitTestCheckNoTicks()
    {
        return m_tickReq.empty();
    }

    void unitTestTick(unsigned ms = 0, bool forceTick = false)
    {
        assert(!m_tickReq.empty());
        auto& tick = m_tickReq.front();
        auto rem = tick.m_requested - tick.m_elapsed;
        assert(ms <= rem);
        if (ms == 0U) {
            ms = rem;
        }

        if (ms < rem) {
            tick.m_elapsed += ms;

            if (!forceTick) {
                return;
            }
        }

        m_tickReq.erase(m_tickReq.begin());
        assert(m_client);
        cc_mqtt5_client_tick(m_client.get(), ms);
    }

    CC_Mqtt5ErrorCode unitTestSendConnect(CC_Mqtt5ConnectHandle connect)
    {
        return cc_mqtt5_client_connect_send(connect, &UnitTestCommonBase::unitTestConnectCompleteCb, this);
    }

    UniTestsMsgPtr unitTestGetSentMessage()
    {
        UniTestsMsgPtr msg;
        UnitTestsFrame frame;
        
        assert(!m_sentData.empty());
        UnitTestMessage::ReadIterator begIter = &m_sentData[0];
        auto readIter = begIter;
        [[maybe_unused]] auto readEs = frame.read(msg, readIter, m_sentData.size());
        assert(readEs == comms::ErrorStatus::Success);
        auto consumed = std::distance(begIter, readIter);
        m_sentData.erase(m_sentData.begin(), m_sentData.begin() + consumed);
        return msg;
    }

    bool unitTestHasSentMessage() const
    {
        return !m_sentData.empty();
    }

    bool unitTestIsConnectComplete()
    {
        return (!m_connectResp.empty()) && (m_connectResp.front());
    }

    const UnitTestConnectResponseInfo& unitTestConnectResponseInfo()
    {
        assert(unitTestIsConnectComplete());
        return *m_connectResp.front();
    }

    void unitTestPopConnectResponseInfo()
    {
        assert(!m_connectResp.empty());
        m_connectResp.erase(m_connectResp.begin());
    }

    void unitTestReceiveMessage(const UnitTestMessage& msg, bool reportReceivedData = true)
    {
        UnitTestsFrame frame;
        auto prevSize = m_receivedData.size();
        m_receivedData.reserve(prevSize + frame.length(msg));
        auto writeIter = std::back_inserter(m_receivedData);
        auto es = frame.write(msg, writeIter, m_receivedData.max_size());
        if (es == comms::ErrorStatus::UpdateRequired) {
            auto* updateIter = &m_receivedData[prevSize];
            es = frame.update(msg, updateIter, m_receivedData.size() - prevSize);
        }
        assert(es == comms::ErrorStatus::Success);

        if (!reportReceivedData) {
            return;
        }

        auto consumed = cc_mqtt5_client_process_data(m_client.get(), &m_receivedData[0], static_cast<unsigned>(m_receivedData.size()));
        m_receivedData.erase(m_receivedData.begin(), m_receivedData.begin() + consumed);
    }

    CC_Mqtt5ErrorCode unitTestConfigAuth(CC_Mqtt5ConnectHandle handle, const std::string& method, const std::vector<std::uint8_t>& data)
    {
        auto config = CC_Mqtt5ConnectAuthConfig();
        cc_mqtt5_client_connect_init_config_auth(&config);

        config.m_authMethod = method.c_str();
        comms::cast_assign(config.m_authDataLen) = data.size();
        if (!data.empty()) {
            config.m_authData = &data[0];
        }

        config.m_authCb = &UnitTestCommonBase::unitTestAuthCb;
        config.m_authCbData = this;
        return cc_mqtt5_client_connect_config_auth(handle, &config);
    }

    void unitTestAddOutAuth(const UnitTestAuthInfo& info)
    {
        m_outAuthInfo.push_back(info);
    }

    void unitTestClearAuth()
    {
        m_inAuthInfo.clear();
        m_outAuthInfo.clear();
    }

    const UnitTestAuthInfo& unitTestInAuthInfo() const
    {
        assert(!m_inAuthInfo.empty());
        return m_inAuthInfo.front();
    }

    void unitTestPopInAuthInfo()
    {
        assert(!m_inAuthInfo.empty());
        m_inAuthInfo.erase(m_inAuthInfo.begin());
    }

    bool unitTestIsDisconnected() const
    {
        return m_disconnected;
    }

    bool unitTestHasDisconnectInfo() const
    {
        return (!m_disconnectInfo.empty());
    }

    const UnitTestDisconnectInfo& unitTestDisconnectInfo() const
    {
        assert(!m_disconnectInfo.empty());
        return m_disconnectInfo.front();
    }

    void unitTestPopDisconnectInfo()
    {
        assert(!m_disconnectInfo.empty());
        m_disconnectInfo.erase(m_disconnectInfo.begin());
    }

private:

    static void unitTestBrokerDisconnectedCb(void* obj, const CC_Mqtt5DisconnectInfo* info)
    {
        auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
        assert(!realObj->m_disconnected);
        realObj->m_disconnected = true;
        if (info != nullptr) {
            assert(realObj->m_disconnectInfo.empty());
            realObj->m_disconnectInfo.emplace_back(*info);
        }
    }

    static void unitTestSendOutputDataCb(void* obj, const unsigned char* buf, unsigned bufLen)
    {
        auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
        std::copy_n(buf, bufLen, std::back_inserter(realObj->m_sentData));
    }

    static void unitTestProgramNextTickCb(void* obj, unsigned duration)
    {
        auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
        assert(realObj->m_tickReq.empty());
        realObj->m_tickReq.push_back({duration, 0U});
    }

    static unsigned unitTestCancelNextTickWaitCb(void* obj) {
        auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
        assert(!realObj->m_tickReq.empty());
        auto elapsed = realObj->m_tickReq.front().m_elapsed;
        realObj->m_tickReq.erase(realObj->m_tickReq.begin());
        return elapsed;
    }

    static void unitTestConnectCompleteCb(void* obj, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response)
    {
        auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
        assert(realObj->m_connectResp.empty());
        realObj->m_connectResp.resize(realObj->m_connectResp.size() + 1U);
        auto& ptr = realObj->m_connectResp.back();
        ptr = std::make_unique<UnitTestConnectResponseInfo>();
        ptr->m_status = status;
        if (response != nullptr) {
            ptr->m_response = *response;
        }
    }

    static CC_Mqtt5AuthErrorCode unitTestAuthCb(void* obj, const CC_Mqtt5AuthInfo* authInfoIn, CC_Mqtt5AuthInfo* authInfoOut)
    {
        assert(authInfoIn != nullptr);
        auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
  
        auto idx = realObj->m_inAuthInfo.size();
        realObj->m_inAuthInfo.push_back(UnitTestAuthInfo{*authInfoIn});
        if (realObj->m_outAuthInfo.size() <= idx) {
            return CC_Mqtt5AuthErrorCode_Disconnect;
        }

        assert(authInfoOut != nullptr);
        auto& outInfo = realObj->m_outAuthInfo[idx];
        comms::cast_assign(authInfoOut->m_authDataLen) = outInfo.m_authData.size();
        if (!outInfo.m_authData.empty()) {
            authInfoOut->m_authData = &outInfo.m_authData[0];
        }

        if (!outInfo.m_reasonStr.empty()) {
            authInfoOut->m_reasonStr = outInfo.m_reasonStr.c_str();
        }

        realObj->m_userPropsTmp.clear();
        comms::cast_assign(authInfoOut->m_userPropsCount) = outInfo.m_userProps.size();
        if (!outInfo.m_userProps.empty()) {
            std::transform(
                outInfo.m_userProps.begin(), outInfo.m_userProps.end(), std::back_inserter(realObj->m_userPropsTmp),
                [](auto& p)
                {
                    return CC_Mqtt5UserProp{p.m_key.c_str(), p.m_value.c_str()};
                });
            authInfoOut->m_userProps = &realObj->m_userPropsTmp[0];
        }

        return CC_Mqtt5AuthErrorCode_Continue;
    }

    UnitTestClientPtr m_client;
    std::vector<TickInfo> m_tickReq;
    std::vector<std::uint8_t> m_sentData;
    std::vector<std::uint8_t> m_receivedData;
    std::vector<UnitTestConnectResponseInfoPtr> m_connectResp;
    std::vector<UnitTestAuthInfo> m_inAuthInfo;
    std::vector<UnitTestAuthInfo> m_outAuthInfo;
    std::vector<CC_Mqtt5UserProp> m_userPropsTmp;
    std::vector<UnitTestDisconnectInfo> m_disconnectInfo;
    bool m_disconnected = false;
};