if (NOT CC_MQTT5_BUILD_UNIT_TESTS)
    return ()
endif ()

##################################
set (COMMON_BASE_LIB_NAME "UnitTestCommonBase")
set (COMMON_BASE_SRC "UnitTestCommonBase.cpp")
add_library(${COMMON_BASE_LIB_NAME} ${COMMON_BASE_SRC})
target_link_libraries(${COMMON_BASE_LIB_NAME} PUBLIC cc::cc_mqtt5_client cc::cc_mqtt5)
target_include_directories(
    ${COMMON_BASE_LIB_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

##################################

function (cc_mqtt5_client_add_unit_test name)
    set (src ${CMAKE_CURRENT_SOURCE_DIR}/${name}.th)
    cc_cxxtest_add_test (NAME unit.${name} SRC ${src})    
    target_link_libraries(unit.${name} PRIVATE ${COMMON_BASE_LIB_NAME} cc::cc_mqtt5_client cxxtest::cxxtest)
endfunction ()

##################################

cc_mqtt5_client_add_unit_test(UnitTestClient)
cc_mqtt5_client_add_unit_test(UnitTestConnect)
cc_mqtt5_client_add_unit_test(UnitTestPublish)
cc_mqtt5_client_add_unit_test(UnitTestReceive)
cc_mqtt5_client_add_unit_test(UnitTestSubscribe)