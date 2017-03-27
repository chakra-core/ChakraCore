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

#include "generate-names.h"

#include <assert.h>
#include <stdio.h>

#include <string>
#include <vector>

#include "ast.h"

#define CHECK_RESULT(expr) \
  do {                     \
    if (WABT_FAILED(expr)) \
      return Result::Error;   \
  } while (0)

namespace wabt {

namespace {

struct Context {
  Context() : module(nullptr), label_count(0) { WABT_ZERO_MEMORY(visitor); }

  Module* module;
  ExprVisitor visitor;
  std::vector<std::string> index_to_name;
  uint32_t label_count;
};

}  // namespace

static bool has_name(StringSlice* str) {
  return str->length > 0;
}

static void generate_name(const char* prefix,
                          uint32_t index,
                          StringSlice* str) {
  size_t prefix_len = strlen(prefix);
  size_t buffer_len = prefix_len + 20; /* add space for the number */
  char* buffer = static_cast<char*>(alloca(buffer_len));
  int actual_len = wabt_snprintf(buffer, buffer_len, "%s%u", prefix, index);

  StringSlice buf;
  buf.length = actual_len;
  buf.start = buffer;
  *str = dup_string_slice(buf);
}

static void maybe_generate_name(const char* prefix,
                                uint32_t index,
                                StringSlice* str) {
  if (!has_name(str))
    generate_name(prefix, index, str);
}

static void generate_and_bind_name(BindingHash* bindings,
                                   const char* prefix,
                                   uint32_t index,
                                   StringSlice* str) {
  generate_name(prefix, index, str);
  bindings->emplace(string_slice_to_string(*str), Binding(index));
}

static void maybe_generate_and_bind_name(BindingHash* bindings,
                                         const char* prefix,
                                         uint32_t index,
                                         StringSlice* str) {
  if (!has_name(str))
    generate_and_bind_name(bindings, prefix, index, str);
}

static void generate_and_bind_local_names(
    Context* ctx,
    BindingHash* bindings,
    const char* prefix) {
  for (size_t i = 0; i < ctx->index_to_name.size(); ++i) {
    const std::string& old_name = ctx->index_to_name[i];
    if (!old_name.empty())
      continue;

    StringSlice new_name;
    generate_and_bind_name(bindings, prefix, i, &new_name);
    ctx->index_to_name[i] = string_slice_to_string(new_name);
    destroy_string_slice(&new_name);
  }
}

static Result begin_block_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  maybe_generate_name("$B", ctx->label_count++, &expr->block->label);
  return Result::Ok;
}

static Result begin_loop_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  maybe_generate_name("$L", ctx->label_count++, &expr->loop->label);
  return Result::Ok;
}

static Result begin_if_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  maybe_generate_name("$L", ctx->label_count++, &expr->if_.true_->label);
  return Result::Ok;
}

static Result visit_func(Context* ctx, uint32_t func_index, Func* func) {
  maybe_generate_and_bind_name(&ctx->module->func_bindings, "$f", func_index,
                               &func->name);

  make_type_binding_reverse_mapping(func->decl.sig.param_types,
                                    func->param_bindings, &ctx->index_to_name);
  generate_and_bind_local_names(ctx, &func->param_bindings, "$p");

  make_type_binding_reverse_mapping(func->local_types, func->local_bindings,
                                    &ctx->index_to_name);
  generate_and_bind_local_names(ctx, &func->local_bindings, "$l");

  ctx->label_count = 0;
  CHECK_RESULT(visit_func(func, &ctx->visitor));
  return Result::Ok;
}

static Result visit_global(Context* ctx,
                           uint32_t global_index,
                           Global* global) {
  maybe_generate_and_bind_name(&ctx->module->global_bindings, "$g",
                               global_index, &global->name);
  return Result::Ok;
}

static Result visit_func_type(Context* ctx,
                              uint32_t func_type_index,
                              FuncType* func_type) {
  maybe_generate_and_bind_name(&ctx->module->func_type_bindings, "$t",
                               func_type_index, &func_type->name);
  return Result::Ok;
}

static Result visit_table(Context* ctx, uint32_t table_index, Table* table) {
  maybe_generate_and_bind_name(&ctx->module->table_bindings, "$T", table_index,
                               &table->name);
  return Result::Ok;
}

static Result visit_memory(Context* ctx,
                           uint32_t memory_index,
                           Memory* memory) {
  maybe_generate_and_bind_name(&ctx->module->memory_bindings, "$M",
                               memory_index, &memory->name);
  return Result::Ok;
}

static Result visit_module(Context* ctx, Module* module) {
  for (size_t i = 0; i < module->globals.size(); ++i)
    CHECK_RESULT(visit_global(ctx, i, module->globals[i]));
  for (size_t i = 0; i < module->func_types.size(); ++i)
    CHECK_RESULT(visit_func_type(ctx, i, module->func_types[i]));
  for (size_t i = 0; i < module->funcs.size(); ++i)
    CHECK_RESULT(visit_func(ctx, i, module->funcs[i]));
  for (size_t i = 0; i < module->tables.size(); ++i)
    CHECK_RESULT(visit_table(ctx, i, module->tables[i]));
  for (size_t i = 0; i < module->memories.size(); ++i)
    CHECK_RESULT(visit_memory(ctx, i, module->memories[i]));
  return Result::Ok;
}

Result generate_names(Module* module) {
  Context ctx;
  ctx.visitor.user_data = &ctx;
  ctx.visitor.begin_block_expr = begin_block_expr;
  ctx.visitor.begin_loop_expr = begin_loop_expr;
  ctx.visitor.begin_if_expr = begin_if_expr;
  ctx.module = module;
  Result result = visit_module(&ctx, module);
  return result;
}

}  // namespace wabt
