if (CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
    set(TEST_BUILD_TYPE --test)
elseif (CMAKE_BUILD_TYPE STREQUAL Debug)
    set(TEST_BUILD_TYPE --debug)
endif ()

if (NO_ICU)
    set(TEST_ICU --not-tag exclude_noicu)
elseif (NOT EMBED_ICU)
    set(TEST_ICU --not-tag exclude_icu62AndAboveTestFailures)
endif()

if (BuildJIT)
    set(VARIANT --jit)
else ()
    set(TEST_VARIANT --variants=disable_jit)
    set(VARIANT --noJit)
endif()

if (CMAKE_BUILD_TYPE STREQUAL Release)
    set(TEST_SUITE echo "Running Hello World test" && ${CMAKE_BINARY_DIR}/ch ${CMAKE_CURRENT_SOURCE_DIR}/Basics/hello.js)
else()
    set(BYTE_CODE_CHECK ${CMAKE_CURRENT_SOURCE_DIR}/../tools/regenByteCode.py ${VARIANT} --verify --binary=${CMAKE_BINARY_DIR}/ch)
    set(TEST_SUITE ${CMAKE_CURRENT_SOURCE_DIR}/runtests.py ${TEST_BUILD_TYPE} ${TEST_ICU} ${TEST_VARIANT} --binary=${CMAKE_BINARY_DIR}/ch)
endif()

if (NOT NO_ICU AND NOT EMBED_ICU)
    set(NATIVE_TEST cd ${CMAKE_CURRENT_SOURCE_DIR}/native-tests && ./test_native.sh ${CMAKE_BINARY_DIR}/ch ${CMAKE_BUILD_TYPE} ${CMAKE_C_COMPILER} ${CMAKE_CXX_COMPILER} ${CMAKE_SOURCE_DIR})
else()
    set(NATIVE_TEST echo "Native tests NOT RUN, they require ICU to be installed not embedded. NO_ICU = ${NO_ICU} EMBED_ICU = ${EMBED_ICU}")
endif()

add_custom_target(check
    COMMAND ${BYTE_CODE_CHECK}
    COMMAND ${TEST_SUITE}
    COMMAND ${NATIVE_TEST}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    USES_TERMINAL
    DEPENDS ch
    )

if (NOT STATIC_LIBRARY)
    add_dependencies(check ChakraCore)
endif()

