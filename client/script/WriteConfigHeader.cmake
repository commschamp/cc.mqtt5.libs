# CMAKE_CONFIG_FILE - input cmake file
# CMAKE_DEFAULT_CONFIG_VARS - input cmake file setting default variables
# CONFIG_HEADER_TEMPL - config template file
# OUT_FILE - output header file

if ((NOT "${CMAKE_CONFIG_FILE}" STREQUAL "") AND (NOT EXISTS "${CMAKE_CONFIG_FILE}"))
    message (FATAL_ERROR "Input file \"${CMAKE_CONFIG_FILE}\" doesn't exist!")
endif ()

if (NOT EXISTS "${CMAKE_DEFAULT_CONFIG_VARS}")
    message (FATAL_ERROR "Input file \"${CMAKE_DEFAULT_CONFIG_VARS}\" doesn't exist!")
endif ()

if (NOT EXISTS "${CONFIG_HEADER_TEMPL}")
    message (FATAL_ERROR "Input file \"${CONFIG_HEADER_TEMPL}\" doesn't exist!")
endif ()

if (NOT "${CMAKE_CONFIG_FILE}" STREQUAL "")
    include (${CMAKE_CONFIG_FILE})
endif ()

file (READ ${CONFIG_HEADER_TEMPL} text)

include (${CMAKE_DEFAULT_CONFIG_VARS} NO_POLICY_SCOPE)

#########################################

macro (adjust_bool_value name adjusted_name)
    if (${${name}})
        set (${adjusted_name} "true")
    else ()
        set (${adjusted_name} "false")
    endif ()
endmacro ()

adjust_bool_value ("CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC" "CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC_CPP")
adjust_bool_value ("CC_MQTT5_CLIENT_HAS_USER_PROPS" "CC_MQTT5_CLIENT_HAS_USER_PROPS_CPP")
adjust_bool_value ("CC_MQTT5_CLIENT_HAS_TOPIC_ALIASES" "CC_MQTT5_CLIENT_HAS_TOPIC_ALIASES_CPP")

#########################################

replace_in_text (CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC_CPP)
replace_in_text (CC_MQTT5_CLIENT_ALLOC_LIMIT)
replace_in_text (CC_MQTT5_CLIENT_STRING_FIELD_FIXED_LEN)
replace_in_text (CC_MQTT5_CLIENT_PROPERTIES_LIST_FIELD_FIXED_LEN)
replace_in_text (CC_MQTT5_CLIENT_BIN_DATA_FIELD_FIXED_LEN)
replace_in_text (CC_MQTT5_CLIENT_MAX_OUTPUT_PACKET_SIZE)
replace_in_text (CC_MQTT5_CLIENT_HAS_USER_PROPS_CPP)
replace_in_text (CC_MQTT5_CLIENT_USER_PROPS_LIMIT)
replace_in_text (CC_MQTT5_CLIENT_RECEIVE_MAX_LIMIT)
replace_in_text (CC_MQTT5_CLIENT_SEND_MAX_LIMIT)
replace_in_text (CC_MQTT5_CLIENT_HAS_TOPIC_ALIASES_CPP)
replace_in_text (CC_MQTT5_CLIENT_TOPIC_ALIASES_LIMIT)
replace_in_text (CC_MQTT5_CLIENT_ASYNC_SUBS_LIMIT)


file (WRITE "${OUT_FILE}.tmp" "${text}")

execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OUT_FILE}.tmp" "${OUT_FILE}")    

execute_process(
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${OUT_FILE}.tmp")