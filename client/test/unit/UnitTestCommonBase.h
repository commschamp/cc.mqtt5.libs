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
public:
    using UnitTestData = std::vector<std::uint8_t>;

protected:
    static constexpr unsigned UnitTestDefaultOpTimeoutMs = 2000;

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

    struct UnitTestSubscribeResponse
    {
        std::vector<CC_Mqtt5ReasonCode> m_reasonCodes;
        std::string m_reasonStr;
        std::vector<UnitTestUserProp> m_userProps;

        UnitTestSubscribeResponse() = default;
        UnitTestSubscribeResponse(const UnitTestSubscribeResponse& other) = default;
        explicit UnitTestSubscribeResponse(const CC_Mqtt5SubscribeResponse& other)
        {
            *this = other;
        }

        UnitTestSubscribeResponse& operator=(const UnitTestSubscribeResponse&) = default;
        UnitTestSubscribeResponse& operator=(const CC_Mqtt5SubscribeResponse& response);
    };

    struct UnitTestUnsubscribeResponse
    {
        std::vector<CC_Mqtt5ReasonCode> m_reasonCodes;
        std::string m_reasonStr;
        std::vector<UnitTestUserProp> m_userProps;

        UnitTestUnsubscribeResponse() = default;
        UnitTestUnsubscribeResponse(const UnitTestUnsubscribeResponse& other) = default;
        UnitTestUnsubscribeResponse(const CC_Mqtt5UnsubscribeResponse& other)
        {
            *this = other;
        }

        UnitTestUnsubscribeResponse& operator=(const UnitTestUnsubscribeResponse&) = default;
        UnitTestUnsubscribeResponse& operator=(const CC_Mqtt5UnsubscribeResponse& response);
    };    

    struct UnitTestConnectResponseInfo
    {
        CC_Mqtt5AsyncOpStatus m_status = CC_Mqtt5AsyncOpStatus_ValuesLimit;
        UnitTestConnectResponse m_response;
    };

    struct UnitTestSubscribeResponseInfo
    {
        CC_Mqtt5AsyncOpStatus m_status = CC_Mqtt5AsyncOpStatus_ValuesLimit;
        UnitTestSubscribeResponse m_response;
    };  

    struct UnitTestUnsubscribeResponseInfo
    {
        CC_Mqtt5AsyncOpStatus m_status = CC_Mqtt5AsyncOpStatus_ValuesLimit;
        UnitTestUnsubscribeResponse m_response;
    };          

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

    struct UnitTestPublishResponse
    {
        CC_Mqtt5ReasonCode m_reasonCode = CC_Mqtt5ReasonCode_UnspecifiedError;
        std::string m_reasonStr;
        UnitTestUserProp::List m_userProps;

        UnitTestPublishResponse() = default;
        UnitTestPublishResponse(const UnitTestPublishResponse&) = default;

        UnitTestPublishResponse(const CC_Mqtt5PublishResponse& other)
        {
            *this = other;
        }

        UnitTestPublishResponse& operator=(const UnitTestPublishResponse&) = default;
        UnitTestPublishResponse& operator=(const CC_Mqtt5PublishResponse& other);
    };    

    struct UnitTestMessageInfo
    {
        std::string m_topic;
        UnitTestData m_data;
        std::string m_responseTopic;
        UnitTestData m_correlationData;
        UnitTestUserProp::List m_userProps;
        std::string m_contentType;
        std::vector<unsigned> m_subIds;
        CC_Mqtt5QoS m_qos = CC_Mqtt5QoS_ValuesLimit;
        CC_Mqtt5PayloadFormat m_format = CC_Mqtt5PayloadFormat_Unspecified;
        bool m_retained = false;     

        UnitTestMessageInfo() = default;
        UnitTestMessageInfo(const UnitTestMessageInfo&) = default;

        UnitTestMessageInfo(const CC_Mqtt5MessageInfo& other)
        {
            *this = other;
        }

        UnitTestMessageInfo& operator=(const UnitTestMessageInfo&) = default;
        UnitTestMessageInfo& operator=(const CC_Mqtt5MessageInfo& other);        
    };    

    struct UnitTestPublishResponseInfo
    {
        CC_Mqtt5AsyncOpStatus m_status = CC_Mqtt5AsyncOpStatus_ValuesLimit;
        UnitTestPublishResponse m_response;
    };          

    struct TickInfo
    {
        unsigned m_requested = 0U;
        unsigned m_elapsed = 0U;
    };    

    struct UnitTestConnectResponseConfig
    {
        CC_Mqtt5ReasonCode m_reasonCode = CC_Mqtt5ReasonCode_Success;
        CC_Mqtt5QoS m_maxQos = CC_Mqtt5QoS_ValuesLimit;
        unsigned m_topicAliasMax = 0U;
        unsigned m_sessionExpiryInterval = 0U;
        unsigned m_maxPacketSize = 0U;
        bool m_sessionPresent = false;
        bool* m_retainAvailable = nullptr;
    };

    void unitTestSetUp();
    void unitTestTearDown();
    UnitTestClientPtr::pointer unitTestAllocClient(bool addLog = false);

    decltype(auto) unitTestSentData()
    {
        return m_sentData;
    }

    const TickInfo* unitTestTickReq();
    bool unitTestCheckNoTicks();
    void unitTestTick(unsigned ms = 0, bool forceTick = false);
    CC_Mqtt5ErrorCode unitTestSendConnect(CC_Mqtt5ConnectHandle& connect);
    CC_Mqtt5ErrorCode unitTestSendSubscribe(CC_Mqtt5SubscribeHandle& subscribe);
    CC_Mqtt5ErrorCode unitTestSendUnsubscribe(CC_Mqtt5UnsubscribeHandle& unsubscribe);
    CC_Mqtt5ErrorCode unitTestSendPublish(CC_Mqtt5PublishHandle& publish);
    UniTestsMsgPtr unitTestGetSentMessage();
    bool unitTestHasSentMessage() const;
    bool unitTestIsConnectComplete();
    const UnitTestConnectResponseInfo& unitTestConnectResponseInfo();
    void unitTestPopConnectResponseInfo();
    bool unitTestIsSubscribeComplete();
    const UnitTestSubscribeResponseInfo& unitTestSubscribeResponseInfo();
    void unitTestPopSubscribeResponseInfo();
    bool unitTestIsUnsubscribeComplete();
    const UnitTestUnsubscribeResponseInfo& unitTestUnsubscribeResponseInfo();
    void unitTestPopUnsubscribeResponseInfo();
    bool unitTestIsPublishComplete();
    const UnitTestPublishResponseInfo& unitTestPublishResponseInfo();
    void unitTestPopPublishResponseInfo();
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
    bool unitTestHasMessageRecieved();
    const UnitTestMessageInfo& unitTestReceivedMessageInfo();
    void unitTestPopReceivedMessageInfo();      
    void unitTestPerformConnect(
        CC_Mqtt5Client* client, 
        const CC_Mqtt5ConnectBasicConfig* basicConfig,
        const CC_Mqtt5ConnectWillConfig* willConfig = nullptr,
        const CC_Mqtt5ConnectExtraConfig* extraConfig = nullptr,
        CC_Mqtt5ConnectAuthConfig* authConfig = nullptr,
        const UnitTestConnectResponseConfig* responseConfig = nullptr);

    void unitTestPerformBasicConnect(
        CC_Mqtt5Client* client, 
        const char* clientId, 
        bool cleanStart = true);

    void unitTestPerformPubTopicAliasConnect(
        CC_Mqtt5Client* client, 
        const char* clientId, 
        unsigned topicAliasMax,
        bool cleanStart = true);        

    void unitTestPerformSessionExpiryConnect(
        CC_Mqtt5Client* client, 
        const char* clientId, 
        unsigned sessionExpiryInterval,
        bool cleanStart = true);

    void unitTestPerformDisconnect(
        CC_Mqtt5Client* client, 
        const CC_Mqtt5DisconnectConfig* config
    );

    void unitTestPerformBasicDisconnect(CC_Mqtt5Client* client, CC_Mqtt5ReasonCode reasonCode = CC_Mqtt5ReasonCode_NormalDisconnection);

    void unitTestPerformBasicSubscribe(CC_Mqtt5Client* client, const char* topic, unsigned subId = 0U);

    using UnitTestDisconnectReason = UnitTestDisconnectMsg::Field_reasonCode::Field::ValueType;
    void unitTestVerifyDisconnectSent(UnitTestDisconnectReason reason = UnitTestDisconnectReason::Success);

private:

    static void unitTestErrorLogCb(void* obj, const char* msg);
    static void unitTestBrokerDisconnectedCb(void* obj, const CC_Mqtt5DisconnectInfo* info);
    static void unitTestMessageReceivedCb(void* obj, const CC_Mqtt5MessageInfo* info);
    static void unitTestSendOutputDataCb(void* obj, const unsigned char* buf, unsigned bufLen);
    static void unitTestProgramNextTickCb(void* obj, unsigned duration);
    static unsigned unitTestCancelNextTickWaitCb(void* obj);
    static void unitTestConnectCompleteCb(void* obj, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response);
    static void unitTestSubscribeCompleteCb(void* obj, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response);
    static void unitTestUnsubscribeCompleteCb(void* obj, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5UnsubscribeResponse* response);
    static void unitTestPublishCompleteCb(void* obj, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response);
    static CC_Mqtt5AuthErrorCode unitTestAuthCb(void* obj, const CC_Mqtt5AuthInfo* authInfoIn, CC_Mqtt5AuthInfo* authInfoOut);
    
    UnitTestClientPtr m_client;
    std::vector<TickInfo> m_tickReq;
    std::vector<std::uint8_t> m_sentData;
    std::vector<std::uint8_t> m_receivedData;
    std::vector<UnitTestConnectResponseInfo> m_connectResp;
    std::vector<UnitTestSubscribeResponseInfo> m_subscribeResp;
    std::vector<UnitTestUnsubscribeResponseInfo> m_unsubscribeResp;
    std::vector<UnitTestPublishResponseInfo> m_publishResp;
    std::vector<UnitTestAuthInfo> m_inAuthInfo;
    std::vector<UnitTestAuthInfo> m_outAuthInfo;
    std::vector<CC_Mqtt5UserProp> m_userPropsTmp;
    std::vector<UnitTestDisconnectInfo> m_disconnectInfo;
    std::vector<UnitTestMessageInfo> m_receivedMessages;
    bool m_disconnected = false;
};