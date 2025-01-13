//
// Copyright 2023 - 2025 (C). Alex Robenko. All rights reserved.
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

using TopicStr = PublishMsg::Field_topic::ValueType;
using RecvTopicsMap = ObjListType<TopicStr, Config::TopicAliasesLimit, Config::HasTopicAliases>;

struct TopicAliasInfo
{
    static constexpr std::uint8_t DefaultLowQosRegRemCount = 3;

    TopicStr m_topic;
    unsigned m_alias = 0U;
    std::uint8_t m_lowQosRegRemCount = DefaultLowQosRegRemCount;
};

using SendTopicsMap = ObjListType<TopicAliasInfo, Config::TopicAliasesLimit, Config::HasTopicAliases>;
using SendTopicsFreeAliasList = ObjListType<unsigned, Config::TopicAliasesLimit, Config::HasTopicAliases>;

} // namespace cc_mqtt5_client
