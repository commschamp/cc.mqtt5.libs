//
// Copyright 2026 - 2026 (C). Alex Robenko. All rights reserved.
//
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "PubStressProgramOptions.h"

#include <boost/program_options.hpp>

namespace po = boost::program_options;

namespace cc_mqtt5_client_app
{

PubStressProgramOptions::PubStressProgramOptions()
{
    addCommon();
    addNetwork();
    addTls();
    addConnect();
    addSubscribe();
    addPublish();

    po::options_description opts("Stress Options");
    opts.add_options()
        ("threads,j", po::value<unsigned>()->default_value(0), "Number of execution threads. 0 means available hardware cores.")
        ("client-count,c", po::value<unsigned>()->default_value(100), "Number of sepearate client allocations.")
    ;

    desc().add(opts);    
}

unsigned PubStressProgramOptions::clientCount() const
{
    return vm()["client-count"].as<unsigned>();
}

} // namespace cc_mqtt5_client_app