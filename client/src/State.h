//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

namespace cc_mqtt5_client
{

struct State
{
    static constexpr unsigned DefaultOpTimeoutMs = 2000;

    unsigned m_opTimeoutMs = DefaultOpTimeoutMs;
    bool m_initialized = false;
    bool m_connected = false;
};

} // namespace cc_mqtt5_client
