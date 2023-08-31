# CMAKE_CONFIG_FILE - input cmake file
# CMAKE_DEFAULT_CONFIG_VARS - input cmake file setting default variables
# PROT_OPTS_HEADER_TEMPL - protocol options template file
# OUT_FILE - output header file

if ((NOT "${CMAKE_CONFIG_FILE}" STREQUAL "") AND (NOT EXISTS "${CMAKE_CONFIG_FILE}"))
    message (FATAL_ERROR "Input file \"${CMAKE_CONFIG_FILE}\" doesn't exist!")
endif ()

if (NOT EXISTS "${CMAKE_DEFAULT_CONFIG_VARS}")
    message (FATAL_ERROR "Input file \"${CMAKE_DEFAULT_CONFIG_VARS}\" doesn't exist!")
endif ()

if (NOT EXISTS "${PROT_OPTS_HEADER_TEMPL}")
    message (FATAL_ERROR "Input file \"${PROT_OPTS_HEADER_TEMPL}\" doesn't exist!")
endif ()

if (NOT "${CMAKE_CONFIG_FILE}" STREQUAL "")
    include (${CMAKE_CONFIG_FILE})
endif ()

file (READ ${PROT_OPTS_HEADER_TEMPL} text)

include (${CMAKE_DEFAULT_CONFIG_VARS} NO_POLICY_SCOPE)

#########################################

macro (set_default_opt name)
    set (${name} "comms::option::EmptyOption")
endmacro()

#########################################

set_default_opt (FIELD_BIN_DATA)
set_default_opt (FIELD_PROPERTIES_LIST)
set_default_opt (FIELD_PROTOCOL_NAME)
set_default_opt (FIELD_STRING)

#########################################

# Update options

if (NOT ${CC_MQTT5_CLIENT_BIN_DATA_FIELD_FIXED_LEN} EQUAL 0)
    set (FIELD_BIN_DATA "comms::option::app::FixedSizeStorage<${CC_MQTT5_CLIENT_BIN_DATA_FIELD_FIXED_LEN}>")
endif ()

if (NOT ${CC_MQTT5_CLIENT_PROPERTIES_LIST_FIELD_FIXED_LEN} EQUAL 0)
    set (FIELD_PROPERTIES_LIST "comms::option::app::FixedSizeStorage<${CC_MQTT5_CLIENT_PROPERTIES_LIST_FIELD_FIXED_LEN}>")
endif ()

if (NOT ${CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC})
    set (FIELD_PROTOCOL_NAME "comms::option::app::FixedSizeStorage<4>")
endif ()

if (NOT ${CC_MQTT5_CLIENT_STRING_FIELD_FIXED_LEN} EQUAL 0)
    set (FIELD_STRING "comms::option::app::FixedSizeStorage<${CC_MQTT5_CLIENT_STRING_FIELD_FIXED_LEN}>")
endif ()

#########################################

string (REPLACE "##FIELD_BIN_DATA##" "${FIELD_BIN_DATA}" text "${text}")
string (REPLACE "##FIELD_PROPERTIES_LIST##" "${FIELD_PROPERTIES_LIST}" text "${text}")
string (REPLACE "##FIELD_PROTOCOL_NAME##" "${FIELD_PROTOCOL_NAME}" text "${text}")
string (REPLACE "##FIELD_STRING##" "${FIELD_STRING}" text "${text}")

file (WRITE "${OUT_FILE}.tmp" "${text}")

execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OUT_FILE}.tmp" "${OUT_FILE}")    

execute_process(
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${OUT_FILE}.tmp")      

