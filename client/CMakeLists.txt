if ((NOT CC_MQTT5_CLIENT_DEFAULT_LIB) OR
    (CC_MQTT5_CUSTOM_CLIENT_CONFIG_FILES STREQUAL ""))
    return()
endif ()


######################################################################

set (HEADER_TEMPL ${CMAKE_CURRENT_SOURCE_DIR}/templ/client.h.templ)
set (SRC_TEMPL ${CMAKE_CURRENT_SOURCE_DIR}/templ/client.cpp.templ)
set (CONFIG_TEMPL ${CMAKE_CURRENT_SOURCE_DIR}/templ/Config.h.templ)
set (PROT_OPTS_TEMPL ${CMAKE_CURRENT_SOURCE_DIR}/templ/ProtocolOptions.h.templ)
set (TEMPL_PROCESS_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/script/ProcessTemplate.cmake)
set (WRITE_CONFIG_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/script/WriteConfigHeader.cmake)
set (WRITE_PROT_OPTS_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/script/WriteProtocolOptions.cmake)
set (DEFAULT_CONFIG_VARS_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/script/DefineDefaultConfigVars.cmake)
set (DEFAULT_CLIENT_DIR_NAME "default")
set (COMMON_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

######################################################################

function (gen_lib_mqtt5_client config_file)
    if (NOT "${config_file}" STREQUAL "")
        include (${config_file})
    endif()

    set (name "${CC_MQTT5_CLIENT_CUSTOM_NAME}")
    if ("${CC_MQTT5_CLIENT_CUSTOM_NAME}" STREQUAL "")
        set (dir ${DEFAULT_CLIENT_DIR_NAME})
        set (lib_name "cc_mqtt5_client")
    else ()
        set (dir "${CC_MQTT5_CLIENT_CUSTOM_NAME}")
        set (name "${name}_")
        set (lib_name "cc_mqtt5_${name}client")
    endif ()
    
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/${dir})    

    set (header_output ${CMAKE_CURRENT_BINARY_DIR}/${dir}/${name}client.h)
    set (src_output ${CMAKE_CURRENT_BINARY_DIR}/${dir}/${name}client.cpp)
    set (config_output ${CMAKE_CURRENT_BINARY_DIR}/${dir}/Config.h)
    set (prot_opts_output ${CMAKE_CURRENT_BINARY_DIR}/${dir}/ProtocolOptions.h)

    # ---------------------------------    
    
    add_custom_command(
        OUTPUT "${header_output}"
        COMMAND ${CMAKE_COMMAND} 
            -DIN_FILE="${HEADER_TEMPL}"
            -DOUT_FILE="${header_output}"
            -DNAME="${name}"
            -P ${TEMPL_PROCESS_SCRIPT}
        DEPENDS ${HEADER_TEMPL} ${TEMPL_PROCESS_SCRIPT}
    )
    
    set_source_files_properties(
        ${header_output}
        PROPERTIES GENERATED TRUE
    )
    
    set (header_tgt_name "${name}client.h.tgt") 
    add_custom_target(
        ${header_tgt_name}
        DEPENDS "${header_output}" ${HEADER_TEMPL} ${TEMPL_PROCESS_SCRIPT}
    )

    # ---------------------------------
    
    add_custom_command(
        OUTPUT "${src_output}"
        COMMAND ${CMAKE_COMMAND} 
            -DIN_FILE="${SRC_TEMPL}"
            -DOUT_FILE="${src_output}"
            -DNAME="${name}"
            -P ${TEMPL_PROCESS_SCRIPT}
        DEPENDS ${SRC_TEMPL} ${TEMPL_PROCESS_SCRIPT}
    )
    
    set_source_files_properties(
        ${src_output}
        PROPERTIES GENERATED TRUE
    )
    
    set (src_tgt_name "${name}client.cpp.tgt") 
    add_custom_target(
        ${src_tgt_name}
        DEPENDS "${src_output}" ${SRC_TEMPL} ${TEMPL_PROCESS_SCRIPT}
    )

    # ---------------------------------

    add_custom_command(
        OUTPUT "${config_output}"
        COMMAND ${CMAKE_COMMAND} 
            -DCMAKE_CONFIG_FILE="${config_file}"
            -DCMAKE_DEFAULT_CONFIG_VARS="${DEFAULT_CONFIG_VARS_SCRIPT}"
            -DCONFIG_HEADER_TEMPL="${CONFIG_TEMPL}"
            -DOUT_FILE="${config_output}"
            -P ${WRITE_CONFIG_SCRIPT}
        DEPENDS ${config_file} ${CONFIG_TEMPL} ${WRITE_CONFIG_SCRIPT}
    )
    
    set_source_files_properties(
        ${config_output}
        PROPERTIES GENERATED TRUE
    )
    
    set (config_tgt_name "${name}Config.h.tgt") 
    add_custom_target(
        ${config_tgt_name}
        DEPENDS "${config_output}" ${CONFIG_TEMPL} ${WRITE_CONFIG_SCRIPT}
    )    

    # ---------------------------------

    add_custom_command(
        OUTPUT "${prot_opts_output}"
        COMMAND ${CMAKE_COMMAND} 
            -DCMAKE_CONFIG_FILE="${config_file}"
            -DCMAKE_DEFAULT_CONFIG_VARS="${DEFAULT_CONFIG_VARS_SCRIPT}"
            -DPROT_OPTS_HEADER_TEMPL="${PROT_OPTS_TEMPL}"
            -DOUT_FILE="${prot_opts_output}"
            -P ${WRITE_PROT_OPTS_SCRIPT}
        DEPENDS ${config_file} ${PROT_OPTS_TEMPL} ${WRITE_PROT_OPTS_SCRIPT}
    )
    
    set_source_files_properties(
        ${prot_opts_output}
        PROPERTIES GENERATED TRUE
    )
    
    set (prot_opts_tgt_name "${name}ProtocolOptions.h.tgt") 
    add_custom_target(
        ${prot_opts_tgt_name}
        DEPENDS "${prot_opts_output}" ${PROT_OPTS_TEMPL} ${WRITE_PROT_OPTS_SCRIPT}
    )    

    # ---------------------------------    
    
    message (STATUS "Defining library ${lib_name}") 
    set (src
        src/op/ConnectOp.cpp
        src/op/Op.cpp
        src/Client.cpp
        src/TimerMgr.cpp
    )
    add_library (${lib_name} STATIC ${src} ${src_output})
    add_library (cc::${lib_name} ALIAS ${lib_name})
    target_link_libraries(${lib_name} PRIVATE cc::cc_mqtt5 cc::comms)
    target_include_directories(
            ${lib_name} BEFORE
                PUBLIC
                    $<INSTALL_INTERFACE:include>
                    $<BUILD_INTERFACE:${COMMON_INC_DIR}>
                PRIVATE 
                    ${CMAKE_CURRENT_BINARY_DIR}/${dir}
                    ${CMAKE_CURRENT_SOURCE_DIR}/src)    

    set_target_properties(
        ${lib_name} PROPERTIES 
        INTERFACE_LINK_LIBRARIES ""
    )
    add_dependencies(${lib_name} ${header_tgt_name} ${src_tgt_name} ${config_tgt_name} ${prot_opts_tgt_name})
    
    if (NOT "${extra_flags}" STREQUAL "")
        string(REPLACE ";" " " extra_flags "${extra_flags}")
    
        set_target_properties(
            ${lib_name} PROPERTIES 
            COMPILE_FLAGS ${extra_flags}
        )
    endif ()

    install (
        FILES ${header_output}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cc_mqtt5_client
    )
    
    install (
        TARGETS ${lib_name}
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        EXPORT ${lib_name}Config
    )

    install(EXPORT ${lib_name}Config NAMESPACE cc::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/${lib_name}/cmake
    )        
endfunction()

######################################################################

install (
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/cc_mqtt5_client
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if (CC_MQTT5_CLIENT_DEFAULT_LIB) 
    gen_lib_mqtt5_client("")
endif ()