//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "op/Op.h"

#include "Client.h"

namespace cc_mqtt5_client
{

namespace op
{

void Op::sendMessage(const ProtMessage& msg)
{
    m_client.sendMessage(msg);
}

void Op::opComplete()
{
    // TODO
}

} // namespace op

} // namespace cc_mqtt5_client
