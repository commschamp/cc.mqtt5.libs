//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/UnsubscribeOp.h"
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

inline UnsubscribeOp* asUnsubscribeOp(void* data)
{
    return reinterpret_cast<UnsubscribeOp*>(data);
}

template <typename TField>
constexpr unsigned reasonCodesLength()
{
    return ReasonCodesCountHeper<TField, TField::hasFixedSize()>::Value;
}

} // namespace     

UnsubscribeOp::UnsubscribeOp(ClientImpl& client) : 
    Base(client),
    m_timer(client.timerMgr().allocTimer())
{
}    

UnsubscribeOp::~UnsubscribeOp()
{
    releasePacketId(m_unsubMsg.field_packetId().value());
}

CC_Mqtt5ErrorCode UnsubscribeOp::configTopic(const CC_Mqtt5UnsubscribeTopicConfig& config)
{
    if ((config.m_topic == nullptr) || (config.m_topic[0] == '\0')) {
        errorLog("Topic is not provided in unsubscribe configuration.");
        return CC_Mqtt5ErrorCode_BadParam;
    }

    if (!verifySubFilter(config.m_topic)) {
        errorLog("Bad topic filter format in unsubscribe.");
        return CC_Mqtt5ErrorCode_BadParam;
    }    

    if constexpr (Config::HasSubTopicVerification) {
        if (client().configState().m_verifySubFilter) {
            auto& filtersMap = client().reuseState().m_subFilters;
            auto iter = 
                std::lower_bound(
                    filtersMap.begin(), filtersMap.end(), config.m_topic,
                    [](auto& storedTopic, const char* topicParam)
                    {
                        return storedTopic < topicParam;
                    });

            if ((iter == filtersMap.end()) || (*iter != config.m_topic)) {
                errorLog("Requested unsubscribe hasn't been used for subscription before");
                return CC_Mqtt5ErrorCode_BadParam;
            }
        }
    }    

    auto& topicVec = m_unsubMsg.field_list().value();
    if (topicVec.max_size() <= topicVec.size()) {
        errorLog("Too many configured topics for unsubscribe operation.");
        return CC_Mqtt5ErrorCode_OutOfMemory;
    }

    topicVec.resize(topicVec.size() + 1U);
    auto& element = topicVec.back();
    element.value() = config.m_topic;

    if (maxStringLen() < element.value().size()) {
        errorLog("Unsubscription topic value is too long");
        topicVec.pop_back();
        return CC_Mqtt5ErrorCode_BadParam;
    }  

    return CC_Mqtt5ErrorCode_Success;
}

CC_Mqtt5ErrorCode UnsubscribeOp::addUserProp(const CC_Mqtt5UserProp& prop)
{
    auto& propsField = m_unsubMsg.field_propertiesList();
    return addUserPropToList(propsField, prop);
}

CC_Mqtt5ErrorCode UnsubscribeOp::send(CC_Mqtt5UnsubscribeCompleteCb cb, void* cbData) 
{
    auto completeOnError = 
        comms::util::makeScopeGuard(
            [this]()
            {
                opComplete();
            });

    if (cb == nullptr) {
        errorLog("Unsubscribe completion callback is not provided.");
        return CC_Mqtt5ErrorCode_BadParam;
    }

    if (m_unsubMsg.field_list().value().empty()) {
        errorLog("No unsubscribe topic has been configured.");
        return CC_Mqtt5ErrorCode_InsufficientConfig;
    }

    if (!m_timer.isValid()) {
        errorLog("The library cannot allocate required number of timers.");
        return CC_Mqtt5ErrorCode_InternalError;
    }    

    m_cb = cb;
    m_cbData = cbData;

    m_unsubMsg.field_packetId().setValue(allocPacketId());
    auto result = client().sendMessage(m_unsubMsg); 
    if (result != CC_Mqtt5ErrorCode_Success) {
        return result;
    }

    completeOnError.release(); // don't complete op yet
    auto guard = client().apiEnter();
    restartTimer();
    return CC_Mqtt5ErrorCode_Success;
}

void UnsubscribeOp::handle(UnsubackMsg& msg)
{
    if (msg.field_packetId().value() != m_unsubMsg.field_packetId().value()) {
        return;
    }

    using ReasonCodesListField = UnsubackMsg::Field_list;
    using ReasonCodesList = ObjListType<CC_Mqtt5ReasonCode, reasonCodesLength<ReasonCodesListField>()>;
    m_timer.cancel();
    auto status = CC_Mqtt5AsyncOpStatus_ProtocolError;
    ReasonCodesList reasonCodes; // Will be referenced in response
    UserPropsList userProps; // Will be referenced in response
    auto response = CC_Mqtt5UnsubscribeResponse();

    auto terminationReason = DisconnectReason::ProtocolError;
    auto terminateOnExit = 
        comms::util::makeScopeGuard(
            [&cl = client(), &terminationReason]()
            {
                terminationWithReason(cl, terminationReason);
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

    auto& unsubFiltersVec = m_unsubMsg.field_list().value();
    auto& reasonCodesVec = msg.field_list().value();

    if (unsubFiltersVec.size() != reasonCodesVec.size()) {
        errorLog("Amount of reason codes in UNSUBACK doesn't match amount of unsubcribe topics");
        return;
    }

    PropsHandler propsHandler;
    for (auto& p : msg.field_propertiesList().value()) {
        p.currentFieldExec(propsHandler);
    }

    if (propsHandler.isProtocolError()) {
        errorLog("Protocol error in UNSUBACK properties");
        return;
    }  

    reasonCodes.reserve(std::min(reasonCodesVec.size(), reasonCodesVec.max_size()));
    for (auto idx = 0U; idx < reasonCodesVec.size(); ++idx) {
        auto rc = msg.field_list().value()[idx];
        if (reasonCodes.max_size() <= idx) {
            errorLog("Cannot accumulate all the unsubscribe reported reason codes, insufficient memory");
            status = CC_Mqtt5AsyncOpStatus_InternalError;
            terminationReason = DisconnectReason::ImplSpecificError;
            return;
        }

        reasonCodes.push_back(static_cast<CC_Mqtt5ReasonCode>(rc.value()));

        if constexpr (Config::HasSubTopicVerification) {
            if (reasonCodes.back() >=  CC_Mqtt5ReasonCode_UnspecifiedError) {
                // Unsubscribe is not confirmed
                continue;
            }

            // Remove from the subscribed topics record regardless of the client().configState().m_verifySubFilter
            auto& topicStr = m_unsubMsg.field_list().value()[idx].value();
            auto& filtersMap = client().reuseState().m_subFilters;
            auto iter = 
                std::lower_bound(
                    filtersMap.begin(), filtersMap.end(), topicStr,
                    [](auto& storedTopic, auto& topicParam)
                    {
                        return storedTopic < topicParam;
                    });

            if ((iter == filtersMap.end()) || (*iter != topicStr)) {
                continue;
            }

            filtersMap.erase(iter);
        }        
    }

    comms::cast_assign(response.m_reasonCodesCount) = reasonCodes.size();
    if (response.m_reasonCodesCount > 0U) {
        response.m_reasonCodes = &reasonCodes[0];
    }

    if (propsHandler.m_reasonStr != nullptr) {
        if (!client().sessionState().m_problemInfoAllowed) {
            errorLog("Received reason string in UNSUBACK when \"problem information\" was disabled in CONNECT.");
            return; 
        }

        response.m_reasonStr = propsHandler.m_reasonStr->field_value().value().c_str();
    }       

    if (!propsHandler.m_userProps.empty()) {
        if (!client().sessionState().m_problemInfoAllowed) {
            errorLog("Received user properties in UNSUBACK when \"problem information\" was disabled in CONNECT.");
            return; 
        }

        fillUserProps(propsHandler, userProps);
        response.m_userProps = &userProps[0];
        comms::cast_assign(response.m_userPropsCount) = userProps.size();
    }    

    if (response.m_reasonCodesCount != m_unsubMsg.field_list().value().size()) {
        return;
    }

    terminateOnExit.release();
    status = CC_Mqtt5AsyncOpStatus_Complete;
}

Op::Type UnsubscribeOp::typeImpl() const
{
    return Type_Unsubscribe;
}

void UnsubscribeOp::terminateOpImpl(CC_Mqtt5AsyncOpStatus status)
{
    completeOpInternal(status);
}

void UnsubscribeOp::networkConnectivityChangedImpl()
{
    m_timer.setSuspended(client().sessionState().m_networkDisconnected);
}

void UnsubscribeOp::completeOpInternal(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5UnsubscribeResponse* response)
{
    auto cb = m_cb;
    auto* cbData = m_cbData;
    opComplete(); // mustn't access data members after destruction
    if (cb != nullptr) {
        cb(cbData, status, response);    
    }
}

void UnsubscribeOp::opTimeoutInternal()
{
    completeOpInternal(CC_Mqtt5AsyncOpStatus_Timeout);
}

void UnsubscribeOp::restartTimer()
{
    m_timer.wait(getResponseTimeout(), &UnsubscribeOp::opTimeoutCb, this);
}

void UnsubscribeOp::opTimeoutCb(void* data)
{
    asUnsubscribeOp(data)->opTimeoutInternal();
}

} // namespace op

} // namespace cc_mqtt5_client
