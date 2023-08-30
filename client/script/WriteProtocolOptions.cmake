# CMAKE_CONFIG_FILE - input cmake file
# PROT_OPTS_HEADER_TEMPL - protocol options template file
# OUT_FILE - output header file

if (NOT EXISTS "${CMAKE_CONFIG_FILE}")
    message (FATAL_ERROR "Input file \"${CMAKE_CONFIG_FILE}\" doesn't exist!")
endif ()

if (NOT EXISTS "${PROT_OPTS_HEADER_TEMPL}")
    message (FATAL_ERROR "Input file \"${PROT_OPTS_HEADER_TEMPL}\" doesn't exist!")
endif ()

include (${CMAKE_CONFIG_FILE})
file (READ ${PROT_OPTS_HEADER_TEMPL} text)

cmake_policy(SET CMP0012 NEW)

#########################################

function (verify_variable_set name)
    if ("${${name}}" STREQUAL "")
        message (FATAL_ERROR "Variable \"${name}\" is not set, refer to the configuration documentation.")
    endif ()
endfunction()

macro (set_default_opt name)
    set (${name} "comms::option::EmptyOption")
endmacro()

verify_variable_set("CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC")

#########################################

set_default_opt (FIELD_BIN_DATA)
set_default_opt (FIELD_PROPERTIES_LIST)
set_default_opt (FIELD_PROTOCOL_NAME)
set_default_opt (FIELD_STRING)

#########################################

# TODO: update options

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

