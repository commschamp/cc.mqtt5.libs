//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "State.h"
#include "TimerMgr.h"

#include "cc_mqtt5_client/common.h"

namespace cc_mqtt5_client
{

class Client
{
public:

    CC_Mqtt5ErrorCode init();
    void tick(unsigned ms);

    void setNextTickProgramCallback(CC_Mqtt5NextTickProgramFn cb, void* data)
    {
        if (cb != nullptr) {
            m_nextTickProgramFn = cb;
            m_nextTickProgramData = data;
        }
    }

    void setCancelNextTickWaitCallback(CC_Mqtt5CancelNextTickWaitFn cb, void* data)
    {
        if (cb != nullptr) {
            m_cancelNextTickWaitFn = cb;
            m_cancelNextTickWaitData = data;
        }
    }

    void setSendOutputDataCallback(CC_Mqtt5SendOutputDataFn cb, void* data)
    {
        if (cb != nullptr) {
            m_sendOutputDataFn = cb;
            m_sendOutputDataData = data;
        }
    }
    
private:

    class ApiEnterGuard
    {
    public:
        ApiEnterGuard(Client& client) : m_client(client)
        {
            m_client.doApiEnter();
        }

        ~ApiEnterGuard() noexcept
        {
            m_client.doApiExit();
        }

    private:
        Client& m_client;
    };

    friend class ApiEnterGuard;

    ApiEnterGuard apiEnter()
    {
        return ApiEnterGuard(*this);
    }

    void doApiEnter();
    void doApiExit();

    CC_Mqtt5NextTickProgramFn m_nextTickProgramFn = nullptr;
    void* m_nextTickProgramData = nullptr;

    CC_Mqtt5CancelNextTickWaitFn m_cancelNextTickWaitFn = nullptr;
    void* m_cancelNextTickWaitData = nullptr;

    CC_Mqtt5SendOutputDataFn m_sendOutputDataFn = nullptr;
    void* m_sendOutputDataData = nullptr;

    State m_state;
    TimerMgr m_timerMgr;
    unsigned m_apiEnterCount = 0U;
};

} // namespace cc_mqtt5_client
