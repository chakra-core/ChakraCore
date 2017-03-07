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

struct Context {
  OpcntData* opcnt_data;
};

static Result add_int_counter_value(IntCounterVector* vec,
                                        intmax_t value) {
  size_t i;
  for (i = 0; i < vec->size; ++i) {
    if (vec->data[i].value == value) {
      ++vec->data[i].count;
      return Result::Ok;
    }
  }
  IntCounter counter;
  counter.value = value;
  counter.count = 1;
  append_int_counter_value(vec, &counter);
  return Result::Ok;
}

static Result add_int_pair_counter_value(IntPairCounterVector* vec,
                                             intmax_t first,
                                             intmax_t second) {
  size_t i;
  for (i = 0; i < vec->size; ++i) {
    if (vec->data[i].first == first && vec->data[i].second == second) {
      ++vec->data[i].count;
      return Result::Ok;
    }
  }
  IntPairCounter counter;
  counter.first = first;
  counter.second = second;
  counter.count = 1;
  append_int_pair_counter_value(vec, &counter);
  return Result::Ok;
}

static Result on_opcode(BinaryReaderContext* context,
                            Opcode opcode) {
  Context* ctx = static_cast<Context*>(context->user_data);
  IntCounterVector* opcnt_vec = &ctx->opcnt_data->opcode_vec;
  while (static_cast<size_t>(opcode) >= opcnt_vec->size) {
    IntCounter Counter;
    Counter.value = opcnt_vec->size;
    Counter.count = 0;
    append_int_counter_value(opcnt_vec, &Counter);
  }
  ++opcnt_vec->data[static_cast<size_t>(opcode)].count;
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

static void on_error(BinaryReaderContext* ctx, const char* message) {
  DefaultErrorHandlerInfo info;
  info.header = "error reading binary";
  info.out_file = stdout;
  info.print_header = PrintErrorHeader::Once;
  default_binary_error_callback(ctx->offset, message, &info);
}

void init_opcnt_data(OpcntData* data) {
  WABT_ZERO_MEMORY(*data);
}

void destroy_opcnt_data(OpcntData* data) {
  destroy_int_counter_vector(&data->opcode_vec);
  destroy_int_counter_vector(&data->i32_const_vec);
  destroy_int_counter_vector(&data->get_local_vec);
  destroy_int_pair_counter_vector(&data->i32_load_vec);
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
  reader.on_error = on_error;
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
