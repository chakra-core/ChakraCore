/*
 * Copyright 2018 WebAssembly Community Group participants
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

#include "src/opcode-code-table.h"

#include "config.h"

#include <stdint.h>

typedef enum WabtOpcodeEnum {
#define WABT_OPCODE(rtype, type1, type2, type3, mem_size, prefix, code, Name, \
                    text)                                                     \
  Name,
#include "opcode.def"
#undef WABT_OPCODE
  Invalid,
} WabtOpcodeEnum;

WABT_STATIC_ASSERT(Invalid <= WABT_OPCODE_CODE_TABLE_SIZE);

/* The array index calculated below must match the one in Opcode::FromCode. */
uint32_t WabtOpcodeCodeTable[WABT_OPCODE_CODE_TABLE_SIZE] = {
#define WABT_OPCODE(rtype, type1, type2, type3, mem_size, prefix, code, Name, \
                    text)                                                     \
  [(prefix << 8) + code] = Name,
#include "opcode.def"
#undef WABT_OPCODE
};
