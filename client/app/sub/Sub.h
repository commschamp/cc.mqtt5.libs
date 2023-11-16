//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "AppClient.h"
#include "ProgramOptions.h"

#include <boost/asio.hpp>

namespace cc_mqtt5_client_app
{

class Sub : public AppClient
{
    using Base = AppClient;
public:
    Sub(boost::asio::io_context& io, int& result);

protected:
    virtual void brokerConnectedImpl() override;    
    virtual void messageReceivedImpl(const CC_Mqtt5MessageInfo* info) override;
    
private:
    void subscribeCompleteInternal(CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response);

    static void subscribeCompleteCb(void* data, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response);
};

} // namespace cc_mqtt5_client_app
