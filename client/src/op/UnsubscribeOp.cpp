//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/UnsubscribeOp.h"
#include "Client.h"

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

UnsubscribeOp::UnsubscribeOp(Client& client) : 
    Base(client),
    m_timer(client.timerMgr().allocTimer())
{
}    

CC_Mqtt5ErrorCode UnsubscribeOp::configTopic(const CC_Mqtt5UnsubscribeTopicConfig& config)
{
    if (config.m_topic == nullptr) {
        return CC_Mqtt5ErrorCode_BadParam;
    }

    auto& topicVec = m_unsubMsg.field_list().value();
    if (topicVec.max_size() <= topicVec.size()) {
        return CC_Mqtt5ErrorCode_OutOfMemory;
    }

    topicVec.resize(topicVec.size() + 1U);
    auto& element = topicVec.back();
    element.value() = config.m_topic;
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
        return CC_Mqtt5ErrorCode_BadParam;
    }

    if (m_unsubMsg.field_list().value().empty()) {
        return CC_Mqtt5ErrorCode_InsufficientConfig;
    }

    if (!m_timer.isValid()) {
        return CC_Mqtt5ErrorCode_OutOfMemory;
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

    auto completeOpOnExit = 
        comms::util::makeScopeGuard(
            [this, &status, &response]()
            {
                completeOpInternal(status, &response);
            });

    PropsHandler propsHandler;
    for (auto& p : msg.field_propertiesList().value()) {
        p.currentFieldExec(propsHandler);
    }

    if (propsHandler.isProtocolError()) {
        sendDisconnectWithReason(DisconnectMsg::Field_reasonCode::Field::ValueType::ProtocolError);
        return;
    }  

    reasonCodes.reserve(msg.field_list().value().size());
    for (auto rc : msg.field_list().value()) {
        reasonCodes.push_back(static_cast<CC_Mqtt5ReasonCode>(rc.value()));
    }

    comms::cast_assign(response.m_reasonCodesCount) = reasonCodes.size();
    if (response.m_reasonCodesCount > 0U) {
        response.m_reasonCodes = &reasonCodes[0];
    }

    if (propsHandler.m_reasonStr != nullptr) {
        response.m_reasonStr = propsHandler.m_reasonStr->field_value().value().c_str();
    }       

    if (!propsHandler.m_userProps.empty()) {
        fillUserProps(propsHandler, userProps);
        response.m_userProps = &userProps[0];
        comms::cast_assign(response.m_userPropsCount) = userProps.size();
    }    

    if (response.m_reasonCodesCount != m_unsubMsg.field_list().value().size()) {
        sendDisconnectWithReason(DisconnectMsg::Field_reasonCode::Field::ValueType::ProtocolError);
        return;
    }

    status = CC_Mqtt5AsyncOpStatus_Complete;
}

Op::Type UnsubscribeOp::typeImpl() const
{
    return Type_Unsubscribe;
}

void UnsubscribeOp::completeOpInternal(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5UnsubscribeResponse* response)
{
    COMMS_ASSERT(m_cb != nullptr);
    auto cb = m_cb;
    auto* cbData = m_cbData;
    opComplete(); // mustn't access data members after destruction
    cb(cbData, status, response);    
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
