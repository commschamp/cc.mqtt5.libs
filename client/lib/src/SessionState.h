//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "ExtConfig.h"
#include "ObjListType.h"
#include "ProtocolDefs.h"
#include "TopicAliasDefs.h"

#include "cc_mqtt5_client/common.h"

#include <cstdint>

namespace cc_mqtt5_client
{

struct SessionState
{
    using AuthMethodStorageType = ConnectMsg::Field_properties::ValueType::value_type::Field_authMethod::Field_value::ValueType;

    static constexpr unsigned DefaultKeepAlive = 60;
    static constexpr unsigned DefaultTopicAliasMax = 10;

    RecvTopicsMap m_recvTopicAliases;
    SendTopicsMap m_sendTopicAliases;
    SendTopicsFreeAliasList m_sendTopicFreeAliases;
    AuthMethodStorageType m_authMethod;
    std::uint64_t m_sessionExpiryIntervalMs = 0U;
    unsigned m_connectSessionExpiryInterval = 0U;
    unsigned m_keepAliveMs = 0U;
    unsigned m_highQosSendLimit = 0U;
    unsigned m_highQosRecvLimit = 0U;
    unsigned m_maxRecvTopicAlias = 0U;
    unsigned m_maxSendTopicAlias = 0U;
    unsigned m_maxRecvPacketSize = 0U;
    unsigned m_maxSendPacketSize = 0U;
    CC_Mqtt5QoS m_pubMaxQos = CC_Mqtt5QoS_ExactlyOnceDelivery;
    bool m_problemInfoAllowed = false;
    bool m_initialized = false;
    bool m_connected = false;
    bool m_disconnecting = false;
    bool m_wildcardSubAvailable = false;
    bool m_subIdsAvailable = false;
    bool m_retainAvailable = false;
    bool m_sharedSubsAvailable = false;
};

} // namespace cc_mqtt5_client
