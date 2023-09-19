//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Client.h"

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

} // namespace 
    

CC_Mqtt5ErrorCode Client::init()
{
    if ((m_sendOutputDataCb == nullptr) ||
        (m_brokerDisconnectReportCb == nullptr)) {
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

void Client::tick(unsigned ms)
{
    COMMS_ASSERT(m_apiEnterCount == 0U);
    ++m_apiEnterCount;
    m_timerMgr.tick(ms);
    doApiExit();
}

unsigned Client::processData(const std::uint8_t* iter, unsigned len)
{
    auto guard = apiEnter();
    return static_cast<unsigned>(comms::processAllWithDispatch(iter, len, m_frame, *this));
}

op::ConnectOp* Client::connectPrepare(CC_Mqtt5ErrorCode* ec)
{
    auto updateEc = 
        [&ec](CC_Mqtt5ErrorCode val)
        {
            if (ec != nullptr) {
                *ec = val;
            }
        };

    op::ConnectOp* connectOp = nullptr;
    do {
        if (!m_connectOps.empty()) {
            // Already allocated
            updateEc(CC_Mqtt5ErrorCode_Busy);
            break;
        }

        if (!m_state.m_initialized) {
            updateEc(CC_Mqtt5ErrorCode_NotIntitialized);
            break;
        }

        if (!m_connectOps.empty()) {
            updateEc(CC_Mqtt5ErrorCode_Busy);
            break;
        }

        auto ptr = m_connectOpAlloc.alloc(*this);
        if (!ptr) {
            updateEc(CC_Mqtt5ErrorCode_OutOfMemory);
            break;
        }

        m_ops.push_back(ptr.get());
        m_connectOps.push_back(std::move(ptr));
        connectOp = m_connectOps.back().get();
        updateEc(CC_Mqtt5ErrorCode_Success);
    } while (false);

    return connectOp;
}

op::DisconnectOp* Client::disconnectPrepare(CC_Mqtt5ErrorCode* ec)
{
    auto updateEc = 
        [&ec](CC_Mqtt5ErrorCode val)
        {
            if (ec != nullptr) {
                *ec = val;
            }
        };

    op::DisconnectOp* disconnectOp = nullptr;
    do {
        if (!m_state.m_initialized) {
            updateEc(CC_Mqtt5ErrorCode_NotIntitialized);
            break;
        }

        if (!m_state.m_connected) {
            updateEc(CC_Mqtt5ErrorCode_NotConnected);
            break;
        }

        if (!m_disconnectOps.empty()) {
            updateEc(CC_Mqtt5ErrorCode_Busy);
            break;
        }        

        auto ptr = m_disconnectOpsAlloc.alloc(*this);
        if (!ptr) {
            updateEc(CC_Mqtt5ErrorCode_OutOfMemory);
            break;
        }

        m_ops.push_back(ptr.get());
        m_disconnectOps.push_back(std::move(ptr));
        disconnectOp = m_disconnectOps.back().get();
        updateEc(CC_Mqtt5ErrorCode_Success);
    } while (false);

    return disconnectOp;
}

void Client::handle(ProtMessage& msg)
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

CC_Mqtt5ErrorCode Client::sendMessage(const ProtMessage& msg)
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

void Client::opComplete(const op::Op* op)
{
    auto iter = std::find(m_ops.begin(), m_ops.end(), op);
    COMMS_ASSERT(iter != m_ops.end());
    if (iter == m_ops.end()) {
        return;
    }

    m_ops.erase(iter);

    using ExtraCompleteFunc = void (Client::*)(const op::Op*);
    static const ExtraCompleteFunc Map[] = {
        /* Type_Connect */ &Client::opComplete_Connect,
        /* Type_KeepAlive */ &Client::opComplete_KeepAlive,
        /* Type_Disconnect */ &Client::opComplete_Disconnect,
    };
    static const std::size_t MapSize = std::extent<decltype(Map)>::value;
    static_assert(MapSize == op::Op::Type_NumOfValues);

    auto idx = static_cast<unsigned>(op->type());
    COMMS_ASSERT(MapSize <= idx);
    if (MapSize <= idx) {
        return;
    }

    auto func = Map[idx];
    (this->*func)(op);
}

void Client::doApiGuard()
{
    auto guard = apiEnter();
}

void Client::notifyConnected()
{
    m_state.m_connected = true;
    createKeepAliveOpIfNeeded();
}

void Client::notifyDisconnected(bool reportDisconnection, const CC_Mqtt5DisconnectInfo* info)
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

void Client::doApiEnter()
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

void Client::doApiExit()
{
    COMMS_ASSERT(m_apiEnterCount > 0U);
    --m_apiEnterCount;
    if ((m_apiEnterCount > 0U) || (m_nextTickProgramCb == nullptr)) {
        return;
    }

    auto nextWait = m_timerMgr.getMinWait();
    if (nextWait == 0U) {
        return;
    }

    m_nextTickProgramCb(m_nextTickProgramData, nextWait);
}

void Client::createKeepAliveOpIfNeeded()
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

void Client::terminateAllOps(CC_Mqtt5AsyncOpStatus status)
{
    while (!m_ops.empty()) {
        m_ops.front()->terminateOp(status);
        // The terminated op is expected to invoke opCompleted() and remove itself from the list
    }
}

void Client::opComplete_Connect(const op::Op* op)
{
    eraseFromList(op, m_connectOps);
}

void Client::opComplete_KeepAlive(const op::Op* op)
{
    eraseFromList(op, m_keepAliveOps);
}

void Client::opComplete_Disconnect(const op::Op* op)
{
    eraseFromList(op, m_disconnectOps);
}

} // namespace cc_mqtt5_client
