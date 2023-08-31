cmake_policy(SET CMP0012 NEW)

macro (set_default_var_value name val)
    if (NOT DEFINED ${name})
        set (${name} ${val})
    endif ()
endmacro()

set_default_var_value(CC_MQTT5_CLIENT_CUSTOM_NAME "")
set_default_var_value(CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC TRUE)
set_default_var_value(CC_MQTT5_CLIENT_ALLOC_LIMIT 0)
set_default_var_value(CC_MQTT5_CLIENT_STRING_FIELD_FIXED_LEN 0)
set_default_var_value(CC_MQTT5_CLIENT_PROPERTIES_LIST_FIELD_FIXED_LEN 0)
set_default_var_value(CC_MQTT5_CLIENT_BIN_DATA_FIELD_FIXED_LEN 0)