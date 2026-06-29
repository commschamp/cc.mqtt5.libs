//
// Copyright 2026 - 2026 (C). Alex Robenko. All rights reserved.
//
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "PubStressMgr.h"
#include "PubStressProgramOptions.h"

#include <boost/asio.hpp>

#include <atomic>
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <thread>

int main(int argc, const char* argv[])
{
    int result = 0U;
    try {
        cc_mqtt5_client_app::PubStressProgramOptions opts;
        if (!opts.parseArgs(argc, argv)) {
            std::cerr << "ERROR: Failed to parse arguments." << std::endl;
            return -1;
        }

        if (opts.helpRequested()) {
            std::cout << "Usage: " << argv[0] << " [options...]" << '\n';
            opts.printHelp();
            return 0;
        }

        boost::asio::io_context io;
        std::vector<boost::asio::io_context> workers(opts.threadsCount());
        std::vector<std::thread> workerThreads(workers.size());

        std::atomic<bool> terminating = false;
        auto terminateAll =
            [&io, &workers, &terminating]()
            {
                if (terminating) {
                    return;
                }

                terminating = true;

                for (auto& w : workers) {
                    w.stop();
                }

                io.stop();
            };

        bool failure = false;
        for (auto idx = 0U; idx < workerThreads.size(); ++idx) {
            auto& th = workerThreads[idx];

            std::cout << "INFO: Creating worker thread " << idx << std::endl;

            th = std::thread(
                [&workers, &opts, &result, &terminateAll, &failure, idx]()
                {
                    auto& workerIo = workers[idx];
                    cc_mqtt5_client_app::PubStressMgr app(workerIo, opts, result, idx);
                    if (!app.start()) {
                        failure = true;
                        return;
                    }

                    std::cout << "INFO: Starting worker " << idx << std::endl;
                    workerIo.run();
                    std::cout << "INFO: Worker complete " << idx << std::endl;
                    terminateAll();
                });
        }

        if (failure) {
            terminateAll();
            for (auto& th : workerThreads) {
                if (th.joinable()) {
                    th.join();
                }
            }

            return -2;
        }

        boost::asio::signal_set signals(io, SIGINT, SIGTERM);
        signals.async_wait(
            [&io, &result](const boost::system::error_code& ec, int sigNum)
            {
                if (ec == boost::asio::error::operation_aborted) {
                    return;
                }

                if (ec) {
                    std::cerr << "ERROR: Unexpected error in signal handling: " << ec.message() << std::endl;
                    result = 150;
                    io.stop();
                    return;
                }

                std::cerr << "Terminated with signal " << sigNum << std::endl;
                result = 100;
                io.stop();
            });

        io.run();
        terminateAll();
        for (auto& th : workerThreads) {
            if (th.joinable()) {
                th.join();
            }
        }
    }
    catch (const std::exception& ec)
    {
        std::cerr << "ERROR: Unexpected exception: " << ec.what() << std::endl;
        result = 200;
    }

    return result;
}