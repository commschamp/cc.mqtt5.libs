//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <boost/program_options.hpp>

namespace cc_mqtt5_client_app
{

class ProgramOptions
{
public:
    static void addCommon(boost::program_options::options_description& desc);

    bool parseArgs(int argc, const char* argv[], const boost::program_options::options_description& desc);

    bool helpRequested() const;

private:
    boost::program_options::variables_map m_vm;
};

} // namespace cc_mqtt5_client_app
