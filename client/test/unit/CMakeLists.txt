if (NOT CC_MQTT5_BUILD_UNIT_TESTS)
    return ()
endif ()

##################################

function (cc_mqtt5_client_add_test name)
    set (src ${CMAKE_CURRENT_SOURCE_DIR}/${name}.th)
    cc_cxxtest_add_test (NAME ${name} SRC ${src})    
    target_link_libraries(${name} PRIVATE cc::cc_mqtt5_client cxxtest::cxxtest)
endfunction ()

##################################

cc_mqtt5_client_add_test(ConnectUnitTest)