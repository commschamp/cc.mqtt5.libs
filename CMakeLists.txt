cmake_minimum_required (VERSION 3.10)
project ("cc_mqtt5_libs")

# Options
option (CC_MQTT5_CLIENT_DEFAULT_LIB "Build and install default variant of MQTT5 client library" ON)
option (CC_MQTT5_WARN_AS_ERR "Do NOT treat warning as error" ON)
option (CC_MQTT5_USE_CCACHE "Use ccache on unix system" ON)

# Extra Configuration Variables
# CC_MQTT5_CUSTOM_CLIENT_CONFIG_FILES - List of custom client configuration files

# Other variables
set(CMAKE_CXX_STANDARD 17 CACHE STRING "The C++ standard to use")

##########################################################################

# Dependencies
find_package(LibComms REQUIRED)
find_package(cc_mqtt5 REQUIRED)

set (compile_opts)
if (CC_MQTT5_USE_CCACHE)
    list (APPEND compile_opts USE_CCACHE)
endif ()

if (CC_MQTT5_WARN_AS_ERR)
    list (APPEND compile_opts WARN_AS_ERR)
endif ()

#if (CC_MQTT5_WITH_SANITIZERS)
#    list (APPEND compile_opts DEFAULT_SANITIZERS)

#    if (CMAKE_COMPILER_IS_GNUCC)
#        # gcc gives false pasitive on vptr sanitizer
#        list (APPEND compile_opts EXTRA -fno-sanitize=vptr)
#    endif ()
#endif ()

if (EXISTS ${LibComms_DIR}/CC_Compile.cmake)
    include (${LibComms_DIR}/CC_Compile.cmake)
    cc_compile(${compile_opts})
else ()
    message (WARNING "Unexpected COMMS cmake scripts installation path, cannot reuse compilation options")
endif ()

include (GNUInstallDirs)
add_subdirectory(client)