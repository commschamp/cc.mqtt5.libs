if (NOT CC_MQTT5_BUILD_INTEGRATION_TESTS)
    return ()
endif ()

##################################

find_package(Boost REQUIRED COMPONENTS system)

set (COMMON_BASE_LIB_NAME "IntegrationTestCommonBase")
set (COMMON_BASE_SRC "IntegrationTestCommonBase.cpp")
add_library(${COMMON_BASE_LIB_NAME} ${COMMON_BASE_SRC})
target_link_libraries(${COMMON_BASE_LIB_NAME} PRIVATE cc::cc_mqtt5_client Boost::system Boost::boost)


