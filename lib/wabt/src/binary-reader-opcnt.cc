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

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include "binary-reader.h"
#include "common.h"

namespace wabt {

namespace {

struct Context {
  OpcntData* opcnt_data;
};

}  // namespace

static Result add_int_counter_value(IntCounterVector* vec, intmax_t value) {
  for (IntCounter& counter : *vec) {
    if (counter.value == value) {
      ++counter.count;
      return Result::Ok;
    }
  }
  vec->emplace_back(value, 1);
  return Result::Ok;
}

static Result add_int_pair_counter_value(IntPairCounterVector* vec,
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

static Result on_opcode(BinaryReaderContext* context, Opcode opcode) {
  Context* ctx = static_cast<Context*>(context->user_data);
  IntCounterVector& opcnt_vec = ctx->opcnt_data->opcode_vec;
  while (static_cast<size_t>(opcode) >= opcnt_vec.size()) {
    opcnt_vec.emplace_back(opcnt_vec.size(), 0);
  }
  ++opcnt_vec[static_cast<size_t>(opcode)].count;
  return Result::Ok;
}

static Result on_i32_const_expr(uint32_t value, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  return add_int_counter_value(&ctx->opcnt_data->i32_const_vec,
                               static_cast<int32_t>(value));
}

static Result on_get_local_expr(uint32_t local_index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  return add_int_counter_value(&ctx->opcnt_data->get_local_vec, local_index);
}

static Result on_set_local_expr(uint32_t local_index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  return add_int_counter_value(&ctx->opcnt_data->set_local_vec, local_index);
}

static  Result on_tee_local_expr(uint32_t local_index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  return add_int_counter_value(&ctx->opcnt_data->tee_local_vec, local_index);
}

static  Result on_load_expr(Opcode opcode,
                                uint32_t alignment_log2,
                                uint32_t offset,
                                void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  if (opcode == Opcode::I32Load)
    return add_int_pair_counter_value(&ctx->opcnt_data->i32_load_vec,
                                      alignment_log2, offset);
  return Result::Ok;
}

static  Result on_store_expr(Opcode opcode,
                                 uint32_t alignment_log2,
                                 uint32_t offset,
                                 void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  if (opcode == Opcode::I32Store)
    return add_int_pair_counter_value(&ctx->opcnt_data->i32_store_vec,
                                      alignment_log2, offset);
  return Result::Ok;
}

Result read_binary_opcnt(const void* data,
                                  size_t size,
                                  const struct ReadBinaryOptions* options,
                                  OpcntData* opcnt_data) {
  Context ctx;
  WABT_ZERO_MEMORY(ctx);
  ctx.opcnt_data = opcnt_data;

  BinaryReader reader;
  WABT_ZERO_MEMORY(reader);
  reader.user_data = &ctx;
  reader.on_opcode = on_opcode;
  reader.on_i32_const_expr = on_i32_const_expr;
  reader.on_get_local_expr = on_get_local_expr;
  reader.on_set_local_expr = on_set_local_expr;
  reader.on_tee_local_expr = on_tee_local_expr;
  reader.on_load_expr = on_load_expr;
  reader.on_store_expr = on_store_expr;

  return read_binary(data, size, &reader, 1, options);
}

}  // namespace wabt
