//
// Copyright 2026 - 2026 (C). Alex Robenko. All rights reserved.
//
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "ProgramOptions.h"

namespace cc_mqtt5_client_app
{

class PubStressProgramOptions : public ProgramOptions
{
    using Base = ProgramOptions;

public:
    PubStressProgramOptions();

    unsigned threadsCount() const;
    unsigned clientCount() const;
    unsigned wait() const;
};

} // namespace cc_mqtt5_client_app