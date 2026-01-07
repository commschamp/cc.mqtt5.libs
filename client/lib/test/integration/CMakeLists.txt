if (NOT CC_MQTT5_BUILD_INTEGRATION_TESTS)
    return ()
endif ()

find_package(Boost REQUIRED COMPONENTS system)

set (COMMON_BASE_LIB_NAME "IntegrationTestCommonBase")
set (COMMON_BASE_SRC "IntegrationTestCommonBase.cpp")
add_library(${COMMON_BASE_LIB_NAME} STATIC ${COMMON_BASE_SRC})
target_link_libraries(${COMMON_BASE_LIB_NAME} PUBLIC cc::cc_mqtt5_client Boost::system Boost::boost)
target_include_directories(
    ${COMMON_BASE_LIB_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

##################################

function (cc_mqtt5_client_add_integration_test name)
    set (src ${CMAKE_CURRENT_SOURCE_DIR}/${name}.cpp)
    add_executable(integration.${name} ${src})
    target_link_libraries(integration.${name} PRIVATE  ${COMMON_BASE_LIB_NAME} cc::cc_mqtt5_client)
    add_test(
        NAME integration.${name}
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/integration.${name})
endfunction ()

##################################

cc_mqtt5_client_add_integration_test(IntegrationTestBasicConnect)
cc_mqtt5_client_add_integration_test(IntegrationTestBasicPubSub)

