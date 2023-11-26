//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/SubscribeOp.h"
#include "ClientImpl.h"

#include "comms/util/ScopeGuard.h"

namespace cc_mqtt5_client
{

namespace op
{

namespace 
{

template <typename TField, bool THasFixedLength>
struct ReasonCodesCountHeper
{
    static constexpr unsigned Value = 0U;
};  

template <typename TField>
struct ReasonCodesCountHeper<TField, true>
{
    static constexpr unsigned Value = TField::fixedSize();
};

inline SubscribeOp* asSubscribeOp(void* data)
{
    return reinterpret_cast<SubscribeOp*>(data);
}

template <typename TField>
constexpr unsigned reasonCodesLength()
{
    return ReasonCodesCountHeper<TField, TField::hasFixedSize()>::Value;
}

} // namespace     

SubscribeOp::SubscribeOp(ClientImpl& client) : 
    Base(client),
    m_timer(client.timerMgr().allocTimer())
{
}  

SubscribeOp::~SubscribeOp()
{
    releasePacketId(m_subMsg.field_packetId().value());
}

CC_Mqtt5ErrorCode SubscribeOp::configTopic(const CC_Mqtt5SubscribeTopicConfig& config)
{
    if ((config.m_topic == nullptr) || (config.m_topic[0] == '\0')) {
        errorLog("Topic is not provided in subscribe configuration.");
        return CC_Mqtt5ErrorCode_BadParam;
    }

    if (!verifySubFilter(config.m_topic)) {
        errorLog("Bad topic filter format in subscribe.");
        return CC_Mqtt5ErrorCode_BadParam;
    }

    if (config.m_noLocal && isSharedTopicFilter(config.m_topic)) {
        errorLog("Shared subscriptions cannot set \"no local\" flag.");
        return CC_Mqtt5ErrorCode_BadParam;
    }    

    if (CC_Mqtt5QoS_ValuesLimit <= config.m_maxQos) {
        errorLog("Bad subscription qos value.");
        return CC_Mqtt5ErrorCode_BadParam;        
    }

    auto& topicVec = m_subMsg.field_list().value();
    if (topicVec.max_size() <= topicVec.size()) {
        errorLog("Too many configured topics for subscribe operation.");
        return CC_Mqtt5ErrorCode_OutOfMemory;
    }

    topicVec.resize(topicVec.size() + 1U);
    auto& element = topicVec.back();
    element.field_topic().value() = config.m_topic;
    element.field_options().field_qos().setValue(config.m_maxQos);
    element.field_options().field_bits().setBitValue_NL(config.m_noLocal);
    element.field_options().field_bits().setBitValue_RAP(config.m_retainAsPublished);
    element.field_options().field_retainHandling().setValue(config.m_retainHandling);

    if (maxStringLen() < element.field_topic().value().size()) {
        errorLog("Subscription topic value is too long");
        topicVec.pop_back();
        return CC_Mqtt5ErrorCode_BadParam;
    }   

    return CC_Mqtt5ErrorCode_Success;
}

CC_Mqtt5ErrorCode SubscribeOp::configExtra(const CC_Mqtt5SubscribeExtraConfig& config)
{
    auto& propsField = m_subMsg.field_properties();
    if (config.m_subId > 0) {
        static constexpr unsigned MaxSubId = 268435455;
        if (MaxSubId <= config.m_subId) {
            errorLog("Provided subscribe ID is too high.");
            return CC_Mqtt5ErrorCode_BadParam;
        }

        auto& state = client().sessionState();
        if (!state.m_subIdsAvailable) {
            errorLog("Usage of subscribe IDs is not supported by broker.");
            return CC_Mqtt5ErrorCode_BadParam;
        }

        if (!canAddProp(propsField)) {
            errorLog("Cannot add subscribe property, reached available limit.");
            return CC_Mqtt5ErrorCode_OutOfMemory;
        }

        auto& propVar = addProp(propsField);
        auto& propBundle = propVar.initField_subscriptionId();
        auto& valueField = propBundle.field_value();        
        valueField.setValue(config.m_subId);
    }

    return CC_Mqtt5ErrorCode_Success;
}

CC_Mqtt5ErrorCode SubscribeOp::addUserProp(const CC_Mqtt5UserProp& prop)
{
    auto& propsField = m_subMsg.field_properties();
    return addUserPropToList(propsField, prop);
}

CC_Mqtt5ErrorCode SubscribeOp::send(CC_Mqtt5SubscribeCompleteCb cb, void* cbData) 
{
    auto completeOnError = 
        comms::util::makeScopeGuard(
            [this]()
            {
                opComplete();
            });

    if (cb == nullptr) {
        errorLog("Subscribe completion callback is not provided.");
        return CC_Mqtt5ErrorCode_BadParam;
    }

    if (m_subMsg.field_list().value().empty()) {
        errorLog("No subscribe topic has been configured.");
        return CC_Mqtt5ErrorCode_InsufficientConfig;
    }

    if (!m_timer.isValid()) {
        errorLog("The library cannot allocate required number of timers.");
        return CC_Mqtt5ErrorCode_InternalError;
    }    

    m_cb = cb;
    m_cbData = cbData;

    m_subMsg.field_packetId().setValue(allocPacketId());
    auto result = client().sendMessage(m_subMsg); 
    if (result != CC_Mqtt5ErrorCode_Success) {
        return result;
    }

    completeOnError.release(); // don't complete op yet
    auto guard = client().apiEnter();
    restartTimer();
    return CC_Mqtt5ErrorCode_Success;
}

CC_Mqtt5ErrorCode SubscribeOp::cancel()
{
    opComplete();
    return CC_Mqtt5ErrorCode_Success;
}

void SubscribeOp::handle(SubackMsg& msg)
{
    auto packetId = msg.field_packetId().value();
    if (packetId != m_subMsg.field_packetId().value()) {
        return;
    }

    using ReasonCodesListField = SubackMsg::Field_list;
    using ReasonCodesList = ObjListType<CC_Mqtt5ReasonCode, reasonCodesLength<ReasonCodesListField>()>;
    m_timer.cancel();
    auto status = CC_Mqtt5AsyncOpStatus_ProtocolError;
    ReasonCodesList reasonCodes; // Will be referenced in response
    UserPropsList userProps; // Will be referenced in response
    auto response = CC_Mqtt5SubscribeResponse();

    auto terminationReason = DisconnectReason::ProtocolError;
    auto terminateOnExit = 
        comms::util::makeScopeGuard(
            [&cl = client(), &terminationReason]()
            {
                terminationWithReasonStatic(cl, terminationReason);
            }
        );    

    auto completeOpOnExit = 
        comms::util::makeScopeGuard(
            [this, &status, &response]()
            {
                auto* responsePtr = &response;
                if (status != CC_Mqtt5AsyncOpStatus_Complete) {
                    responsePtr = nullptr;
                }
                completeOpInternal(status, responsePtr);
            });     

    PropsHandler propsHandler;
    for (auto& p : msg.field_properties().value()) {
        p.currentFieldExec(propsHandler);
    }

    if (propsHandler.isProtocolError()) {
        errorLog("Protocol error in SUBACK properties");
        return;
    }  

    auto& topicsVec = m_subMsg.field_list().value();
    auto& reasonCodesVec = msg.field_list().value();

    if (topicsVec.size() != reasonCodesVec.size()) {
        errorLog("Amount of reason codes in SUBACK doesn't match amount of subcribe topics");
        return;
    }

    reasonCodes.reserve(std::min(reasonCodesVec.size(), reasonCodesVec.max_size()));
    for (auto idx = 0U; idx < reasonCodesVec.size(); ++idx) {
        auto rc = msg.field_list().value()[idx];
        if (reasonCodes.max_size() <= idx) {
            errorLog("Cannot accumulate all the subscribe reported reason codes, insufficient memory");
            status = CC_Mqtt5AsyncOpStatus_InternalError;
            terminationReason = DisconnectReason::ImplSpecificError;
            return;
        }

        reasonCodes.push_back(static_cast<CC_Mqtt5ReasonCode>(rc.value()));

        if constexpr (Config::HasSubTopicVerification) {
            if (!client().configState().m_verifySubFilter) {
                continue;
            }

            if (reasonCodes.back() >=  CC_Mqtt5ReasonCode_UnspecifiedError) {
                // Subscribe is not confirmed
                continue;
            }            
            
            auto& topicStr = m_subMsg.field_list().value()[idx].field_topic().value();
            auto& filtersMap = client().reuseState().m_subFilters;
            auto iter = 
                std::lower_bound(
                    filtersMap.begin(), filtersMap.end(), topicStr,
                    [](auto& storedTopic, auto& topicParam)
                    {
                        return storedTopic < topicParam;
                    });

            if ((iter != filtersMap.end()) && (*iter == topicStr)) {
                continue;
            }

            if (filtersMap.max_size() <= filtersMap.size()) {
                errorLog("Subscibe filters storage reached its maximum, can't store any more topics");
                status = CC_Mqtt5AsyncOpStatus_InternalError;
                terminationReason = DisconnectReason::ImplSpecificError;
                return;
            }

            filtersMap.insert(iter, topicStr);
        }
    }

    comms::cast_assign(response.m_reasonCodesCount) = reasonCodes.size();
    if (response.m_reasonCodesCount > 0U) {
        response.m_reasonCodes = &reasonCodes[0];
    }

    if (propsHandler.m_reasonStr != nullptr) {
        if (!client().sessionState().m_problemInfoAllowed) {
            errorLog("Received reason string in SUBACK when \"problem information\" was disabled in CONNECT.");
            return; 
        }

        response.m_reasonStr = propsHandler.m_reasonStr->field_value().value().c_str();
    }       

    if (!propsHandler.m_userProps.empty()) {
        if (!client().sessionState().m_problemInfoAllowed) {
            errorLog("Received user properties in SUBACK when \"problem information\" was disabled in CONNECT.");
            return; 
        }

        fillUserProps(propsHandler, userProps);
        response.m_userProps = &userProps[0];
        comms::cast_assign(response.m_userPropsCount) = userProps.size();
    }    

    terminateOnExit.release();
    status = CC_Mqtt5AsyncOpStatus_Complete;
}

Op::Type SubscribeOp::typeImpl() const
{
    return Type_Subscribe;
}

void SubscribeOp::terminateOpImpl(CC_Mqtt5AsyncOpStatus status)
{
    completeOpInternal(status);
}

void SubscribeOp::networkConnectivityChangedImpl()
{
    m_timer.setSuspended(client().sessionState().m_networkDisconnected);
}

void SubscribeOp::completeOpInternal(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response)
{
    auto cb = m_cb;
    auto* cbData = m_cbData;
    opComplete(); // mustn't access data members after destruction
    if (cb != nullptr) {
        cb(cbData, status, response);    
    }
}

void SubscribeOp::opTimeoutInternal()
{
    completeOpInternal(CC_Mqtt5AsyncOpStatus_Timeout);
}

void SubscribeOp::restartTimer()
{
    m_timer.wait(getResponseTimeout(), &SubscribeOp::opTimeoutCb, this);
}

void SubscribeOp::opTimeoutCb(void* data)
{
    asSubscribeOp(data)->opTimeoutInternal();
}

} // namespace op

} // namespace cc_mqtt5_client
