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

#include "src/common.h"
#include "src/opcode.h"
#include "src/stream.h"

namespace wabt {

struct Module;
struct Script;

struct WriteBinaryOptions {
  bool canonicalize_lebs = true;
  bool relocatable = false;
  bool write_debug_names = false;
};

Result WriteBinaryModule(Stream*, const Module*, const WriteBinaryOptions*);

void WriteType(Stream* stream, Type type);

void WriteStr(Stream* stream,
              string_view s,
              const char* desc,
              PrintChars print_chars = PrintChars::No);

void WriteOpcode(Stream* stream, Opcode opcode);

void WriteLimits(Stream* stream, const Limits* limits);

}  // namespace wabt

#endif /* WABT_BINARY_WRITER_H_ */
