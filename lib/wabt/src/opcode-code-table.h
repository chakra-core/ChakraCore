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

#ifndef WABT_OPCODE_CODE_TABLE_H_
#define WABT_OPCODE_CODE_TABLE_H_

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WABT_OPCODE_CODE_TABLE_SIZE 65536

/* This structure is defined in C because C++ doesn't (yet) allow you to use
 * designated array initializers, i.e. [10] = {foo}.
 */
extern uint32_t WabtOpcodeCodeTable[WABT_OPCODE_CODE_TABLE_SIZE];

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* WABT_OPCODE_CODE_TABLE_H_ */
