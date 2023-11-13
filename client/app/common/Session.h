//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "ProgramOptions.h"

#include <boost/asio.hpp>

#include <iosfwd>
#include <memory>

namespace cc_mqtt5_client_app
{

class Session
{
public:
    using Ptr = std::unique_ptr<Session>;

    virtual ~Session() = default;

    static Ptr create(boost::asio::io_context& io, const ProgramOptions& opts);

    bool start()
    {
        return startImpl();
    }

    using DataReportCb = std::function<unsigned (const std::uint8_t* buf, std::size_t bufLen)>;
    template <typename TFunc>
    void setDataReportCb(TFunc&& func)
    {
        m_dataReportCb = std::forward<TFunc>(func);
    }


protected:
    Session(boost::asio::io_context& io, const ProgramOptions& opts);

    boost::asio::io_context& io()
    {
        return m_io;
    }

    const ProgramOptions& opts() const
    {
        return m_opts;
    }

    static std::ostream& logError();
    unsigned reportData(const std::uint8_t* buf, std::size_t bufLen);

    virtual bool startImpl() = 0;

private:
    boost::asio::io_context& m_io; 
    const ProgramOptions& m_opts;
    DataReportCb m_dataReportCb;
};

using SessionPtr = Session::Ptr;

} // namespace cc_mqtt5_client_app
