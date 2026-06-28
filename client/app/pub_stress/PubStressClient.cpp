//
// Copyright 2026 - 2026 (C). Alex Robenko. All rights reserved.
//
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "PubStressClient.h"

#include <iostream>

namespace cc_mqtt5_client_app
{

namespace
{

PubStressClient* asThis(void* data)
{
    return reinterpret_cast<PubStressClient*>(data);
}

} // namespace

void PubStressClient::brokerConnectedImpl()
{
}

std::string PubStressClient::clientIdImpl()
{
    return Base::clientIdImpl() + std::to_string(m_idCnt);
}

void PubStressClient::publishCompleteInternal([[maybe_unused]] CC_Mqtt5PublishHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response)
{
    if (status != CC_Mqtt5AsyncOpStatus_Complete) {
        logError() << "Publish failed: " << toString(status) << std::endl;
        doTerminate();
        return;
    }

    int result = 1;
    if ((response == nullptr) || (response->m_reasonCode < CC_Mqtt5ReasonCode_UnspecifiedError)) {
        std::cout << "Publish successful" << std::endl;
        result = 0;
    }
    else {
        std::cout << "Publish failed" << std::endl;
    }

    if ((response != nullptr) && (opts().verbose())) {
        print(*response);
    }

    doTerminate(result);
}

void PubStressClient::publishCompleteCb(void* data, CC_Mqtt5PublishHandle handle, CC_Mqtt5AsyncOpStatus status, const CC_Mqtt5PublishResponse* response)
{
    asThis(data)->publishCompleteInternal(handle, status, response);
}

} // namespace cc_mqtt5_client_app
