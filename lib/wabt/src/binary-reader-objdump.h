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

#ifndef WABT_BINARY_READER_OBJDUMP_H_
#define WABT_BINARY_READER_OBJDUMP_H_

#include "common.h"
#include "stream.h"
#include "vector.h"

namespace wabt {

struct Module;
struct ReadBinaryOptions;

struct Reloc {
  RelocType type;
  size_t offset;
};
WABT_DEFINE_VECTOR(reloc, Reloc);

WABT_DEFINE_VECTOR(string_slice, StringSlice);

enum class ObjdumpMode {
  Prepass,
  Headers,
  Details,
  Disassemble,
  RawData,
};

struct ObjdumpOptions {
  bool headers;
  bool details;
  bool raw;
  bool disassemble;
  bool debug;
  bool relocs;
  ObjdumpMode mode;
  const char* infile;
  const char* section_name;
  bool print_header;
  StringSliceVector function_names;
  RelocVector code_relocations;
};

Result read_binary_objdump(const uint8_t* data,
                           size_t size,
                           ObjdumpOptions* options);

}  // namespace wabt

#endif /* WABT_BINARY_READER_OBJDUMP_H_ */
