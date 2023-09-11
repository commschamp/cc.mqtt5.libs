//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "ProtocolDefs.h"

namespace cc_mqtt5_client
{

class Client;

namespace op
{

class Op : public ProtMsgHandler
{
public:
    enum Type
    {
        Type_Connect,
        Type_NumOfValues // Must be last
    };

    virtual ~Op() noexcept = default;

    Type type() const
    {
        return typeImpl();
    }

protected:
    explicit Op(Client& client);

    virtual Type typeImpl() const = 0;

    void sendMessage(const ProtMessage& msg);
    void opComplete();

    Client& client()
    {
        return m_client;
    }

    unsigned getOpTimeout() const
    {
        return m_opTimeoutMs;
    }

    void setOpTimeout(unsigned ms)
    {
        m_opTimeoutMs = ms;
    }

private:
    Client& m_client;    
    unsigned m_opTimeoutMs = 0U;
};

} // namespace op

} // namespace cc_mqtt5_client