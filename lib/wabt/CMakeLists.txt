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

cmake_minimum_required(VERSION 3.10)
project(WABT)

set(COMPILER_IS_CLANG 1)
set(COMPILER_IS_GNU 0)
set(COMPILER_IS_MSVC 0)

include(CheckIncludeFile)
include(CheckSymbolExists)

check_include_file("alloca.h" HAVE_ALLOCA_H)
check_include_file("unistd.h" HAVE_UNISTD_H)
check_symbol_exists(snprintf "stdio.h" HAVE_SNPRINTF)
check_symbol_exists(sysconf "unistd.h" HAVE_SYSCONF)
check_symbol_exists(strcasecmp "strings.h" HAVE_STRCASECMP)

if (WIN32)
  check_symbol_exists(ENABLE_VIRTUAL_TERMINAL_PROCESSING "windows.h" HAVE_WIN32_VT100)
endif ()

include(CheckTypeSize)
check_type_size(ssize_t SSIZE_T)
check_type_size(size_t SIZEOF_SIZE_T)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/config.h
)

include_directories(chakra ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

# disable -Wunused-parameter: this is really common when implementing
#   interfaces, etc.
# disable -Wpointer-arith: this is a GCC extension, and doesn't work in MSVC.

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


include_directories(src/prebuilt)

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
set(WAST_LEXER_GEN_CC src/prebuilt/wast-lexer-gen.cc)

add_custom_target(everything)

add_library(libwabt OBJECT
  chakra/wabtapi.cc
  src/apply-names.cc
  src/binary.cc
  src/binary-reader.cc
  src/binary-reader-ir.cc
  src/binary-reader-logging.cc
  src/binary-writer.cc
  src/binary-writer-spec.cc
  src/binding-hash.cc
  src/color.cc
  src/common.cc
  src/config.cc
  src/error-formatter.cc
  src/expr-visitor.cc
  src/feature.cc
  src/filenames.cc
  src/generate-names.cc
  src/hash-util.cc
  src/ir.cc
  src/leb128.cc
  src/lexer-source.cc
  src/lexer-source-line-finder.cc
  src/literal.cc
  src/opcode.cc
  src/opcode-code-table.c
  src/option-parser.cc
  src/resolve-names.cc
  src/stream.cc
  src/string-view.cc
  src/token.cc
  src/tracing.cc
  src/type-checker.cc
  src/utf8.cc
  src/validator.cc
  src/wast-parser.cc
  src/wat-writer.cc
  ${WAST_LEXER_GEN_CC}
)
