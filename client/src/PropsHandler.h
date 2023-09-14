//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Config.h"
#include "ObjListType.h"
#include "ProtocolOptions.h"

#include "cc_mqtt5/field/Property.h"

namespace cc_mqtt5_client
{

class PropsHandler
{
    using Property = cc_mqtt5::field::Property<ProtocolOptions>;

public:
    using PayloadFormatIndicator = Property::Field_payloadFormatIndicator;
    const PayloadFormatIndicator* m_payloadFormatIndicator = nullptr;
    template <std::size_t TIdx>
    void operator()(const PayloadFormatIndicator& field)
    {
        storeProp(field, m_payloadFormatIndicator);
    }

    using MessageExpiryInterval = Property::Field_messageExpiryInterval;
    const MessageExpiryInterval* m_messageExpiryInterval = nullptr;
    template <std::size_t TIdx>
    void operator()(const MessageExpiryInterval& field)
    {
        storeProp(field, m_messageExpiryInterval);
    }

    using ContentType = Property::Field_contentType;
    const ContentType* m_contentType = nullptr;
    template <std::size_t TIdx>
    void operator()(const ContentType& field)
    {
        storeProp(field, m_contentType);
    }

    using ResponseTopic = Property::Field_responseTopic;
    const ResponseTopic* m_responseTopic = nullptr;
    template <std::size_t TIdx>
    void operator()(const ResponseTopic& field)
    {
        storeProp(field, m_responseTopic);
    }

    using CorrelationData = Property::Field_correlationData;
    const CorrelationData* m_correlationData = nullptr;
    template <std::size_t TIdx>
    void operator()(const CorrelationData& field)
    {
        storeProp(field, m_correlationData);
    }    

    using SubscriptionId = Property::Field_subscriptionId;
    const SubscriptionId* m_subscriptionId = nullptr;
    template <std::size_t TIdx>
    void operator()(const SubscriptionId& field)
    {
        // TODO: convert to list
        m_subscriptionId = &field;
    }        

    using SessionExpiryInterval = Property::Field_sessionExpiryInterval;
    const SessionExpiryInterval* m_sessionExpiryInterval = nullptr;
    template <std::size_t TIdx>
    void operator()(const SessionExpiryInterval& field)
    {
        storeProp(field, m_sessionExpiryInterval);
    }    

    using AssignedClientId = Property::Field_assignedClientId;
    const AssignedClientId* m_assignedClientId = nullptr;
    template <std::size_t TIdx>
    void operator()(const AssignedClientId& field)
    {
        storeProp(field, m_assignedClientId);
    }

    using ServerKeepAlive = Property::Field_serverKeepAlive;
    const ServerKeepAlive* m_serverKeepAlive = nullptr;
    template <std::size_t TIdx>
    void operator()(const ServerKeepAlive& field)
    {
        storeProp(field, m_serverKeepAlive);
    }

    using AuthMethod = Property::Field_authMethod;
    const AuthMethod* m_authMethod = nullptr;
    template <std::size_t TIdx>
    void operator()(const AuthMethod& field)
    {
        storeProp(field, m_authMethod);
    }

    using AuthData = Property::Field_authData;
    const AuthData* m_authData = nullptr;
    template <std::size_t TIdx>
    void operator()(const AuthData& field)
    {
        storeProp(field, m_authData);
    }

    using RequestProblemInfo = Property::Field_requestProblemInfo;
    const RequestProblemInfo* m_requestProblemInfo = nullptr;
    template <std::size_t TIdx>
    void operator()(const RequestProblemInfo& field)
    {
        storeProp(field, m_requestProblemInfo);
    }    

    using WillDelayInterval = Property::Field_willDelayInterval;
    const WillDelayInterval* m_willDelayInterval = nullptr;
    template <std::size_t TIdx>
    void operator()(const WillDelayInterval& field)
    {
        storeProp(field, m_willDelayInterval);
    }

    using RequestResponseInfo = Property::Field_requestResponseInfo;
    const RequestResponseInfo* m_requestResponseInfo = nullptr;
    template <std::size_t TIdx>
    void operator()(const RequestResponseInfo& field)
    {
        storeProp(field, m_requestResponseInfo);
    }    

    using ResponseInfo = Property::Field_responseInfo;
    const ResponseInfo* m_responseInfo = nullptr;
    template <std::size_t TIdx>
    void operator()(const ResponseInfo& field)
    {
        storeProp(field, m_responseInfo);
    }   

    using ServerRef = Property::Field_serverRef;
    const ServerRef* m_serverRef = nullptr;
    template <std::size_t TIdx>
    void operator()(const ServerRef& field)
    {
        storeProp(field, m_serverRef);
    }   

    using ReasonStr = Property::Field_reasonStr;
    const ReasonStr* m_reasonStr = nullptr;
    template <std::size_t TIdx>
    void operator()(const ReasonStr& field)
    {
        storeProp(field, m_reasonStr);
    }      

    using ReceiveMax = Property::Field_receiveMax;
    const ReceiveMax* m_receiveMax = nullptr;
    template <std::size_t TIdx>
    void operator()(const ReceiveMax& field)
    {
        storeProp(field, m_receiveMax);
    }     

    using TopicAliasMax = Property::Field_topicAliasMax;
    const TopicAliasMax* m_topicAliasMax = nullptr;
    template <std::size_t TIdx>
    void operator()(const TopicAliasMax& field)
    {
        storeProp(field, m_topicAliasMax);
    } 

    using TopicAlias = Property::Field_topicAlias;
    const TopicAlias* m_topicAlias = nullptr;
    template <std::size_t TIdx>
    void operator()(const TopicAlias& field)
    {
        storeProp(field, m_topicAlias);
    } 

    using MaxQos = Property::Field_maxQos;
    const MaxQos* m_maxQos = nullptr;
    template <std::size_t TIdx>
    void operator()(const MaxQos& field)
    {
        storeProp(field, m_maxQos);
    }          

    using RetainAvailable = Property::Field_retainAvailable;
    const RetainAvailable* m_retainAvailable = nullptr;
    template <std::size_t TIdx>
    void operator()(const RetainAvailable& field)
    {
        storeProp(field, m_retainAvailable);
    }      

    using UserProperty = Property::Field_userProperty;
    using UserPropsList = ObjListType<const UserProperty*, Config::UserPropsLimit, Config::HasUserProps>;
    UserPropsList m_userProps;
    template <std::size_t TIdx>
    void operator()(const UserProperty& field)
    {
        if constexpr (Config::HasUserProps) {
            if (m_userProps.capacity() <= m_userProps.size()) {
                return;
            }

            m_userProps.push_back(&field);
        }
    }     

    using MaxPacketSize = Property::Field_maxPacketSize;
    const MaxPacketSize* m_maxPacketSize = nullptr;
    template <std::size_t TIdx>
    void operator()(const MaxPacketSize& field)
    {
        storeProp(field, m_maxPacketSize);
    }      

    using WildcardSubAvail = Property::Field_wildcardSubAvail;
    const WildcardSubAvail* m_wildcardSubAvail = nullptr;
    template <std::size_t TIdx>
    void operator()(const WildcardSubAvail& field)
    {
        storeProp(field, m_wildcardSubAvail);
    }        

    using SubIdAvail = Property::Field_subIdAvail;
    const SubIdAvail* m_subIdAvail = nullptr;
    template <std::size_t TIdx>
    void operator()(const SubIdAvail& field)
    {
        storeProp(field, m_subIdAvail);
    }   

    using SharedSubAvail = Property::Field_sharedSubAvail;
    const SharedSubAvail* m_sharedSubAvail = nullptr;
    template <std::size_t TIdx>
    void operator()(const SharedSubAvail& field)
    {
        storeProp(field, m_sharedSubAvail);
    }      

    bool isProtocolError() const
    {
        return m_protocolError;
    }

private:
    template <typename TField>
    void storeProp(const TField& field, const TField*& ptr)
    {
        if (ptr != nullptr) {
            m_protocolError = true;
        }

        ptr = &field;
    }

    bool m_protocolError = false;
};

} // namespace cc_mqtt5_client
