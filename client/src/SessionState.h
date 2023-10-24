//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "ExtConfig.h"
#include "ObjListType.h"
#include "TopicAliasDefs.h"

#include "cc_mqtt5_client/common.h"

#include <cstdint>

namespace cc_mqtt5_client
{

struct SessionState
{
    using PacketIdsList = ObjListType<std::uint16_t, ExtConfig::PacketIdsLimit>;

    static constexpr unsigned DefaultKeepAlive = 60;
    static constexpr unsigned DefaultTopicAliasMax = 10;

    RecvTopicsMap m_recvTopicAliases;
    SendTopicsMap m_sendTopicAliases;
    SendTopicsFreeAliasList m_sendTopicFreeAliases;
    std::uint64_t m_sessionExpiryIntervalMs = 0U;
    unsigned m_keepAliveMs = 0U;
    unsigned m_sendLimit = 0U;
    unsigned m_maxRecvTopicAlias = 0U;
    unsigned m_maxSendTopicAlias = 0U;
    unsigned m_maxPacketSize = 0U;
    PacketIdsList m_allocatedPacketIds;
    CC_Mqtt5QoS m_pubMaxQos = CC_Mqtt5QoS_ExactlyOnceDelivery;
    std::uint16_t m_lastPacketId = 0U;
    bool m_problemInfoAllowed = false;
    bool m_initialized = false;
    bool m_connected = false;
    bool m_terminating = false;
    bool m_subIdsAvailable = false;
    bool m_firstConnect = true;
    bool m_networkDisconnected = false;
};

} // namespace cc_mqtt5_client
