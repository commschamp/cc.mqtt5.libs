//
// Copyright 2023 - 2026 (C). Alex Robenko. All rights reserved.
//
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "PubProgramOptions.h"

namespace cc_mqtt5_client_app
{

PubProgramOptions::PubProgramOptions()
{
    addCommon();
    addNetwork();
    addTls();
    addConnect();
    addPublish();
}

} // namespace cc_mqtt5_client_app