//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Pub.h"

#include <iostream>

namespace po = boost::program_options;

namespace cc_mqtt5_client_app
{

Pub::Pub(boost::asio::io_context& io) : 
    Base(io)
{
    opts().addCommon();
    opts().addNetwork();
}    

} // namespace cc_mqtt5_client_app
