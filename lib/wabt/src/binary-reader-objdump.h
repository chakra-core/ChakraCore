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

#include <string>
#include <vector>

#include "src/common.h"
#include "src/feature.h"
#include "src/stream.h"

namespace wabt {

struct Module;
struct ReadBinaryOptions;

enum class ObjdumpMode {
  Prepass,
  Headers,
  Details,
  Disassemble,
  RawData,
};

struct ObjdumpOptions {
  Stream* log_stream;
  bool headers;
  bool details;
  bool raw;
  bool disassemble;
  bool debug;
  bool relocs;
  ObjdumpMode mode;
  const char* filename;
  const char* section_name;
};

// read_binary_objdump uses this state to store information from previous runs
// and use it to display more useful information.
struct ObjdumpState {
  std::vector<Reloc> code_relocations;
  std::vector<Reloc> data_relocations;
  std::vector<std::string> function_names;
};

Result ReadBinaryObjdump(const uint8_t* data,
                         size_t size,
                         ObjdumpOptions* options,
                         ObjdumpState* state);

}  // namespace wabt

#endif /* WABT_BINARY_READER_OBJDUMP_H_ */
