//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/RecvOp.h"
#include "ClientImpl.h"

namespace cc_mqtt5_client
{

namespace op
{

namespace 
{

inline RecvOp* asRecvOp(void* data)
{
    return reinterpret_cast<RecvOp*>(data);
}

} // namespace     

RecvOp::RecvOp(ClientImpl& client) : 
    Base(client),
    m_recvTimer(client.timerMgr().allocTimer())
{
    COMMS_ASSERT(m_recvTimer.isValid());
}    

void RecvOp::handle(PublishMsg& msg)
{
    using Qos = PublishMsg::TransportField_flags::Field_qos::ValueType;
    if (msg.transportField_flags().field_qos().value() == Qos::AtMostOnceDelivery) {
        auto msgInfo = CC_Mqtt5MessageInfo();
        populateMsgInfo(msg, msgInfo);
        reportMsgInfo(msgInfo);
        opComplete();
        return;
    }

    if (msg.transportField_flags().field_qos().value() == Qos::AtLeastOnceDelivery) {
        PubackMsg pubackMsg;
        pubackMsg.field_packetId().value() = msg.field_packetId().field().value();
        pubackMsg.field_reasonCode().value() = PubackMsg::Field_reasonCode::ValueType::Success;
        sendMessage(pubackMsg);

        auto msgInfo = CC_Mqtt5MessageInfo();
        populateMsgInfo(msg, msgInfo);
        reportMsgInfo(msgInfo);
        opComplete();
        return;
    }    

    if (msg.transportField_flags().field_qos().value() != Qos::ExactlyOnceDelivery) {
        sendDisconnectWithReason(DisconnectMsg::Field_reasonCode::Field::ValueType::ProtocolError);
        opComplete();
        return;
    }

    m_pubMsg = msg;
    PubrecMsg pubrecMsg;
    pubrecMsg.field_packetId().value() = msg.field_packetId().field().value();
    pubrecMsg.field_reasonCode().value() = PubackMsg::Field_reasonCode::ValueType::Success;
    sendMessage(pubrecMsg);
    restartRecvTimer();
}

Op::Type RecvOp::typeImpl() const
{
    return Type_Recv;
}

void RecvOp::restartRecvTimer()
{
    auto& state = client().state();
    m_recvTimer.wait(state.m_responseTimeoutMs, &RecvOp::recvTimeoutCb, this);
}

void RecvOp::recvTimoutInternal()
{
    // TODO:
    COMMS_ASSERT(!m_recvTimer.isActive());
    // sendDisconnectWithReason(DisconnectMsg::Field_reasonCode::Field::ValueType::KeepAliveTimeout);
    // client().notifyDisconnected(true);
}

void RecvOp::recvTimeoutCb(void* data)
{
    asRecvOp(data)->recvTimoutInternal();
}

void RecvOp::populateMsgInfo(const PublishMsg& msg, CC_Mqtt5MessageInfo& info)
{
    // TODO:
    static_cast<void>(msg);
    static_cast<void>(info);
}

void RecvOp::reportMsgInfo(const CC_Mqtt5MessageInfo& info)
{
    client().reportMsgInfo(info);
}

} // namespace op

} // namespace cc_mqtt5_client
