if (CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
    set(TEST_BUILD_TYPE --test)
elseif (CMAKE_BUILD_TYPE STREQUAL Debug)
    set(TEST_BUILD_TYPE --debug)
endif ()

if (NO_ICU)
    set(TEST_EXCLUDE --not-tag exclude_noicu)
endif()

add_custom_target(check
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/runtests.py ${TEST_BUILD_TYPE} ${TEST_EXCLUDE} --binary ${CMAKE_BINARY_DIR}/ch --logfile ${CMAKE_BINARY_DIR}/check.log
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    USES_TERMINAL
    DEPENDS ch
    )

