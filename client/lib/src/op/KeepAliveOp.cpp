//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/KeepAliveOp.h"
#include "ClientImpl.h"

namespace cc_mqtt5_client
{

namespace op
{

namespace 
{

inline KeepAliveOp* asKeepAliveOp(void* data)
{
    return reinterpret_cast<KeepAliveOp*>(data);
}

} // namespace     

KeepAliveOp::KeepAliveOp(ClientImpl& client) : 
    Base(client),
    m_pingTimer(client.timerMgr().allocTimer()),
    m_recvTimer(client.timerMgr().allocTimer()),
    m_respTimer(client.timerMgr().allocTimer())
{
    COMMS_ASSERT(m_pingTimer.isValid());
    COMMS_ASSERT(m_recvTimer.isValid());
    COMMS_ASSERT(m_respTimer.isValid());

    restartPingTimer();
}    

void KeepAliveOp::messageSent()
{
    restartPingTimer();
}

void KeepAliveOp::handle([[maybe_unused]] PingrespMsg& msg)
{
    m_respTimer.cancel();
    COMMS_ASSERT(!m_respTimer.isActive());
    restartRecvTimer();
}

void KeepAliveOp::handle(DisconnectMsg& msg)
{
    m_pingTimer.cancel();
    m_recvTimer.cancel();  
    m_respTimer.cancel();

    if (!msg.doValid()) {
        // Required by the [MQTT-3.14.1-1] clause of the spec
        sendDisconnectWithReason(client(), DisconnectReason::MalformedPacket);
        client().brokerDisconnected(true, CC_Mqtt5AsyncOpStatus_BrokerDisconnected);
        return;
    }

    auto info = CC_Mqtt5DisconnectInfo();
    
    if (msg.field_reasonCode().doesExist()) {
        comms::cast_assign(info.m_reasonCode) = msg.field_reasonCode().field().value();
    }

    if (msg.field_properties().doesExist()) {
        PropsHandler propsHandler;
        for (auto& p : msg.field_properties().field().value()) {
            p.currentFieldExec(propsHandler);
        } 

        if (propsHandler.m_reasonStr != nullptr) {
            info.m_reasonStr = propsHandler.m_reasonStr->field_value().value().c_str();
        }     

        if (propsHandler.m_serverRef != nullptr) {
            info.m_serverRef = propsHandler.m_serverRef->field_value().value().c_str();
        }

        if constexpr (Config::HasUserProps) {
            if (!propsHandler.m_userProps.empty()) {
                UserPropsList userProps;
                fillUserProps(propsHandler, userProps);
                info.m_userProps = &userProps[0];
                comms::cast_assign(info.m_userPropsCount) = userProps.size();
            }
        }        
    }

    client().brokerDisconnected(true, CC_Mqtt5AsyncOpStatus_BrokerDisconnected, &info);
    // No members access after this point, the op will be deleted    
}

void KeepAliveOp::handle([[maybe_unused]] ProtMessage& msg)
{
    restartRecvTimer();
}

Op::Type KeepAliveOp::typeImpl() const
{
    return Type_KeepAlive;
}

void KeepAliveOp::restartPingTimer()
{
    auto& state = client().sessionState();
    if (state.m_keepAliveMs == 0U) {
        return;
    }

    m_pingTimer.wait(state.m_keepAliveMs, &KeepAliveOp::sendPingCb, this);
}

void KeepAliveOp::restartRecvTimer()
{
    auto& state = client().sessionState();
    if (state.m_keepAliveMs == 0U) {
        return;
    }

    m_recvTimer.wait(state.m_keepAliveMs, &KeepAliveOp::recvTimeoutCb, this);
}

void KeepAliveOp::sendPing()
{
    if (m_respTimer.isActive()) {
        return; // Ping has already been sent, waiting for response
    }

    PingreqMsg msg;
    client().sendMessage(msg);
    auto& state = client().configState();
    m_respTimer.wait(state.m_responseTimeoutMs, &KeepAliveOp::pingTimeoutCb, this);
}

void KeepAliveOp::pingTimeoutInternal()
{
    errorLog("The broker did not respond to PING");
    COMMS_ASSERT(!m_respTimer.isActive());
    sendDisconnectWithReason(DisconnectReason::KeepAliveTimeout);
    client().brokerDisconnected(true);
}

void KeepAliveOp::sendPingCb(void* data)
{
    asKeepAliveOp(data)->sendPing();
}

void KeepAliveOp::recvTimeoutCb(void* data)
{
    asKeepAliveOp(data)->sendPing();
}

void KeepAliveOp::pingTimeoutCb(void* data)
{
    asKeepAliveOp(data)->pingTimeoutInternal();
}

} // namespace op

} // namespace cc_mqtt5_client
