//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
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

    Generator(Logger& logger) : m_logger(logger) {};

    bool prepare(const std::string& inputFile);
    void processData(const std::uint8_t* buf, unsigned bufLen);

    template <typename TFunc>
    void setDataReportCb(TFunc&& func)
    {
        m_dataReportCb = std::forward<TFunc>(func);
    }

    void handle(const Mqtt5ConnectMsg& msg);
    void handle(const Mqtt5SubscribeMsg& msg);
    void handle(const Mqtt5AuthMsg& msg);
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

        // using MessageExpiryInterval = Property::Field_messageExpiryInterval;
        // unsigned m_messageExpiryInterval = nullptr;
        // template <std::size_t TIdx>
        // void operator()(const MessageExpiryInterval& field)
        // {
        //     m_messageExpiryInterval = field.field_value().getValue();
        // }

        // using ContentType = Property::Field_contentType;
        // const ContentType* m_contentType = nullptr;
        // template <std::size_t TIdx>
        // void operator()(const ContentType& field)
        // {
        //     storeProp(field, m_contentType);
        // }

        // using ResponseTopic = Property::Field_responseTopic;
        // const ResponseTopic* m_responseTopic = nullptr;
        // template <std::size_t TIdx>
        // void operator()(const ResponseTopic& field)
        // {
        //     storeProp(field, m_responseTopic);
        // }

        // using CorrelationData = Property::Field_correlationData;
        // const CorrelationData* m_correlationData = nullptr;
        // template <std::size_t TIdx>
        // void operator()(const CorrelationData& field)
        // {
        //     storeProp(field, m_correlationData);
        // }    

        // using SubscriptionId = Property::Field_subscriptionId;
        // using SubscriptionIdsList = ObjListType<const SubscriptionId*, Config::SubIdsLimit, Config::HasSubIds>;
        // SubscriptionIdsList m_subscriptionIds;
        // template <std::size_t TIdx>
        // void operator()(const SubscriptionId& field)
        // {
        //     if constexpr (Config::HasSubIds) {
        //         if (m_subscriptionIds.max_size() <= m_subscriptionIds.size()) {
        //             return;
        //         }

        //         m_subscriptionIds.push_back(&field);
        //         if (field.field_value().value() == 0U) {
        //             m_protocolError = true;
        //         }
        //     }        
        // }        

        // using SessionExpiryInterval = Property::Field_sessionExpiryInterval;
        // const SessionExpiryInterval* m_sessionExpiryInterval = nullptr;
        // template <std::size_t TIdx>
        // void operator()(const SessionExpiryInterval& field)
        // {
        //     storeProp(field, m_sessionExpiryInterval);
        // }    

        // using AssignedClientId = Property::Field_assignedClientId;
        // const AssignedClientId* m_assignedClientId = nullptr;
        // template <std::size_t TIdx>
        // void operator()(const AssignedClientId& field)
        // {
        //     storeProp(field, m_assignedClientId);
        // }

        // using ServerKeepAlive = Property::Field_serverKeepAlive;
        // const ServerKeepAlive* m_serverKeepAlive = nullptr;
        // template <std::size_t TIdx>
        // void operator()(const ServerKeepAlive& field)
        // {
        //     storeProp(field, m_serverKeepAlive);
        // }

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

        // using RequestProblemInfo = Property::Field_requestProblemInfo;
        // const RequestProblemInfo* m_requestProblemInfo = nullptr;
        // template <std::size_t TIdx>
        // void operator()(const RequestProblemInfo& field)
        // {
        //     storeProp(field, m_requestProblemInfo);
        // }    

        // using WillDelayInterval = Property::Field_willDelayInterval;
        // const WillDelayInterval* m_willDelayInterval = nullptr;
        // template <std::size_t TIdx>
        // void operator()(const WillDelayInterval& field)
        // {
        //     storeProp(field, m_willDelayInterval);
        // }

        // using RequestResponseInfo = Property::Field_requestResponseInfo;
        // const RequestResponseInfo* m_requestResponseInfo = nullptr;
        // template <std::size_t TIdx>
        // void operator()(const RequestResponseInfo& field)
        // {
        //     storeProp(field, m_requestResponseInfo);
        // }    

        // using ResponseInfo = Property::Field_responseInfo;
        // const ResponseInfo* m_responseInfo = nullptr;
        // template <std::size_t TIdx>
        // void operator()(const ResponseInfo& field)
        // {
        //     storeProp(field, m_responseInfo);
        // }   

        // using ServerRef = Property::Field_serverRef;
        // const ServerRef* m_serverRef = nullptr;
        // template <std::size_t TIdx>
        // void operator()(const ServerRef& field)
        // {
        //     storeProp(field, m_serverRef);
        // }   

        // using ReasonStr = Property::Field_reasonStr;
        // const ReasonStr* m_reasonStr = nullptr;
        // template <std::size_t TIdx>
        // void operator()(const ReasonStr& field)
        // {
        //     storeProp(field, m_reasonStr);
        // }      

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

        // using TopicAlias = Property::Field_topicAlias;
        // const TopicAlias* m_topicAlias = nullptr;
        // template <std::size_t TIdx>
        // void operator()(const TopicAlias& field)
        // {
        //     storeProp(field, m_topicAlias);
        // } 

        // using MaxQos = Property::Field_maxQos;
        // const MaxQos* m_maxQos = nullptr;
        // template <std::size_t TIdx>
        // void operator()(const MaxQos& field)
        // {
        //     storeProp(field, m_maxQos);
        //     if (field.field_value().value() >= MaxQos::Field_value::ValueType::ExactlyOnceDelivery) {
        //         m_protocolError = true;
        //     }
        // }          

        // using RetainAvailable = Property::Field_retainAvailable;
        // const RetainAvailable* m_retainAvailable = nullptr;
        // template <std::size_t TIdx>
        // void operator()(const RetainAvailable& field)
        // {
        //     storeProp(field, m_retainAvailable);
        //     if (!field.field_value().valid()) {
        //         m_protocolError = true;
        //     }
        // }      

        // using UserProperty = Property::Field_userProperty;
        // using UserPropsList = ObjListType<const UserProperty*, Config::UserPropsLimit, Config::HasUserProps>;
        // UserPropsList m_userProps;
        // template <std::size_t TIdx>
        // void operator()(const UserProperty& field)
        // {
        //     if constexpr (Config::HasUserProps) {
        //         if (m_userProps.max_size() <= m_userProps.size()) {
        //             return;
        //         }

        //         m_userProps.push_back(&field);
        //     }
        // }     

        using MaxPacketSize = Property::Field_maxPacketSize;
        unsigned m_maxPacketSize = 0U;
        template <std::size_t TIdx>
        void operator()(const MaxPacketSize& field)
        {
            m_maxPacketSize = field.field_value().getValue();
        }      

        // using WildcardSubAvail = Property::Field_wildcardSubAvail;
        // const WildcardSubAvail* m_wildcardSubAvail = nullptr;
        // template <std::size_t TIdx>
        // void operator()(const WildcardSubAvail& field)
        // {
        //     storeProp(field, m_wildcardSubAvail);
        //     if (!field.field_value().valid()) {
        //         m_protocolError = true;
        //     }        
        // }        

        // using SubIdAvail = Property::Field_subIdAvail;
        // const SubIdAvail* m_subIdAvail = nullptr;
        // template <std::size_t TIdx>
        // void operator()(const SubIdAvail& field)
        // {
        //     storeProp(field, m_subIdAvail);
        //     if (!field.field_value().valid()) {
        //         m_protocolError = true;
        //     }           
        // }   

        // using SharedSubAvail = Property::Field_sharedSubAvail;
        // const SharedSubAvail* m_sharedSubAvail = nullptr;
        // template <std::size_t TIdx>
        // void operator()(const SharedSubAvail& field)
        // {
        //     storeProp(field, m_sharedSubAvail);
        //     if (!field.field_value().valid()) {
        //         m_protocolError = true;
        //     }             
        // }
    };

    void sendMessage(Mqtt5Message& msg);
    void sendConnack(const Mqtt5ConnectMsg& msg, const PropsHandler& propsHandler);
    void sendAuth(const PropsHandler& propsHandler, Mqtt5AuthMsg::Field_reasonCode::Field::ValueType resonCode);

    Logger& m_logger;
    std::ofstream m_stream;
    DataReportCb m_dataReportCb;
    Mqtt5Frame m_frame;
    std::unique_ptr<Mqtt5ConnectMsg> m_cachedConnect;
    bool m_connected = false;;
};

using GeneratorPtr = std::unique_ptr<Generator>;

} // namespace cc_mqtt5_client_afl_fuzz
