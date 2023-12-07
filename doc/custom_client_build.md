# Custom Client Build
The [BUILD.md](BUILD.md) file described general build process and explained
usage of **CC_MQTT5_CUSTOM_CLIENT_CONFIG_FILES** option. Such option specifies
a single custom client build configuration file or a list of such files. These
files get included during the [CMake](https://cmake.org) parsing stage and 
are expected to specify multiple variables, which in turn influence the
way the client library is built.

This page describes and explains the meaning of these variables.

## Variables
In general, the client library implements all the features specified in the MQTT5 
specification. Some of the variables below allow removing some specific features 
from the compilation reducing the total library size as well as improving performance. 

The client library also uses
[std::string](http://en.cppreference.com/w/cpp/string/basic_string) type to 
hold strings and 
[std::vector](http://en.cppreference.com/w/cpp/container/vector) type to hold
various lists. It is because there is no known and predefined limit on string length
and/or number of elements in the list. However, if such limit is specified, the
library will use [comms::util::StaticString](https://github.com/commschamp/comms/blob/master/include/comms/util/StaticString.h)
and [comms::util::StaticVector](https://github.com/commschamp/comms/blob/master/include/comms/util/StaticVector.h)
from the [COMMS library](https://github.com/commschamp/comms)
instead. These classes don't use exceptions and/or dynamic memory allocation
and can be suitable for bare-metal systems. Some of the variables below allow
data storage types customization for selected fields.

---
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

---
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

---
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

---
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
# Limit the length of all the string fields
set(CC_MQTT5_CLIENT_STRING_FIELD_FIXED_LEN 256)
```

Having **CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC** set to **FALSE** requires setting
of the **CC_MQTT5_CLIENT_STRING_FIELD_FIXED_LEN** to a non-**0** value.

The **CC_MQTT5_CLIENT_STRING_FIELD_FIXED_LEN** variable sets a global default
for **all** the string fields. It is possible to refine the static length of
other strings fields using other variables

---
### CC_MQTT5_CLIENT_CLIENT_ID_FIELD_FIXED_LEN
To limit the length of the string used to store the "Client ID" information, use
the **CC_MQTT5_CLIENT_CLIENT_ID_FIELD_FIXED_LEN** variable. When it is set to **0** (default)
the global fixed length of a string field set by the **CC_MQTT5_CLIENT_STRING_FIELD_FIXED_LEN** is used.

```
# Limit the max "client ID" length
set(CC_MQTT5_CLIENT_CLIENT_ID_FIELD_FIXED_LEN 50)
```

---
### CC_MQTT5_CLIENT_USERNAME_FIELD_FIXED_LEN
To limit the length of the string used to store the "User Name" information, use
the **CC_MQTT5_CLIENT_USERNAME_FIELD_FIXED_LEN** variable. When it is set to **0** (default)
the global fixed length of a string field set by the **CC_MQTT5_CLIENT_STRING_FIELD_FIXED_LEN** is used.

```
# Limit the max "username" length
set(CC_MQTT5_CLIENT_USERNAME_FIELD_FIXED_LEN 20)
```

---
### CC_MQTT5_CLIENT_TOPIC_FIELD_FIXED_LEN
To limit the length of the string used to store the topic names and topic filters, use
the **CC_MQTT5_CLIENT_TOPIC_FIELD_FIXED_LEN** variable. When it is set to **0** (default)
the global fixed length of a string field set by the **CC_MQTT5_CLIENT_STRING_FIELD_FIXED_LEN** is used.

```
# Limit the max length of the topics
set(CC_MQTT5_CLIENT_TOPIC_FIELD_FIXED_LEN 100)
```

---
### CC_MQTT5_CLIENT_BIN_DATA_FIELD_FIXED_LEN
By default the storage type of any binary data field defined using the
[COMMS Library](https://github.com/commschamp/comms) is `std::vector<std::uint8_t>`, which
uses dynamic memory allocation. Setting the **CC_MQTT5_CLIENT_BIN_DATA_FIELD_FIXED_LEN**
to a non-**0** value will force usage of the
[comms::util::StaticVector](https://github.com/commschamp/comms/blob/master/include/comms/util/StaticVector.h)
as a storage type. It can be useful for bare-metal embedded systems without heap.
When the **CC_MQTT5_CLIENT_BIN_DATA_FIELD_FIXED_LEN** variable is set to a non-**0** value,
appropriate **comms** library option is passed to the
[cc_mqtt5::field::BinData](https://github.com/commschamp/cc.mqtt5.generated/blob/master/include/cc_mqtt5/field/BinData.h)
field.
```
# Limit the length of all the binary data fields
set (CC_MQTT5_CLIENT_BIN_DATA_FIELD_FIXED_LEN 512)
```

Having **CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC** set to **FALSE** requires setting
of the **CC_MQTT5_CLIENT_BIN_DATA_FIELD_FIXED_LEN** to a non-**0** value.

The **CC_MQTT5_CLIENT_BIN_DATA_FIELD_FIXED_LEN** variable sets a global default
for **all** the binary data fields. It is possible to refine the static length of
other binary data fields using other variables.

---
#### CC_MQTT5_CLIENT_PASSWORD_FIELD_FIXED_LEN
To limit the length of the binary data used to store the "Password" information, use
the **CC_MQTT5_CLIENT_PASSWORD_FIELD_FIXED_LEN** variable. When it is set to **0** (default)
the global fixed length of a string field set by the **CC_MQTT5_CLIENT_BIN_DATA_FIELD_FIXED_LEN** is used.

```
# Limit the max "password" length
set(CC_MQTT5_CLIENT_PASSWORD_FIELD_FIXED_LEN 50)
```

---
### CC_MQTT5_CLIENT_PROPERTIES_LIST_FIELD_FIXED_LEN
The **CC_MQTT5_CLIENT_PROPERTIES_LIST_FIELD_FIXED_LEN** variable is used to
control the storage type of the properties list. When it is set to **0** (default),
the storage type is `std::vector<...>` of an appropriate element type.
When the non-**0** value is assigned to the variable, the
[comms::util::StaticVector](https://github.com/commschamp/comms/blob/master/include/comms/util/StaticVector.h)
is used instead. It can be useful for bare-metal embedded systems without heap.

```
# Limit the amount of properties per list
set (CC_MQTT5_CLIENT_PROPERTIES_LIST_FIELD_FIXED_LEN 5)
```

Having **CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC** set to **FALSE** requires setting
of the **CC_MQTT5_CLIENT_PROPERTIES_LIST_FIELD_FIXED_LEN** to a non-**0** value.

---
### CC_MQTT5_CLIENT_MAX_OUTPUT_PACKET_SIZE
When serialiazing the output message the client library needs to allocate an output
buffer. When set to **0** (default), the output buffer type will be dynamic sized
`std::vector<std::uint8_t>`. When the non-**0** value is assigned to the variable, the
[comms::util::StaticVector](https://github.com/commschamp/comms/blob/master/include/comms/util/StaticVector.h)
is used instead. It can be useful for bare-metal embedded systems without heap.

```
# Limit the length of the buffer required to store serialized message
set (CC_MQTT5_CLIENT_MAX_OUTPUT_PACKET_SIZE 1024)
```

Having **CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC** set to **FALSE** requires setting
of the **CC_MQTT5_CLIENT_MAX_OUTPUT_PACKET_SIZE** to a non-**0** value.

---
### CC_MQTT5_CLIENT_HAS_USER_PROPS
The client library implements support for "User Properties" as defined in the MQTT5
specification. By default the feature is enabled (**CC_MQTT5_CLIENT_HAS_USER_PROPS**
defaults to **TRUE**). When the **CC_MQTT5_CLIENT_HAS_USER_PROPS** variable is set
to **FALSE** the feature will be disabled resulting is smaller code size.

```
# Disable usage of the user props
set (CC_MQTT5_CLIENT_HAS_USER_PROPS FALSE)
```

---
### CC_MQTT5_CLIENT_USER_PROPS_LIMIT
When the "User Properties" are enabled (**CC_MQTT5_CLIENT_HAS_USER_PROPS** is set to **TRUE**)
the storage type of the "User Properties" is controlled by the **CC_MQTT5_CLIENT_USER_PROPS_LIMIT**
variable. Setting the **CC_MQTT5_CLIENT_USER_PROPS_LIMIT** variable to **0** (default), means there is no limit and
the storage type will be `std::vector<...>`. When the **CC_MQTT5_CLIENT_USER_PROPS_LIMIT**
variable is set to a non-**0** value the
[comms::util::StaticVector](https://github.com/commschamp/comms/blob/master/include/comms/util/StaticVector.h)
is used instead. It can be useful for bare-metal embedded systems without heap.

```
# Limit of user props in case they are enabled
#set (CC_MQTT5_CLIENT_USER_PROPS_LIMIT 3)
```

Having **CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC** set to **FALSE** and
**CC_MQTT5_CLIENT_HAS_USER_PROPS** set to **TRUE** requires setting
of the **CC_MQTT5_CLIENT_USER_PROPS_LIMIT** to a non-**0** value.

---
### CC_MQTT5_CLIENT_RECEIVE_MAX_LIMIT
When broker publishes QoS2 messages to the client, the latter must keep the state
of the message until the exchange of the relevant control messages between
client and broker is complete. The **CC_MQTT5_CLIENT_RECEIVE_MAX_LIMIT** variable
can be used to specify a hard-coded limit to such publishes.
When the value of the **CC_MQTT5_CLIENT_RECEIVE_MAX_LIMIT**
variable is **0** (default), it means that there is no limit. In such chase the
client library uses `std::vector<...>` to store the relevant message states in memory.
When **CC_MQTT5_CLIENT_RECEIVE_MAX_LIMIT** is set to a non-**0** value the
[comms::util::StaticVector](https://github.com/commschamp/comms/blob/master/include/comms/util/StaticVector.h)
is used instead. It can be useful for bare-metal embedded systems without heap.

```
# Limit the amount of incomplete QoS2 messages being received in parallel
set (CC_MQTT5_CLIENT_RECEIVE_MAX_LIMIT 4)
```

Having **CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC** set to **FALSE** requires setting
of the **CC_MQTT5_CLIENT_RECEIVE_MAX_LIMIT** to a non-**0** value.

---
### CC_MQTT5_CLIENT_SEND_MAX_LIMIT
When publishing QoS1 and QoS2 messages to the broker, the client library needs
to preserve the relevant states of the operations in memory.
The **CC_MQTT5_CLIENT_SEND_MAX_LIMIT** variable
can be used to specify a hard-coded limit to such publishes.
When the value of the **CC_MQTT5_CLIENT_SEND_MAX_LIMIT**
variable is **0** (default), it means that there is no limit. In such chase the
client library uses `std::vector<...>` to store the relevant message states in memory.
When **CC_MQTT5_CLIENT_RECEIVE_MAX_LIMIT** is set to a non-**0** value the
[comms::util::StaticVector](https://github.com/commschamp/comms/blob/master/include/comms/util/StaticVector.h)
is used instead. It can be useful for bare-metal embedded systems without heap.

```
# Limit the amount of unacknowledged QoS1 and QoS2 messages being sent in parallel
set (CC_MQTT5_CLIENT_SEND_MAX_LIMIT 6)
```

Having **CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC** set to **FALSE** requires setting
of the **CC_MQTT5_CLIENT_SEND_MAX_LIMIT** to a non-**0** value.

---
### CC_MQTT5_CLIENT_HAS_TOPIC_ALIASES
The client library implements support for "Topic Aliases" as defined in the MQTT5
specification. By default the feature is enabled (**CC_MQTT5_CLIENT_HAS_TOPIC_ALIASES**
defaults to **TRUE**). When the **CC_MQTT5_CLIENT_HAS_TOPIC_ALIASES** variable is set
to **FALSE** the feature will be disabled resulting is smaller code size and
slightly improved runtime performance.

```
# Disable usage of topic aliases
set (CC_MQTT5_CLIENT_HAS_TOPIC_ALIASES FALSE)
```

---
### CC_MQTT5_CLIENT_TOPIC_ALIASES_LIMIT
When the "Topic Aliases" are enabled (**CC_MQTT5_CLIENT_HAS_TOPIC_ALIASES** is set to **TRUE**)
the storage type of the relevant data structures is controlled by the **CC_MQTT5_CLIENT_TOPIC_ALIASES_LIMIT**
variable. Setting the **CC_MQTT5_CLIENT_TOPIC_ALIASES_LIMIT** variable to **0** (default), means there is no limit and
the storage type will be `std::vector<...>`. When the **CC_MQTT5_CLIENT_TOPIC_ALIASES_LIMIT**
variable is set to a non-**0** value the
[comms::util::StaticVector](https://github.com/commschamp/comms/blob/master/include/comms/util/StaticVector.h)
is used instead. It can be useful for bare-metal embedded systems without heap.

```
# Limit the amount topic aliases the client is allowed to record when topic aliases are enabled
set (CC_MQTT5_CLIENT_TOPIC_ALIASES_LIMIT 10)
```

Having **CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC** set to **FALSE** and
**CC_MQTT5_CLIENT_HAS_TOPIC_ALIASES** set to **TRUE** requires setting
of the **CC_MQTT5_CLIENT_TOPIC_ALIASES_LIMIT** to a non-**0** value.

---
### CC_MQTT5_CLIENT_HAS_SUB_IDS
The client library implements support for "Subscription Identifiers" as defined in the MQTT5
specification. By default the feature is enabled (**CC_MQTT5_CLIENT_HAS_SUB_IDS**
defaults to **TRUE**). When the **CC_MQTT5_CLIENT_HAS_SUB_IDS** variable is set
to **FALSE** the feature will be disabled resulting is smaller code size.

```
# Disable usage of the "Subscription IDs"
set (CC_MQTT5_CLIENT_HAS_SUB_IDS FALSE)
```

---
### CC_MQTT5_CLIENT_SUB_IDS_LIMIT
When the "Subscription Identifiers" are enabled (**CC_MQTT5_CLIENT_HAS_SUB_IDS** is set to **TRUE**)
the storage type of the relevant data structures is controlled by the **CC_MQTT5_CLIENT_SUB_IDS_LIMIT**
variable. Setting the **CC_MQTT5_CLIENT_SUB_IDS_LIMIT** variable to **0** (default), means there is no limit and
the storage type will be `std::vector<...>`. When the **CC_MQTT5_CLIENT_SUB_IDS_LIMIT**
variable is set to a non-**0** value the
[comms::util::StaticVector](https://github.com/commschamp/comms/blob/master/include/comms/util/StaticVector.h)
is used instead. It can be useful for bare-metal embedded systems without heap.

```
# Limit the amount of allowed subcription IDs reported for a received message
set (CC_MQTT5_CLIENT_SUB_IDS_LIMIT 3)
```

Having **CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC** set to **FALSE** and
**CC_MQTT5_CLIENT_HAS_TOPIC_ALIASES** set to **TRUE** requires setting
of the **CC_MQTT5_CLIENT_TOPIC_ALIASES_LIMIT** to a non-**0** value.

---
### CC_MQTT5_CLIENT_ASYNC_SUBS_LIMIT
The client library allows issuing asynchronous subscription operation. The operation
state needs to be preserved in the memory until
the appropriate acknowledgement message is received from the broker. Setting
the **CC_MQTT5_CLIENT_ASYNC_SUBS_LIMIT** variable to **0** (default) means there
is no limit to parallel subscription operations and their relative states are
stored using `std::vector<...>` storage type.
When the **CC_MQTT5_CLIENT_ASYNC_SUBS_LIMIT**
variable is set to a non-**0** value the
[comms::util::StaticVector](https://github.com/commschamp/comms/blob/master/include/comms/util/StaticVector.h)
is used instead. It can be useful for bare-metal embedded systems without heap.

```
# Limit the amount of ongoing (unacknowledged) subscribe operations
set (CC_MQTT5_CLIENT_ASYNC_SUBS_LIMIT 3)
```

Having **CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC** set to **FALSE** requires setting
of the **CC_MQTT5_CLIENT_ASYNC_SUBS_LIMIT** to a non-**0** value.

---
### CC_MQTT5_CLIENT_ASYNC_UNSUBS_LIMIT
The client library allows issuing asynchronous unsubscription operation. The operation
state needs to be preserved in the memory until
the appropriate acknowledgement message is received from the broker. Setting
the **CC_MQTT5_CLIENT_ASYNC_UNSUBS_LIMIT** variable to **0** (default) means there
is no limit to parallel unsubscription operations and their relative states are
stored using `std::vector<...>` storage type.
When the **CC_MQTT5_CLIENT_ASYNC_UNSUBS_LIMIT**
variable is set to a non-**0** value the
[comms::util::StaticVector](https://github.com/commschamp/comms/blob/master/include/comms/util/StaticVector.h)
is used instead. It can be useful for bare-metal embedded systems without heap.

```
# Limit the amount of ongoing (unacknowledged) unsubscribe operations
set (CC_MQTT5_CLIENT_ASYNC_UNSUBS_LIMIT 1)
```

Having **CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC** set to **FALSE** requires setting
of the **CC_MQTT5_CLIENT_ASYNC_UNSUBS_LIMIT** to a non-**0** value.

---
### CC_MQTT5_CLIENT_HAS_ERROR_LOG
The client library allows reporting various error log messages via callback.
When **CC_MQTT5_CLIENT_HAS_ERROR_LOG** variable is set to **TRUE** (default) such
error reporting is enabled. Setting the **CC_MQTT5_CLIENT_HAS_ERROR_LOG** to
**FALSE** removes the error reporting functionality and as the result
reduces the code size and may slightly improve the runtime performance.

```
# Disable the error logging functionality
set (CC_MQTT5_CLIENT_HAS_ERROR_LOG FALSE)
```

---
### CC_MQTT5_CLIENT_HAS_TOPIC_FORMAT_VERIFICATION
The client library implements verification of the used topics format to be a
valid one. When the **CC_MQTT5_CLIENT_HAS_TOPIC_FORMAT_VERIFICATION** variable is
set to **TRUE** (default) the functionality is enabled and the library allows
runtime control of the feature via the API. When the **CC_MQTT5_CLIENT_HAS_TOPIC_FORMAT_VERIFICATION**
is set to **FALSE** the relevant verification code is removed by the compiler
resulting in smaller code size and improved runtime performance.

```
# Disable the topic format verification functionality
set (CC_MQTT5_CLIENT_HAS_TOPIC_FORMAT_VERIFICATION FALSE)
```

---
### CC_MQTT5_CLIENT_HAS_SUB_TOPIC_VERIFICATION
The client library implements tracking of the subscribed topics and discarding
the "rogue" messages from the broker if the received message is not supposed to
be received. When the **CC_MQTT5_CLIENT_HAS_SUB_TOPIC_VERIFICATION** variable is
set to **TRUE** (default) the functionality is enabled and the library allows
runtime control of the feature via the API. When the **CC_MQTT5_CLIENT_HAS_SUB_TOPIC_VERIFICATION**
is set to **FALSE** the relevant verification code is removed by the compiler
resulting in smaller code size and improved runtime performance.

```
# Disable the verification that the relevant subscription was performed when the message is reported from the broker
set (CC_MQTT5_CLIENT_HAS_SUB_TOPIC_VERIFICATION FALSE)
```

---
### CC_MQTT5_CLIENT_SUB_FILTERS_LIMIT
When the subscription topic verification is enabled
(**CC_MQTT5_CLIENT_HAS_SUB_TOPIC_VERIFICATION** is set to **TRUE**) the client
library needs to preserve the subscribed topics in the memory. When the
**CC_MQTT5_CLIENT_SUB_FILTERS_LIMIT** variable is set to **0** (default), it means
that there is no limit to the amount of such topics and as the result
`std::vector<...>` is used to store them in memory.
When the **CC_MQTT5_CLIENT_SUB_FILTERS_LIMIT**
variable is set to a non-**0** value the
[comms::util::StaticVector](https://github.com/commschamp/comms/blob/master/include/comms/util/StaticVector.h)
is used instead. It can be useful for bare-metal embedded systems without heap.

```
# Limit the amount of topic filters to store when the subscription verification is enabled
#set (CC_MQTT5_CLIENT_SUB_FILTERS_LIMIT 20)
```

Having **CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC** set to **FALSE** and
**CC_MQTT5_CLIENT_HAS_TOPIC_FORMAT_VERIFICATION** set to **TRUE** requires setting
of the **CC_MQTT5_CLIENT_SUB_FILTERS_LIMIT** to a non-**0** value.

---
## Example for Bare-Metal Without Heap Configuration 
The content of the custom client configuration file, which explicitly specifies
all compile time limits and constants to prevent usage of dynamic 
memory allocation and STL types like [std::string](http://en.cppreference.com/w/cpp/string/basic_string)
and [std::vector](http://en.cppreference.com/w/cpp/container/vector), may look
like [this](../client/lib/script/BareMetalConfig.cmake):

Setting "bm" as a custom client name results in having a static library called `cc_mqtt5_bm_client`.
All the API functions are defined in `cc_mqtt5_client/bm_client.h` header file:
```c
CC_Mqtt5ClientHandle cc_mqtt5_bm_client_alloc();

void cc_mqtt5_bm_client_free(CC_Mqtt5ClientHandle client);

...
    
```
