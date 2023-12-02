# Name of the client API
set (CC_MQTT5_CLIENT_CUSTOM_NAME "bm")

# Exclude dynamic memory allocation
set (CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC FALSE)

# Allow only a single client
set(CC_MQTT5_CLIENT_ALLOC_LIMIT 1)

# Limit the length of all the string fields
set(CC_MQTT5_CLIENT_STRING_FIELD_FIXED_LEN 256)

# Limit the max "client ID" length
set(CC_MQTT5_CLIENT_CLIENT_ID_FIELD_FIXED_LEN 50)

# Limit the max "username" length
set(CC_MQTT5_CLIENT_USERNAME_FIELD_FIXED_LEN 20)

# Limit the max length of the topics
#set(CC_MQTT5_CLIENT_TOPIC_FIELD_FIXED_LEN 256)

# Limit the length of all the binary data fields
set (CC_MQTT5_CLIENT_BIN_DATA_FIELD_FIXED_LEN 512)

# Limit the max "password" length
set(CC_MQTT5_CLIENT_PASSWORD_FIELD_FIXED_LEN 50)

# Limit the amount of properties per list
set (CC_MQTT5_CLIENT_PROPERTIES_LIST_FIELD_FIXED_LEN 5)

# Limit the length of the buffer required to store serialized message
set (CC_MQTT5_CLIENT_MAX_OUTPUT_PACKET_SIZE 1024)

# Disable usage of the user props
set (CC_MQTT5_CLIENT_HAS_USER_PROPS FALSE)

# Limit of user props in case they are enaled
#set (CC_MQTT5_CLIENT_USER_PROPS_LIMIT 3) 

# Limit the amount of incomplete QoS2 messages being received in parallel
set (CC_MQTT5_CLIENT_RECEIVE_MAX_LIMIT 4)

# Limit the amount of unacknowledged QoS1 and QoS2 messages being sent in parallel
set (CC_MQTT5_CLIENT_SEND_MAX_LIMIT 6)

# Disable usage of topic aliases
set (CC_MQTT5_CLIENT_HAS_TOPIC_ALIASES FALSE)

# Limit the amount topic aliases the client is allowed to record when topic aliases are enabled
#set (CC_MQTT5_CLIENT_TOPIC_ALIASES_LIMIT 10)

# Disable usage of the "Subscription IDs"
set (CC_MQTT5_CLIENT_HAS_SUB_IDS FALSE)

# Limit the amount of allowed subcription IDs reported for a received message
#set (CC_MQTT5_CLIENT_SUB_IDS_LIMIT 3)

# Limit the amount of ongoing (unacknowledged) subscribe operations
set (CC_MQTT5_CLIENT_ASYNC_SUBS_LIMIT 3)

# Limit the amount of ongoing (unacknowledged) unsubscribe operations
set (CC_MQTT5_CLIENT_ASYNC_UNSUBS_LIMIT 1)

# Disable the error logging functionality 
set (CC_MQTT5_CLIENT_HAS_ERROR_LOG FALSE)

# Disable the topic format verification functionality
set (CC_MQTT5_CLIENT_HAS_TOPIC_FORMAT_VERIFICATION FALSE)

# Disable the verification that the relevant subscription was performed when the message is reported from the broker
set (CC_MQTT5_CLIENT_HAS_SUB_TOPIC_VERIFICATION FALSE)

# Limit the amount of topic filters to store when the subscription verification is enabled
#set (CC_MQTT5_CLIENT_SUB_FILTERS_LIMIT 20)