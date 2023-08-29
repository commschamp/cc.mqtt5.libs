//
// Copyright 2023 - 2024 (C). Alex Robenko. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "comms/util/StaticVector.h"
#include "comms/util/type_traits.h"

#include <vector>

namespace cc_mqtt5_client
{

namespace details
{

template <typename TObj, unsigned TLimit>
class ObjListTypeHelper
{
    template <typename ...>
    using DynVector = std::vector<TObj>;

    template <typename ...>
    using StaticVector = comms::util::StaticVector<TObj, TLimit>;

    template <typename... TParams>
    using Vector = 
        typename comms::util::LazyShallowConditional<
            TLimit == 0U
        >::template Type<
            DynVector,
            StaticVector
        >;

public:
    using VectorType = Vector<>;
};

} // namespace details

template <typename TObj, unsigned TLimit>
using ObjListType = details::ObjListTypeHelper<TObj, TLimit>;

} // namespace cc_mqtt5_client
