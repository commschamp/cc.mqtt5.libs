#pragma once

#include "cc_mqtt5/Message.h"
#include "cc_mqtt5/frame/Frame.h"
#include "cc_mqtt5/input/AllMessages.h"
#include "comms/GenericHandler.h"

#include <cstdint>
#include <iterator>

class UnitTestMsgHandler;

using UnitTestMessage = cc_mqtt5::Message<
    comms::option::app::ReadIterator<const std::uint8_t*>,
    comms::option::app::WriteIterator<std::back_insert_iterator<std::vector<std::uint8_t> > >,
    comms::option::app::LengthInfoInterface,
    comms::option::app::IdInfoInterface,
    comms::option::app::Handler<UnitTestMsgHandler>
>;

CC_MQTT5_ALIASES_FOR_ALL_MESSAGES_DEFAULT_OPTIONS(UnitTest, Msg, UnitTestMessage)

using UnitTestsFrame = cc_mqtt5::frame::Frame<UnitTestMessage>;
using UniTestsMsgPtr = UnitTestsFrame::MsgPtr;

class UnitTestMsgHandler : public comms::GenericHandler<UnitTestMessage, cc_mqtt5::input::AllMessages<UnitTestMessage> >
{
protected:
    UnitTestMsgHandler() = default;
    ~UnitTestMsgHandler() noexcept = default;
};

