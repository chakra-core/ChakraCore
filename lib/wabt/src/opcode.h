/*
 * Copyright 2017 WebAssembly Community Group participants
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

#ifndef WABT_OPCODE_H_
#define WABT_OPCODE_H_

#include "common.h"

namespace wabt {

enum class Opcode {

#define WABT_OPCODE(rtype, type1, type2, mem_size, code, Name, text) \
  Name = code,
#include "opcode.def"
#undef WABT_OPCODE

  First = Unreachable,
  Last = F64ReinterpretI64,
};
static const int kOpcodeCount = WABT_ENUM_COUNT(Opcode);

struct OpcodeInfo {
  const char* name;
  Type result_type;
  Type param1_type;
  Type param2_type;
  Address memory_size;
};

// Return 1 if |alignment| matches the alignment of |opcode|, or if |alignment|
// is WABT_USE_NATURAL_ALIGNMENT.
bool is_naturally_aligned(Opcode opcode, Address alignment);

// If |alignment| is WABT_USE_NATURAL_ALIGNMENT, return the alignment of
// |opcode|, else return |alignment|.
Address get_opcode_alignment(Opcode opcode, Address alignment);

extern OpcodeInfo g_opcode_info[];

inline const char* get_opcode_name(Opcode opcode) {
  assert(static_cast<int>(opcode) < kOpcodeCount);
  return g_opcode_info[static_cast<size_t>(opcode)].name;
}

inline Type get_opcode_result_type(Opcode opcode) {
  assert(static_cast<int>(opcode) < kOpcodeCount);
  return g_opcode_info[static_cast<size_t>(opcode)].result_type;
}

inline Type get_opcode_param_type_1(Opcode opcode) {
  assert(static_cast<int>(opcode) < kOpcodeCount);
  return g_opcode_info[static_cast<size_t>(opcode)].param1_type;
}

inline Type get_opcode_param_type_2(Opcode opcode) {
  assert(static_cast<int>(opcode) < kOpcodeCount);
  return g_opcode_info[static_cast<size_t>(opcode)].param2_type;
}

inline int get_opcode_memory_size(Opcode opcode) {
  assert(static_cast<int>(opcode) < kOpcodeCount);
  return g_opcode_info[static_cast<size_t>(opcode)].memory_size;
}

}  // namespace

#endif  // WABT_OPCODE_H_
