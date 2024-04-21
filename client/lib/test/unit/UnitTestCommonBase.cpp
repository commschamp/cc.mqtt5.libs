#include "UnitTestCommonBase.h"

#include "UnitTestPropsHandler.h"

#include <cstdlib>
#include <iostream>

namespace 
{

#define test_assert(cond_) \
    assert(cond_); \
    if (!(cond_)) { \
        std::cerr << "\nAssertion failure (" << #cond_ << ") in " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::exit(1); \
    }


void assignStringInternal(std::string& dest, const char* source)
{
    dest.clear();
    if (source != nullptr)
    {
        dest = source;
    }
}

template <typename TDest, typename TSrc>
void assignDataInternal(TDest& dest, TSrc* source, unsigned count)
{
    dest.clear();
    if (count > 0U) {
        std::copy_n(source, count, std::back_inserter(dest));
    }
}

} // namespace 

UnitTestCommonBase::UnitTestCommonBase(const LibFuncs& funcs) :
    m_funcs(funcs),
    m_client(nullptr, UnitTestDeleter(funcs))
{
    test_assert(m_funcs.m_alloc != nullptr);
    test_assert(m_funcs.m_free != nullptr);
    test_assert(m_funcs.m_tick != nullptr);
    test_assert(m_funcs.m_process_data != nullptr);
    test_assert(m_funcs.m_notify_network_disconnected != nullptr);
    test_assert(m_funcs.m_is_network_disconnected != nullptr);
    test_assert(m_funcs.m_set_default_response_timeout != nullptr);
    test_assert(m_funcs.m_get_default_response_timeout != nullptr);
    test_assert(m_funcs.m_pub_topic_alias_alloc != nullptr);
    test_assert(m_funcs.m_pub_topic_alias_free != nullptr);
    test_assert(m_funcs.m_pub_topic_alias_count != nullptr);
    test_assert(m_funcs.m_pub_topic_alias_is_allocated != nullptr);
    test_assert(m_funcs.m_set_verify_outgoing_topic_enabled != nullptr);
    test_assert(m_funcs.m_get_verify_outgoing_topic_enabled != nullptr);
    test_assert(m_funcs.m_set_verify_incoming_topic_enabled != nullptr);
    test_assert(m_funcs.m_get_verify_incoming_topic_enabled != nullptr);
    test_assert(m_funcs.m_set_verify_incoming_msg_subscribed != nullptr);
    test_assert(m_funcs.m_get_verify_incoming_msg_subscribed != nullptr);
    test_assert(m_funcs.m_init_user_prop != nullptr);
    test_assert(m_funcs.m_connect_prepare != nullptr);
    test_assert(m_funcs.m_connect_init_config_basic != nullptr);
    test_assert(m_funcs.m_connect_init_config_will != nullptr);
    test_assert(m_funcs.m_connect_init_config_extra != nullptr);
    test_assert(m_funcs.m_connect_init_config_auth != nullptr);
    test_assert(m_funcs.m_connect_init_auth_info != nullptr);
    test_assert(m_funcs.m_connect_set_response_timeout != nullptr);
    test_assert(m_funcs.m_connect_get_response_timeout != nullptr);
    test_assert(m_funcs.m_connect_config_basic != nullptr);
    test_assert(m_funcs.m_connect_config_will != nullptr);
    test_assert(m_funcs.m_connect_config_extra != nullptr);
    test_assert(m_funcs.m_connect_config_auth != nullptr);
    test_assert(m_funcs.m_connect_add_user_prop != nullptr);
    test_assert(m_funcs.m_connect_add_will_user_prop != nullptr);
    test_assert(m_funcs.m_connect_send != nullptr);
    test_assert(m_funcs.m_connect_cancel != nullptr);
    test_assert(m_funcs.m_connect_simple != nullptr);
    test_assert(m_funcs.m_connect_full != nullptr);
    test_assert(m_funcs.m_is_connected != nullptr);
    test_assert(m_funcs.m_disconnect_prepare != nullptr);
    test_assert(m_funcs.m_disconnect_init_config != nullptr);
    test_assert(m_funcs.m_disconnect_config != nullptr);
    test_assert(m_funcs.m_disconnect_add_user_prop != nullptr);
    test_assert(m_funcs.m_disconnect_send != nullptr);
    test_assert(m_funcs.m_disconnect_cancel != nullptr);
    test_assert(m_funcs.m_disconnect != nullptr);
    test_assert(m_funcs.m_subscribe_prepare != nullptr);
    test_assert(m_funcs.m_subscribe_set_response_timeout != nullptr);
    test_assert(m_funcs.m_subscribe_get_response_timeout != nullptr);
    test_assert(m_funcs.m_subscribe_init_config_topic != nullptr);
    test_assert(m_funcs.m_subscribe_init_config_extra != nullptr);
    test_assert(m_funcs.m_subscribe_config_topic != nullptr);
    test_assert(m_funcs.m_subscribe_config_extra != nullptr);
    test_assert(m_funcs.m_subscribe_add_user_prop != nullptr);
    test_assert(m_funcs.m_subscribe_send != nullptr);
    test_assert(m_funcs.m_subscribe_cancel != nullptr);
    test_assert(m_funcs.m_subscribe_simple != nullptr);
    test_assert(m_funcs.m_subscribe_full != nullptr);
    test_assert(m_funcs.m_unsubscribe_prepare != nullptr);
    test_assert(m_funcs.m_unsubscribe_set_response_timeout != nullptr);
    test_assert(m_funcs.m_unsubscribe_get_response_timeout != nullptr);
    test_assert(m_funcs.m_unsubscribe_init_config_topic != nullptr);
    test_assert(m_funcs.m_unsubscribe_config_topic != nullptr);
    test_assert(m_funcs.m_unsubscribe_add_user_prop != nullptr);
    test_assert(m_funcs.m_unsubscribe_send != nullptr);
    test_assert(m_funcs.m_unsubscribe_cancel != nullptr);    
    test_assert(m_funcs.m_unsubscribe_simple != nullptr);    
    test_assert(m_funcs.m_unsubscribe_full != nullptr);    
    test_assert(m_funcs.m_publish_prepare != nullptr);    
    test_assert(m_funcs.m_publish_count != nullptr);    
    test_assert(m_funcs.m_publish_init_config_basic != nullptr);    
    test_assert(m_funcs.m_publish_init_config_extra != nullptr);    
    test_assert(m_funcs.m_publish_set_response_timeout != nullptr);    
    test_assert(m_funcs.m_publish_get_response_timeout != nullptr);    
    test_assert(m_funcs.m_publish_set_resend_attempts != nullptr);    
    test_assert(m_funcs.m_publish_get_resend_attempts != nullptr);      
    test_assert(m_funcs.m_publish_config_basic != nullptr);      
    test_assert(m_funcs.m_publish_config_extra != nullptr);  
    test_assert(m_funcs.m_publish_add_user_prop != nullptr);  
    test_assert(m_funcs.m_publish_send != nullptr);  
    test_assert(m_funcs.m_publish_cancel != nullptr);  
    test_assert(m_funcs.m_publish_was_initiated != nullptr);  
    test_assert(m_funcs.m_publish_simple != nullptr);  
    test_assert(m_funcs.m_publish_full != nullptr);  
    test_assert(m_funcs.m_publish_set_ordering != nullptr);  
    test_assert(m_funcs.m_publish_get_ordering != nullptr);  
    test_assert(m_funcs.m_reauth_prepare != nullptr);  
    test_assert(m_funcs.m_reauth_init_config_auth != nullptr);  
    test_assert(m_funcs.m_reauth_set_response_timeout != nullptr);  
    test_assert(m_funcs.m_reauth_get_response_timeout != nullptr);  
    test_assert(m_funcs.m_reauth_config_auth != nullptr);  
    test_assert(m_funcs.m_reauth_add_user_prop != nullptr); 
    test_assert(m_funcs.m_reauth_send != nullptr); 
    test_assert(m_funcs.m_reauth_cancel != nullptr); 
    test_assert(m_funcs.m_reauth != nullptr); 
    test_assert(m_funcs.m_set_next_tick_program_callback != nullptr); 
    test_assert(m_funcs.m_set_cancel_next_tick_wait_callback != nullptr); 
    test_assert(m_funcs.m_set_send_output_data_callback != nullptr); 
    test_assert(m_funcs.m_set_broker_disconnect_report_callback != nullptr); 
    test_assert(m_funcs.m_set_message_received_report_callback != nullptr); 
    test_assert(m_funcs.m_set_error_log_callback != nullptr); 
}


UnitTestCommonBase::UnitTestUserProp& UnitTestCommonBase::UnitTestUserProp::operator=(const CC_Mqtt5UserProp& other)
{
    std::tie(m_key, m_value) = std::make_tuple(other.m_key, other.m_value);
    return *this;
}

void UnitTestCommonBase::UnitTestUserProp::copyProps(const CC_Mqtt5UserProp* userProps, unsigned userPropsCount, List& list)
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

UnitTestCommonBase::UnitTestAuthInfo& UnitTestCommonBase::UnitTestAuthInfo::operator=(const CC_Mqtt5AuthInfo& other)
{
    m_authData.clear();
    assignDataInternal(m_authData, other.m_authData, other.m_authDataLen);
    assignStringInternal(m_reasonStr, other.m_reasonStr);
    UnitTestUserProp::copyProps(other.m_userProps, other.m_userPropsCount, m_userProps);
    return *this;
}

UnitTestCommonBase::UnitTestConnectResponse& UnitTestCommonBase::UnitTestConnectResponse::operator=(const CC_Mqtt5ConnectResponse& response)
{
    auto thisTie = 
        std::tie(
            m_reasonCode, m_sessionExpiryInterval, m_highQosSendLimit, m_maxPacketSize, m_topicAliasMax,
            m_maxQos, m_sessionPresent, m_retainAvailable, m_wildcardSubAvailable, m_subIdsAvailable, 
            m_sharedSubsAvailable);

    auto responseTie = 
        std::forward_as_tuple(
            response.m_reasonCode, response.m_sessionExpiryInterval, response.m_highQosSendLimit, response.m_maxPacketSize, response.m_topicAliasMax,
            response.m_maxQos, response.m_sessionPresent, response.m_retainAvailable, response.m_wildcardSubAvailable, response.m_subIdsAvailable, 
            response.m_sharedSubsAvailable);            

    thisTie = responseTie;

    assignStringInternal(m_assignedClientId, response.m_assignedClientId);
    assignStringInternal(m_responseInfo, response.m_responseInfo);
    assignStringInternal(m_reasonStr, response.m_reasonStr);
    assignStringInternal(m_serverRef, response.m_serverRef);
    assignDataInternal(m_authData, response.m_authData, response.m_authDataLen);
    UnitTestUserProp::copyProps(response.m_userProps, response.m_userPropsCount, m_userProps);
    return *this;
}

UnitTestCommonBase::UnitTestSubscribeResponse& UnitTestCommonBase::UnitTestSubscribeResponse::operator=(const CC_Mqtt5SubscribeResponse& response)
{
    assignDataInternal(m_reasonCodes, response.m_reasonCodes, response.m_reasonCodesCount);
    assignStringInternal(m_reasonStr, response.m_reasonStr);
    UnitTestUserProp::copyProps(response.m_userProps, response.m_userPropsCount, m_userProps);
    return *this;
}

UnitTestCommonBase::UnitTestUnsubscribeResponse& UnitTestCommonBase::UnitTestUnsubscribeResponse::operator=(const CC_Mqtt5UnsubscribeResponse& response)
{
    assignDataInternal(m_reasonCodes, response.m_reasonCodes, response.m_reasonCodesCount);
    assignStringInternal(m_reasonStr, response.m_reasonStr);
    UnitTestUserProp::copyProps(response.m_userProps, response.m_userPropsCount, m_userProps);
    return *this;
}

UnitTestCommonBase::UnitTestDisconnectInfo& UnitTestCommonBase::UnitTestDisconnectInfo::operator=(const CC_Mqtt5DisconnectInfo& other)
{
    m_reasonCode = other.m_reasonCode;
    assignStringInternal(m_reasonStr, other.m_reasonStr);
    assignStringInternal(m_serverRef, other.m_serverRef);        
    UnitTestUserProp::copyProps(other.m_userProps, other.m_userPropsCount, m_userProps);
    return *this;
}

UnitTestCommonBase::UnitTestPublishResponse& UnitTestCommonBase::UnitTestPublishResponse::operator=(const CC_Mqtt5PublishResponse& other)
{
    m_reasonCode = other.m_reasonCode;
    assignStringInternal(m_reasonStr, other.m_reasonStr);
    UnitTestUserProp::copyProps(other.m_userProps, other.m_userPropsCount, m_userProps);
    return *this;
}

UnitTestCommonBase::UnitTestMessageInfo& UnitTestCommonBase::UnitTestMessageInfo::operator=(const CC_Mqtt5MessageInfo& other)
{
    assignStringInternal(m_topic, other.m_topic);
    assignDataInternal(m_data, other.m_data, other.m_dataLen);
    assignStringInternal(m_responseTopic, other.m_responseTopic);
    assignDataInternal(m_correlationData, other.m_correlationData, other.m_correlationDataLen);
    UnitTestUserProp::copyProps(other.m_userProps, other.m_userPropsCount, m_userProps);
    assignStringInternal(m_contentType, other.m_contentType);
    assignDataInternal(m_subIds, other.m_subIds, other.m_subIdsCount);
    m_messageExpiryInterval = other.m_messageExpiryInterval;
    m_qos = other.m_qos;
    m_format = other.m_format;
    m_retained = other.m_retained;
    return *this;
}

void UnitTestCommonBase::unitTestSetUp()
{
    test_assert(!m_client);
    unitTestClearState(false);
}

void UnitTestCommonBase::unitTestTearDown()
{
    m_client.reset();
}

UnitTestCommonBase::UnitTestClientPtr::pointer UnitTestCommonBase::unitTestAllocClient(bool addLog)
{
    test_assert(!m_client);
    m_client.reset(m_funcs.m_alloc());
    if (addLog) {
        m_funcs.m_set_error_log_callback(m_client.get(), &UnitTestCommonBase::unitTestErrorLogCb, nullptr);
    }
    unitTestSetBrokerDisconnectReportCb(m_client.get(), &UnitTestCommonBase::unitTestBrokerDisconnectedCb, this);
    unitTestSetMessageReceivedReportCb(m_client.get(), &UnitTestCommonBase::unitTestMessageReceivedCb, this);
    unitTestSetSendOutputDataCb(m_client.get(), &UnitTestCommonBase::unitTestSendOutputDataCb, this);
    unitTestSetNextTickProgramCb(m_client.get(), &UnitTestCommonBase::unitTestProgramNextTickCb, this);
    unitTestSetCancelNextTickWaitCb(m_client.get(), &UnitTestCommonBase::unitTestCancelNextTickWaitCb, this);
    return m_client.get();
}

const UnitTestCommonBase::TickInfo* UnitTestCommonBase::unitTestTickReq()
{
    test_assert(!m_tickReq.empty());
    return &m_tickReq.front();
}

bool UnitTestCommonBase::unitTestCheckNoTicks()
{
    return m_tickReq.empty();
}

void UnitTestCommonBase::unitTestTick(unsigned ms, bool forceTick)
{
    test_assert(!m_tickReq.empty());
    auto& tick = m_tickReq.front();
    auto rem = tick.m_requested - tick.m_elapsed;
    test_assert(ms <= rem);
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
    test_assert(static_cast<bool>(m_client));
    m_funcs.m_tick(m_client.get(), ms);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestSendConnect(CC_Mqtt5ConnectHandle& connect)
{
    auto result = m_funcs.m_connect_send(connect, &UnitTestCommonBase::unitTestConnectCompleteCb, this);
    connect = nullptr;
    return result;
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestSendSubscribe(CC_Mqtt5SubscribeHandle& subscribe)
{
    auto result = m_funcs.m_subscribe_send(subscribe, &UnitTestCommonBase::unitTestSubscribeCompleteCb, this);
    subscribe = nullptr;
    return result;
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestSendUnsubscribe(CC_Mqtt5UnsubscribeHandle& unsubscribe)
{
    auto result = m_funcs.m_unsubscribe_send(unsubscribe, &UnitTestCommonBase::unitTestUnsubscribeCompleteCb, this);
    unsubscribe = nullptr;
    return result;
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestSendPublish(CC_Mqtt5PublishHandle& publish, bool clearHandle)
{
    auto result = m_funcs.m_publish_send(publish, &UnitTestCommonBase::unitTestPublishCompleteCb, this);
    if (clearHandle) {
        publish = nullptr;
    }
    return result;
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestSendReauth(CC_Mqtt5ReauthHandle& reauth)
{
    auto result = m_funcs.m_reauth_send(reauth, &UnitTestCommonBase::unitTestReauthCompleteCb, this);
    reauth = nullptr;
    return result;
}

UniTestsMsgPtr UnitTestCommonBase::unitTestGetSentMessage()
{
    UniTestsMsgPtr msg;
    UnitTestsFrame frame;
    
    test_assert(!m_sentData.empty());
    UnitTestMessage::ReadIterator begIter = &m_sentData[0];
    auto readIter = begIter;
    [[maybe_unused]] auto readEs = frame.read(msg, readIter, m_sentData.size());
    test_assert(readEs == comms::ErrorStatus::Success);
    auto consumed = std::distance(begIter, readIter);
    m_sentData.erase(m_sentData.begin(), m_sentData.begin() + consumed);
    return msg;
}

bool UnitTestCommonBase::unitTestHasSentMessage() const
{
    return !m_sentData.empty();
}

bool UnitTestCommonBase::unitTestIsConnectComplete()
{
    return (!m_connectResp.empty());
}

const UnitTestCommonBase::UnitTestConnectResponseInfo& UnitTestCommonBase::unitTestConnectResponseInfo()
{
    test_assert(unitTestIsConnectComplete());
    return m_connectResp.front();
}

void UnitTestCommonBase::unitTestPopConnectResponseInfo()
{
    test_assert(!m_connectResp.empty());
    m_connectResp.erase(m_connectResp.begin());
}

bool UnitTestCommonBase::unitTestIsSubscribeComplete()
{
    return !m_subscribeResp.empty();
}
const UnitTestCommonBase::UnitTestSubscribeResponseInfo& UnitTestCommonBase::unitTestSubscribeResponseInfo()
{
    test_assert(!m_subscribeResp.empty());
    return m_subscribeResp.front();
}

void UnitTestCommonBase::unitTestPopSubscribeResponseInfo()
{
    test_assert(!m_subscribeResp.empty());
    m_subscribeResp.erase(m_subscribeResp.begin());
}

bool UnitTestCommonBase::unitTestIsUnsubscribeComplete()
{
    return !m_unsubscribeResp.empty();
}
const UnitTestCommonBase::UnitTestUnsubscribeResponseInfo& UnitTestCommonBase::unitTestUnsubscribeResponseInfo()
{
    test_assert(!m_unsubscribeResp.empty());
    return m_unsubscribeResp.front();
}

void UnitTestCommonBase::unitTestPopUnsubscribeResponseInfo()
{
    test_assert(!m_unsubscribeResp.empty());
    m_unsubscribeResp.erase(m_unsubscribeResp.begin());
}

bool UnitTestCommonBase::unitTestIsPublishComplete()
{
    return !m_publishResp.empty();
}

const UnitTestCommonBase::UnitTestPublishResponseInfo& UnitTestCommonBase::unitTestPublishResponseInfo()
{
    test_assert(!m_publishResp.empty());
    return m_publishResp.front();
}

void UnitTestCommonBase::unitTestPopPublishResponseInfo()
{
    test_assert(!m_publishResp.empty());
    m_publishResp.erase(m_publishResp.begin());
}

bool UnitTestCommonBase::unitTestIsReauthComplete() const
{
    return !m_reauthResp.empty();
}

const UnitTestCommonBase::UnitTestReauthResponseInfo& UnitTestCommonBase::unitTestReauthResponseInfo()
{
    test_assert(!m_reauthResp.empty());
    return m_reauthResp.front();
}

void UnitTestCommonBase::unitTestPopReauthResponseInfo()
{
    test_assert(!m_reauthResp.empty());
    m_reauthResp.erase(m_reauthResp.begin());
}

void UnitTestCommonBase::unitTestReceiveMessage(const UnitTestMessage& msg, bool reportReceivedData)
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
    test_assert(es == comms::ErrorStatus::Success);

    if (!reportReceivedData) {
        return;
    }

    auto consumed = m_funcs.m_process_data(m_client.get(), &m_receivedData[0], static_cast<unsigned>(m_receivedData.size()));
    m_receivedData.erase(m_receivedData.begin(), m_receivedData.begin() + consumed);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestConfigAuth(CC_Mqtt5ConnectHandle handle, const std::string& method, const std::vector<std::uint8_t>& data)
{
    auto config = CC_Mqtt5AuthConfig();
    unitTestConnectInitConfigAuth(&config);

    config.m_authMethod = method.c_str();
    comms::cast_assign(config.m_authDataLen) = data.size();
    if (!data.empty()) {
        config.m_authData = &data[0];
    }

    config.m_authCb = &UnitTestCommonBase::unitTestAuthCb;
    config.m_authCbData = this;
    return unitTestConnectConfigAuth(handle, &config);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestConfigReauth(CC_Mqtt5ReauthHandle handle, const std::string& method, const std::vector<std::uint8_t>& data)
{
    auto config = CC_Mqtt5AuthConfig();
    unitTestConnectInitConfigAuth(&config);

    if (!method.empty()) {
        config.m_authMethod = method.c_str();
    }

    comms::cast_assign(config.m_authDataLen) = data.size();
    if (!data.empty()) {
        config.m_authData = &data[0];
    }

    config.m_authCb = &UnitTestCommonBase::unitTestAuthCb;
    config.m_authCbData = this;
    return m_funcs.m_reauth_config_auth(handle, &config);    
}

void UnitTestCommonBase::unitTestAddOutAuth(const UnitTestAuthInfo& info)
{
    m_outAuthInfo.push_back(info);
}

void UnitTestCommonBase::unitTestClearAuth()
{
    m_inAuthInfo.clear();
    m_outAuthInfo.clear();
}

bool UnitTestCommonBase::unitTestHasInAuthInfo() const
{
    return !m_inAuthInfo.empty();
}

const UnitTestCommonBase::UnitTestAuthInfo& UnitTestCommonBase::unitTestInAuthInfo() const
{
    test_assert(!m_inAuthInfo.empty());
    return m_inAuthInfo.front();
}

void UnitTestCommonBase::unitTestPopInAuthInfo()
{
    test_assert(!m_inAuthInfo.empty());
    m_inAuthInfo.erase(m_inAuthInfo.begin());
}

bool UnitTestCommonBase::unitTestHasOutAuthInfo() const
{
    return !m_outAuthInfo.empty();
}

void UnitTestCommonBase::unitTestPopOutAuthInfo()
{
    test_assert(!m_outAuthInfo.empty());
    m_outAuthInfo.erase(m_outAuthInfo.begin());

}

bool UnitTestCommonBase::unitTestIsDisconnected() const
{
    return m_disconnected;
}

bool UnitTestCommonBase::unitTestHasDisconnectInfo() const
{
    return (!m_disconnectInfo.empty());
}

const UnitTestCommonBase::UnitTestDisconnectInfo& UnitTestCommonBase::unitTestDisconnectInfo() const
{
    test_assert(!m_disconnectInfo.empty());
    return m_disconnectInfo.front();
}

void UnitTestCommonBase::unitTestPopDisconnectInfo()
{
    test_assert(!m_disconnectInfo.empty());
    m_disconnectInfo.erase(m_disconnectInfo.begin());
}

bool UnitTestCommonBase::unitTestHasMessageRecieved()
{
    return (!m_receivedMessages.empty());
}

const UnitTestCommonBase::UnitTestMessageInfo& UnitTestCommonBase::unitTestReceivedMessageInfo()
{
    test_assert(!m_receivedMessages.empty());
    return m_receivedMessages.front();
}

void UnitTestCommonBase::unitTestPopReceivedMessageInfo()
{
    test_assert(!m_receivedMessages.empty());
    m_receivedMessages.erase(m_receivedMessages.begin());
}

void UnitTestCommonBase::unitTestPerformConnect(
    CC_Mqtt5Client* client, 
    const CC_Mqtt5ConnectBasicConfig* basicConfig,
    const CC_Mqtt5ConnectWillConfig* willConfig,
    const CC_Mqtt5ConnectExtraConfig* extraConfig,
    CC_Mqtt5AuthConfig* authConfig,
    const UnitTestConnectResponseConfig* responseConfig)
{
    if (authConfig != nullptr) {
        test_assert(authConfig->m_authCb == nullptr);
        authConfig->m_authCb = &UnitTestCommonBase::unitTestAuthCb;
        authConfig->m_authCbData = this;        
    }   

    [[maybe_unused]] auto ec = m_funcs.m_connect_full(client, basicConfig, willConfig, extraConfig, authConfig, &UnitTestCommonBase::unitTestConnectCompleteCb, this);
    test_assert(ec == CC_Mqtt5ErrorCode_Success);

    auto sentMsg = unitTestGetSentMessage();
    test_assert(static_cast<bool>(sentMsg));
    test_assert(sentMsg->getId() == cc_mqtt5::MsgId_Connect);
    test_assert(!unitTestIsConnectComplete());    

    auto* tickReq = unitTestTickReq();
    test_assert(tickReq->m_requested <= UnitTestDefaultOpTimeoutMs);    

    if (authConfig != nullptr) {
        const UnitTestData AuthData = {0x11, 0x22, 0x33, 0x44};
        UnitTestAuthInfo authOutInfo;
        authOutInfo.m_authData = AuthData;
        unitTestAddOutAuth(authOutInfo);        
        
        UnitTestAuthMsg brokerAuth;
        brokerAuth.field_reasonCode().setExists();
        brokerAuth.field_reasonCode().field().value() = UnitTestAuthMsg::Field_reasonCode::Field::ValueType::ContinueAuth;
        brokerAuth.field_properties().setExists();
        auto& authPropsVec = brokerAuth.field_properties().field().value();

        if (authConfig->m_authMethod != nullptr) {
            authPropsVec.resize(authPropsVec.size() + 1U);
            auto& field = authPropsVec.back().initField_authMethod();
            field.field_value().setValue(authConfig->m_authMethod);
        }

        if (authConfig->m_authDataLen > 0) {
            authPropsVec.resize(authPropsVec.size() + 1U);
            auto& field = authPropsVec.back().initField_authData();
            comms::util::assign(field.field_value().value(), authConfig->m_authData, authConfig->m_authData + authConfig->m_authDataLen);
        }       

        unitTestTick(1000);
        unitTestReceiveMessage(brokerAuth);  

        test_assert(unitTestHasInAuthInfo()); 
        unitTestPopInAuthInfo();
        unitTestPopOutAuthInfo();

        sentMsg = unitTestGetSentMessage();
        test_assert(static_cast<bool>(sentMsg));
        test_assert(sentMsg->getId() == cc_mqtt5::MsgId_Auth);
        test_assert(!unitTestIsConnectComplete());        
        auto* authMsg = dynamic_cast<UnitTestAuthMsg*>(sentMsg.get());
        test_assert(authMsg != nullptr);
        test_assert(authMsg->field_reasonCode().doesExist());
        test_assert(authMsg->field_reasonCode().field().value() == UnitTestAuthMsg::Field_reasonCode::Field::ValueType::ContinueAuth);
    }

    unitTestTick(1000);
    UnitTestConnackMsg connackMsg;
    connackMsg.field_reasonCode().value() = UnitTestConnackMsg::Field_reasonCode::ValueType::Success;
    auto& propsVec = connackMsg.field_properties().value();

    if (authConfig != nullptr) {
        if (authConfig->m_authMethod != nullptr) {
            propsVec.resize(propsVec.size() + 1U);
            auto& field = propsVec.back().initField_authMethod();
            field.field_value().setValue(authConfig->m_authMethod);
        }

        if (authConfig->m_authDataLen > 0) {
            propsVec.resize(propsVec.size() + 1U);
            auto& field = propsVec.back().initField_authData();
            comms::util::assign(field.field_value().value(), authConfig->m_authData, authConfig->m_authData + authConfig->m_authDataLen);
        }        
    }

    do {
        if (responseConfig == nullptr) {
            break;
        }

        connackMsg.field_reasonCode().setValue(responseConfig->m_reasonCode);
        connackMsg.field_flags().setBitValue_sp(responseConfig->m_sessionPresent);

        if (responseConfig->m_topicAliasMax > 0U) {
            propsVec.resize(propsVec.size() + 1U);
            auto& field = propsVec.back().initField_topicAliasMax();
            field.field_value().setValue(responseConfig->m_topicAliasMax);
        }

        if (responseConfig->m_sessionExpiryInterval > 0U) {
            propsVec.resize(propsVec.size() + 1U);
            auto& field = propsVec.back().initField_sessionExpiryInterval();
            field.field_value().setValue(responseConfig->m_sessionExpiryInterval);
        }

        if (responseConfig->m_maxQos < CC_Mqtt5QoS_ValuesLimit) {
            propsVec.resize(propsVec.size() + 1U);
            auto& field = propsVec.back().initField_maxQos();
            field.field_value().setValue(responseConfig->m_maxQos);
        }

        if (responseConfig->m_retainAvailable != nullptr) {
            propsVec.resize(propsVec.size() + 1U);
            auto& field = propsVec.back().initField_retainAvailable();
            field.field_value().setValue(*responseConfig->m_retainAvailable);
        }

        if (responseConfig->m_maxPacketSize > 0U) {
            propsVec.resize(propsVec.size() + 1U);
            auto& field = propsVec.back().initField_maxPacketSize();
            field.field_value().setValue(responseConfig->m_maxPacketSize);
        }

        if (responseConfig->m_wildcardSubAvailable != nullptr) {
            propsVec.resize(propsVec.size() + 1U);
            auto& field = propsVec.back().initField_wildcardSubAvail();
            field.field_value().setValue(*responseConfig->m_wildcardSubAvailable);
        }     

        if (responseConfig->m_subIdsAvailable != nullptr) {
            propsVec.resize(propsVec.size() + 1U);
            auto& field = propsVec.back().initField_subIdAvail();
            field.field_value().setValue(*responseConfig->m_subIdsAvailable);
        }   

        if (responseConfig->m_sharedSubsAvailable != nullptr) {
            propsVec.resize(propsVec.size() + 1U);
            auto& field = propsVec.back().initField_sharedSubAvail();
            field.field_value().setValue(*responseConfig->m_sharedSubsAvailable);
        }      

        if (responseConfig->m_recvMaximum > 0U) {
            propsVec.resize(propsVec.size() + 1U);
            auto& field = propsVec.back().initField_receiveMax();
            field.field_value().setValue(responseConfig->m_recvMaximum);
        }

    } while (false);

    unitTestReceiveMessage(connackMsg);
    test_assert(unitTestIsConnectComplete());   

    auto& connectInfo = unitTestConnectResponseInfo();
    test_assert(connectInfo.m_status == CC_Mqtt5AsyncOpStatus_Complete);
    test_assert((responseConfig != nullptr) || (connectInfo.m_response.m_reasonCode == CC_Mqtt5ReasonCode_Success));
    test_assert((responseConfig != nullptr) || (connectInfo.m_response.m_maxQos == CC_Mqtt5QoS_ExactlyOnceDelivery));
    //test_assert((responseConfig != nullptr) || (connectInfo.m_response.m_maxPacketSize == 0U));

    if (responseConfig != nullptr) {
        test_assert(connectInfo.m_response.m_reasonCode == responseConfig->m_reasonCode);

        if (responseConfig->m_sessionExpiryInterval > 0U) {
            test_assert(connectInfo.m_response.m_sessionExpiryInterval == responseConfig->m_sessionExpiryInterval);
        }

        if (responseConfig->m_maxQos < CC_Mqtt5QoS_ValuesLimit) {
            test_assert(connectInfo.m_response.m_maxQos == responseConfig->m_maxQos);
        }    
        else {
            test_assert(connectInfo.m_response.m_maxQos == CC_Mqtt5QoS_ExactlyOnceDelivery);
        }    

        if (responseConfig->m_retainAvailable != nullptr) {
            test_assert(connectInfo.m_response.m_retainAvailable == *responseConfig->m_retainAvailable);
        }
        else {
            test_assert(connectInfo.m_response.m_retainAvailable);
        }

        if (responseConfig->m_maxPacketSize > 0U) {
            test_assert(connectInfo.m_response.m_maxPacketSize == responseConfig->m_maxPacketSize);
        }

        if (responseConfig->m_wildcardSubAvailable != nullptr) {
            test_assert(connectInfo.m_response.m_wildcardSubAvailable == *responseConfig->m_wildcardSubAvailable);
        }
        else {
            test_assert(connectInfo.m_response.m_wildcardSubAvailable);
        }      

        if (responseConfig->m_subIdsAvailable != nullptr) {
            test_assert(connectInfo.m_response.m_subIdsAvailable == *responseConfig->m_subIdsAvailable);
        }
        else {
            test_assert(connectInfo.m_response.m_subIdsAvailable);
        }     

        if (responseConfig->m_sharedSubsAvailable != nullptr) {
            test_assert(connectInfo.m_response.m_sharedSubsAvailable == *responseConfig->m_sharedSubsAvailable);
        }
        else {
            test_assert(connectInfo.m_response.m_sharedSubsAvailable);
        }                     
    }
    unitTestPopConnectResponseInfo();    
}

void UnitTestCommonBase::unitTestPerformBasicConnect(
    CC_Mqtt5Client* client, 
    const char* clientId, 
    bool cleanStart)
{
    auto basicConfig = CC_Mqtt5ConnectBasicConfig();
    unitTestConnectInitConfigBasic(&basicConfig);
    basicConfig.m_clientId = clientId;
    basicConfig.m_cleanStart = cleanStart;

    unitTestPerformConnect(client, &basicConfig);
}

void UnitTestCommonBase::unitTestPerformPubTopicAliasConnect(
    CC_Mqtt5Client* client, 
    const char* clientId, 
    unsigned topicAliasMax,
    bool cleanStart)
{
    auto basicConfig = CC_Mqtt5ConnectBasicConfig();
    unitTestConnectInitConfigBasic(&basicConfig);
    basicConfig.m_clientId = clientId;
    basicConfig.m_cleanStart = cleanStart;

    UnitTestConnectResponseConfig responseConfig;
    responseConfig.m_topicAliasMax = topicAliasMax;

    unitTestPerformConnect(client, &basicConfig, nullptr, nullptr, nullptr, &responseConfig);
}

void UnitTestCommonBase::unitTestPerformSessionExpiryConnect(
    CC_Mqtt5Client* client, 
    const char* clientId, 
    unsigned sessionExpiryInterval,
    bool cleanStart)
{
    auto basicConfig = CC_Mqtt5ConnectBasicConfig();
    unitTestConnectInitConfigBasic(&basicConfig);
    basicConfig.m_clientId = clientId;
    basicConfig.m_cleanStart = cleanStart;

    auto extraConfig = CC_Mqtt5ConnectExtraConfig();
    unitTestConnectInitConfigExtra(&extraConfig);
    extraConfig.m_sessionExpiryInterval = sessionExpiryInterval;

    unitTestPerformConnect(client, &basicConfig, nullptr, &extraConfig);
}

void UnitTestCommonBase::unitTestPerformDisconnect(
    CC_Mqtt5Client* client, 
    const CC_Mqtt5DisconnectConfig* config)
{
    [[maybe_unused]] auto ec = m_funcs.m_disconnect(client, config);
    test_assert(ec == CC_Mqtt5ErrorCode_Success);

    auto reason = UnitTestDisconnectReason::Success;
    if (config != nullptr) {
        reason = static_cast<decltype(reason)>(config->m_reasonCode);
    }

    unitTestVerifyDisconnectSent(reason);
}

void UnitTestCommonBase::unitTestPerformBasicDisconnect(CC_Mqtt5Client* client, CC_Mqtt5ReasonCode reasonCode)
{
    auto config = CC_Mqtt5DisconnectConfig();
    config.m_reasonCode = reasonCode;
    unitTestPerformDisconnect(client, &config);
}

void UnitTestCommonBase::unitTestPerformSubscribe(
    CC_Mqtt5Client* client, 
    CC_Mqtt5SubscribeTopicConfig* topicConfigs, 
    unsigned topicConfigsCount,
    const CC_Mqtt5SubscribeExtraConfig* extraConfig)
{
    auto ec = m_funcs.m_subscribe_full(client, topicConfigs, topicConfigsCount, extraConfig, &UnitTestCommonBase::unitTestSubscribeCompleteCb, this);
    test_assert(ec == CC_Mqtt5ErrorCode_Success);
    test_assert(!unitTestIsSubscribeComplete());

    auto sentMsg = unitTestGetSentMessage();
    test_assert(static_cast<bool>(sentMsg));
    test_assert(sentMsg->getId() == cc_mqtt5::MsgId_Subscribe);    
    [[maybe_unused]] auto* subscribeMsg = dynamic_cast<UnitTestSubscribeMsg*>(sentMsg.get());
    test_assert(subscribeMsg != nullptr);
    if ((extraConfig != nullptr) && (extraConfig->m_subId > 0U)) {
        UnitTestPropsHandler propsHandler;
        for (auto& p : subscribeMsg->field_properties().value()) {
            p.currentFieldExec(propsHandler);
        }

        assert (!propsHandler.m_subscriptionIds.empty());
        test_assert(propsHandler.m_subscriptionIds.front()->field_value().value() == extraConfig->m_subId);
    }

    unitTestTick(1000);
    UnitTestSubackMsg subackMsg;
    subackMsg.field_packetId().value() = subscribeMsg->field_packetId().value();
    subackMsg.field_list().value().resize(topicConfigsCount);
    for (auto idx = 0U; idx < topicConfigsCount; ++idx) {
        subackMsg.field_list().value()[0].setValue(static_cast<unsigned>(CC_Mqtt5ReasonCode_GrantedQos0) + topicConfigs[idx].m_maxQos);
    }

    unitTestReceiveMessage(subackMsg);
    test_assert(unitTestIsSubscribeComplete());    

    [[maybe_unused]] auto& subackInfo = unitTestSubscribeResponseInfo();
    test_assert(subackInfo.m_status == CC_Mqtt5AsyncOpStatus_Complete);
    test_assert(subackInfo.m_response.m_reasonCodes.size() == topicConfigsCount);
    unitTestPopSubscribeResponseInfo();    
}

void UnitTestCommonBase::unitTestPerformBasicSubscribe(CC_Mqtt5Client* client, const char* topic, unsigned subId)
{
    auto config = CC_Mqtt5SubscribeTopicConfig();
    unitTestSubscribeInitConfigTopic(&config);
    config.m_topic = topic;

    auto extra = CC_Mqtt5SubscribeExtraConfig();
    const CC_Mqtt5SubscribeExtraConfig* extraPtr = nullptr;

    if (subId > 0U) {
        extra.m_subId = subId;
        extraPtr = &extra;
    }    

    unitTestPerformSubscribe(client, &config, 1U, extraPtr);
}

void UnitTestCommonBase::unitTestVerifyDisconnectSent(UnitTestDisconnectReason reason)
{
    test_assert(unitTestHasSentMessage());
    auto sentMsg = unitTestGetSentMessage();
    test_assert(static_cast<bool>(sentMsg));
    test_assert(sentMsg->getId() == cc_mqtt5::MsgId_Disconnect);    
    auto* disconnectMsg = dynamic_cast<UnitTestDisconnectMsg*>(sentMsg.get());
    test_assert(disconnectMsg != nullptr);
    if (disconnectMsg->field_reasonCode().isMissing()) {
        test_assert(reason == UnitTestDisconnectReason::Success);
        return;
    }

    test_assert(disconnectMsg->field_reasonCode().field().value() == reason);        
}

UnitTestCommonBase::UnitTestClientPtr UnitTestCommonBase::unitTestAlloc()
{
    test_assert(m_funcs.m_alloc != nullptr);
    return UnitTestClientPtr(m_funcs.m_alloc(), UnitTestDeleter(m_funcs));
}

void UnitTestCommonBase::unitTestNotifyNetworkDisconnected(CC_Mqtt5Client* client)
{
    m_funcs.m_notify_network_disconnected(client);
}

bool UnitTestCommonBase::unitTestIsNetworkDisconnected(CC_Mqtt5Client* client)
{
    return m_funcs.m_is_network_disconnected(client);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestSetDefaultResponseTimeout(CC_Mqtt5Client* client, unsigned ms)
{
    return m_funcs.m_set_default_response_timeout(client, ms);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestPubTopicAliasAlloc(CC_Mqtt5Client* client, const char* topic, unsigned char qos0RegsCount)
{
    return m_funcs.m_pub_topic_alias_alloc(client, topic, qos0RegsCount);
}

unsigned UnitTestCommonBase::unitTestPubTopicAliasCount(CC_Mqtt5Client* client)
{
    return m_funcs.m_pub_topic_alias_count(client);
}

void UnitTestCommonBase::unitTestSetVerifyIncomingMsgSubscribed(CC_Mqtt5Client* client, bool enabled)
{
    m_funcs.m_set_verify_incoming_msg_subscribed(client, enabled);
}

CC_Mqtt5ConnectHandle UnitTestCommonBase::unitTestConnectPrepare(CC_Mqtt5Client* client, CC_Mqtt5ErrorCode* ec)
{
    return m_funcs.m_connect_prepare(client, ec);
}

void UnitTestCommonBase::unitTestConnectInitConfigBasic(CC_Mqtt5ConnectBasicConfig* config)
{
    return m_funcs.m_connect_init_config_basic(config);
}

void UnitTestCommonBase::unitTestConnectInitConfigWill(CC_Mqtt5ConnectWillConfig* config)
{
    return m_funcs.m_connect_init_config_will(config);
}

void UnitTestCommonBase::unitTestConnectInitConfigExtra(CC_Mqtt5ConnectExtraConfig* config)
{
    return m_funcs.m_connect_init_config_extra(config);
}

void UnitTestCommonBase::unitTestConnectInitConfigAuth(CC_Mqtt5AuthConfig* config)
{
    return m_funcs.m_connect_init_config_auth(config);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestConnectConfigBasic(CC_Mqtt5ConnectHandle handle, const CC_Mqtt5ConnectBasicConfig* config)
{
    return m_funcs.m_connect_config_basic(handle, config);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestConnectConfigWill(CC_Mqtt5ConnectHandle handle, const CC_Mqtt5ConnectWillConfig* config)
{
    return m_funcs.m_connect_config_will(handle, config);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestConnectConfigExtra(CC_Mqtt5ConnectHandle handle, const CC_Mqtt5ConnectExtraConfig* config)
{
    return m_funcs.m_connect_config_extra(handle, config);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestConnectConfigAuth(CC_Mqtt5ConnectHandle handle, const CC_Mqtt5AuthConfig* config)
{
    return m_funcs.m_connect_config_auth(handle, config);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestConnectAddUserProp(CC_Mqtt5ConnectHandle handle, const CC_Mqtt5UserProp* prop)
{
    return m_funcs.m_connect_add_user_prop(handle, prop);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestConnectAddWillUserProp(CC_Mqtt5ConnectHandle handle, const CC_Mqtt5UserProp* prop)
{
    return m_funcs.m_connect_add_user_prop(handle, prop);
}

bool UnitTestCommonBase::unitTestIsConnected(CC_Mqtt5Client* client)
{
    return m_funcs.m_is_connected(client);
}

CC_Mqtt5DisconnectHandle UnitTestCommonBase::unitTestDisconnectPrepare(CC_Mqtt5Client* client, CC_Mqtt5ErrorCode* ec)
{
    return m_funcs.m_disconnect_prepare(client, ec);
}

void UnitTestCommonBase::unitTestDisconnectInitConfig(CC_Mqtt5DisconnectConfig* config)
{
    return m_funcs.m_disconnect_init_config(config);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestDisconnectConfig(CC_Mqtt5DisconnectHandle handle, const CC_Mqtt5DisconnectConfig* config)
{
    return m_funcs.m_disconnect_config(handle, config);
}

CC_Mqtt5SubscribeHandle UnitTestCommonBase::unitTestSubscribePrepare(CC_Mqtt5Client* client, CC_Mqtt5ErrorCode* ec)
{
    return m_funcs.m_subscribe_prepare(client, ec);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestSubscribeSetResponseTimeout(CC_Mqtt5SubscribeHandle handle, unsigned ms)
{
    return m_funcs.m_subscribe_set_response_timeout(handle, ms);
}

void UnitTestCommonBase::unitTestSubscribeInitConfigTopic(CC_Mqtt5SubscribeTopicConfig* config)
{
    return m_funcs.m_subscribe_init_config_topic(config);
}

void UnitTestCommonBase::unitTestSubscribeInitConfigExtra(CC_Mqtt5SubscribeExtraConfig* config)
{
    return m_funcs.m_subscribe_init_config_extra(config);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestSubscribeConfigTopic(CC_Mqtt5SubscribeHandle handle, const CC_Mqtt5SubscribeTopicConfig* config)
{
    return m_funcs.m_subscribe_config_topic(handle, config);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestSubscribeConfigExtra(CC_Mqtt5SubscribeHandle handle, const CC_Mqtt5SubscribeExtraConfig* config)
{
    return m_funcs.m_subscribe_config_extra(handle, config);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestSubscribeAddUserProp(CC_Mqtt5SubscribeHandle handle, const CC_Mqtt5UserProp* prop)
{
    return m_funcs.m_subscribe_add_user_prop(handle, prop);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestSubscribeSimple(CC_Mqtt5Client* client, CC_Mqtt5SubscribeTopicConfig* config)
{
    return m_funcs.m_subscribe_simple(client, config, &UnitTestCommonBase::unitTestSubscribeCompleteCb, this);
}

CC_Mqtt5UnsubscribeHandle UnitTestCommonBase::unitTestUnsubscribePrepare(CC_Mqtt5Client* client, CC_Mqtt5ErrorCode* ec)
{
    return m_funcs.m_unsubscribe_prepare(client, ec);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestUnsubscribeSetResponseTimeout(CC_Mqtt5UnsubscribeHandle handle, unsigned ms)
{
    return m_funcs.m_unsubscribe_set_response_timeout(handle, ms);
}

void UnitTestCommonBase::unitTestUnsubscribeInitConfigTopic(CC_Mqtt5UnsubscribeTopicConfig* config)
{
    return m_funcs.m_unsubscribe_init_config_topic(config);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestUnsubscribeConfigTopic(CC_Mqtt5UnsubscribeHandle handle, const CC_Mqtt5UnsubscribeTopicConfig* config)
{
    return m_funcs.m_unsubscribe_config_topic(handle, config);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestUnsubscribeAddUserProp(CC_Mqtt5UnsubscribeHandle handle, const CC_Mqtt5UserProp* prop)
{
    return m_funcs.m_unsubscribe_add_user_prop(handle, prop);
}

CC_Mqtt5PublishHandle UnitTestCommonBase::unitTestPublishPrepare(CC_Mqtt5Client* client, CC_Mqtt5ErrorCode* ec)
{
    return m_funcs.m_publish_prepare(client, ec);
}

unsigned UnitTestCommonBase::unitTestPublishCount(CC_Mqtt5Client* client)
{
    return m_funcs.m_publish_count(client);
}

void UnitTestCommonBase::unitTestPublishInitConfigBasic(CC_Mqtt5PublishBasicConfig* config)
{
    return m_funcs.m_publish_init_config_basic(config);
}

void UnitTestCommonBase::unitTestPublishInitConfigExtra(CC_Mqtt5PublishExtraConfig* config)
{
    return m_funcs.m_publish_init_config_extra(config);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestPublishSetResponseTimeout(CC_Mqtt5PublishHandle handle, unsigned ms)
{
    return m_funcs.m_publish_set_response_timeout(handle, ms);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestPublishConfigBasic(CC_Mqtt5PublishHandle handle, const CC_Mqtt5PublishBasicConfig* config)
{
    return m_funcs.m_publish_config_basic(handle, config);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestPublishConfigExtra(CC_Mqtt5PublishHandle handle, const CC_Mqtt5PublishExtraConfig* config)
{
    return m_funcs.m_publish_config_extra(handle, config);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestPublishAddUserProp(CC_Mqtt5PublishHandle handle, const CC_Mqtt5UserProp* prop)
{
    return m_funcs.m_publish_add_user_prop(handle, prop);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestPublishCancel(CC_Mqtt5PublishHandle handle)
{
    return m_funcs.m_publish_cancel(handle);
}

bool UnitTestCommonBase::unitTestPublishWasInitiated(CC_Mqtt5PublishHandle handle)
{
    return m_funcs.m_publish_was_initiated(handle);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestPublishSetOrdering(CC_Mqtt5ClientHandle handle, CC_Mqtt5PublishOrdering ordering)
{
    return m_funcs.m_publish_set_ordering(handle, ordering);
}

CC_Mqtt5PublishOrdering UnitTestCommonBase::unitTestPublishGetOrdering(CC_Mqtt5ClientHandle handle)
{
    return m_funcs.m_publish_get_ordering(handle);
}

CC_Mqtt5ReauthHandle UnitTestCommonBase::unitTestReauthPrepare(CC_Mqtt5Client* client, CC_Mqtt5ErrorCode* ec)
{
    return m_funcs.m_reauth_prepare(client, ec);
}

void UnitTestCommonBase::unitTestReauthInitConfigAuth(CC_Mqtt5AuthConfig* config)
{
    return m_funcs.m_reauth_init_config_auth(config);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestReauthAddUserProp(CC_Mqtt5ReauthHandle handle, const CC_Mqtt5UserProp* prop)
{
    return m_funcs.m_reauth_add_user_prop(handle, prop);
}

void UnitTestCommonBase::unitTestSetNextTickProgramCb(CC_Mqtt5ClientHandle handle, CC_Mqtt5NextTickProgramCb cb, void* data)
{
    return m_funcs.m_set_next_tick_program_callback(handle, cb, data);
}

void UnitTestCommonBase::unitTestSetCancelNextTickWaitCb(CC_Mqtt5ClientHandle handle, CC_Mqtt5CancelNextTickWaitCb cb, void* data)
{
    return m_funcs.m_set_cancel_next_tick_wait_callback(handle, cb, data);
}

void UnitTestCommonBase::unitTestSetSendOutputDataCb(CC_Mqtt5ClientHandle handle, CC_Mqtt5SendOutputDataCb cb, void* data)
{
    return m_funcs.m_set_send_output_data_callback(handle, cb, data);
}

void UnitTestCommonBase::unitTestSetBrokerDisconnectReportCb(CC_Mqtt5ClientHandle handle, CC_Mqtt5BrokerDisconnectReportCb cb, void* data)
{
    return m_funcs.m_set_broker_disconnect_report_callback(handle, cb, data);
}

void UnitTestCommonBase::unitTestSetMessageReceivedReportCb(CC_Mqtt5ClientHandle handle, CC_Mqtt5MessageReceivedReportCb cb, void* data)
{
    return m_funcs.m_set_message_received_report_callback(handle, cb, data);
}

void UnitTestCommonBase::unitTestClearState(bool preserveTicks)
{
    if (!preserveTicks) {
        m_tickReq.clear();
    }
    m_sentData.clear();
    m_receivedData.clear();
    m_connectResp.clear();
    m_subscribeResp.clear();
    m_publishResp.clear();
    m_reauthResp.clear();
    m_inAuthInfo.clear();
    m_outAuthInfo.clear();
    m_userPropsTmp.clear();
    m_disconnectInfo.clear();
    m_disconnected = false;
}

void UnitTestCommonBase::unitTestErrorLogCb([[maybe_unused]] void* obj, const char* msg)
{
    std::cout << "ERROR: " << msg << std::endl;
}

void UnitTestCommonBase::unitTestBrokerDisconnectedCb(void* obj, const CC_Mqtt5DisconnectInfo* info)
{
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
    test_assert(!realObj->m_disconnected);
    realObj->m_disconnected = true;
    if (info != nullptr) {
        test_assert(realObj->m_disconnectInfo.empty());
        realObj->m_disconnectInfo.emplace_back(*info);
    }
}

void UnitTestCommonBase::unitTestMessageReceivedCb(void* obj, const CC_Mqtt5MessageInfo* info)
{
    test_assert(info != nullptr);
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
    realObj->m_receivedMessages.resize(realObj->m_receivedMessages.size() + 1U);
    realObj->m_receivedMessages.back() = *info;
}

void UnitTestCommonBase::unitTestSendOutputDataCb(void* obj, const unsigned char* buf, unsigned bufLen)
{
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
    std::copy_n(buf, bufLen, std::back_inserter(realObj->m_sentData));
}

void UnitTestCommonBase::unitTestProgramNextTickCb(void* obj, unsigned duration)
{
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
    test_assert(realObj->m_tickReq.empty());
    realObj->m_tickReq.push_back({duration, 0U});
}

unsigned UnitTestCommonBase::unitTestCancelNextTickWaitCb(void* obj) {
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
    test_assert(!realObj->m_tickReq.empty());
    auto elapsed = realObj->m_tickReq.front().m_elapsed;
    realObj->m_tickReq.erase(realObj->m_tickReq.begin());
    return elapsed;
}

void UnitTestCommonBase::unitTestConnectCompleteCb(void* obj, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response)
{
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
    test_assert(realObj->m_connectResp.empty());
    realObj->m_connectResp.resize(realObj->m_connectResp.size() + 1U);
    auto& info = realObj->m_connectResp.back();
    info.m_status = status;
    if (response != nullptr) {
        info.m_response = *response;
    }
}

void UnitTestCommonBase::unitTestSubscribeCompleteCb(void* obj, [[maybe_unused]] CC_Mqtt5SubscribeHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response)
{
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
    test_assert(realObj->m_subscribeResp.empty());
    realObj->m_subscribeResp.resize(realObj->m_subscribeResp.size() + 1U);
    auto& info = realObj->m_subscribeResp.back();
    info.m_status = status;
    if (response != nullptr) {
        info.m_response = *response;
    }
}

void UnitTestCommonBase::unitTestUnsubscribeCompleteCb(void* obj, [[maybe_unused]] CC_Mqtt5UnsubscribeHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5UnsubscribeResponse* response)
{
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
    test_assert(realObj->m_unsubscribeResp.empty());
    realObj->m_unsubscribeResp.resize(realObj->m_unsubscribeResp.size() + 1U);
    auto& info = realObj->m_unsubscribeResp.back();
    info.m_status = status;
    if (response != nullptr) {
        info.m_response = *response;
    }
}

void UnitTestCommonBase::unitTestPublishCompleteCb(void* obj, [[maybe_unused]] CC_Mqtt5PublishHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response)
{
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
    // test_assert(realObj->m_publishResp.empty());
    realObj->m_publishResp.resize(realObj->m_publishResp.size() + 1U);
    auto& info = realObj->m_publishResp.back();
    info.m_status = status;
    if (response != nullptr) {
        info.m_response = *response;
    }
}

void UnitTestCommonBase::unitTestReauthCompleteCb(void* obj, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5AuthInfo* response)
{
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
    test_assert(realObj->m_reauthResp.empty());
    realObj->m_reauthResp.resize(realObj->m_reauthResp.size() + 1U);
    auto& info = realObj->m_reauthResp.back();
    info.m_status = status;
    if (response != nullptr) {
        info.m_response = *response;
    }    
}

CC_Mqtt5AuthErrorCode UnitTestCommonBase::unitTestAuthCb(void* obj, const CC_Mqtt5AuthInfo* authInfoIn, CC_Mqtt5AuthInfo* authInfoOut)
{
    test_assert(authInfoIn != nullptr);
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);

    auto idx = realObj->m_inAuthInfo.size();
    realObj->m_inAuthInfo.push_back(UnitTestAuthInfo{*authInfoIn});
    if (realObj->m_outAuthInfo.size() <= idx) {
        return CC_Mqtt5AuthErrorCode_Disconnect;
    }

    test_assert(authInfoOut != nullptr);
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

