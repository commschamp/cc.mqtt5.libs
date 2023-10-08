//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Config.h"
#include "ObjListType.h"
#include "ProtocolDefs.h"


namespace cc_mqtt5_client
{

using TopicFilterStr = SubscribeMsg::Field_list::ValueType::value_type::Field_topic::ValueType;
using SubFiltersMap = ObjListType<TopicFilterStr, Config::SubFiltersLimit, Config::HasSubTopicVerification>;


} // namespace cc_mqtt5_client
