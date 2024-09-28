//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Config.h"
#include "ProtocolOptions.h"

#include "cc_mqtt5/Message.h"
#include "cc_mqtt5/Version.h"
#include "cc_mqtt5/frame/Frame.h"
#include "cc_mqtt5/input/AllMessages.h"
#include "cc_mqtt5/input/ClientInputMessages.h"

#include "comms/GenericHandler.h"

#include <cstdint>

static_assert(COMMS_MAKE_VERSION(2, 9, 0) <= CC_MQTT5_VERSION, 
    "The version of the cc_mqtt5 library is too low.");

namespace cc_mqtt5_client
{

class ProtMsgHandler;

using ProtMessage = cc_mqtt5::Message<
    comms::option::app::ReadIterator<const std::uint8_t*>,
    comms::option::app::WriteIterator<std::uint8_t*>,
    comms::option::app::LengthInfoInterface,
    comms::option::app::IdInfoInterface,
    comms::option::app::Handler<ProtMsgHandler>
>;

CC_MQTT5_ALIASES_FOR_ALL_MESSAGES(, Msg, ProtMessage, ProtocolOptions)

template <typename TBase, typename TOpt>
using Qos1ClientInputMessages =
    std::tuple<
        cc_mqtt5::message::Connack<TBase, TOpt>,
        cc_mqtt5::message::Publish<TBase, TOpt>,
        cc_mqtt5::message::Puback<TBase, TOpt>,
        cc_mqtt5::message::Suback<TBase, TOpt>,
        cc_mqtt5::message::Unsuback<TBase, TOpt>,
        cc_mqtt5::message::Pingresp<TBase, TOpt>,
        cc_mqtt5::message::Disconnect<TBase, TOpt>,
        cc_mqtt5::message::Auth<TBase, TOpt>
    >;

template <typename TBase, typename TOpt>
using Qos0ClientInputMessages =
    std::tuple<
        cc_mqtt5::message::Connack<TBase, TOpt>,
        cc_mqtt5::message::Publish<TBase, TOpt>,
        cc_mqtt5::message::Suback<TBase, TOpt>,
        cc_mqtt5::message::Unsuback<TBase, TOpt>,
        cc_mqtt5::message::Pingresp<TBase, TOpt>,
        cc_mqtt5::message::Disconnect<TBase, TOpt>,
        cc_mqtt5::message::Auth<TBase, TOpt>
    >; 

using ProtInputMessages = 
    std::conditional_t<
        2 <= Config::MaxQos,
        cc_mqtt5::input::ClientInputMessages<ProtMessage, ProtocolOptions>,
        std::conditional_t<
            1 == Config::MaxQos,
            Qos1ClientInputMessages<ProtMessage, ProtocolOptions>,
            Qos0ClientInputMessages<ProtMessage, ProtocolOptions>
        >
    >;

using ProtFrame = cc_mqtt5::frame::Frame<ProtMessage, ProtInputMessages, ProtocolOptions>;
using ProtMsgPtr = ProtFrame::MsgPtr;

class ProtMsgHandler : public comms::GenericHandler<ProtMessage, ProtInputMessages>
{
protected:
    ProtMsgHandler() = default;
    ~ProtMsgHandler() noexcept = default;
};

} // namespace cc_mqtt5_client
