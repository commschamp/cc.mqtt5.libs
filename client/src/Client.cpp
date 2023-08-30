//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Client.h"

#include "comms/Assert.h"

namespace cc_mqtt5_client
{

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
        
        m_connectOps.push_back(std::move(ptr));
        handle.m_ptr = m_connectOps.back().get();
        updateEc(CC_Mqtt5ErrorCode_Success);
    } while (false);

    return handle;
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

} // namespace cc_mqtt5_client
