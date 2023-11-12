set (name "cc_mqtt5_client_pub")
set (src
    main.cpp
    Pub.cpp
)

add_executable(${name} ${src})
target_link_libraries(${name} ${COMMON_APPS_LIB})

install (
    TARGETS ${name}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)