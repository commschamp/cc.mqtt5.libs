#include "UnitTestCommonBase.h"

namespace 
{

void assignStringInternal(std::string& dest, const char* source)
{
    dest.clear();
    if (source != nullptr)
    {
        dest = source;
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

UnitTestCommonBase::UnitTestSubscribeResponse& UnitTestCommonBase::UnitTestSubscribeResponse::operator=(const CC_Mqtt5SubscribeResponse& response)
{
    m_reasonCodes.clear();
    if (response.m_reasonCodesCount > 0U) {
        std::copy_n(response.m_reasonCodes, response.m_reasonCodesCount, std::back_inserter(m_reasonCodes));
    }
    
    assignStringInternal(m_reasonStr, response.m_reasonStr);
    UnitTestUserProp::copyProps(response.m_userProps, response.m_userPropsCount, m_userProps);
    return *this;
}

UnitTestCommonBase::UnitTestDisconnectInfo& UnitTestCommonBase::UnitTestDisconnectInfo::operator=(const CC_Mqtt5DisconnectInfo& other)
{
    std::tie(m_reasonCode, m_reasonStr, m_serverRef) = std::make_tuple(other.m_reasonCode, other.m_reasonStr, other.m_serverRef);
    UnitTestUserProp::copyProps(other.m_userProps, other.m_userPropsCount, m_userProps);
    return *this;
}

void UnitTestCommonBase::unitTestSetUp()
{
    assert(!m_client);
    m_tickReq.clear();
    m_sentData.clear();
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

UnitTestClientPtr::pointer UnitTestCommonBase::unitTestAllocClient()
{
    m_client.reset(cc_mqtt5_client_new());
    cc_mqtt5_client_set_broker_disconnect_report_callback(m_client.get(), &UnitTestCommonBase::unitTestBrokerDisconnectedCb, this);
    cc_mqtt5_client_set_message_received_report_callback(m_client.get(), &UnitTestCommonBase::unitTestMessageReceivedCb, this);
    cc_mqtt5_client_set_send_output_data_callback(m_client.get(), &UnitTestCommonBase::unitTestSendOutputDataCb, this);
    cc_mqtt5_client_set_next_tick_program_callback(m_client.get(), &UnitTestCommonBase::unitTestProgramNextTickCb, this);
    cc_mqtt5_client_set_cancel_next_tick_wait_callback(m_client.get(), &UnitTestCommonBase::unitTestCancelNextTickWaitCb, this);
    auto ec = cc_mqtt5_client_init(m_client.get());
    assert(ec == CC_Mqtt5ErrorCode_Success);
    return m_client.get();
}

const UnitTestCommonBase::TickInfo* UnitTestCommonBase::unitTestTickReq()
{
    assert(!m_tickReq.empty());
    return &m_tickReq.front();
}

bool UnitTestCommonBase::unitTestCheckNoTicks()
{
    return m_tickReq.empty();
}

void UnitTestCommonBase::unitTestTick(unsigned ms, bool forceTick)
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

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestSendConnect(CC_Mqtt5ConnectHandle connect)
{
    return ::cc_mqtt5_client_connect_send(connect, &UnitTestCommonBase::unitTestConnectCompleteCb, this);
}

CC_Mqtt5ErrorCode UnitTestCommonBase::unitTestSendSubscribe(CC_Mqtt5SubscribeHandle subscribe)
{
    return ::cc_mqtt5_client_subscribe_send(subscribe, &UnitTestCommonBase::unitTestSubscribeCompleteCb, this);
}

UniTestsMsgPtr UnitTestCommonBase::unitTestGetSentMessage()
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
    assert(unitTestIsConnectComplete());
    return m_connectResp.front();
}

void UnitTestCommonBase::unitTestPopConnectResponseInfo()
{
    assert(!m_connectResp.empty());
    m_connectResp.erase(m_connectResp.begin());
}

bool UnitTestCommonBase::unitTestIsSubscribeComplete()
{
    return !m_subscribeResp.empty();
}
const UnitTestCommonBase::UnitTestSubscribeResponseInfo& UnitTestCommonBase::unitTestSubscribeResponseInfo()
{
    assert(!m_subscribeResp.empty());
    return m_subscribeResp.front();
}

void UnitTestCommonBase::unitTestPopSubscribeResponseInfo()
{
    assert(!m_subscribeResp.empty());
    m_subscribeResp.erase(m_subscribeResp.begin());
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
    assert(es == comms::ErrorStatus::Success);

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
    assert(!m_inAuthInfo.empty());
    return m_inAuthInfo.front();
}

void UnitTestCommonBase::unitTestPopInAuthInfo()
{
    assert(!m_inAuthInfo.empty());
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
    assert(!m_disconnectInfo.empty());
    return m_disconnectInfo.front();
}

void UnitTestCommonBase::unitTestPopDisconnectInfo()
{
    assert(!m_disconnectInfo.empty());
    m_disconnectInfo.erase(m_disconnectInfo.begin());
}


void UnitTestCommonBase::unitTestBrokerDisconnectedCb(void* obj, const CC_Mqtt5DisconnectInfo* info)
{
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
    assert(!realObj->m_disconnected);
    realObj->m_disconnected = true;
    if (info != nullptr) {
        assert(realObj->m_disconnectInfo.empty());
        realObj->m_disconnectInfo.emplace_back(*info);
    }
}

void UnitTestCommonBase::unitTestMessageReceivedCb(void* obj, const CC_Mqtt5MessageInfo* info)
{
    // TODO: 
    static_cast<void>(obj);
    static_cast<void>(info);
}

void UnitTestCommonBase::unitTestSendOutputDataCb(void* obj, const unsigned char* buf, unsigned bufLen)
{
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
    std::copy_n(buf, bufLen, std::back_inserter(realObj->m_sentData));
}

void UnitTestCommonBase::unitTestProgramNextTickCb(void* obj, unsigned duration)
{
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
    assert(realObj->m_tickReq.empty());
    realObj->m_tickReq.push_back({duration, 0U});
}

unsigned UnitTestCommonBase::unitTestCancelNextTickWaitCb(void* obj) {
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
    assert(!realObj->m_tickReq.empty());
    auto elapsed = realObj->m_tickReq.front().m_elapsed;
    realObj->m_tickReq.erase(realObj->m_tickReq.begin());
    return elapsed;
}

void UnitTestCommonBase::unitTestConnectCompleteCb(void* obj, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5ConnectResponse* response)
{
    auto* realObj = reinterpret_cast<UnitTestCommonBase*>(obj);
    assert(realObj->m_connectResp.empty());
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
    assert(realObj->m_subscribeResp.empty());
    realObj->m_subscribeResp.resize(realObj->m_subscribeResp.size() + 1U);
    auto& info = realObj->m_subscribeResp.back();
    info.m_status = status;
    if (response != nullptr) {
        info.m_response = *response;
    }
}

CC_Mqtt5AuthErrorCode UnitTestCommonBase::unitTestAuthCb(void* obj, const CC_Mqtt5AuthInfo* authInfoIn, CC_Mqtt5AuthInfo* authInfoOut)
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

