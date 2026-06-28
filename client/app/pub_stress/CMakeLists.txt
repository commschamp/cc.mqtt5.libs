if (NOT CC_MQTT5_BUILD_STRESS_TESTS)
    return ()
endif ()

set (name "cc_mqtt5_client_pub_stress")
set (src
    main.cpp
    PubStressClient.cpp
    PubStressMgr.cpp
    PubStressProgramOptions.cpp
)

add_executable(${name} ${src})
target_link_libraries(${name} ${COMMON_APPS_LIB})

install (
    TARGETS ${name}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)