if ((NOT CC_MQTT5_CLIENT_DEFAULT_LIB) OR
    (CC_MQTT5_CUSTOM_CLIENT_CONFIG_FILES STREQUAL ""))
    return()
endif ()


######################################################################

set (HEADER_TEMPL ${CMAKE_CURRENT_SOURCE_DIR}/templ/client.h.templ)
set (SRC_TEMPL ${CMAKE_CURRENT_SOURCE_DIR}/templ/client.cpp.templ)
set (TEMPL_PROCESS_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/script/ProcessTemplate.cmake)
set (DEFAULT_CLIENT_NAME "")
set (DEFAULT_CLIENT_DIR_NAME "default")
set (COMMON_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

######################################################################

function (gen_lib_mqtt5_client name)
    if ("${name}" STREQUAL "${DEFAULT_CLIENT_NAME}")
        set (dir ${DEFAULT_CLIENT_DIR_NAME})
        set (lib_name "cc_mqtt5_client")
    else ()
        set (dir "${name}")
        set (name "${name}_")
        set (lib_name "cc_mqtt5_${name}client")
    endif ()
    
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/${dir})    
    
    set (header_output ${CMAKE_CURRENT_BINARY_DIR}/${dir}/${name}client.h)
    set (src_output ${CMAKE_CURRENT_BINARY_DIR}/${dir}/${name}client.cpp)
    
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
    
    message (STATUS "Defining library ${lib_name}") 
    add_library (${lib_name} STATIC ${src_output})
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
    add_dependencies(${lib_name} ${header_tgt_name} ${src_tgt_name})
    
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
    gen_lib_mqtt5_client("${DEFAULT_CLIENT_NAME}")
endif ()