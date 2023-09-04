//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "ExtConfig.h"
#include "ObjAllocator.h"
#include "ObjListType.h"
#include "ProtocolDefs.h"
#include "State.h"
#include "TimerMgr.h"

#include "op/ConnectOp.h"
#include "op/Op.h"

#include "cc_mqtt5_client/common.h"

namespace cc_mqtt5_client
{

class Client final : public ProtMsgHandler
{
    using Base = ProtMsgHandler;

public:
    CC_Mqtt5ErrorCode init();
    void tick(unsigned ms);
    unsigned processData(const std::uint8_t* iter, unsigned len);

    CC_Mqtt5ConnectHandle connectPrepare(CC_Mqtt5ErrorCode* ec);

    void setNextTickProgramCallback(CC_Mqtt5NextTickProgramCb cb, void* data)
    {
        if (cb != nullptr) {
            m_nextTickProgramCb = cb;
            m_nextTickProgramData = data;
        }
    }

    void setCancelNextTickWaitCallback(CC_Mqtt5CancelNextTickWaitCb cb, void* data)
    {
        if (cb != nullptr) {
            m_cancelNextTickWaitCb = cb;
            m_cancelNextTickWaitData = data;
        }
    }

    void setSendOutputDataCallback(CC_Mqtt5SendOutputDataCb cb, void* data)
    {
        if (cb != nullptr) {
            m_sendOutputDataCb = cb;
            m_sendOutputDataData = data;
        }
    }

    void setBrokerDisconnectReportCallback(CC_Mqtt5BrokerDisconnectReportCb cb, void* data)
    {
        if (cb != nullptr) {
            m_brokerDisconnectReportCb = cb;
            m_brokerDisconnectReportData = data;
        }
    }

    using Base::handle;
    void handle(ProtMessage& msg);

    void sendMessage(const ProtMessage& msg);
    void opComplete(const op::Op* op);
    
private:
    using ConnectOpAlloc = ObjAllocator<op::ConnectOp, ExtConfig::ConnectOpsLimit>;
    using ConnectOpsList = ObjListType<ConnectOpAlloc::Ptr, ExtConfig::ConnectOpsLimit>;
    using OpPtrsList = ObjListType<op::Op*, ExtConfig::OpsLimit>;

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

    void opComplete_Connect(const op::Op* op);

    CC_Mqtt5NextTickProgramCb m_nextTickProgramCb = nullptr;
    void* m_nextTickProgramData = nullptr;

    CC_Mqtt5CancelNextTickWaitCb m_cancelNextTickWaitCb = nullptr;
    void* m_cancelNextTickWaitData = nullptr;

    CC_Mqtt5SendOutputDataCb m_sendOutputDataCb = nullptr;
    void* m_sendOutputDataData = nullptr;

    CC_Mqtt5BrokerDisconnectReportCb m_brokerDisconnectReportCb = nullptr;
    void* m_brokerDisconnectReportData = nullptr;

    State m_state;
    TimerMgr m_timerMgr;
    unsigned m_apiEnterCount = 0U;

    ConnectOpAlloc m_connectOpAlloc;
    ConnectOpsList m_connectOps;

    OpPtrsList m_ops;

    ProtFrame m_frame;
};

} // namespace cc_mqtt5_client
