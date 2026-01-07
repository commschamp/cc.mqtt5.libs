cmake_minimum_required (VERSION 3.10)
project ("cc_mqtt5_libs")

# Options
option (CC_MQTT5_CLIENT_DEFAULT_LIB "Build and install default variant of MQTT5 client library" ON)
option (CC_MQTT5_CLIENT_LIB_FORCE_PIC "Force Position Independent Code (PIC) when compiling client library(ies)" OFF)
option (CC_MQTT5_CLIENT_APPS "Build and install client applications" ${CC_MQTT5_CLIENT_DEFAULT_LIB})
option (CC_MQTT5_CLIENT_APPS_WITH_OPENSSL "Allow OpenSSL based functionality when building apps" ON)
option (CC_MQTT5_CLIENT_AFL_FUZZ "Build and install client AFL++ fuzzing application" OFF)
option (CC_MQTT5_WARN_AS_ERR "Treat warning as error" ON)
option (CC_MQTT5_USE_CCACHE "Use ccache" OFF)
option (CC_MQTT5_BUILD_UNIT_TESTS "Build unit tests" OFF)
option (CC_MQTT5_BUILD_INTEGRATION_TESTS "Build integration tests which require MQTT broker on local port 1883." OFF)
option (CC_MQTT5_WITH_DEFAULT_SANITIZERS "Build with sanitizers" OFF)

# CMake built-in options
option (BUILD_SHARED_LIBS "Build as shared library" OFF)

# Extra Configuration Variables
# CC_MQTT5_CUSTOM_CLIENT_CONFIG_FILES - List of custom client configuration files
# CC_MQTT5_CCACHE_EXECUTABLE - Custom ccache executable

# Other variables
set(CMAKE_CXX_STANDARD 17 CACHE STRING "The C++ standard to use")


##########################################################################

cmake_policy(SET CMP0079 NEW)

if (("${CMAKE_VERSION}" VERSION_GREATER_EQUAL "3.30") AND
    (NOT DEFINED CMAKE_POLICY_DEFAULT_CMP0167))
    # Find boost cmake configuration from the boost installation
    cmake_policy(SET CMP0167 NEW)
endif ()

set (CC_MQTT5_EXTERNALS_DIR "${PROJECT_SOURCE_DIR}/externals")

# Dependencies
find_package(LibComms REQUIRED)
find_package(cc_mqtt5 REQUIRED)

include (${PROJECT_SOURCE_DIR}/cmake/Compile.cmake)
cc_mqtt5_compile ()

while (TRUE)
    if ((NOT CC_MQTT5_BUILD_UNIT_TESTS) AND (NOT CC_MQTT5_BUILD_INTEGRATION_TESTS))
        break()
    endif ()

    message (STATUS "Unit / Integration testts are enabled")

    if (NOT EXISTS ${LibComms_DIR}/CC_CxxtestFuncs.cmake)
        message (WARNING "Unexpected COMMS cmake scripts installation path, cannot test")
        break ()
    endif ()

    include (${LibComms_DIR}/CC_CxxtestFuncs.cmake)
    cc_get_cxxtest(INSTALL_PREFIX ${CC_MQTT5_EXTERNALS_DIR} TAG "master")
    # BUILD_TESTING should be set at this point
    break()
endwhile ()


include (GNUInstallDirs)
add_subdirectory(client)