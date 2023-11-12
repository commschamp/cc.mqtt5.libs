set (src
    AppClient.cpp
    ProgramOptions.cpp
)

add_library(${COMMON_APPS_LIB} STATIC ${src})
target_link_libraries(${COMMON_APPS_LIB} PUBLIC cc::cc_mqtt5_client Boost::system Boost::program_options)
target_include_directories(
    ${COMMON_APPS_LIB} BEFORE
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)    