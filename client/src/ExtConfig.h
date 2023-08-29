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
    static constexpr unsigned TimersLimit = 0U; // TODO:

    static_assert(HasDynMemAlloc || (TimersLimit > 0U), "Must calculate timers limit");
};

} // namespace cc_mqtt5_client