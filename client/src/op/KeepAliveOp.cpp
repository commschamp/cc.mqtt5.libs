//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/KeepAliveOp.h"
#include "Client.h"

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

KeepAliveOp::KeepAliveOp(Client& client) : 
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
    auto& state = client().state();
    if (state.m_keepAliveMs == 0U) {
        return;
    }

    m_pingTimer.wait(state.m_keepAliveMs, &KeepAliveOp::sendPingCb, this);
}

void KeepAliveOp::restartRecvTimer()
{
    auto& state = client().state();
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
    auto& state = client().state();
    m_respTimer.wait(state.m_opTimeoutMs, &KeepAliveOp::pingTimeoutCb, this);
}

void KeepAliveOp::pingTimeoutInternal()
{
    COMMS_ASSERT(!m_respTimer.isActive());
    clent().notifyDisconnected();
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
