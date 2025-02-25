find_package (OpenSSL QUIET)
if (TARGET OpenSSL::SSL)
    find_package (OpenSSL REQUIRED)
endif ()

set (src
    AppClient.cpp
    ProgramOptions.cpp
    Session.cpp
    TcpSession.cpp
    TlsSession.cpp
)

add_library(${COMMON_APPS_LIB} STATIC ${src})
target_link_libraries(${COMMON_APPS_LIB} PUBLIC cc::cc_mqtt5_client Boost::system Boost::program_options ${EXTRA_BOOST_TARGETS} ${CMAKE_THREAD_LIBS_INIT})

if (TARGET OpenSSL::SSL)
    target_link_libraries (${COMMON_APPS_LIB} PRIVATE OpenSSL::SSL)
    target_compile_definitions(${COMMON_APPS_LIB} PUBLIC CC_MQTT5_CLIENT_APP_HAS_OPENSSL)
endif ()

target_include_directories(
    ${COMMON_APPS_LIB} BEFORE
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)    

target_compile_options(${COMMON_APPS_LIB} PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>:/D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS>
) 
