//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Client.h"

#include "comms/Assert.h"

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
    // TODO: more callbacks
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

    // TODO: abort pending ops
    m_state = State();
    m_state.m_initialized = true;
    return CC_Mqtt5ErrorCode_Success;
}

void Client::tick(unsigned ms)
{
    ++m_apiEnterCount;
    m_timerMgr.tick(ms);
    doApiExit();
}

unsigned Client::processData(const std::uint8_t* iter, unsigned len)
{
    auto guard = apiEnter();

    std::size_t consumed = 0;
    while (true) {
        auto iterTmp = iter;
        ProtMsgPtr msg;
        auto es = m_frame.read(msg, iterTmp, len - consumed);
        if (es == comms::ErrorStatus::NotEnoughData) {
            break;
        }

        if (es == comms::ErrorStatus::ProtocolError) {
            ++iter;
            continue;
        }

        if (es == comms::ErrorStatus::Success) {
            COMMS_ASSERT(msg);
            //m_lastRecvMsgTimestamp = m_timestamp;
            msg->dispatch(*this);
        }

        consumed += static_cast<std::size_t>(std::distance(iter, iterTmp));
        iter = iterTmp;
    }

    return static_cast<unsigned>(consumed);    
}

CC_Mqtt5ConnectHandle Client::connectPrepare(CC_Mqtt5ErrorCode* ec)
{
    auto updateEc = 
        [&ec](CC_Mqtt5ErrorCode val)
        {
            if (ec != nullptr) {
                *ec = val;
            }
        };


    auto handle = CC_Mqtt5ConnectHandle();
    COMMS_ASSERT(handle.m_ptr == nullptr);
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

        auto ptr = m_connectOpAlloc.alloc(*this);
        if (!ptr) {
            updateEc(CC_Mqtt5ErrorCode_OutOfMemory);
            break;
        }

        // TODO
        
        m_ops.push_back(ptr.get());
        m_connectOps.push_back(std::move(ptr));
        handle.m_ptr = m_ops.back();
        updateEc(CC_Mqtt5ErrorCode_Success);
    } while (false);

    return handle;
}

void Client::handle(ProtMessage& msg)
{
    for (auto* op : m_ops) {
        msg.dispatch(*op);
    }
}

void Client::sendMessage(const ProtMessage& msg)
{
    // TODO: send
    static_cast<void>(msg);
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

void Client::doApiEnter()
{
    ++m_apiEnterCount;
    if ((m_apiEnterCount > 1U) || (m_cancelNextTickWaitCb == nullptr)) {
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

void Client::opComplete_Connect(const op::Op* op)
{
    eraseFromList(op, m_connectOps);
}

} // namespace cc_mqtt5_client
