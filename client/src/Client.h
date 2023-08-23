//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

namespace cc_mqtt5_client
{

class Client
{
public:
    Client()
    {
        static_cast<void>(m_dummy);
    }
    
private:
    int m_dummy = 0;
};

} // namespace cc_mqtt5_client
