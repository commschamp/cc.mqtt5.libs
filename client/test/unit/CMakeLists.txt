if (NOT CC_MQTT5_BUILD_UNIT_TESTS)
    return ()
endif ()

##################################
set (COMMON_BASE_LIB_NAME "UnitTestCommonBase")
set (COMMON_BASE_SRC 
    "UnitTestCommonBase.cpp")


add_library(${COMMON_BASE_LIB_NAME} ${COMMON_BASE_SRC})
target_link_libraries(${COMMON_BASE_LIB_NAME} PUBLIC cc::cc_mqtt5)
target_include_directories(
    ${COMMON_BASE_LIB_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

if (TARGET cc::cc_mqtt5_client)
    list (APPEND COMMON_BASE_SRC "UnitTestDefaultBase.cpp")
endif ()    

##################################

function (cc_mqtt5_client_add_unit_test name test_lib)
    set (src ${CMAKE_CURRENT_SOURCE_DIR}/${name}.th)
    cc_cxxtest_add_test (NAME unit.${name} SRC ${src})    
    target_link_libraries(unit.${name} PRIVATE ${test_lib} cxxtest::cxxtest)
endfunction ()

##################################

if (TARGET cc::cc_mqtt5_client)
    set (DEFAULT_BASE_LIB_NAME "UnitTestDefaultBase")
    set (DEFAULT_BASE_SRC 
        "UnitTestDefaultBase.cpp")    

    add_library(${DEFAULT_BASE_LIB_NAME} ${DEFAULT_BASE_SRC})
    target_link_libraries(${DEFAULT_BASE_LIB_NAME} PUBLIC ${COMMON_BASE_LIB_NAME} cc::cc_mqtt5_client)
    target_include_directories(
        ${DEFAULT_BASE_LIB_NAME} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    )

    cc_mqtt5_client_add_unit_test(UnitTestClient ${DEFAULT_BASE_LIB_NAME})
    cc_mqtt5_client_add_unit_test(UnitTestConnect ${DEFAULT_BASE_LIB_NAME})
    cc_mqtt5_client_add_unit_test(UnitTestDisconnect ${DEFAULT_BASE_LIB_NAME})
    cc_mqtt5_client_add_unit_test(UnitTestPublish ${DEFAULT_BASE_LIB_NAME})
    cc_mqtt5_client_add_unit_test(UnitTestReauth ${DEFAULT_BASE_LIB_NAME})
    cc_mqtt5_client_add_unit_test(UnitTestReceive ${DEFAULT_BASE_LIB_NAME})
    cc_mqtt5_client_add_unit_test(UnitTestSubscribe ${DEFAULT_BASE_LIB_NAME})
    cc_mqtt5_client_add_unit_test(UnitTestUnsubscribe ${DEFAULT_BASE_LIB_NAME})
endif ()    

if (TARGET cc::cc_mqtt5_bm_client)
    set (BM_BASE_LIB_NAME "UnitTestBmBase")
    set (BM_BASE_SRC 
        "UnitTestBmBase.cpp")    

    add_library(${BM_BASE_LIB_NAME} ${BM_BASE_SRC})
    target_link_libraries(${BM_BASE_LIB_NAME} PUBLIC ${COMMON_BASE_LIB_NAME} cc::cc_mqtt5_bm_client)
    target_include_directories(
        ${BM_BASE_LIB_NAME} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    )

    cc_mqtt5_client_add_unit_test(UnitTestBmClient ${BM_BASE_LIB_NAME})
    cc_mqtt5_client_add_unit_test(UnitTestBmConnect ${BM_BASE_LIB_NAME})
endif ()  
