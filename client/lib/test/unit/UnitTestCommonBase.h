#pragma once

#include "cc_mqtt5_client/common.h"

#include "UnitTestProtocolDefs.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <optional>
#include <tuple>
#include <vector>

class UnitTestCommonBase
{
public:
    using UnitTestData = std::vector<std::uint8_t>;

    struct LibFuncs
    {
        CC_Mqtt5ClientHandle (*m_alloc)() = nullptr;
        void (*m_free)(CC_Mqtt5ClientHandle) = nullptr;
        CC_Mqtt5ErrorCode (*m_init)(CC_Mqtt5ClientHandle) = nullptr;
        void (*m_tick)(CC_Mqtt5ClientHandle, unsigned) = nullptr;
        unsigned (*m_process_data)(CC_Mqtt5ClientHandle, const unsigned char*, unsigned) = nullptr;
        void (*m_notify_network_disconnected)(CC_Mqtt5ClientHandle) = nullptr;
        bool (*m_is_network_disconnected)(CC_Mqtt5ClientHandle) = nullptr;
        CC_Mqtt5ErrorCode (*m_set_default_response_timeout)(CC_Mqtt5ClientHandle, unsigned) = nullptr;
        unsigned (*m_get_default_response_timeout)(CC_Mqtt5ClientHandle) = nullptr;
        CC_Mqtt5ErrorCode (*m_pub_topic_alias_alloc)(CC_Mqtt5ClientHandle, const char*, unsigned) = nullptr;
        CC_Mqtt5ErrorCode (*m_pub_topic_alias_free)(CC_Mqtt5ClientHandle, const char*) = nullptr;
        unsigned (*m_pub_topic_alias_count)(CC_Mqtt5ClientHandle) = nullptr;
        bool (*m_pub_topic_alias_is_allocated)(CC_Mqtt5ClientHandle, const char*) = nullptr;
        CC_Mqtt5ErrorCode (*m_set_verify_outgoing_topic_enabled)(CC_Mqtt5ClientHandle, bool) = nullptr;
        bool (*m_get_verify_outgoing_topic_enabled)(CC_Mqtt5ClientHandle) = nullptr;
        CC_Mqtt5ErrorCode (*m_set_verify_incoming_topic_enabled)(CC_Mqtt5ClientHandle, bool) = nullptr;
        bool (*m_get_verify_incoming_topic_enabled)(CC_Mqtt5ClientHandle) = nullptr;
        CC_Mqtt5ErrorCode (*m_set_verify_incoming_msg_subscribed)(CC_Mqtt5ClientHandle, bool) = nullptr;
        bool (*m_get_verify_incoming_msg_subscribed)(CC_Mqtt5ClientHandle) = nullptr;
        void (*m_init_user_prop)(CC_Mqtt5UserProp*) = nullptr;
        CC_Mqtt5ConnectHandle (*m_connect_prepare)(CC_Mqtt5ClientHandle, CC_Mqtt5ErrorCode*) = nullptr;
        void (*m_connect_init_config_basic)(CC_Mqtt5ConnectBasicConfig*) = nullptr;
        void (*m_connect_init_config_will)(CC_Mqtt5ConnectWillConfig*) = nullptr;
        void (*m_connect_init_config_extra)(CC_Mqtt5ConnectExtraConfig*) = nullptr;
        void (*m_connect_init_config_auth)(CC_Mqtt5AuthConfig*) = nullptr;
        void (*m_connect_init_auth_info)(CC_Mqtt5AuthInfo*) = nullptr;
        CC_Mqtt5ErrorCode (*m_connect_set_response_timeout)(CC_Mqtt5ConnectHandle, unsigned) = nullptr;
        unsigned (*m_connect_get_response_timeout)(CC_Mqtt5ConnectHandle) = nullptr;
        CC_Mqtt5ErrorCode (*m_connect_config_basic)(CC_Mqtt5ConnectHandle, const CC_Mqtt5ConnectBasicConfig*) = nullptr;        
        CC_Mqtt5ErrorCode (*m_connect_config_will)(CC_Mqtt5ConnectHandle, const CC_Mqtt5ConnectWillConfig*) = nullptr;
        CC_Mqtt5ErrorCode (*m_connect_config_extra)(CC_Mqtt5ConnectHandle, const CC_Mqtt5ConnectExtraConfig*) = nullptr;
        CC_Mqtt5ErrorCode (*m_connect_config_auth)(CC_Mqtt5ConnectHandle, const CC_Mqtt5AuthConfig*) = nullptr;
        CC_Mqtt5ErrorCode (*m_connect_add_user_prop)(CC_Mqtt5ConnectHandle, const CC_Mqtt5UserProp*) = nullptr;
        CC_Mqtt5ErrorCode (*m_connect_add_will_user_prop)(CC_Mqtt5ConnectHandle, const CC_Mqtt5UserProp*) = nullptr;
        CC_Mqtt5ErrorCode (*m_connect_send)(CC_Mqtt5ConnectHandle, CC_Mqtt5ConnectCompleteCb, void*) = nullptr;
        CC_Mqtt5ErrorCode (*m_connect_cancel)(CC_Mqtt5ConnectHandle) = nullptr;
        CC_Mqtt5ErrorCode (*m_connect_simple)(CC_Mqtt5ClientHandle handle, const CC_Mqtt5ConnectBasicConfig*, CC_Mqtt5ConnectCompleteCb, void*) = nullptr;
        CC_Mqtt5ErrorCode (*m_connect_full)(CC_Mqtt5ClientHandle handle, const CC_Mqtt5ConnectBasicConfig*, const CC_Mqtt5ConnectWillConfig*, const CC_Mqtt5ConnectExtraConfig*, const CC_Mqtt5AuthConfig*, CC_Mqtt5ConnectCompleteCb, void*) = nullptr;
        bool (*m_is_connected)(CC_Mqtt5ClientHandle) = nullptr;
        CC_Mqtt5DisconnectHandle (*m_disconnect_prepare)(CC_Mqtt5ClientHandle, CC_Mqtt5ErrorCode*) = nullptr; 
        void (*m_disconnect_init_config)(CC_Mqtt5DisconnectConfig*) = nullptr;
        CC_Mqtt5ErrorCode (*m_disconnect_config)(CC_Mqtt5DisconnectHandle, const CC_Mqtt5DisconnectConfig*) = nullptr;
        CC_Mqtt5ErrorCode (*m_disconnect_add_user_prop)(CC_Mqtt5DisconnectHandle, const CC_Mqtt5UserProp*) = nullptr;
        CC_Mqtt5ErrorCode (*m_disconnect_send)(CC_Mqtt5DisconnectHandle) = nullptr;
        CC_Mqtt5ErrorCode (*m_disconnect_cancel)(CC_Mqtt5DisconnectHandle) = nullptr;
        CC_Mqtt5ErrorCode (*m_disconnect)(CC_Mqtt5ClientHandle, const CC_Mqtt5DisconnectConfig*) = nullptr;
        CC_Mqtt5SubscribeHandle (*m_subscribe_prepare)(CC_Mqtt5ClientHandle, CC_Mqtt5ErrorCode*) = nullptr;
        CC_Mqtt5ErrorCode (*m_subscribe_set_response_timeout)(CC_Mqtt5SubscribeHandle, unsigned) = nullptr;
        unsigned (*m_subscribe_get_response_timeout)(CC_Mqtt5SubscribeHandle) = nullptr;
        void (*m_subscribe_init_config_topic)(CC_Mqtt5SubscribeTopicConfig*) = nullptr;
        void (*m_subscribe_init_config_extra)(CC_Mqtt5SubscribeExtraConfig*) = nullptr;
        CC_Mqtt5ErrorCode (*m_subscribe_config_topic)(CC_Mqtt5SubscribeHandle, const CC_Mqtt5SubscribeTopicConfig*) = nullptr;
        CC_Mqtt5ErrorCode (*m_subscribe_config_extra)(CC_Mqtt5SubscribeHandle, const CC_Mqtt5SubscribeExtraConfig*) = nullptr;
        CC_Mqtt5ErrorCode (*m_subscribe_add_user_prop)(CC_Mqtt5SubscribeHandle, const CC_Mqtt5UserProp*) = nullptr;
        CC_Mqtt5ErrorCode (*m_subscribe_send)(CC_Mqtt5SubscribeHandle, CC_Mqtt5SubscribeCompleteCb, void*) = nullptr;
        CC_Mqtt5ErrorCode (*m_subscribe_cancel)(CC_Mqtt5SubscribeHandle) = nullptr;
        CC_Mqtt5ErrorCode (*m_subscribe_simple)(CC_Mqtt5ClientHandle, const CC_Mqtt5SubscribeTopicConfig*, CC_Mqtt5SubscribeCompleteCb, void*) = nullptr;
        CC_Mqtt5ErrorCode (*m_subscribe_full)(CC_Mqtt5ClientHandle, const CC_Mqtt5SubscribeTopicConfig*, unsigned, const CC_Mqtt5SubscribeExtraConfig*, CC_Mqtt5SubscribeCompleteCb, void*) = nullptr;
        CC_Mqtt5UnsubscribeHandle (*m_unsubscribe_prepare)(CC_Mqtt5ClientHandle, CC_Mqtt5ErrorCode*) = nullptr;
        CC_Mqtt5ErrorCode (*m_unsubscribe_set_response_timeout)(CC_Mqtt5UnsubscribeHandle, unsigned) = nullptr;
        unsigned (*m_unsubscribe_get_response_timeout)(CC_Mqtt5UnsubscribeHandle) = nullptr;
        void (*m_unsubscribe_init_config_topic)(CC_Mqtt5UnsubscribeTopicConfig*) = nullptr;
        CC_Mqtt5ErrorCode (*m_unsubscribe_config_topic)(CC_Mqtt5UnsubscribeHandle, const CC_Mqtt5UnsubscribeTopicConfig*) = nullptr;
        CC_Mqtt5ErrorCode (*m_unsubscribe_add_user_prop)(CC_Mqtt5UnsubscribeHandle, const CC_Mqtt5UserProp*) = nullptr;
        CC_Mqtt5ErrorCode (*m_unsubscribe_send)(CC_Mqtt5UnsubscribeHandle, CC_Mqtt5UnsubscribeCompleteCb, void*) = nullptr;
        CC_Mqtt5ErrorCode (*m_unsubscribe_cancel)(CC_Mqtt5UnsubscribeHandle) = nullptr;
        CC_Mqtt5ErrorCode (*m_unsubscribe_simple)(CC_Mqtt5ClientHandle, const CC_Mqtt5UnsubscribeTopicConfig*, CC_Mqtt5UnsubscribeCompleteCb, void*) = nullptr;
        CC_Mqtt5ErrorCode (*m_unsubscribe_full)(CC_Mqtt5ClientHandle, const CC_Mqtt5UnsubscribeTopicConfig*, unsigned, CC_Mqtt5UnsubscribeCompleteCb, void*) = nullptr;
        CC_Mqtt5PublishHandle (*m_publish_prepare)(CC_Mqtt5ClientHandle, CC_Mqtt5ErrorCode*) = nullptr;
        unsigned (*m_publish_count)(CC_Mqtt5ClientHandle) = nullptr;
        void (*m_publish_init_config_basic)(CC_Mqtt5PublishBasicConfig*) = nullptr;
        void (*m_publish_init_config_extra)(CC_Mqtt5PublishExtraConfig*) = nullptr;
        CC_Mqtt5ErrorCode (*m_publish_set_response_timeout)(CC_Mqtt5PublishHandle, unsigned) = nullptr;
        unsigned (*m_publish_get_response_timeout)(CC_Mqtt5PublishHandle) = nullptr;
        CC_Mqtt5ErrorCode (*m_publish_set_resend_attempts)(CC_Mqtt5PublishHandle, unsigned) = nullptr;
        unsigned (*m_publish_get_resend_attempts)(CC_Mqtt5PublishHandle) = nullptr;
        CC_Mqtt5ErrorCode (*m_publish_config_basic)(CC_Mqtt5PublishHandle, const CC_Mqtt5PublishBasicConfig*) = nullptr;
        CC_Mqtt5ErrorCode (*m_publish_config_extra)(CC_Mqtt5PublishHandle, const CC_Mqtt5PublishExtraConfig*) = nullptr;
        CC_Mqtt5ErrorCode (*m_publish_add_user_prop)(CC_Mqtt5PublishHandle, const CC_Mqtt5UserProp*) = nullptr;
        CC_Mqtt5ErrorCode (*m_publish_send)(CC_Mqtt5PublishHandle, CC_Mqtt5PublishCompleteCb, void*) = nullptr;
        CC_Mqtt5ErrorCode (*m_publish_cancel)(CC_Mqtt5PublishHandle) = nullptr;
        bool (*m_publish_was_initiated)(CC_Mqtt5PublishHandle) = nullptr;
        CC_Mqtt5ErrorCode (*m_publish_simple)(CC_Mqtt5ClientHandle, const CC_Mqtt5PublishBasicConfig*, CC_Mqtt5PublishCompleteCb, void*) = nullptr;
        CC_Mqtt5ErrorCode (*m_publish_full)(CC_Mqtt5ClientHandle, const CC_Mqtt5PublishBasicConfig*, const CC_Mqtt5PublishExtraConfig*, CC_Mqtt5PublishCompleteCb, void*) = nullptr;
        CC_Mqtt5ErrorCode (*m_publish_set_ordering)(CC_Mqtt5ClientHandle, CC_Mqtt5PublishOrdering) = nullptr;
        CC_Mqtt5PublishOrdering (*m_publish_get_ordering)(CC_Mqtt5ClientHandle) = nullptr;
        CC_Mqtt5ReauthHandle (*m_reauth_prepare)(CC_Mqtt5ClientHandle, CC_Mqtt5ErrorCode*) = nullptr;
        void (*m_reauth_init_config_auth)(CC_Mqtt5AuthConfig*) = nullptr;
        CC_Mqtt5ErrorCode (*m_reauth_set_response_timeout)(CC_Mqtt5ReauthHandle, unsigned) = nullptr;
        unsigned (*m_reauth_get_response_timeout)(CC_Mqtt5ReauthHandle) = nullptr;
        CC_Mqtt5ErrorCode (*m_reauth_config_auth)(CC_Mqtt5ReauthHandle, const CC_Mqtt5AuthConfig*) = nullptr;
        CC_Mqtt5ErrorCode (*m_reauth_add_user_prop)(CC_Mqtt5ReauthHandle, const CC_Mqtt5UserProp*) = nullptr;
        CC_Mqtt5ErrorCode (*m_reauth_send)(CC_Mqtt5ReauthHandle, CC_Mqtt5ReauthCompleteCb, void*) = nullptr;
        CC_Mqtt5ErrorCode (*m_reauth_cancel)(CC_Mqtt5ReauthHandle) = nullptr;
        CC_Mqtt5ErrorCode (*m_reauth)(CC_Mqtt5ClientHandle, const CC_Mqtt5AuthConfig*, CC_Mqtt5ReauthCompleteCb, void*) = nullptr;
        void (*m_set_next_tick_program_callback)(CC_Mqtt5ClientHandle, CC_Mqtt5NextTickProgramCb, void*) = nullptr;
        void (*m_set_cancel_next_tick_wait_callback)(CC_Mqtt5ClientHandle, CC_Mqtt5CancelNextTickWaitCb, void*) = nullptr;        
        void (*m_set_send_output_data_callback)(CC_Mqtt5ClientHandle, CC_Mqtt5SendOutputDataCb, void*) = nullptr;
        void (*m_set_broker_disconnect_report_callback)(CC_Mqtt5ClientHandle, CC_Mqtt5BrokerDisconnectReportCb, void*) = nullptr;        
        void (*m_set_message_received_report_callback)(CC_Mqtt5ClientHandle, CC_Mqtt5MessageReceivedReportCb, void*) = nullptr;        
        void (*m_set_error_log_callback)(CC_Mqtt5ClientHandle, CC_Mqtt5ErrorLogCb, void*) = nullptr;        
    };

    struct UnitTestDeleter
    {
        UnitTestDeleter() = default;
        explicit UnitTestDeleter(const LibFuncs& ops) : 
            m_free(ops.m_free)
        {
        }

        void operator()(CC_Mqtt5Client* ptr)
        {
            m_free(ptr);
        }

    private:
        void (*m_free)(CC_Mqtt5ClientHandle) = nullptr;
    }; 

    using UnitTestClientPtr = std::unique_ptr<CC_Mqtt5Client, UnitTestDeleter>;


protected:

    explicit UnitTestCommonBase(const LibFuncs& funcs);

    static constexpr unsigned UnitTestDefaultOpTimeoutMs = 2000;
    static constexpr unsigned UnitTestDefaultKeepAliveMs = 60000;

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
        unsigned m_highQosSendLimit = 0;
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
        unsigned m_messageExpiryInterval = 0U;
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

    struct UnitTestReauthResponseInfo
    {
        CC_Mqtt5AsyncOpStatus m_status = CC_Mqtt5AsyncOpStatus_ValuesLimit;
        UnitTestAuthInfo m_response;
    };        

    struct UnitTestBrokerDisconnectionReport
    {
        CC_Mqtt5BrokerDisconnectReason m_reason;
        std::optional<UnitTestDisconnectInfo> m_info;
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
        unsigned m_recvMaximum = 0U;
        bool m_sessionPresent = false;
        bool* m_retainAvailable = nullptr;
        bool* m_wildcardSubAvailable = nullptr;
        bool* m_subIdsAvailable = nullptr;
        bool* m_sharedSubsAvailable = nullptr;
    };

    void unitTestSetUp();
    void unitTestTearDown();
    UnitTestClientPtr apiAllocClient(bool addLog = false);

    decltype(auto) unitTestSentData()
    {
        return m_sentData;
    }

    const TickInfo* unitTestTickReq();
    bool unitTestCheckNoTicks();
    void unitTestTick(CC_Mqtt5Client* client, unsigned ms = 0, bool forceTick = false);
    CC_Mqtt5ErrorCode unitTestSendConnect(CC_Mqtt5ConnectHandle& connect);
    CC_Mqtt5ErrorCode unitTestSendSubscribe(CC_Mqtt5SubscribeHandle& subscribe);
    CC_Mqtt5ErrorCode unitTestSendUnsubscribe(CC_Mqtt5UnsubscribeHandle& unsubscribe);
    CC_Mqtt5ErrorCode unitTestSendPublish(CC_Mqtt5PublishHandle& publish, bool clearHandle = true);
    CC_Mqtt5ErrorCode unitTestSendReauth(CC_Mqtt5ReauthHandle& reauth);
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
    bool unitTestIsReauthComplete() const;
    const UnitTestReauthResponseInfo& unitTestReauthResponseInfo();
    void unitTestPopReauthResponseInfo();
    void unitTestReceiveMessage(CC_Mqtt5Client* client, const UnitTestMessage& msg, bool reportReceivedData = true);
    CC_Mqtt5ErrorCode unitTestConfigAuth(CC_Mqtt5ConnectHandle handle, const std::string& method, const std::vector<std::uint8_t>& data);
    CC_Mqtt5ErrorCode unitTestConfigReauth(CC_Mqtt5ReauthHandle handle, const std::string& method, const std::vector<std::uint8_t>& data);
    void unitTestAddOutAuth(const UnitTestAuthInfo& info);
    void unitTestClearAuth();
    bool unitTestHasInAuthInfo() const;
    const UnitTestAuthInfo& unitTestInAuthInfo() const;
    void unitTestPopInAuthInfo();
    bool unitTestHasOutAuthInfo() const;
    void unitTestPopOutAuthInfo();
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
        CC_Mqtt5AuthConfig* authConfig = nullptr,
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

    void unitTestPerformSubscribe(
        CC_Mqtt5Client* client, 
        CC_Mqtt5SubscribeTopicConfig* topicConfigs, 
        unsigned topicConfigsCount = 1U,
        const CC_Mqtt5SubscribeExtraConfig* extraConfig = nullptr);
    void unitTestPerformBasicSubscribe(CC_Mqtt5Client* client, const char* topic, unsigned subId = 0U);

    using UnitTestDisconnectReason = UnitTestDisconnectMsg::Field_reasonCode::Field::ValueType;
    void unitTestVerifyDisconnectSent(UnitTestDisconnectReason reason = UnitTestDisconnectReason::Success);

    void unitTestClearState(bool preserveTicks = true);

    // API wrappers
    UnitTestClientPtr apiAlloc();
    void apiNotifyNetworkDisconnected(CC_Mqtt5Client* client);
    bool apiIsNetworkDisconnected(CC_Mqtt5Client* client);
    CC_Mqtt5ErrorCode apiSetDefaultResponseTimeout(CC_Mqtt5Client* client, unsigned ms);
    CC_Mqtt5ErrorCode apiPubTopicAliasAlloc(CC_Mqtt5Client* client, const char* topic, unsigned char qos0RegsCount);
    unsigned apiPubTopicAliasCount(CC_Mqtt5Client* client);
    void apiSetVerifyIncomingMsgSubscribed(CC_Mqtt5Client* client, bool enabled);
    CC_Mqtt5ConnectHandle apiConnectPrepare(CC_Mqtt5Client* client, CC_Mqtt5ErrorCode* ec);
    void apiConnectInitConfigBasic(CC_Mqtt5ConnectBasicConfig* config);
    void apiConnectInitConfigWill(CC_Mqtt5ConnectWillConfig* config);
    void apiConnectInitConfigExtra(CC_Mqtt5ConnectExtraConfig* config);
    void apiConnectInitConfigAuth(CC_Mqtt5AuthConfig* config);
    CC_Mqtt5ErrorCode apiConnectConfigBasic(CC_Mqtt5ConnectHandle handle, const CC_Mqtt5ConnectBasicConfig* config);
    CC_Mqtt5ErrorCode apiConnectConfigWill(CC_Mqtt5ConnectHandle handle, const CC_Mqtt5ConnectWillConfig* config);
    CC_Mqtt5ErrorCode apiConnectConfigExtra(CC_Mqtt5ConnectHandle handle, const CC_Mqtt5ConnectExtraConfig* config);
    CC_Mqtt5ErrorCode apiConnectConfigAuth(CC_Mqtt5ConnectHandle handle, const CC_Mqtt5AuthConfig* config);
    CC_Mqtt5ErrorCode apiConnectAddUserProp(CC_Mqtt5ConnectHandle handle, const CC_Mqtt5UserProp* prop);
    CC_Mqtt5ErrorCode apiConnectAddWillUserProp(CC_Mqtt5ConnectHandle handle, const CC_Mqtt5UserProp* prop);
    bool apiIsConnected(CC_Mqtt5Client* client);
    CC_Mqtt5DisconnectHandle apiDisconnectPrepare(CC_Mqtt5Client* client, CC_Mqtt5ErrorCode* ec);
    void apiDisconnectInitConfig(CC_Mqtt5DisconnectConfig* config);
    CC_Mqtt5ErrorCode apiDisconnectConfig(CC_Mqtt5DisconnectHandle handle, const CC_Mqtt5DisconnectConfig* config);
    CC_Mqtt5SubscribeHandle apiSubscribePrepare(CC_Mqtt5Client* client, CC_Mqtt5ErrorCode* ec);
    CC_Mqtt5ErrorCode apiSubscribeSetResponseTimeout(CC_Mqtt5SubscribeHandle handle, unsigned ms);
    void apiSubscribeInitConfigTopic(CC_Mqtt5SubscribeTopicConfig* config);
    void apiSubscribeInitConfigExtra(CC_Mqtt5SubscribeExtraConfig* config);
    CC_Mqtt5ErrorCode apiSubscribeConfigTopic(CC_Mqtt5SubscribeHandle handle, const CC_Mqtt5SubscribeTopicConfig* config);
    CC_Mqtt5ErrorCode apiSubscribeConfigExtra(CC_Mqtt5SubscribeHandle handle, const CC_Mqtt5SubscribeExtraConfig* config);
    CC_Mqtt5ErrorCode apiSubscribeAddUserProp(CC_Mqtt5SubscribeHandle handle, const CC_Mqtt5UserProp* prop);
    CC_Mqtt5ErrorCode apiSubscribeSimple(CC_Mqtt5Client* client, CC_Mqtt5SubscribeTopicConfig* config);
    CC_Mqtt5UnsubscribeHandle apiUnsubscribePrepare(CC_Mqtt5Client* client, CC_Mqtt5ErrorCode* ec);
    CC_Mqtt5ErrorCode apiUnsubscribeSetResponseTimeout(CC_Mqtt5UnsubscribeHandle handle, unsigned ms);
    void apiUnsubscribeInitConfigTopic(CC_Mqtt5UnsubscribeTopicConfig* config);
    CC_Mqtt5ErrorCode apiUnsubscribeConfigTopic(CC_Mqtt5UnsubscribeHandle handle, const CC_Mqtt5UnsubscribeTopicConfig* config);
    CC_Mqtt5ErrorCode apiUnsubscribeAddUserProp(CC_Mqtt5UnsubscribeHandle handle, const CC_Mqtt5UserProp* prop);
    CC_Mqtt5PublishHandle apiPublishPrepare(CC_Mqtt5Client* client, CC_Mqtt5ErrorCode* ec);
    unsigned apiPublishCount(CC_Mqtt5Client* client);
    void apiPublishInitConfigBasic(CC_Mqtt5PublishBasicConfig* config);
    void apiPublishInitConfigExtra(CC_Mqtt5PublishExtraConfig* config);
    CC_Mqtt5ErrorCode apiPublishSetResponseTimeout(CC_Mqtt5PublishHandle handle, unsigned ms);
    CC_Mqtt5ErrorCode apiPublishConfigBasic(CC_Mqtt5PublishHandle handle, const CC_Mqtt5PublishBasicConfig* config);
    CC_Mqtt5ErrorCode apiPublishConfigExtra(CC_Mqtt5PublishHandle handle, const CC_Mqtt5PublishExtraConfig* config);
    CC_Mqtt5ErrorCode apiPublishAddUserProp(CC_Mqtt5PublishHandle handle, const CC_Mqtt5UserProp* prop);
    CC_Mqtt5ErrorCode apiPublishCancel(CC_Mqtt5PublishHandle handle);
    bool apiPublishWasInitiated(CC_Mqtt5PublishHandle handle);
    CC_Mqtt5ErrorCode apiPublishSetOrdering(CC_Mqtt5ClientHandle handle, CC_Mqtt5PublishOrdering ordering);
    CC_Mqtt5PublishOrdering apiPublishGetOrdering(CC_Mqtt5ClientHandle handle);
    CC_Mqtt5ReauthHandle apiReauthPrepare(CC_Mqtt5Client* client, CC_Mqtt5ErrorCode* ec);
    void apiReauthInitConfigAuth(CC_Mqtt5AuthConfig* config);
    CC_Mqtt5ErrorCode apiReauthAddUserProp(CC_Mqtt5ReauthHandle handle, const CC_Mqtt5UserProp* prop);
    void apiSetNextTickProgramCb(CC_Mqtt5ClientHandle handle, CC_Mqtt5NextTickProgramCb cb, void* data);    
    void apiSetCancelNextTickWaitCb(CC_Mqtt5ClientHandle handle, CC_Mqtt5CancelNextTickWaitCb cb, void* data);    
    void apiSetSendOutputDataCb(CC_Mqtt5ClientHandle handle, CC_Mqtt5SendOutputDataCb cb, void* data);    
    void apiSetBrokerDisconnectReportCb(CC_Mqtt5ClientHandle handle, CC_Mqtt5BrokerDisconnectReportCb cb, void* data);    
    void apiSetMessageReceivedReportCb(CC_Mqtt5ClientHandle handle, CC_Mqtt5MessageReceivedReportCb cb, void* data);    

private:

    static void unitTestErrorLogCb(void* obj, const char* msg);
    static void unitTestBrokerDisconnectedCb(void* obj, CC_Mqtt5BrokerDisconnectReason reason, const CC_Mqtt5DisconnectInfo* info);
    static void unitTestMessageReceivedCb(void* obj, const CC_Mqtt5MessageInfo* info);
    static void unitTestSendOutputDataCb(void* obj, const unsigned char* buf, unsigned bufLen);
    static void unitTestProgramNextTickCb(void* obj, unsigned duration);
    static unsigned unitTestCancelNextTickWaitCb(void* obj);
    static void unitTestConnectCompleteCb(void* obj, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response);
    static void unitTestSubscribeCompleteCb(void* obj, CC_Mqtt5SubscribeHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response);
    static void unitTestUnsubscribeCompleteCb(void* obj, CC_Mqtt5UnsubscribeHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5UnsubscribeResponse* response);
    static void unitTestPublishCompleteCb(void* obj, CC_Mqtt5PublishHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response);
    static void unitTestReauthCompleteCb(void* obj, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5AuthInfo* response);
    static CC_Mqtt5AuthErrorCode unitTestAuthCb(void* obj, const CC_Mqtt5AuthInfo* authInfoIn, CC_Mqtt5AuthInfo* authInfoOut);

    LibFuncs m_funcs;  
    std::vector<TickInfo> m_tickReq;
    std::vector<std::uint8_t> m_sentData;
    std::vector<std::uint8_t> m_receivedData;
    std::vector<UnitTestConnectResponseInfo> m_connectResp;
    std::vector<UnitTestSubscribeResponseInfo> m_subscribeResp;
    std::vector<UnitTestUnsubscribeResponseInfo> m_unsubscribeResp;
    std::vector<UnitTestPublishResponseInfo> m_publishResp;
    std::vector<UnitTestReauthResponseInfo> m_reauthResp;
    std::vector<UnitTestAuthInfo> m_inAuthInfo;
    std::vector<UnitTestAuthInfo> m_outAuthInfo;
    std::vector<CC_Mqtt5UserProp> m_userPropsTmp;
    std::vector<UnitTestBrokerDisconnectionReport> m_disconnectInfo;
    std::vector<UnitTestMessageInfo> m_receivedMessages;
};