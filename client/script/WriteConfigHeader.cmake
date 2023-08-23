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

string (REPLACE "##CC_MQTT5_CLIENT_ALLOC_LIMIT##" "${CC_MQTT5_CLIENT_ALLOC_LIMIT}" text "${text}")

file (WRITE "${OUT_FILE}.tmp" "${text}")

execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OUT_FILE}.tmp" "${OUT_FILE}")    

