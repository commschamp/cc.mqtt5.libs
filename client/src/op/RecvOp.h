//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "op/Op.h"
#include "ExtConfig.h"
#include "ProtocolDefs.h"
#include "TopicAliasDefs.h"

#include "TimerMgr.h"

namespace cc_mqtt5_client
{

namespace op
{

class RecvOp final : public Op
{
    using Base = Op;
public:
    explicit RecvOp(ClientImpl& client);

    using Base::handle;
    void handle(PublishMsg& msg) override;

protected:
    virtual Type typeImpl() const override;    

private:
    using UserPropKeyStorage = PublishMsg::Field_propertiesList::ValueType::value_type::Field_userProperty::Field_value::Field_first::ValueType;
    using UserPropValueStorage = PublishMsg::Field_propertiesList::ValueType::value_type::Field_userProperty::Field_value::Field_second::ValueType;
    using DataStorage = PublishMsg::Field_payload::ValueType;
    using ResponseTopicStorage = PublishMsg::Field_propertiesList::ValueType::value_type::Field_responseTopic::Field_value::ValueType;
    using CorrelationDataStorage = PublishMsg::Field_propertiesList::ValueType::value_type::Field_correlationData::Field_value::ValueType;
    using ContentTypeStorage = PublishMsg::Field_propertiesList::ValueType::value_type::Field_contentType::Field_value::ValueType;
    using ReasonStrStorage = PublishMsg::Field_propertiesList::ValueType::value_type::Field_reasonStr::Field_value::ValueType;

    struct UserPropInfo
    {
        UserPropKeyStorage m_key;
        UserPropValueStorage m_value;
    };
    using UserPropsStorage = ObjListType<UserPropInfo, Config::UserPropsLimit, Config::HasUserProps>;
    using SubIdsStorage = ObjListType<unsigned, Config::SubIdsLimit, Config::HasSubIds>;

    void restartRecvTimer();
    void recvTimoutInternal();
    void reportMsgInfoAndComplete();

    static void recvTimeoutCb(void* data);

    TimerMgr::Timer m_recvTimer;  
    TopicStr m_topicStr;
    DataStorage m_data;
    ResponseTopicStorage m_responseTopic;
    CorrelationDataStorage m_correlationData;
    UserPropsStorage m_userPropsCpy;
    UserPropsList m_userProps;
    ContentTypeStorage m_contentType;
    ReasonStrStorage m_reasonStr;
    SubIdsStorage m_subIds;
    CC_Mqtt5MessageInfo m_info;

    static_assert(ExtConfig::RecvOpTimers == 1U);
};

} // namespace op


} // namespace cc_mqtt5_client
