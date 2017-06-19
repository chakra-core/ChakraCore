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

#include "opcode.h"

namespace wabt {

OpcodeInfo g_opcode_info[kOpcodeCount] = {

#define WABT_OPCODE(rtype, type1, type2, mem_size, code, Name, text) \
  {text, Type::rtype, Type::type1, Type::type2, mem_size},
#include "opcode.def"
#undef WABT_OPCODE

};

bool is_naturally_aligned(Opcode opcode, Address alignment) {
  Address opcode_align = get_opcode_memory_size(opcode);
  return alignment == WABT_USE_NATURAL_ALIGNMENT || alignment == opcode_align;
}

Address get_opcode_alignment(Opcode opcode, Address alignment) {
  if (alignment == WABT_USE_NATURAL_ALIGNMENT)
    return get_opcode_memory_size(opcode);
  return alignment;
}

}  // namespace
