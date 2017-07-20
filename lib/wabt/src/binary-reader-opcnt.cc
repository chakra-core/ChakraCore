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

#include "binary-reader-opcnt.h"

#include <cassert>
#include <cinttypes>
#include <cstdarg>
#include <cstdint>
#include <cstdio>

#include "binary-reader-nop.h"
#include "common.h"

namespace wabt {

namespace {

class BinaryReaderOpcnt : public BinaryReaderNop {
 public:
  explicit BinaryReaderOpcnt(OpcntData* data);

  Result OnOpcode(Opcode opcode) override;
  Result OnI32ConstExpr(uint32_t value) override;
  Result OnGetLocalExpr(Index local_index) override;
  Result OnSetLocalExpr(Index local_index) override;
  Result OnTeeLocalExpr(Index local_index) override;
  Result OnLoadExpr(Opcode opcode,
                    uint32_t alignment_log2,
                    Address offset) override;
  Result OnStoreExpr(Opcode opcode,
                     uint32_t alignment_log2,
                     Address offset) override;

 private:
  OpcntData* opcnt_data;
};

static Result AddIntCounterValue(IntCounterVector* vec, intmax_t value) {
  for (IntCounter& counter : *vec) {
    if (counter.value == value) {
      ++counter.count;
      return Result::Ok;
    }
  }
  vec->emplace_back(value, 1);
  return Result::Ok;
}

static Result AddIntPairCounterValue(IntPairCounterVector* vec,
                                     intmax_t first,
                                     intmax_t second) {
  for (IntPairCounter& pair : *vec) {
    if (pair.first == first && pair.second == second) {
      ++pair.count;
      return Result::Ok;
    }
  }
  vec->emplace_back(first, second, 1);
  return Result::Ok;
}

BinaryReaderOpcnt::BinaryReaderOpcnt(OpcntData* data) : opcnt_data(data) {}

Result BinaryReaderOpcnt::OnOpcode(Opcode opcode) {
  IntCounterVector& opcnt_vec = opcnt_data->opcode_vec;
  while (static_cast<size_t>(opcode) >= opcnt_vec.size()) {
    opcnt_vec.emplace_back(opcnt_vec.size(), 0);
  }
  ++opcnt_vec[static_cast<size_t>(opcode)].count;
  return Result::Ok;
}

Result BinaryReaderOpcnt::OnI32ConstExpr(uint32_t value) {
  return AddIntCounterValue(&opcnt_data->i32_const_vec,
                            static_cast<int32_t>(value));
}

Result BinaryReaderOpcnt::OnGetLocalExpr(Index local_index) {
  return AddIntCounterValue(&opcnt_data->get_local_vec, local_index);
}

Result BinaryReaderOpcnt::OnSetLocalExpr(Index local_index) {
  return AddIntCounterValue(&opcnt_data->set_local_vec, local_index);
}

Result BinaryReaderOpcnt::OnTeeLocalExpr(Index local_index) {
  return AddIntCounterValue(&opcnt_data->tee_local_vec, local_index);
}

Result BinaryReaderOpcnt::OnLoadExpr(Opcode opcode,
                                     uint32_t alignment_log2,
                                     Address offset) {
  if (opcode == Opcode::I32Load) {
    return AddIntPairCounterValue(&opcnt_data->i32_load_vec, alignment_log2,
                                  offset);
  }
  return Result::Ok;
}

Result BinaryReaderOpcnt::OnStoreExpr(Opcode opcode,
                                      uint32_t alignment_log2,
                                      Address offset) {
  if (opcode == Opcode::I32Store) {
    return AddIntPairCounterValue(&opcnt_data->i32_store_vec, alignment_log2,
                                  offset);
  }
  return Result::Ok;
}

}  // namespace

Result read_binary_opcnt(const void* data,
                         size_t size,
                         const struct ReadBinaryOptions* options,
                         OpcntData* opcnt_data) {
  BinaryReaderOpcnt reader(opcnt_data);
  return read_binary(data, size, &reader, options);
}

}  // namespace wabt
