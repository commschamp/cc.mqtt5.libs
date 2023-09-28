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

        UnitTestUserProp& operator=(const CC_Mqtt5UserProp& other);
        UnitTestUserProp& operator=(const UnitTestUserProp&) = default;

        using List = std::vector<UnitTestUserProp>;
        static void copyProps(const CC_Mqtt5UserProp* userProps, unsigned userPropsCount, List& list);
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
        UnitTestAuthInfo& operator=(const CC_Mqtt5AuthInfo& other);
    };

    struct UnitTestConnectResponse
    {
        CC_Mqtt5ReasonCode m_reasonCode = CC_Mqtt5ReasonCode_UnspecifiedError;
        std::string m_assignedClientId;
        std::string m_responseInfo;
        std::string m_reasonStr;
        std::string m_serverRef;
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

        UnitTestConnectResponse& operator=(const CC_Mqtt5ConnectResponse& response);
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
        UnitTestDisconnectInfo& operator=(const CC_Mqtt5DisconnectInfo& other);
    };    

    struct TickInfo
    {
        unsigned m_requested = 0U;
        unsigned m_elapsed = 0U;
    };    

    void unitTestSetUp();
    void unitTestTearDown();
    UnitTestClientPtr::pointer unitTestAllocClient();

    decltype(auto) unitTestSentData()
    {
        return m_sentData;
    }

    const TickInfo* unitTestTickReq();
    bool unitTestCheckNoTicks();
    void unitTestTick(unsigned ms = 0, bool forceTick = false);
    CC_Mqtt5ErrorCode unitTestSendConnect(CC_Mqtt5ConnectHandle connect);
    UniTestsMsgPtr unitTestGetSentMessage();
    bool unitTestHasSentMessage() const;
    bool unitTestIsConnectComplete();
    const UnitTestConnectResponseInfo& unitTestConnectResponseInfo();
    void unitTestPopConnectResponseInfo();
    void unitTestReceiveMessage(const UnitTestMessage& msg, bool reportReceivedData = true);
    CC_Mqtt5ErrorCode unitTestConfigAuth(CC_Mqtt5ConnectHandle handle, const std::string& method, const std::vector<std::uint8_t>& data);
    void unitTestAddOutAuth(const UnitTestAuthInfo& info);
    void unitTestClearAuth();
    const UnitTestAuthInfo& unitTestInAuthInfo() const;
    void unitTestPopInAuthInfo();
    bool unitTestIsDisconnected() const;
    bool unitTestHasDisconnectInfo() const;
    const UnitTestDisconnectInfo& unitTestDisconnectInfo() const;
    void unitTestPopDisconnectInfo();

private:

    static void unitTestBrokerDisconnectedCb(void* obj, const CC_Mqtt5DisconnectInfo* info);
    static void unitTestSendOutputDataCb(void* obj, const unsigned char* buf, unsigned bufLen);
    static void unitTestProgramNextTickCb(void* obj, unsigned duration);
    static unsigned unitTestCancelNextTickWaitCb(void* obj);
    static void unitTestConnectCompleteCb(void* obj, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response);
    static CC_Mqtt5AuthErrorCode unitTestAuthCb(void* obj, const CC_Mqtt5AuthInfo* authInfoIn, CC_Mqtt5AuthInfo* authInfoOut);
    
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