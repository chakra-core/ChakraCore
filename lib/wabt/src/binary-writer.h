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

#ifndef WABT_BINARY_WRITER_H_
#define WABT_BINARY_WRITER_H_

#include "common.h"
#include "opcode.h"
#include "stream.h"

namespace wabt {

class Writer;
struct Module;
struct Script;

#define WABT_WRITE_BINARY_OPTIONS_DEFAULT \
  { nullptr, true, false, false }

struct WriteBinaryOptions {
  Stream* log_stream;
  bool canonicalize_lebs;
  bool relocatable;
  bool write_debug_names;
};

Result write_binary_module(Writer*, const Module*, const WriteBinaryOptions*);

/* returns the length of the leb128 */
Offset u32_leb128_length(uint32_t value);

void write_u32_leb128(Stream* stream, uint32_t value, const char* desc);

void write_i32_leb128(Stream* stream, int32_t value, const char* desc);

void write_fixed_u32_leb128(Stream* stream, uint32_t value, const char* desc);

Offset write_fixed_u32_leb128_at(Stream* stream,
                                 Offset offset,
                                 uint32_t value,
                                 const char* desc);

Offset write_fixed_u32_leb128_raw(uint8_t* data, uint8_t* end, uint32_t value);

void write_type(Stream* stream, Type type);

void write_str(Stream* stream,
               const char* s,
               size_t length,
               const char* desc,
               PrintChars print_chars = PrintChars::No);

void write_opcode(Stream* stream, Opcode opcode);

void write_limits(Stream* stream, const Limits* limits);

/* Convenience functions for writing enums as LEB128s. */
template <typename T>
void write_u32_leb128_enum(Stream* stream, T value, const char* desc) {
  write_u32_leb128(stream, static_cast<uint32_t>(value), desc);
}

template <typename T>
void write_i32_leb128_enum(Stream* stream, T value, const char* desc) {
  write_i32_leb128(stream, static_cast<int32_t>(value), desc);
}

}  // namespace wabt

#endif /* WABT_BINARY_WRITER_H_ */
