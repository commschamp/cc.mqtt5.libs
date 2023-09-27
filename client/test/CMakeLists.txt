if (NOT BUILD_TESTING)
    # testing is disabled
    return ()
endif ()

add_subdirectory(unit)
add_subdirectory(integration)