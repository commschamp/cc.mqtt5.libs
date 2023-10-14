//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "ExtConfig.h"
#include "ObjListType.h"
#include "PropsHandler.h"
#include "ProtocolDefs.h"

#include "cc_mqtt5_client/common.h"

namespace cc_mqtt5_client
{

class ClientImpl;

namespace op
{

class Op : public ProtMsgHandler
{
public:
    enum Type
    {
        Type_Connect,
        Type_KeepAlive,
        Type_Disconnect,
        Type_Subscribe,
        Type_Unsubscribe,
        Type_Recv,
        Type_Send,
        Type_NumOfValues // Must be last
    };

    virtual ~Op() noexcept = default;

    Type type() const
    {
        return typeImpl();
    }

    void terminateOp(CC_Mqtt5AsyncOpStatus status)
    {
        terminateOpImpl(status);
    }

    unsigned getResponseTimeout() const
    {
        return m_responseTimeoutMs;
    }

    void setResponseTimeout(unsigned ms)
    {
        m_responseTimeoutMs = ms;
    }    

    void networkConnectivityChanged()
    {
        networkConnectivityChangedImpl();
    }

protected:
    using UserPropsList = ObjListType<CC_Mqtt5UserProp, Config::UserPropsLimit, Config::HasUserProps>;
    using DisconnectReason = DisconnectMsg::Field_reasonCode::Field::ValueType;

    explicit Op(ClientImpl& client);

    virtual Type typeImpl() const = 0;
    virtual void terminateOpImpl(CC_Mqtt5AsyncOpStatus status);
    virtual void networkConnectivityChangedImpl();

    void sendMessage(const ProtMessage& msg);
    void opComplete();
    void doApiGuard();
    std::uint16_t allocPacketId();
    void releasePacketId(std::uint16_t id);

    ClientImpl& client()
    {
        return m_client;
    }

    static void sendDisconnectWithReason(ClientImpl& client, DisconnectReason reason);
    void sendDisconnectWithReason(DisconnectReason reason);
    static void terminationWithReason(ClientImpl& client, DisconnectReason reason);
    void terminationWithReason(DisconnectReason reason);
    static void protocolErrorTermination(ClientImpl& client);
    void protocolErrorTermination();
    inline void errorLog(const char* msg)
    {
        if constexpr (Config::HasErrorLog) {
            errorLogInternal(msg);
        }
    }

    inline bool verifySubFilter(const char* filter)
    {
        if (Config::HasTopicFormatVerification) {
            return verifySubFilterInternal(filter);
        }
        else {
            return true;
        }
    }    

    inline bool verifyPubTopic(const char* topic, bool outgoing)
    {
        if (Config::HasTopicFormatVerification) {
            return verifyPubTopicInternal(topic, outgoing);
        }
        else {
            return true;
        }
    }     

    static void fillUserProps(const PropsHandler& propsHandler, UserPropsList& userProps);

    template <typename TField>
    static bool canAddProp(const TField& field)
    {
        return field.value().size() < field.value().max_size();
    }

    template <typename TField>
    static decltype(auto) addProp(TField& field)
    {
        auto& vec = field.value();
        vec.resize(vec.size() + 1U);
        return vec.back();    
    }

    template <typename TField>
    CC_Mqtt5ErrorCode addUserPropToList(TField& field, const CC_Mqtt5UserProp& prop)
    {
        if constexpr (ExtConfig::HasUserProps) {
            if (prop.m_key == nullptr) {
                errorLog("User property key is empty.");
                return CC_Mqtt5ErrorCode_BadParam;
            }

            if (!canAddProp(field)) {
                errorLog("Cannot add user property, reached available limit.");
                return CC_Mqtt5ErrorCode_OutOfMemory;
            }

            auto& propVar = addProp(field);
            auto& propBundle = propVar.initField_userProperty();
            auto& valueField = propBundle.field_value();
            valueField.field_first().value() = prop.m_key;

            if (prop.m_value != nullptr) {
                valueField.field_second().value() = prop.m_value;
            }

            return CC_Mqtt5ErrorCode_Success;
        }
        else {
            errorLog("User properties are note supported.");
            return CC_Mqtt5ErrorCode_NotSupported;
        }        
    }    

private:
    void errorLogInternal(const char* msg);
    bool verifySubFilterInternal(const char* filter);
    bool verifyPubTopicInternal(const char* topic, bool outgoing);

    ClientImpl& m_client;    
    unsigned m_responseTimeoutMs = 0U;
};

} // namespace op

} // namespace cc_mqtt5_client
