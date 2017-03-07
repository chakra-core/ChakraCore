/*
 * Copyright 2016 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef WABT_EMSCRIPTEN_HELPERS_H_
#define WABT_EMSCRIPTEN_HELPERS_H_

#include <stddef.h>

#include "ast.h"
#include "binary-reader.h"
#include "binary-writer.h"
#include "common.h"
#include "stream.h"
#include "writer.h"

/* TODO(binji): it would be nicer to generate this as static data, but it's not
 * currently easy to do. Maybe use LLVM's python bindings for this? */

#define DEFINE_SIZEOF(Name, name) \
  size_t wabt_sizeof_##name(void) { return sizeof(Name); }

#define DEFINE_OFFSETOF(Name, name, member)      \
  size_t wabt_offsetof_##name##_##member(void) { \
    return offsetof(Name, member);               \
  }

/* See http://stackoverflow.com/a/1872506 */
#define CONCAT(a, b) CONCAT_(a, b)
#define CONCAT_(a, b) CONCAT__(a, b)
#define CONCAT__(a, b) a##b
#define FOREACH_1(m, S, s, x, ...) m(S, s, x)
#define FOREACH_2(m, S, s, x, ...) m(S, s, x) FOREACH_1(m, S, s, __VA_ARGS__)
#define FOREACH_3(m, S, s, x, ...) m(S, s, x) FOREACH_2(m, S, s, __VA_ARGS__)
#define FOREACH_4(m, S, s, x, ...) m(S, s, x) FOREACH_3(m, S, s, __VA_ARGS__)
#define FOREACH_5(m, S, s, x, ...) m(S, s, x) FOREACH_4(m, S, s, __VA_ARGS__)
#define FOREACH_6(m, S, s, x, ...) m(S, s, x) FOREACH_5(m, S, s, __VA_ARGS__)
#define FOREACH_7(m, S, s, x, ...) m(S, s, x) FOREACH_6(m, S, s, __VA_ARGS__)
#define FOREACH_8(m, S, s, x, ...) m(S, s, x) FOREACH_7(m, S, s, __VA_ARGS__)
#define NARG(...) NARG_(__VA_ARGS__, RSEQ_N())
#define NARG_(...) ARG_N(__VA_ARGS__)
#define ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N
#define RSEQ_N() 8, 7, 6, 5, 4, 3, 2, 1, 0
#define FOREACH(m, S, s, ...) FOREACH_(NARG(__VA_ARGS__), m, S, s, __VA_ARGS__)
#define FOREACH_(N, m, S, s, ...) CONCAT(FOREACH_, N)(m, S, s, __VA_ARGS__)
#define DEFINE_STRUCT0(Name, name) DEFINE_SIZEOF(Name, name)
#define DEFINE_STRUCT(Name, name, ...) \
  DEFINE_SIZEOF(Name, name)            \
  FOREACH(DEFINE_OFFSETOF, Name, name, __VA_ARGS__)

WABT_EXTERN_C_BEGIN

/* clang-format off */
DEFINE_STRUCT(
    WabtBinaryErrorHandler, binary_error_handler,
    on_error, user_data)

DEFINE_STRUCT(
    WabtLocation, location,
    filename, line, first_column, last_column)

DEFINE_STRUCT(
    WabtMemoryWriter, memory_writer,
    base, buf)

DEFINE_STRUCT0(
    WabtModule, module)

DEFINE_STRUCT(
    WabtOutputBuffer, output_buffer,
    start, size, capacity)

DEFINE_STRUCT(
    WabtReadBinaryOptions, read_binary_options,
    read_debug_names)

DEFINE_STRUCT0(
    WabtScript, script)

DEFINE_STRUCT(
    WabtSourceErrorHandler, source_error_handler,
    on_error, source_line_max_length, user_data)

DEFINE_STRUCT(
    WabtStream, stream,
    writer, offset, result, log_stream)

DEFINE_STRUCT(
    WabtStringSlice, string_slice,
    start, length)

DEFINE_STRUCT(
    WabtWriter, writer,
    write_data, move_data)

DEFINE_STRUCT(
    WabtWriteBinaryOptions, write_binary_options,
    log_stream, canonicalize_lebs, write_debug_names)
/* clang-format on */

WABT_EXTERN_C_END

#endif /* WABT_EMSCRIPTEN_HELPERS_H_ */
