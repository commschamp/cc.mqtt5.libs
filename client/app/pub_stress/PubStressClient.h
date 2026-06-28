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
    PubStressClient(boost::asio::io_context& io, PubStressProgramOptions& opts, int& result, unsigned idCnt) :
        Base(io, opts, result),
        m_idCnt(idCnt)
    {
    }

    virtual ~PubStressClient() = default;

protected:
    virtual void brokerConnectedImpl() override;
    virtual std::string clientIdImpl() override;

private:
    void publishCompleteInternal(CC_Mqtt5PublishHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response);

    static void publishCompleteCb(void* data, CC_Mqtt5PublishHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response);

    const unsigned m_idCnt = 0U;
};

using PubStressClientPtr = std::unique_ptr<PubStressClient>;

} // namespace cc_mqtt5_client_app
