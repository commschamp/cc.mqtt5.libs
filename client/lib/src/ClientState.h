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

struct ClientState
{
    using PacketIdsList = ObjListType<std::uint16_t, ExtConfig::PacketIdsLimit>;

    static constexpr unsigned DefaultKeepAlive = 60;
    static constexpr unsigned DefaultTopicAliasMax = 10;

    SendTopicsMap m_sendTopicAliases;
    PacketIdsList m_allocatedPacketIds;
    std::uint16_t m_lastPacketId = 0U;
    unsigned m_inFlightSends = 0U;    
    
    bool m_firstConnect = true;
    bool m_networkDisconnected = false;
};

} // namespace cc_mqtt5_client
