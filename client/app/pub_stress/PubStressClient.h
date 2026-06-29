//
// Copyright 2026 - 2026 (C). Alex Robenko. All rights reserved.
//
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "AppClient.h"
#include "PubStressProgramOptions.h"

#include <boost/asio.hpp>

#include <memory>

namespace cc_mqtt5_client_app
{

class PubStressClient : public AppClient
{
    using Base = AppClient;

public:
    PubStressClient(boost::asio::io_context& io, PubStressProgramOptions& opts, int& result, unsigned threadId, unsigned clientId);
    virtual ~PubStressClient() = default;

protected:
    virtual void brokerConnectedImpl() override;
    virtual void messageReceivedImpl(const CC_Mqtt5MessageInfo* info) override;
    virtual std::string clientIdImpl() const override;
    virtual std::string logPrefixImpl() const override;

private:
    using Timer = boost::asio::steady_timer;

    void subscribeCompleteInternal(CC_Mqtt5SubscribeHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response);
    void publishCompleteInternal(CC_Mqtt5PublishHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response);
    std::string clientLogInternal() const;
    void doPublishInternal();
    void publishMsgInternal();

    static void subscribeCompleteCb(void* data, CC_Mqtt5SubscribeHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5SubscribeResponse* response);
    static void publishCompleteCb(void* data, CC_Mqtt5PublishHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response);

    Timer m_timer;
    const unsigned m_threadId = 0U;
    const unsigned m_clientId = 0U;
    bool m_verbose = false;
};

using PubStressClientPtr = std::unique_ptr<PubStressClient>;

} // namespace cc_mqtt5_client_app
