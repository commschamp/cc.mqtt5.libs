//
// Copyright 2023 - 2025 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "op/Op.h"
#include "ExtConfig.h"
#include "ProtocolDefs.h"
#include "TopicAliasDefs.h"

#include "TimerMgr.h"

namespace cc_mqtt5_client
{

namespace op
{

class RecvOp final : public Op
{
    using Base = Op;
public:
    explicit RecvOp(ClientImpl& client);

    using Base::handle;
    void handle(PublishMsg& msg) override;

#if CC_MQTT5_CLIENT_MAX_QOS >= 2
    void handle(PubrelMsg& msg) override;
#endif // #if CC_MQTT5_CLIENT_MAX_QOS >= 2    

    unsigned packetId() const
    {
        return m_packetId;
    }

    void resetTimer();
    void postReconnectionResume();

protected:
    virtual Type typeImpl() const override;    
    virtual void connectivityChangedImpl() override;

private:
    using SubIdsStorage = ObjListType<unsigned, Config::SubIdsLimit, Config::HasSubIds>;

    void restartResponseTimer();
    void responseTimeoutInternal();

    static void recvTimeoutCb(void* data);

    TimerMgr::Timer m_responseTimer;  
    unsigned m_packetId = 0U;

    static_assert(ExtConfig::RecvOpTimers == 1U);
};

} // namespace op


} // namespace cc_mqtt5_client
