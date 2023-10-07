//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

namespace cc_mqtt5_client
{

struct ConfigState
{
    static constexpr unsigned DefaultResponseTimeoutMs = 2000;
    unsigned m_responseTimeoutMs = DefaultResponseTimeoutMs;
    bool m_verifyOutgoingTopic = true;
    bool m_verifyIncomingTopic = true;
};

} // namespace cc_mqtt5_client