//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ClientImpl.h"

#include "comms/Assert.h"
#include "comms/process.h"

#include <algorithm>
#include <type_traits>

namespace cc_mqtt5_client
{

namespace 
{

template <typename TList>
void eraseFromList(const op::Op* op, TList& list)
{
    auto iter = 
        std::find_if(
            list.begin(), list.end(),
            [op](auto& opPtr)
            {
                return op == opPtr.get();
            });

    COMMS_ASSERT(iter != list.end());
    if (iter == list.end()) {
        return;
    }

    list.erase(iter);
}

void updateEc(CC_Mqtt5ErrorCode* ec, CC_Mqtt5ErrorCode val)
{
    if (ec != nullptr) {
        *ec = val;
    }
}

} // namespace 
    

CC_Mqtt5ErrorCode ClientImpl::init()
{
    auto guard = apiEnter();
    if ((m_sendOutputDataCb == nullptr) ||
        (m_brokerDisconnectReportCb == nullptr) ||
        (m_messageReceivedReportCb == nullptr)) {
        return CC_Mqtt5ErrorCode_BadParam;
    }

    bool hasTimerCallbacks = 
        (m_nextTickProgramCb != nullptr) ||
        (m_cancelNextTickWaitCb != nullptr);

    if (hasTimerCallbacks) {
        bool hasAllTimerCallbacks = 
            (m_nextTickProgramCb != nullptr) &&
            (m_cancelNextTickWaitCb != nullptr);

        if (!hasAllTimerCallbacks) {
            return CC_Mqtt5ErrorCode_BadParam;
        }
    }

    terminateAllOps(CC_Mqtt5AsyncOpStatus_Aborted);
    m_state = State();
    m_state.m_initialized = true;
    return CC_Mqtt5ErrorCode_Success;
}

void ClientImpl::tick(unsigned ms)
{
    COMMS_ASSERT(m_apiEnterCount == 0U);
    ++m_apiEnterCount;
    m_timerMgr.tick(ms);
    doApiExit();
}

unsigned ClientImpl::processData(const std::uint8_t* iter, unsigned len)
{
    auto guard = apiEnter();
    return static_cast<unsigned>(comms::processAllWithDispatch(iter, len, m_frame, *this));
}

op::ConnectOp* ClientImpl::connectPrepare(CC_Mqtt5ErrorCode* ec)
{
    op::ConnectOp* connectOp = nullptr;
    do {
        if (!m_connectOps.empty()) {
            // Already allocated
            updateEc(ec, CC_Mqtt5ErrorCode_Busy);
            break;
        }

        if (!m_state.m_initialized) {
            updateEc(ec, CC_Mqtt5ErrorCode_NotIntitialized);
            break;
        }

        if (!m_connectOps.empty()) {
            updateEc(ec, CC_Mqtt5ErrorCode_Busy);
            break;
        }

        auto ptr = m_connectOpAlloc.alloc(*this);
        if (!ptr) {
            updateEc(ec, CC_Mqtt5ErrorCode_OutOfMemory);
            break;
        }

        m_ops.push_back(ptr.get());
        m_connectOps.push_back(std::move(ptr));
        connectOp = m_connectOps.back().get();
        updateEc(ec, CC_Mqtt5ErrorCode_Success);
    } while (false);

    return connectOp;
}

op::DisconnectOp* ClientImpl::disconnectPrepare(CC_Mqtt5ErrorCode* ec)
{
    op::DisconnectOp* disconnectOp = nullptr;
    do {
        if (!m_state.m_initialized) {
            updateEc(ec, CC_Mqtt5ErrorCode_NotIntitialized);
            break;
        }

        if (!m_state.m_connected) {
            updateEc(ec, CC_Mqtt5ErrorCode_NotConnected);
            break;
        }

        if (!m_disconnectOps.empty()) {
            updateEc(ec, CC_Mqtt5ErrorCode_Busy);
            break;
        }        

        auto ptr = m_disconnectOpsAlloc.alloc(*this);
        if (!ptr) {
            updateEc(ec, CC_Mqtt5ErrorCode_OutOfMemory);
            break;
        }

        m_ops.push_back(ptr.get());
        m_disconnectOps.push_back(std::move(ptr));
        disconnectOp = m_disconnectOps.back().get();
        updateEc(ec, CC_Mqtt5ErrorCode_Success);
    } while (false);

    return disconnectOp;
}

op::SubscribeOp* ClientImpl::subscribePrepare(CC_Mqtt5ErrorCode* ec)
{
    op::SubscribeOp* subOp = nullptr;
    do {
        if (!m_state.m_initialized) {
            updateEc(ec, CC_Mqtt5ErrorCode_NotIntitialized);
            break;
        }

        if (!m_state.m_connected) {
            updateEc(ec, CC_Mqtt5ErrorCode_NotConnected);
            break;
        }

        auto ptr = m_subscribeOpsAlloc.alloc(*this);
        if (!ptr) {
            updateEc(ec, CC_Mqtt5ErrorCode_OutOfMemory);
            break;
        }

        m_ops.push_back(ptr.get());
        m_subscribeOps.push_back(std::move(ptr));
        subOp = m_subscribeOps.back().get();
        updateEc(ec, CC_Mqtt5ErrorCode_Success);
    } while (false);

    return subOp;
}

op::UnsubscribeOp* ClientImpl::unsubscribePrepare(CC_Mqtt5ErrorCode* ec)
{
    op::UnsubscribeOp* unsubOp = nullptr;
    do {
        if (!m_state.m_initialized) {
            updateEc(ec, CC_Mqtt5ErrorCode_NotIntitialized);
            break;
        }

        if (!m_state.m_connected) {
            updateEc(ec, CC_Mqtt5ErrorCode_NotConnected);
            break;
        }

        auto ptr = m_unsubscribeOpsAlloc.alloc(*this);
        if (!ptr) {
            updateEc(ec, CC_Mqtt5ErrorCode_OutOfMemory);
            break;
        }

        m_ops.push_back(ptr.get());
        m_unsubscribeOps.push_back(std::move(ptr));
        unsubOp = m_unsubscribeOps.back().get();
        updateEc(ec, CC_Mqtt5ErrorCode_Success);
    } while (false);

    return unsubOp;
}

void ClientImpl::handle(PublishMsg& msg)
{
    if (m_state.m_terminating) {
        return;
    }

    do {

        auto createRecvOp = 
            [this]()
            {
                auto ptr = m_recvOpsAlloc.alloc(*this);
                if (!ptr) {
                    // TODO: send report exceeding number of available receives
                    return; 
                }

                m_ops.push_back(ptr.get());
                m_recvOps.push_back(std::move(ptr));
            };

        using Qos = PublishMsg::TransportField_flags::Field_qos::ValueType;
        if ((msg.transportField_flags().field_qos().value() == Qos::AtMostOnceDelivery) || 
            (msg.transportField_flags().field_qos().value() == Qos::AtLeastOnceDelivery)) {
            createRecvOp();
            break;
        }

        // TODO: check packet id;

        // TODO: consider duplicate

        // Duplicate, check
    } while (false);

    // TODO: publish op    

    handle(static_cast<ProtMessage&>(msg));
}

void ClientImpl::handle(ProtMessage& msg)
{
    if (m_state.m_terminating) {
        return;
    }

    for (auto* op : m_ops) {
        msg.dispatch(*op);

        // After message dispatching the whole session may be in terminating state
        // Don't continue iteration
        if (m_state.m_terminating) {
            break;
        }    
    }
}

CC_Mqtt5ErrorCode ClientImpl::sendMessage(const ProtMessage& msg)
{
    auto len = m_frame.length(msg);
    if (m_buf.max_size() < len) {
        return CC_Mqtt5ErrorCode_BufferOverflow;
    }

    m_buf.resize(len);
    auto writeIter = comms::writeIteratorFor<ProtMessage>(&m_buf[0]);
    auto es = m_frame.write(msg, writeIter, len);
    COMMS_ASSERT(es == comms::ErrorStatus::Success);
    if (es != comms::ErrorStatus::Success) {
        return CC_Mqtt5ErrorCode_InternalError;
    }

    m_sendOutputDataCb(m_sendOutputDataData, &m_buf[0], static_cast<unsigned>(len));

    for (auto& opPtr : m_keepAliveOps) {
        opPtr->messageSent();
    }

    return CC_Mqtt5ErrorCode_Success;
}

void ClientImpl::opComplete(const op::Op* op)
{
    auto iter = std::find(m_ops.begin(), m_ops.end(), op);
    COMMS_ASSERT(iter != m_ops.end());
    if (iter == m_ops.end()) {
        return;
    }

    *iter = nullptr;
    m_opsDeleted = true;

    using ExtraCompleteFunc = void (ClientImpl::*)(const op::Op*);
    static const ExtraCompleteFunc Map[] = {
        /* Type_Connect */ &ClientImpl::opComplete_Connect,
        /* Type_KeepAlive */ &ClientImpl::opComplete_KeepAlive,
        /* Type_Disconnect */ &ClientImpl::opComplete_Disconnect,
        /* Type_Subscribe */ &ClientImpl::opComplete_Subscribe,
        /* Type_Unsubscribe */ &ClientImpl::opComplete_Unsubscribe,
        /* Type_Recv */ &ClientImpl::opComplete_Recv,
    };
    static const std::size_t MapSize = std::extent<decltype(Map)>::value;
    static_assert(MapSize == op::Op::Type_NumOfValues);

    auto idx = static_cast<unsigned>(op->type());
    COMMS_ASSERT(idx < MapSize);
    if (MapSize <= idx) {
        return;
    }

    auto func = Map[idx];
    (this->*func)(op);
}

void ClientImpl::doApiGuard()
{
    auto guard = apiEnter();
}

void ClientImpl::notifyConnected()
{
    m_state.m_connected = true;
    createKeepAliveOpIfNeeded();
}

void ClientImpl::notifyDisconnected(bool reportDisconnection, const CC_Mqtt5DisconnectInfo* info)
{
    COMMS_ASSERT(reportDisconnection || (info == nullptr));
    m_state.m_initialized = false; // Require re-initialization
    m_state.m_connected = false;
    m_state.m_terminating = true;

    terminateAllOps(CC_Mqtt5AsyncOpStatus_BrokerDisconnected);

    if (reportDisconnection) {
        COMMS_ASSERT(m_brokerDisconnectReportCb != nullptr);
        m_brokerDisconnectReportCb(m_brokerDisconnectReportData, info);
    }
}

void ClientImpl::reportMsgInfo(const CC_Mqtt5MessageInfo& info)
{
    COMMS_ASSERT(m_messageReceivedReportCb != nullptr);
    m_messageReceivedReportCb(m_messageReceivedReportData, &info);
}

void ClientImpl::doApiEnter()
{
    ++m_apiEnterCount;
    if ((m_apiEnterCount > 1U) || (m_cancelNextTickWaitCb == nullptr)) {
        return;
    }

    auto prevWait = m_timerMgr.getMinWait();
    if (prevWait == 0U) {
        return;
    }

    auto elapsed = m_cancelNextTickWaitCb(m_cancelNextTickWaitData);
    m_timerMgr.tick(elapsed);
}

void ClientImpl::doApiExit()
{
    COMMS_ASSERT(m_apiEnterCount > 0U);
    --m_apiEnterCount;
    if (m_apiEnterCount > 0U) {
        return;
    }

    cleanOps();

    if (m_nextTickProgramCb == nullptr) {
        return;
    }

    auto nextWait = m_timerMgr.getMinWait();
    if (nextWait == 0U) {
        return;
    }

    m_nextTickProgramCb(m_nextTickProgramData, nextWait);
}

void ClientImpl::createKeepAliveOpIfNeeded()
{
    if (!m_keepAliveOps.empty()) {
        return;
    }

    auto ptr = m_keepAliveOpsAlloc.alloc(*this);
    if (!ptr) {
        COMMS_ASSERT(false); // Should not happen
        return;
    }    

    m_ops.push_back(ptr.get());
    m_keepAliveOps.push_back(std::move(ptr));
}

void ClientImpl::terminateAllOps(CC_Mqtt5AsyncOpStatus status)
{
    for (auto* op : m_ops) {
        if (op != nullptr) {
            op->terminateOp(status);
        }
    }
}

void ClientImpl::cleanOps()
{
    if (!m_opsDeleted) {
        return;
    }

    m_ops.erase(
        std::remove_if(
            m_ops.begin(), m_ops.end(),
            [](auto* op)
            {
                return op == nullptr;
            }),
        m_ops.end());
}

void ClientImpl::opComplete_Connect(const op::Op* op)
{
    eraseFromList(op, m_connectOps);
}

void ClientImpl::opComplete_KeepAlive(const op::Op* op)
{
    eraseFromList(op, m_keepAliveOps);
}

void ClientImpl::opComplete_Disconnect(const op::Op* op)
{
    eraseFromList(op, m_disconnectOps);
}

void ClientImpl::opComplete_Subscribe(const op::Op* op)
{
    eraseFromList(op, m_subscribeOps);
}

void ClientImpl::opComplete_Unsubscribe(const op::Op* op)
{
    eraseFromList(op, m_unsubscribeOps);
}

void ClientImpl::opComplete_Recv(const op::Op* op)
{
    eraseFromList(op, m_recvOps);
}



} // namespace cc_mqtt5_client
