//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Client.h"
#include "Config.h"

#include "comms/util/alloc.h"
#include "comms/util/type_traits.h"

namespace cc_mqtt5_client
{

class ClientCreator
{
    template <typename ...>
    using DynMemoryAlloc = comms::util::alloc::DynMemory<Client>;

    template <typename ...>
    using InPlaceAlloc = comms::util::alloc::InPlacePool<Client, Config::AllocLimit>;

    template <typename... TParams>
    using Alloc = 
        typename comms::util::LazyShallowConditional<
            Config::AllocLimit == 0U
        >::template Type<
            DynMemoryAlloc,
            InPlaceAlloc
        >;

    using AllocType = Alloc<>;
        
public:
    using ClientPtr = typename AllocType::Ptr;

    ClientPtr alloc()
    {
        return m_alloc.template alloc<Client>();
    }

    void free(Client* client) {
        auto ptr = m_alloc.wrap(client);
        static_cast<void>(ptr);
    }

private:
    AllocType m_alloc;
};

} // namespace cc_mqtt5_client
