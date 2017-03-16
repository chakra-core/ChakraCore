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

#ifndef WABT_BINARY_READER_OPCNT_H_
#define WABT_BINARY_READER_OPCNT_H_

#include "common.h"
#include "vector.h"

namespace wabt {

struct Module;
struct ReadBinaryOptions;

struct IntCounter {
  intmax_t value;
  size_t count;
};

WABT_DEFINE_VECTOR(int_counter, IntCounter)

struct IntPairCounter {
  intmax_t first;
  intmax_t second;
  size_t count;
};

WABT_DEFINE_VECTOR(int_pair_counter, IntPairCounter);

struct OpcntData {
  IntCounterVector opcode_vec;
  IntCounterVector i32_const_vec;
  IntCounterVector get_local_vec;
  IntCounterVector set_local_vec;
  IntCounterVector tee_local_vec;
  IntPairCounterVector i32_load_vec;
  IntPairCounterVector i32_store_vec;
};

void init_opcnt_data(OpcntData* data);
void destroy_opcnt_data(OpcntData* data);

Result read_binary_opcnt(const void* data,
                         size_t size,
                         const struct ReadBinaryOptions* options,
                         OpcntData* opcnt_data);

}  // namespace wabt

#endif /* WABT_BINARY_READER_OPCNT_H_ */
