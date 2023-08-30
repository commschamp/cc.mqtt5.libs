# CMAKE_CONFIG_FILE - input cmake file
# CONFIG_HEADER_TEMPL - config template file
# OUT_FILE - output header file

if (NOT EXISTS "${CMAKE_CONFIG_FILE}")
    message (FATAL_ERROR "Input file \"${CMAKE_CONFIG_FILE}\" doesn't exist!")
endif ()

if (NOT EXISTS "${CONFIG_HEADER_TEMPL}")
    message (FATAL_ERROR "Input file \"${CONFIG_HEADER_TEMPL}\" doesn't exist!")
endif ()

include (${CMAKE_CONFIG_FILE})
file (READ ${CONFIG_HEADER_TEMPL} text)

cmake_policy(SET CMP0012 NEW)

#########################################

function (verify_variable_set name)
    if ("${${name}}" STREQUAL "")
        message (FATAL_ERROR "Variable \"${name}\" is not set, refer to the configuration documentation.")
    endif ()
endfunction()

macro (adjust_bool_value name adjusted_name)
    if (${${name}})
        set (${adjusted_name} "true")
    else ()
        set (${adjusted_name} "false")
    endif ()
endmacro ()

verify_variable_set("CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC")
verify_variable_set("CC_MQTT5_CLIENT_ALLOC_LIMIT")

adjust_bool_value ("CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC" "CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC_CPP")

#########################################

string (REPLACE "##CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC##" "${CC_MQTT5_CLIENT_HAS_DYN_MEM_ALLOC_CPP}" text "${text}")
string (REPLACE "##CC_MQTT5_CLIENT_ALLOC_LIMIT##" "${CC_MQTT5_CLIENT_ALLOC_LIMIT}" text "${text}")

file (WRITE "${OUT_FILE}.tmp" "${text}")

execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OUT_FILE}.tmp" "${OUT_FILE}")    

execute_process(
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${OUT_FILE}.tmp")