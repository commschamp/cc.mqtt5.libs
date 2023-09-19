//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "ExtConfig.h"

#include "comms/util/StaticVector.h"
#include "comms/util/type_traits.h"

#include <limits>

namespace cc_mqtt5_client
{

class TimerMgr
{
public:
    using TimeoutCb = void (*)(void*);

    class Timer
    {
    public:
        bool isValid() const
        {
            return m_idx < InvalidIdx;
        }

        ~Timer()
        {
            if (isValid()) {
                m_timerMgr.freeTimer(m_idx);
            }
        }

        void wait(unsigned timeoutMs, TimeoutCb cb, void* data)
        {
            m_timerMgr.timerWait(m_idx, timeoutMs, cb, data);
        }

        void cancel()
        {
            m_timerMgr.timerCancel(m_idx);
        }

        bool isActive() const
        {
            return m_timerMgr.timerIsActive(m_idx);
        }

    private:
        Timer (TimerMgr& timerMgr, unsigned idx) :
            m_timerMgr(timerMgr),
            m_idx(idx)
        {
        }

        Timer (TimerMgr& timerMgr) :
            m_timerMgr(timerMgr)
        {
        }        

        TimerMgr& m_timerMgr;
        unsigned m_idx = InvalidIdx;

        friend class TimerMgr;

        static const unsigned InvalidIdx = std::numeric_limits<unsigned>::max();
    };

    Timer allocTimer();
    void tick(unsigned ms);
    unsigned getMinWait() const;

private:
    struct TimerInfo
    {
        unsigned m_timeoutMs = 0U;
        TimeoutCb m_timeoutCb = nullptr;
        void* m_timeoutData = nullptr;
        bool m_allocated = false;
    };

    template <typename ...>
    using DynMemoryStorage = std::vector<TimerInfo>;

    template <typename ...>
    using StaticStorage = comms::util::StaticVector<TimerInfo, ExtConfig::TimersLimit>;

    template <typename... TParams>
    using Storage = 
        typename comms::util::LazyShallowConditional<
            ExtConfig::TimersLimit == 0U
        >::template Type<
            DynMemoryStorage,
            StaticStorage
        >;

    using StorageType = Storage<>;

    friend class Timer;

    void freeTimer(unsigned idx);
    void timerWait(unsigned idx, unsigned timeoutMs, TimeoutCb cb, void* data);
    void timerCancel(unsigned idx);
    bool timerIsActive(unsigned idx) const;

    StorageType m_timers;
    unsigned m_allocatedTimers = 0U;
};

} // namespace cc_mqtt5_client
