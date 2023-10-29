#include "UnitTestCommonBase.h"

#include "UnitTestPropsHandler.h"

#include <cstdlib>
#include <iostream>

namespace 
{

void test_assert(bool cond)
{
    if (!cond) {
        std::exit(1);
    }
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
            m_reasonCode, m_sessionExpiryInterval, m_highQosPubLimit, m_maxPacketSize, m_topicAliasMax,
            m_maxQos, m_sessionPresent, m_retainAvailable, m_wildcardSubAvailable, m_subIdsAvailable, 
            m_sharedSubsAvailable);

    auto responseTie = 
        std::forward_as_tuple(
            response.m_reasonCode, response.m_sessionExpiryInterval, response.m_highQosPubLimit, response.m_maxPacketSize, response.m_topicAliasMax,
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
    m_tickReq.clear();
    m_sentData.clear();
    m_receivedData.clear();
    m_connectResp.clear();
    m_subscribeResp.clear();
    m_inAuthInfo.clear();
    m_outAuthInfo.clear();
    m_disconnectInfo.clear();
    m_disconnected = false;
}

void UnitTestCommonBase::unitTestTearDown()
{
    m_client.reset();
}

UnitTestClientPtr::pointer UnitTestCommonBase::unitTestAllocClient(bool addLog)
{
    test_assert(!m_client);
    m_client.reset(cc_mqtt5_client_new());
    test_assert(!::cc_mqtt5_client_is_initialized(m_client.get()));
    if (addLog) {
        cc_mqtt5_client_set_error_log_callback(m_client.get(), &UnitTestCommonBase::unitTestErrorLogCb, nullptr);
    }
    cc_mqtt5_client_set_broker_disconnect_report_callback(m_client.get(), &UnitTestCommonBase::unitTestBrokerDisconnectedCb, this);
    cc_mqtt5_client_set_message_received_report_callback(m_client.get(), &UnitTestCommonBase::unitTestMessageReceivedCb, this);
    cc_mqtt5_client_set_send_output_data_callback(m_client.get(), &UnitTestCommonBase::unitTestSendOutputDataCb, this);
    cc_mqtt5_client_set_next_tick_program_callback(m_client.get(), &UnitTestCommonBase::unitTestProgramNextTickCb, this);
    cc_mqtt5_client_set_cancel_next_tick_wait_callback(m_client.get(), &UnitTestCommonBase::unitTestCancelNextTickWaitCb, this);
    [[maybe_unused]] auto ec = cc_mqtt5_client_init(m_client.get());
    test_assert(ec == CC_Mqtt5ErrorCode_Success);
    test_assert(::cc_mqtt5_client_is_initialized(m_client.get()));
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
    cc_mqtt5_client_tick(m_client.get(), ms);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestSendConnect(CC_Mqtt5ConnectHandle& connect)
{
    auto result = ::cc_mqtt5_client_connect_send(connect, &UnitTestCommonBase::unitTestConnectCompleteCb, this);
    connect = nullptr;
    return result;
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestSendSubscribe(CC_Mqtt5SubscribeHandle& subscribe)
{
    auto result = ::cc_mqtt5_client_subscribe_send(subscribe, &UnitTestCommonBase::unitTestSubscribeCompleteCb, this);
    subscribe = nullptr;
    return result;
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestSendUnsubscribe(CC_Mqtt5UnsubscribeHandle& unsubscribe)
{
    auto result = ::cc_mqtt5_client_unsubscribe_send(unsubscribe, &UnitTestCommonBase::unitTestUnsubscribeCompleteCb, this);
    unsubscribe = nullptr;
    return result;
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestSendPublish(CC_Mqtt5PublishHandle& publish)
{
    auto result = ::cc_mqtt5_client_publish_send(publish, &UnitTestCommonBase::unitTestPublishCompleteCb, this);
    publish = nullptr;
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

    auto consumed = cc_mqtt5_client_process_data(m_client.get(), &m_receivedData[0], static_cast<unsigned>(m_receivedData.size()));
    m_receivedData.erase(m_receivedData.begin(), m_receivedData.begin() + consumed);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestConfigAuth(CC_Mqtt5ConnectHandle handle, const std::string& method, const std::vector<std::uint8_t>& data)
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

void UnitTestCommonBase::unitTestAddOutAuth(const UnitTestAuthInfo& info)
{
    m_outAuthInfo.push_back(info);
}

void UnitTestCommonBase::unitTestClearAuth()
{
    m_inAuthInfo.clear();
    m_outAuthInfo.clear();
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
    CC_Mqtt5ConnectAuthConfig* authConfig,
    const UnitTestConnectResponseConfig* responseConfig)
{
    auto* connect = cc_mqtt5_client_connect_prepare(client, nullptr);
    test_assert(connect != nullptr);

    auto ec = CC_Mqtt5ErrorCode_Success;
    if (basicConfig != nullptr) {
        ec = cc_mqtt5_client_connect_config_basic(connect, basicConfig);
        test_assert(ec == CC_Mqtt5ErrorCode_Success);
    }

    if (willConfig != nullptr) {
        ec = cc_mqtt5_client_connect_config_will(connect, willConfig);
        test_assert(ec == CC_Mqtt5ErrorCode_Success);
    }

    if (extraConfig != nullptr) {
        ec = cc_mqtt5_client_connect_config_extra(connect, extraConfig);
        test_assert(ec == CC_Mqtt5ErrorCode_Success);
    }

    if (authConfig != nullptr) {
        test_assert(authConfig->m_authCb == nullptr);
        authConfig->m_authCb = &UnitTestCommonBase::unitTestAuthCb;
        authConfig->m_authCbData = this;        
        ec = cc_mqtt5_client_connect_config_auth(connect, authConfig);
        test_assert(ec == CC_Mqtt5ErrorCode_Success);
    }    

    ec = unitTestSendConnect(connect);
    test_assert(ec == CC_Mqtt5ErrorCode_Success);

    auto sentMsg = unitTestGetSentMessage();
    test_assert(static_cast<bool>(sentMsg));
    test_assert(sentMsg->getId() == cc_mqtt5::MsgId_Connect);
    test_assert(!unitTestIsConnectComplete());    

    auto* tickReq = unitTestTickReq();
    test_assert(tickReq->m_requested == UnitTestDefaultOpTimeoutMs);

    unitTestTick(1000);
    UnitTestConnackMsg connackMsg;
    connackMsg.field_reasonCode().value() = UnitTestConnackMsg::Field_reasonCode::ValueType::Success;
    auto& propsVec = connackMsg.field_propertiesList().value();

    if (authConfig != nullptr) {
        {
            propsVec.resize(propsVec.size() + 1U);
            auto& field = propsVec.back().initField_authMethod();
            field.field_value().setValue(authConfig->m_authMethod);
        }

        {
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
    test_assert((responseConfig != nullptr) || (connectInfo.m_response.m_maxPacketSize == 0U));

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
    ::cc_mqtt5_client_connect_init_config_basic(&basicConfig);
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
    ::cc_mqtt5_client_connect_init_config_basic(&basicConfig);
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
    ::cc_mqtt5_client_connect_init_config_basic(&basicConfig);
    basicConfig.m_clientId = clientId;
    basicConfig.m_cleanStart = cleanStart;

    auto extraConfig = CC_Mqtt5ConnectExtraConfig();
    ::cc_mqtt5_client_connect_init_config_extra(&extraConfig);
    extraConfig.m_sessionExpiryInterval = sessionExpiryInterval;

    unitTestPerformConnect(client, &basicConfig, nullptr, &extraConfig);
}

void UnitTestCommonBase::unitTestPerformDisconnect(
    CC_Mqtt5Client* client, 
    const CC_Mqtt5DisconnectConfig* config)
{
    auto* disconnect = ::cc_mqtt5_client_disconnect_prepare(client, nullptr);
    test_assert(disconnect != nullptr);
    if (config != nullptr) {
        [[maybe_unused]] auto ec = cc_mqtt5_client_diconnect_config(disconnect, config);
        test_assert(ec == CC_Mqtt5ErrorCode_Success);
    }

    [[maybe_unused]] auto ec = cc_mqtt5_client_disconnect_send(disconnect);
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

void UnitTestCommonBase::unitTestPerformBasicSubscribe(CC_Mqtt5Client* client, const char* topic, unsigned subId)
{
    auto config = CC_Mqtt5SubscribeTopicConfig();
    ::cc_mqtt5_client_subscribe_init_config_topic(&config);
    config.m_topic = topic;

    auto subscribe = ::cc_mqtt5_client_subscribe_prepare(client, nullptr);
    test_assert(subscribe != nullptr);

    [[maybe_unused]] auto ec = ::cc_mqtt5_client_subscribe_config_topic(subscribe, &config);
    test_assert(ec == CC_Mqtt5ErrorCode_Success);

    if (subId > 0U) {
        auto extra = CC_Mqtt5SubscribeExtraConfig();
        extra.m_subId = subId;
        ec = cc_mqtt5_client_subscribe_config_extra(subscribe, &extra);
        test_assert(ec == CC_Mqtt5ErrorCode_Success);
    }

    ec = unitTestSendSubscribe(subscribe);
    test_assert(ec == CC_Mqtt5ErrorCode_Success);
    test_assert(!unitTestIsSubscribeComplete());

    auto sentMsg = unitTestGetSentMessage();
    test_assert(static_cast<bool>(sentMsg));
    test_assert(sentMsg->getId() == cc_mqtt5::MsgId_Subscribe);    
    [[maybe_unused]] auto* subscribeMsg = dynamic_cast<UnitTestSubscribeMsg*>(sentMsg.get());
    test_assert(subscribeMsg != nullptr);
    if (subId > 0U) {
        UnitTestPropsHandler propsHandler;
        for (auto& p : subscribeMsg->field_propertiesList().value()) {
            p.currentFieldExec(propsHandler);
        }

        assert (!propsHandler.m_subscriptionIds.empty());
        test_assert(propsHandler.m_subscriptionIds.front()->field_value().value() == subId);
    }

    unitTestTick(1000);
    UnitTestSubackMsg subackMsg;
    subackMsg.field_packetId().value() = subscribeMsg->field_packetId().value();
    subackMsg.field_list().value().resize(1);
    subackMsg.field_list().value()[0].setValue(CC_Mqtt5ReasonCode_Success);
    unitTestReceiveMessage(subackMsg);
    test_assert(unitTestIsSubscribeComplete());    

    [[maybe_unused]] auto& subackInfo = unitTestSubscribeResponseInfo();
    test_assert(subackInfo.m_status == CC_Mqtt5AsyncOpStatus_Complete);
    test_assert(subackInfo.m_response.m_reasonCodes.size() == 1U);
    test_assert(subackInfo.m_response.m_reasonCodes[0] == CC_Mqtt5ReasonCode_Success);
    unitTestPopSubscribeResponseInfo();    
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

void UnitTestCommonBase::unitTestSubscribeCompleteCb(void* obj, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response)
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

void UnitTestCommonBase::unitTestUnsubscribeCompleteCb(void* obj, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5UnsubscribeResponse* response)
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

void UnitTestCommonBase::unitTestPublishCompleteCb(void* obj, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response)
{
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
    test_assert(realObj->m_publishResp.empty());
    realObj->m_publishResp.resize(realObj->m_publishResp.size() + 1U);
    auto& info = realObj->m_publishResp.back();
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

