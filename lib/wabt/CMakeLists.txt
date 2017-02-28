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

if (NOT USES_GNU_COMPILER)
  set(COMPILER_IS_CLANG 1)
else()
  set(COMPILER_IS_GNU 1)
endif()

set(COMPILER_IS_MSVC 0)

include(CheckIncludeFile)
include(CheckSymbolExists)

check_include_file("alloca.h" HAVE_ALLOCA_H)
check_include_file("unistd.h" HAVE_UNISTD_H)
check_symbol_exists(snprintf "stdio.h" HAVE_SNPRINTF)
check_symbol_exists(sysconf "unistd.h" HAVE_SYSCONF)
check_symbol_exists(strcasecmp "strings.h" HAVE_STRCASECMP)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/built/config.h
)

include_directories(chakra src ${CMAKE_CURRENT_SOURCE_DIR}/built)

if (COMPILER_IS_GNU)
  # disable -Wclobbered: it seems to be guessing incorrectly about a local
  # variable being clobbered by longjmp.
  add_definitions(-Wno-clobbered)
endif ()

if (COMPILER_IS_CLANG)
  add_definitions(-fcolor-diagnostics)
endif ()

  set(AST_PARSER_GEN_C src/prebuilt/ast-parser-gen.cc)
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
  set(AST_LEXER_GEN_C src/prebuilt/ast-lexer-gen.cc)

add_custom_target(everything)

add_library(libwabt OBJECT
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
  src/hash-util.cc
  src/string-view.cc

  src/binary.cc
  src/common.cc
  src/config.cc
  src/literal.cc
  src/option-parser.cc
  src/stream.cc
  src/writer.cc

  chakra/wabtapi.cc
)
