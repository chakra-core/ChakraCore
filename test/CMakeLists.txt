if (CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
    set(TEST_BUILD_TYPE --test)
elseif (CMAKE_BUILD_TYPE STREQUAL Debug)
    set(TEST_BUILD_TYPE --debug)
endif ()

if (NO_ICU)
    set(TEST_ICU --not-tag exclude_noicu)
endif()

if (BuildJIT)
    set(VARIANT --jit)
else ()
    set(TEST_VARIANT --variants=disable_jit)
    set(VARIANT --noJit)
endif()

add_custom_target(check
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../tools/regenByteCode.py ${VARIANT} --verify --binary=${CMAKE_BINARY_DIR}/ch
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/runtests.py ${TEST_BUILD_TYPE} ${TEST_ICU} ${TEST_VARIANT} --binary ${CMAKE_BINARY_DIR}/ch
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    USES_TERMINAL
    DEPENDS ch
    )

if (NOT STATIC_LIBRARY)
    add_dependencies(check ChakraCore)
endif()

