//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Config.h"

#include <vector>

namespace cc_mqtt5_client
{

struct ExtConfig : public Config
{
    static constexpr unsigned ConnectOpsLimit = HasDynMemAlloc ? 0 : 1U;
    static constexpr unsigned KeepAliveOpsLimit = HasDynMemAlloc ? 0 : 1U;
    static constexpr unsigned ConnectOpTimers = 1U;
    static constexpr unsigned KeepAliveOpTimers = 3U;
    static constexpr unsigned TimersLimit = // TODO: complete
        (ConnectOpsLimit * ConnectOpTimers) + 
        (KeepAliveOpsLimit * KeepAliveOpTimers);

    static const unsigned OpsLimit = // TODO: complete
        ConnectOpsLimit + 
        KeepAliveOpsLimit;

    static_assert(HasDynMemAlloc || (TimersLimit > 0U));
    static_assert(HasDynMemAlloc || (ConnectOpsLimit > 0U));
    static_assert(HasDynMemAlloc || (KeepAliveOpsLimit > 0U));
    static_assert(HasDynMemAlloc || (OpsLimit > 0U));
};

} // namespace cc_mqtt5_client
