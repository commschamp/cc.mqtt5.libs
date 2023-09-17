#pragma once

#include "cc_mqtt5_client/client.h"

#include <memory>

namespace details
{

struct UnitTestDeleter
{
    void operator()(CC_Mqtt5Client* ptr)
    {
        ::cc_mqtt5_client_free(ptr);
    }
}; 

} // namespace details

using UnitTestClientPtr = std::unique_ptr<CC_Mqtt5Client, details::UnitTestDeleter>;

