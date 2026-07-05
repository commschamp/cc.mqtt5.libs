//
// Copyright 2023 - 2026 (C). Alex Robenko. All rights reserved.
//
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Sublic
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "SubProgramOptions.h"

namespace cc_mqtt5_client_app
{

SubProgramOptions::SubProgramOptions()
{
    addCommon();
    addNetwork();
    addTls();
    addConnect();
    addSubscribe();
}

} // namespace cc_mqtt5_client_app