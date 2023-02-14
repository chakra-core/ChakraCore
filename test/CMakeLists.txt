find_package (Python COMPONENTS Interpreter)

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

# Target to run all tests
add_custom_target(check)

if (CMAKE_BUILD_TYPE STREQUAL Release)
    add_custom_target(smoke-check
        COMMAND echo "Running Hello World test"
        COMMAND ${CMAKE_BINARY_DIR}/ch ${CMAKE_CURRENT_SOURCE_DIR}/Basics/hello.js
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        USES_TERMINAL
        DEPENDS ch
        )
    add_dependencies(check smoke-check)
else()
    add_custom_target(bytecode-check
        COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../tools/regenByteCode.py ${VARIANT} --verify --binary=${CMAKE_BINARY_DIR}/ch
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        USES_TERMINAL
        DEPENDS ch
        )
    add_custom_target(regression-check
        COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/runtests.py ${TEST_BUILD_TYPE} ${TEST_ICU} ${TEST_VARIANT} --binary=${CMAKE_BINARY_DIR}/ch
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        USES_TERMINAL
        DEPENDS ch
        )
    add_dependencies(check bytecode-check regression-check)
endif()

if (NOT NO_ICU AND NOT EMBED_ICU)
    add_custom_target(native-check
        COMMAND ./test_native.sh ${CMAKE_BINARY_DIR}/ch ${CMAKE_BUILD_TYPE} ${CMAKE_C_COMPILER} ${CMAKE_CXX_COMPILER} ${CMAKE_SOURCE_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/native-tests
        USES_TERMINAL
        DEPENDS ch
        )
    add_dependencies(check native-check)
else()
    set(ICU_MESSAGE "Native tests are DISABLED, as they require ICU to be installed, and not embedded. NO_ICU = ${NO_ICU} EMBED_ICU = ${EMBED_ICU}")
    message(WARNING ${ICU_MESSAGE})
    add_custom_target(warn-icu-tests
        COMMAND echo "${ICU_MESSAGE}"
        USES_TERMINAL
        )
    add_dependencies(check warn-icu-tests)
endif()

if (NOT STATIC_LIBRARY)
    add_dependencies(check ChakraCore)
endif()

