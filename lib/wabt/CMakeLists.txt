#
# Copyright 2016 WebAssembly Community Group participants
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 2.6)
project(WABT)

option(BUILD_TESTS "Build GTest-based tests" ON)
option(RUN_BISON "Run bison" ON)
option(RUN_RE2C "Run re2c" ON)
option(USE_ASAN "Use address sanitizer" OFF)
option(USE_MSAN "Use memory sanitizer" OFF)
option(USE_LSAN "Use leak sanitizer" OFF)
option(USE_UBSAN "Use undefined behavior sanitizer" OFF)
option(CODE_COVERAGE "Build with code coverage enabled" OFF)

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
  set(COMPILER_IS_CLANG 1)
  set(COMPILER_IS_GNU 0)
  set(COMPILER_IS_MSVC 0)
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
  set(COMPILER_IS_CLANG 0)
  set(COMPILER_IS_GNU 1)
  set(COMPILER_IS_MSVC 0)
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
  set(COMPILER_IS_CLANG 0)
  set(COMPILER_IS_GNU 0)
  set(COMPILER_IS_MSVC 1)
elseif ("${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
  set(COMPILER_IS_CLANG 1)
  set(COMPILER_IS_GNU 0)
  set(COMPILER_IS_MSVC 0)
else ()
  set(COMPILER_IS_CLANG 0)
  set(COMPILER_IS_GNU 0)
  set(COMPILER_IS_MSVC 0)
endif ()

include(CheckIncludeFile)
include(CheckSymbolExists)

check_include_file("alloca.h" HAVE_ALLOCA_H)
check_include_file("unistd.h" HAVE_UNISTD_H)
check_symbol_exists(snprintf "stdio.h" HAVE_SNPRINTF)
check_symbol_exists(sysconf "unistd.h" HAVE_SYSCONF)
check_symbol_exists(strcasecmp "strings.h" HAVE_STRCASECMP)

if (EMSCRIPTEN)
  set(SIZEOF_SSIZE_T 4)
  set(SIZEOF_SIZE_T 4)
  set(SIZEOF_INT 4)
  set(SIZEOF_LONG 4)
  set(SIZEOF_LONG_LONG 8)
else ()
  include(CheckTypeSize)
  check_type_size(ssize_t SSIZE_T)
  check_type_size(size_t SIZEOF_SIZE_T)
  check_type_size(int SIZEOF_INT BUILTIN_TYPES_ONLY)
  check_type_size(long SIZEOF_LONG BUILTIN_TYPES_ONLY)
  check_type_size("long long" SIZEOF_LONG_LONG BUILTIN_TYPES_ONLY)
endif ()

configure_file(
  ${WABT_SOURCE_DIR}/src/config.h.in
  ${WABT_BINARY_DIR}/config.h
)

include_directories(src ${WABT_BINARY_DIR})

if (COMPILER_IS_MSVC)
  # disable warning C4018: signed/unsigned mismatch
  # disable warning C4056, C4756: overflow in floating-point constant arithmetic
  #   seems to not like float compare w/ HUGE_VALF; bug?
  add_definitions(-W3 -wd4018 -wd4056 -wd4756 -D_CRT_SECURE_NO_WARNINGS)
else ()
  # disable -Wunused-parameter: this is really common when implementing
  #   interfaces, etc.
  # disable -Wpointer-arith: this is a GCC extension, and doesn't work in MSVC.
  add_definitions(
    -Wall -Wextra -Werror -Wno-unused-parameter -Wpointer-arith -g -std=c++11
    -Wold-style-cast
  )

  if (MINGW)
    # Mingw won't define format (e.g. PRIu64) or limit (e.g. UINT32_MAX) macros
    # in C++ without these.
    add_definitions(-D__STDC_LIMIT_MACROS=1 -D__STDC_FORMAT_MACROS=1)
  endif ()

  if (COMPILER_IS_GNU)
    # disable -Wclobbered: it seems to be guessing incorrectly about a local
    # variable being clobbered by longjmp.
    add_definitions(-Wno-clobbered)
  endif ()

  if (COMPILER_IS_CLANG)
    add_definitions(-fcolor-diagnostics)
  endif ()

  if (NOT EMSCRIPTEN)
    # try to get the target architecture by compiling a dummy.c file and
    # checking the architecture using the file command.
    file(WRITE ${WABT_BINARY_DIR}/dummy.c "main(){}")
    try_compile(
      COMPILE_OK
      ${WABT_BINARY_DIR}
      ${WABT_BINARY_DIR}/dummy.c
      COPY_FILE ${WABT_BINARY_DIR}/dummy
    )
    if (COMPILE_OK)
      execute_process(
        COMMAND file ${WABT_BINARY_DIR}/dummy
        RESULT_VARIABLE FILE_RESULT
        OUTPUT_VARIABLE FILE_OUTPUT
        ERROR_QUIET
      )

      if (FILE_RESULT EQUAL 0)
        if (${FILE_OUTPUT} MATCHES "x86[-_]64")
          set(TARGET_ARCH "x86-64")
        elseif (${FILE_OUTPUT} MATCHES "Intel 80386")
          set(TARGET_ARCH "i386")
        elseif (${FILE_OUTPUT} MATCHES "ARM")
          set(TARGET_ARCH "ARM")
        else ()
          message(WARNING "Unknown target architecture!")
        endif ()
      else ()
        message(WARNING "Error running file on dummy executable")
      endif ()
    else ()
      message(WARNING "Error compiling dummy.c file")
    endif ()

    if (TARGET_ARCH STREQUAL "i386")
      # wasm doesn't allow for x87 floating point math
      add_definitions(-msse2 -mfpmath=sse)
    endif ()
  endif ()
endif ()

set(USE_SANITIZER FALSE)
function(SANITIZER NAME FLAGS)
  if (${NAME})
    message("HERE ${NAME} ${FLAGS}")
    if (USE_SANITIZER)
      message(FATAL_ERROR "Only one sanitizer allowed")
    endif()
    set(USE_SANITIZER TRUE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAGS}" PARENT_SCOPE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAGS}" PARENT_SCOPE)
  endif ()
endfunction()
SANITIZER(USE_ASAN "-fsanitize=address")
SANITIZER(USE_MSAN "-fsanitize=memory")
SANITIZER(USE_LSAN "-fsanitize=leak")
SANITIZER(USE_UBSAN "-fsanitize=undefined -fno-sanitize-recover")

find_package(BISON 3.0)
if (RUN_BISON AND BISON_FOUND)
  set(AST_PARSER_GEN_C ${WABT_BINARY_DIR}/ast-parser-gen.cc)
  BISON_TARGET(AST_PARSER_GEN_C
    ${WABT_SOURCE_DIR}/src/ast-parser.y
    ${AST_PARSER_GEN_C}
    COMPILE_FLAGS --defines=${WABT_BINARY_DIR}/ast-parser-gen.hh
  )
else ()
  set(AST_PARSER_GEN_C src/prebuilt/ast-parser-gen.cc)
  include_directories(src/prebuilt)
endif ()

if (COMPILER_IS_CLANG OR COMPILER_IS_GNU)
  # yyerror passes a non-string-literal to a printf-like function, which is a
  # warning.
  set_source_files_properties(
    ${AST_PARSER_GEN_C}
    PROPERTIES
    COMPILE_FLAGS "-Wno-format-security -Wno-old-style-cast"
  )
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${WABT_SOURCE_DIR}/cmake)
find_package(RE2C 0.13)
if (RUN_RE2C AND RE2C_EXECUTABLE)
  set(AST_LEXER_C ${WABT_SOURCE_DIR}/src/ast-lexer.cc)
  set(AST_LEXER_GEN_C ${WABT_BINARY_DIR}/ast-lexer-gen.cc)
  RE2C_TARGET(
    NAME AST_LEXER_GEN_C
    INPUT ${AST_LEXER_C}
    OUTPUT ${AST_LEXER_GEN_C}
    OPTIONS -bc
  )
else ()
  set(AST_LEXER_GEN_C src/prebuilt/ast-lexer-gen.cc)
endif ()

add_custom_target(everything)

add_library(libwabt STATIC
  src/ast.cc
  src/ast-parser-lexer-shared.cc
  ${AST_LEXER_GEN_C}
  ${AST_PARSER_GEN_C}
  src/type-checker.cc
  src/validator.cc

  src/binary-reader.cc
  src/binary-writer.cc
  src/binary-writer-spec.cc
  src/binary-reader-ast.cc
  src/binding-hash.cc
  src/ast-writer.cc
  src/interpreter.cc
  src/binary-reader-interpreter.cc
  src/apply-names.cc
  src/generate-names.cc
  src/resolve-names.cc

  src/binary.cc
  src/common.cc
  src/config.cc
  src/literal.cc
  src/option-parser.cc
  src/stream.cc
  src/vector.cc
  src/writer.cc
)
set_target_properties(libwabt PROPERTIES OUTPUT_NAME wabt)

if (NOT EMSCRIPTEN)
  if (CODE_COVERAGE)
    add_definitions("-fprofile-arcs -ftest-coverage")
    link_libraries(gcov)
  endif ()

  function(wabt_executable name)
    # ARGV contains all arguments; remove the first one, ${name}, so it's just
    # a list of sources.
    list(REMOVE_AT ARGV 0)
    add_executable(${name} ${ARGV})
    add_dependencies(everything ${name})
    target_link_libraries(${name} libwabt)
    set_property(TARGET ${name} PROPERTY CXX_STANDARD 11)
    set_property(TARGET ${name} PROPERTY CXX_STANDARD_REQUIRED ON)
  endfunction()

  # wast2wasm
  wabt_executable(wast2wasm src/tools/wast2wasm.cc)

  # wasm2wast
  wabt_executable(wasm2wast src/tools/wasm2wast.cc)

  # wasmopcodecnt
  wabt_executable(wasmopcodecnt
    src/tools/wasmopcodecnt.cc src/binary-reader-opcnt.cc)

  # wasmdump
  wabt_executable(wasmdump src/tools/wasmdump.cc src/binary-reader-objdump.cc)

  # wasm-link
  wabt_executable(wasm-link src/tools/wasm-link.cc src/binary-reader-linker.cc)

  # wasm-interp
  wabt_executable(wasm-interp src/tools/wasm-interp.cc)
  if (COMPILER_IS_CLANG OR COMPILER_IS_GNU)
    target_link_libraries(wasm-interp m)
  endif ()

  # wast-desugar
  wabt_executable(wast-desugar src/tools/wast-desugar.cc)

  # symlinks for wast2wasm and wasm2wast
  if (NOT WIN32)
    add_custom_command(
      TARGET wast2wasm
      POST_BUILD
      COMMAND ln -sf wast2wasm sexpr-wasm
    )
    add_custom_command(
      TARGET wasm2wast
      POST_BUILD
      COMMAND ln -sf wasm2wast wasm-wast
    )
  endif ()
  set_property(
    DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
    "sexpr-wasm" "wasm-wast")

  # hexfloat-test
  find_package(Threads)
  if (BUILD_TESTS AND CMAKE_USE_PTHREADS_INIT)
    if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gtest/googletest)
      message(FATAL_ERROR "Can't find third_party/gtest. Run git submodule update --init, or disable with CMake -DBUILD_TESTS=OFF.")
    endif ()

    set(HEXFLOAT_TEST_SRCS
      src/literal.cc
      test/hexfloat.cc
      third_party/gtest/googletest/src/gtest-all.cc
      third_party/gtest/googletest/src/gtest_main.cc
    )
    include_directories(
      third_party/gtest/googletest
      third_party/gtest/googletest/include
    )
    add_executable(hexfloat_test ${HEXFLOAT_TEST_SRCS})
    add_dependencies(everything hexfloat_test)
    target_link_libraries(hexfloat_test ${CMAKE_THREAD_LIBS_INIT})
    set_property(TARGET hexfloat_test PROPERTY CXX_STANDARD 11)
    set_property(TARGET hexfloat_test PROPERTY CXX_STANDARD_REQUIRED ON)
  endif ()

  # test running
  find_package(PythonInterp 2.7 REQUIRED)
  set(RUN_TESTS_PY ${WABT_SOURCE_DIR}/test/run-tests.py)
  add_custom_target(run-tests
    COMMAND ${PYTHON_EXECUTABLE} ${RUN_TESTS_PY} --bindir ${CMAKE_BINARY_DIR}
    DEPENDS wast2wasm wasm2wast wasm-interp
    WORKING_DIRECTORY ${WABT_SOURCE_DIR}
  )

  # install
  install(
    TARGETS wast2wasm wasm2wast wasm-interp wasmopcodecnt wasmdump wast-desugar
        wasm-link
    DESTINATION bin
  )

else ()
  # emscripten stuff

  # just dump everything into one binary so we can reference it from JavaScript
  add_definitions(-Wno-warn-absolute-paths)
  add_executable(libwabtjs src/emscripten-helpers.cc)
  add_dependencies(everything libwabtjs)
  target_link_libraries(libwabtjs libwabt)
  set_target_properties(libwabtjs PROPERTIES OUTPUT_NAME libwabt)

  set(WABT_JS ${WABT_SOURCE_DIR}/src/wabt.js)
  set(EMSCRIPTEN_EXPORTED_JSON ${WABT_SOURCE_DIR}/src/emscripten-exported.json)

  set(LIBWABT_LINK_FLAGS
    --pre-js ${WABT_JS}
    -s EXPORTED_FUNCTIONS=\"@${EMSCRIPTEN_EXPORTED_JSON}\"
    -s RESERVED_FUNCTION_POINTERS=10
    -s NO_EXIT_RUNTIME=1
  )
  string(REPLACE ";" " " LIBWABT_LINK_FLAGS_STR "${LIBWABT_LINK_FLAGS}")

  set_target_properties(libwabtjs
    PROPERTIES
    LINK_FLAGS "${LIBWABT_LINK_FLAGS_STR}"
    LINK_DEPENDS "${WABT_JS};${EMSCRIPTEN_EXPORTED_JSON}"
  )
endif ()
