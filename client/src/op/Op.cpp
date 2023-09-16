//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/Op.h"

#include "Client.h"

#include <algorithm>

namespace cc_mqtt5_client
{

namespace op
{

Op::Op(Client& client) : 
    m_client(client),
    m_opTimeoutMs(client.state().m_opTimeoutMs)
{
}    

void Op::sendMessage(const ProtMessage& msg)
{
    m_client.sendMessage(msg);
}

void Op::opComplete()
{
    m_client.opComplete(this);
}

void Op::sendDisconnectWithReason(DisconnectMsg::Field_reasonCode::Field::ValueType reason)
{
    DisconnectMsg disconnectMsg;
    disconnectMsg.field_reasonCode().setExists();
    disconnectMsg.field_propertiesList().setExists();
    disconnectMsg.field_reasonCode().field().setValue(reason);
    m_client.sendMessage(disconnectMsg);
}

void Op::fillUserProps(const PropsHandler& propsHandler, UserPropsList& userProps)
{
    if constexpr (Config::HasUserProps) {    
        userProps.reserve(std::min(propsHandler.m_userProps.size(), userProps.capacity()));
        auto endIter = propsHandler.m_userProps.end();
        if constexpr (Config::UserPropsLimit > 0U) {
            endIter = propsHandler.m_userProps.begin() + std::min(propsHandler.m_userProps.size(), std::size_t(Config::UserPropsLimit));
        }

        std::transform(
            propsHandler.m_userProps.begin(), endIter, std::back_inserter(userProps),
            [](auto* field)
            {
                return UserPropsList::value_type{field->field_value().field_first().value().c_str(), field->field_value().field_second().value().c_str()};
            });
    }
}

} // namespace op

} // namespace cc_mqtt5_client
