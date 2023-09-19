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

    struct UnitTestUserProp
    {
        std::string m_key;
        std::string m_value;

        UnitTestUserProp() = default;
        UnitTestUserProp(const UnitTestUserProp&) = default;
        UnitTestUserProp(UnitTestUserProp&&) = default;

        explicit UnitTestUserProp(const CC_Mqtt5UserProp& other)
        {
            *this = other;
        }

        UnitTestUserProp operator=(const CC_Mqtt5UserProp& other)
        {
            std::tie(m_key, m_value) = std::tie(other.m_key, m_value);
            return *this;
        }
    };

    struct UnitTestConnectResponse
    {
        CC_Mqtt5ReasonCode m_reasonCode;
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

        UnitTestConnectResponse operator=(const CC_Mqtt5ConnectResponse& response)
        {
            auto thisTie = 
                std::tie(
                    m_reasonCode, m_assignedClientId, m_reasonStr, m_serverRef, m_authMethod,
                    m_sessionExpiryInterval, m_highQosPubLimit, m_maxPacketSize, m_topicAliasMax,
                    m_maxQos, m_sessionPresent, m_retainAvailable, m_wildcardSubAvailable, m_subIdsAvailable, 
                    m_sharedSubsAvailable);

            auto responseTie = 
                std::tie(
                    response.m_reasonCode, response.m_assignedClientId, response.m_reasonStr, response.m_serverRef, response.m_authMethod,
                    response.m_sessionExpiryInterval, response.m_highQosPubLimit, response.m_maxPacketSize, response.m_topicAliasMax,
                    response.m_maxQos, response.m_sessionPresent, response.m_retainAvailable, response.m_wildcardSubAvailable, response.m_subIdsAvailable, 
                    response.m_sharedSubsAvailable);            

            thisTie = responseTie;

            
            if (response.m_authDataLen > 0) {
                m_authData.assign(response.m_authData, response.m_authData + response.m_authDataLen);
            }
            else {
                assert(response.m_authData == nullptr);
                m_authData.clear();
            }

            m_userProps.clear();
            if (response.m_userPropsCount > 0) {
                std::transform(response.m_userProps, response.m_userProps + response.m_userPropsCount, std::back_inserter(m_userProps),
                [](auto& prop)
                {
                    return UnitTestUserProp{prop};
                });
            }

            return *this;
        }
    };

    struct UnitTestConnectResponseInfo
    {
        CC_Mqtt5AsyncOpStatus m_status = CC_Mqtt5AsyncOpStatus_ValuesLimit;
        UnitTestConnectResponse m_response;
    };

    using UnitTestConnectResponseInfoPtr = std::unique_ptr<UnitTestConnectResponseInfo>;

    void unitTestSetUp()
    {
        assert(!m_client);
        m_tickReq.clear();
        m_sentData.clear();
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

    decltype(auto) unitTestTickReq()
    {
        assert(!m_tickReq.empty());
        return m_tickReq.front();
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

    bool unitTestIsConnectComplete()
    {
        return (!m_connectResp.empty());
    }

private:
    struct TickInfo
    {
        unsigned m_requested = 0U;
        unsigned m_elapsed = 0U;
    };

    static void unitTestBrokerDisconnectedCb(void* obj, const CC_Mqtt5DisconnectInfo* info)
    {
        static_cast<void>(obj);
        static_cast<void>(info);
        // TODO:
    }

    static void unitTestSendOutputDataCb(void* obj, const unsigned char* buf, unsigned bufLen)
    {
        auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
        std::copy_n(buf, bufLen, std::back_inserter(realObj->m_sentData));
    }

    static void unitTestProgramNextTickCb(void* obj, unsigned duration)
    {
        std::cout << "!!! Next tick: " << duration << std::endl;
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
        ptr->m_response = *response;
    }

    UnitTestClientPtr m_client;
    std::vector<TickInfo> m_tickReq;
    std::vector<std::uint8_t> m_sentData;
    std::vector<UnitTestConnectResponseInfoPtr> m_connectResp;
};