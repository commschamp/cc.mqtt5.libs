# Custom Client Build
The [BUILD.md](BUILD.md) file described general build process and explained
usage of **CC_MQTT5_CUSTOM_CLIENT_CONFIG_FILES** option. Such option specifies
a single custom client build configuration file or a list of such files. These
files get included during the [CMake](https://cmake.org) parsing stage and 
are expected to specify multiple variables, which in turn influence the
way the client library is built.

This page describes and explains the meaning of these variables.

## Variables
In general, the client library will use 
[std::string](http://en.cppreference.com/w/cpp/string/basic_string) type to 
hold strings and 
[std::vector](http://en.cppreference.com/w/cpp/container/vector) type to hold
various lists, because there is no known and predefined limit on string length
and/or number of elements in the list. However, if such limit is specified the
library will use [comms::util::StaticString](https://github.com/commschamp/comms/blob/master/include/comms/util/StaticString.h)
and [comms::util::StaticVector](https://github.com/commschamp/comms/blob/master/include/comms/util/StaticVector.h)
from the [COMMS library](https://github.com/commschamp/comms)
instead. These classes don't use exceptions and/or dynamic memory allocation
and can be suitable for bare-metal systems.

### CC_MQTT5_CLIENT_CUSTOM_NAME
This variable specifies the name of the custom client library.
It will influence the names of the API functions. The **default** client build
(controlled by **CC_MQTT5_CLIENT_DEFAULT_LIB** option) prefixes all the
functions with `cc_mqtt5_client_`, while client with custom name will produce
functions having `cc_mqtt5_<custom_name>_client_` prefix. For example having the
`set (CC_MQTT5_CLIENT_CUSTOM_NAME "my_name")` statement in configuration file
will produce a library which prefixes all API functions with 
`cc_mqtt5_my_name_client_`.

The **CC_MQTT5_CLIENT_CUSTOM_NAME** variable is a **must have** one, without it
the custom build of the client library won't be possible.
```
set (CC_MQTT5_CLIENT_CUSTOM_NAME "my_name")
```

### CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC
Specify whether usage of the dynamic memory allocation is allowed. The value
defaults to **TRUE**. Setting the configuration to **FALSE** will ensure
that all the necessary configuration to avoid dynamic memory allocation has
been provided. In case some of the configuration is missing the compilation will
fail on `static_assert()` invocation with a message of what variable hasn't been
set property.
```
# Disable dynamic memory allocation
set (CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC FALSE)
```

### CC_MQTT5_CLIENT_ALLOC_LIMIT
The client library allows allocation of multiple client managing objects
(**cc_mqtt5_client_alloc()** function). By default the value is **0**,
which means there is no limit on such
allocations. As a result every such object is dynamically allocated.
However, if there is a known limit for number of client managing objects, the
library will be requested to allocate, use **CC_MQTT5_CLIENT_ALLOC_LIMIT**
variable to specify such limit. If the limit is specified the library will
use static pool for allocation and won't use heap for this purpose.
```
# Only 1 MQTT5 client will be allocated
set(CC_MQTT5_CLIENT_ALLOC_LIMIT 1)
```
Having **CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC** set to **FALSE** requires setting
of the **CC_MQTT5_CLIENT_ALLOC_LIMIT** to a non-**0** value.

### CC_MQTT5_CLIENT_STRING_FIELD_FIXED_LEN
By default the storage type of any string field defined using the
[COMMS Library](https://github.com/commschamp/comms) is `std::string`, which
uses dynamic memory allocation. Setting the **CC_MQTT5_CLIENT_STRING_FIELD_FIXED_LEN**
to a non-**0** value will force usage of the
[comms::util::StaticString](https://github.com/commschamp/comms/blob/master/include/comms/util/StaticString.h)
as a storage type. It can be useful for bare-metal embedded systems without heap.
When the **CC_MQTT5_CLIENT_STRING_FIELD_FIXED_LEN** variable is set to a non-**0** value,
appropriate **comms** library option is passed to the
[cc_mqtt5::field::String](https://github.com/commschamp/cc.mqtt5.generated/blob/master/include/cc_mqtt5/field/String.h)
field.
```
# Allow up to 256 bytes for the string field storage
set(CC_MQTT5_CLIENT_STRING_FIELD_FIXED_LEN 256)
```

Having **CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC** set to **FALSE** requires setting
of the **CC_MQTT5_CLIENT_STRING_FIELD_FIXED_LEN** to a non-**0** value.

The **CC_MQTT5_CLIENT_STRING_FIELD_FIXED_LEN** variable sets a global default
for **all** the string fields. It is possible to refine the static length of
other strings fields using other variables

#### CC_MQTT5_CLIENT_CLIENT_ID_FIELD_FIXED_LEN
To limit the length of the string used to store the "Client ID" information, use
the **CC_MQTT5_CLIENT_CLIENT_ID_FIELD_FIXED_LEN** variable. When it is set to **0** (default)
the global fixed length of a string field set by the **CC_MQTT5_CLIENT_STRING_FIELD_FIXED_LEN** is used.

```
# Allow up to 50 bytes for the "client ID" string field storage
set(CC_MQTT5_CLIENT_CLIENT_ID_FIELD_FIXED_LEN 50)
```

#### CC_MQTT5_CLIENT_USERNAME_FIELD_FIXED_LEN

#### CC_MQTT5_CLIENT_TOPIC_FIELD_FIXED_LEN

### CC_MQTT5_CLIENT_BIN_DATA_FIELD_FIXED_LEN

#### CC_MQTT5_CLIENT_PASSWORD_FIELD_FIXED_LEN

### CC_MQTT5_CLIENT_PROPERTIES_LIST_FIELD_FIXED_LEN

### CC_MQTT5_CLIENT_MAX_OUTPUT_PACKET_SIZE

### CC_MQTT5_CLIENT_HAS_USER_PROPS

### CC_MQTT5_CLIENT_USER_PROPS_LIMIT

### CC_MQTT5_CLIENT_RECEIVE_MAX_LIMIT

### CC_MQTT5_CLIENT_SEND_MAX_LIMIT

### CC_MQTT5_CLIENT_HAS_TOPIC_ALIASES

### CC_MQTT5_CLIENT_TOPIC_ALIASES_LIMIT

### CC_MQTT5_CLIENT_HAS_SUB_IDS

### CC_MQTT5_CLIENT_SUB_IDS_LIMIT

### CC_MQTT5_CLIENT_ASYNC_SUBS_LIMIT

### CC_MQTT5_CLIENT_ASYNC_UNSUBS_LIMIT

### CC_MQTT5_CLIENT_HAS_ERROR_LOG

### CC_MQTT5_CLIENT_HAS_TOPIC_FORMAT_VERIFICATION

### CC_MQTT5_CLIENT_HAS_SUB_TOPIC_VERIFICATION

### CC_MQTT5_CLIENT_SUB_FILTERS_LIMIT

## Example for Bare-Metal Without Heap Configuration 
The content of the custom client configuration file, which explicitly specifies
all unknown compile time limits and constants to prevent usage of dynamic 
memory allocation and STL types like [std::string](http://en.cppreference.com/w/cpp/string/basic_string)
and [std::vector](http://en.cppreference.com/w/cpp/container/vector), may look
like this:
```
# Name of the client API
set (CC_MQTT5_CLIENT_CUSTOM_NAME "bare_metal")

# Allow only a single client
set(CC_MQTT5_CLIENT_ALLOC_LIMIT 1)

# Allow up to 256 bytes for the string field storage
set(CC_MQTT5_CLIENT_STRING_FIELD_FIXED_LEN 256)

# Allow up to 50 bytes for the "client ID" storage
set(CC_MQTT5_CLIENT_CLIENT_ID_FIELD_FIXED_LEN 50)

```
As the result of such configuration, the static library `cc_mqtt5_bare_metal_client`
will be generated, which will contain functions defined in 
`include/cc_mqtt5_client/bare_metal_client.h" header file:
```c
CC_Mqtt5ClientHandle cc_mqtt5_bare_metal_client_alloc();

void mqtt5_bare_metal_client_free(CC_Mqtt5ClientHandle client);

...
    
```
**NOTE**, that all the functions have `cc_mqtt5_bare_metal_client_` prefix due to the
fact of setting value of **CC_MQTT5_CLIENT_CUSTOM_NAME** variable to "bare_metal" string.
