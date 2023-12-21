macro (cc_mqttsn_compile)
    set (compile_opts)
    if (CC_MQTT5_USE_CCACHE)
        list (APPEND compile_opts USE_CCACHE)
    endif ()

    if (CC_MQTT5_WARN_AS_ERR)
        list (APPEND compile_opts WARN_AS_ERR)
    endif ()

    if (CC_MQTT5_WITH_DEFAULT_SANITIZERS)
        list (APPEND compile_opts DEFAULT_SANITIZERS)
    endif ()

    if (EXISTS ${LibComms_DIR}/CC_Compile.cmake)
        include (${LibComms_DIR}/CC_Compile.cmake)
        cc_compile(${compile_opts})
    else ()
        message (WARNING "Unexpected COMMS cmake scripts installation path, cannot reuse compilation options")
    endif ()

    set (extra_flags_list)
    if ((CMAKE_COMPILER_IS_GNUCC OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")) AND
        (NOT "${CC_MQTT5_CUSTOM_CLIENT_CONFIG_FILES}" STREQUAL ""))
        # When features are disabled some functions may remain unused
        list (APPEND extra_flags_list
            "-Wno-unused-function"
        )
    endif ()

    if (("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang") AND ("${CMAKE_CXX_STANDARD}" STREQUAL "20"))
        list (APPEND extra_flags_list
            "-Wno-tautological-constant-out-of-range-compare"
        )
    endif ()

    if (CC_MQTT5_CLIENT_AFL_FUZZ AND 
        (CMAKE_COMPILER_IS_GNUCC OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")))
        list (APPEND extra_flags_list
            "-Wno-old-style-cast"
        )
    endif()

    if (CC_MQTT5_DISABLE_FALSE_POSITIVE_SANITIZERS)
        list (APPEND extra_flags_list
            -fno-sanitize-address-use-after-scope)        
    endif ()    

    string(REPLACE ";" " " extra_flags "${extra_flags_list}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${extra_flags}")
endmacro()