//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

namespace cc_mqtt5_client
{

struct SessionState
{
    static constexpr unsigned DefaultKeepAlive = 60;
    static constexpr unsigned DefaultTopicAliasMax = 10;

    unsigned m_keepAliveMs = 0U;
    unsigned m_sendLimit = 0U;
    unsigned m_packetId = 0U;
    unsigned m_maxRecvTopicAlias = 0U;
    unsigned m_maxSendTopicAlias = 0U;
    bool m_initialized = false;
    bool m_connected = false;
    bool m_terminating = false;
    bool m_subIdsAvailable = false;
    bool m_firstConnect = true;
};

} // namespace cc_mqtt5_client