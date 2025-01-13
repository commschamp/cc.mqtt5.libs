//
// Copyright 2023 - 2025 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Logger.h"

#include "cc_mqtt5/Message.h"
#include "cc_mqtt5/field/Property.h"
#include "cc_mqtt5/frame/Frame.h"

#include "comms/options.h"

#include <cstdint>
#include <iterator>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace cc_mqtt5_client_afl_fuzz
{

class Generator
{
public:
    using RawDataBuf = std::vector<std::uint8_t>;

    using Mqtt5Message = 
        cc_mqtt5::Message<
            comms::option::app::ReadIterator<const std::uint8_t*>,
            comms::option::app::WriteIterator<std::back_insert_iterator<RawDataBuf>>,
            comms::option::app::LengthInfoInterface,
            comms::option::app::IdInfoInterface,
            comms::option::app::NameInterface,
            comms::option::app::RefreshInterface,
            comms::option::app::Handler<Generator>
        >;

    CC_MQTT5_ALIASES_FOR_ALL_MESSAGES_DEFAULT_OPTIONS(Mqtt5, Msg, Mqtt5Message);

    using DataReportCb = std::function<void (const std::uint8_t* buf, std::size_t bufLen)>;

    Generator(Logger& logger, unsigned minPubCount) : m_logger(logger), m_minPubCount(minPubCount) {};

    bool prepare(const std::string& inputFile);
    void processData(const std::uint8_t* buf, unsigned bufLen);

    template <typename TFunc>
    void setDataReportCb(TFunc&& func)
    {
        m_dataReportCb = std::forward<TFunc>(func);
    }

    void handle(const Mqtt5ConnectMsg& msg);
    void handle(const Mqtt5PublishMsg& msg);
    void handle(const Mqtt5PubrecMsg& msg);
    void handle(const Mqtt5PubrelMsg& msg);
    void handle(const Mqtt5AuthMsg& msg);
    void handle(const Mqtt5SubscribeMsg& msg);
    void handle(const Mqtt5UnsubscribeMsg& msg);
    void handle(const Mqtt5Message& msg);

private:
    using Mqtt5Frame = cc_mqtt5::frame::Frame<Mqtt5Message>;

    class PropsHandler
    {
        using Property = cc_mqtt5::field::Property<>;

    public:
        using PayloadFormatIndicator = Property::Field_payloadFormatIndicator;
        const PayloadFormatIndicator* m_payloadFormatIndicator = nullptr;
        template <std::size_t TIdx, typename TField>
        void operator()([[maybe_unused]]const TField& field)
        {
        }

        using AuthMethod = Property::Field_authMethod;
        std::string m_authMethod;
        template <std::size_t TIdx>
        void operator()(const AuthMethod& field)
        {
            m_authMethod = field.field_value().getValue();
        }

        using AuthData = Property::Field_authData;
        RawDataBuf m_authData;
        template <std::size_t TIdx>
        void operator()(const AuthData& field)
        {
            m_authData = field.field_value().getValue();
        }

        using ReceiveMax = Property::Field_receiveMax;
        unsigned m_receiveMax = 0U;
        template <std::size_t TIdx>
        void operator()(const ReceiveMax& field)
        {
            m_receiveMax = field.field_value().getValue();
        }

        using TopicAliasMax = Property::Field_topicAliasMax;
        unsigned m_topicAliasMax = 0U;
        template <std::size_t TIdx>
        void operator()(const TopicAliasMax& field)
        {
            m_topicAliasMax = field.field_value().getValue();
        } 

        using MaxPacketSize = Property::Field_maxPacketSize;
        unsigned m_maxPacketSize = 0U;
        template <std::size_t TIdx>
        void operator()(const MaxPacketSize& field)
        {
            m_maxPacketSize = field.field_value().getValue();
        }      

    };

    unsigned allocPacketId();
    unsigned allocTopicAlias(const std::string& topic);
    void sendMessage(Mqtt5Message& msg);
    void sendConnack(const Mqtt5ConnectMsg& msg, const PropsHandler& propsHandler);
    void sendPublish(const std::string& topic, unsigned qos);
    void sendAuth(const PropsHandler& propsHandler, Mqtt5AuthMsg::Field_reasonCode::Field::ValueType resonCode);
    void doPublish();
    void doNextPublishIfNeeded();

    Logger& m_logger;
    unsigned m_minPubCount = 0U;
    std::ofstream m_stream;
    DataReportCb m_dataReportCb;
    Mqtt5Frame m_frame;
    std::unique_ptr<Mqtt5ConnectMsg> m_cachedConnect;
    unsigned m_lastPacketId = 0U;
    unsigned m_topicAliasLimit = 0U;
    std::map<std::string, unsigned> m_topicAliases;
    unsigned m_nextPubQos = 0U;
    std::string m_lastPubTopic;
    unsigned m_pubCount = 0U;
    bool m_connected = false;
};

using GeneratorPtr = std::unique_ptr<Generator>;

} // namespace cc_mqtt5_client_afl_fuzz
