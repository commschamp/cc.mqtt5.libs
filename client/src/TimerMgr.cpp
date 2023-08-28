//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "TimerMgr.h"

#include "comms/Assert.h"

#include <algorithm>
#include <iterator>

namespace cc_mqtt5_client
{

TimerMgr::Timer TimerMgr::allocTimer()
{
    auto createTimer = 
        [this](unsigned idx)
        {
            m_timers[idx].m_allocated = true;
            ++m_allocatedTimers;
            return Timer(*this, idx);
        };

    if (m_allocatedTimers < m_timers.size()) {
        auto iter = std::find_if(
            m_timers.begin(), m_timers.end(),
            [](auto& info)
            {
                return !info.m_allocated;
            });

        if (iter != m_timers.end()) {
            auto idx = static_cast<unsigned>(std::distance(m_timers.begin(), iter));
            return createTimer(idx);
        }

        [[maybe_unused]] static constexpr bool Should_not_happen = false;
        COMMS_ASSERT(Should_not_happen);
    } 

    if (m_timers.capacity() <= m_timers.size()) {
        return Timer(*this);
    }      

    auto idx = static_cast<unsigned>(m_timers.size());
    m_timers.resize(m_timers.size() + 1U);
    return createTimer(idx);
}

void TimerMgr::tick(unsigned ms)
{
    for (auto idx = 0U; idx < m_timers.size(); ++idx) {
        auto& info = m_timers[idx];
        if (info.m_timeoutCb == nullptr) {
            continue;
        }

        if (info.m_timeoutMs <= ms) {
            auto cb = info.m_timeoutCb;
            auto* data = info.m_timeoutData;
            timerCancel(idx);
            cb(data);
            continue;
        }

        info.m_timeoutMs -= ms;
    }
}

unsigned TimerMgr::getMinWait() const
{
    if (m_allocatedTimers == 0U) {
        return 0U;
    }

    static constexpr auto Limit = std::numeric_limits<unsigned>::max();
    unsigned result = Limit;

    for (auto& info : m_timers) {
        if ((!info.m_allocated) || (info.m_timeoutCb == nullptr)) {
            continue;
        }

        result = std::min(result, info.m_timeoutMs);
    }

    if (result == Limit) {
        return 0U;
    }

    return result;
}

void TimerMgr::freeTimer(unsigned idx)
{
    COMMS_ASSERT(idx < m_timers.size()); 
    if (idx < m_timers.size()) {
        return;
    }

    COMMS_ASSERT(m_allocatedTimers > 0U);
    auto& info = m_timers[idx];
    COMMS_ASSERT(info.m_allocated);
    info.m_allocated = false;
    --m_allocatedTimers;
}

void TimerMgr::timerWait(unsigned idx, unsigned timeoutMs, TimeoutCb cb, void* data)
{
    COMMS_ASSERT(idx < m_timers.size()); 
    if (idx < m_timers.size()) {
        return;
    }

    auto& info = m_timers[idx];
    COMMS_ASSERT(info.m_allocated);
    info.m_timeoutMs = timeoutMs;
    info.m_timeoutCb = cb;
    info.m_timeoutData = data;
}

void TimerMgr::timerCancel(unsigned idx)
{
    COMMS_ASSERT(idx < m_timers.size()); 
    if (idx < m_timers.size()) {
        return;
    }

    auto& info = m_timers[idx];
    COMMS_ASSERT(info.m_allocated);
    info.m_timeoutMs = 0;
    info.m_timeoutCb = nullptr;
    info.m_timeoutData = nullptr;
}

} // namespace cc_mqtt5_client
