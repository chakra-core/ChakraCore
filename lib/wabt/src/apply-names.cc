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

#include "apply-names.h"

#include <assert.h>
#include <stdio.h>

#include <vector>

#include "ast.h"

#define CHECK_RESULT(expr)  \
  do {                      \
    if (WABT_FAILED(expr))  \
      return Result::Error; \
  } while (0)

namespace wabt {

namespace {

struct Context {
  Context();

  Module* module = nullptr;
  Func* current_func = nullptr;
  ExprVisitor visitor;
  /* mapping from param index to its name, if any, for the current func */
  std::vector<std::string> param_index_to_name;
  std::vector<std::string> local_index_to_name;
  std::vector<Label*> labels;
};

Context::Context() {
  WABT_ZERO_MEMORY(visitor);
}

void push_label(Context* ctx, Label* label) {
  ctx->labels.push_back(label);
}

void pop_label(Context* ctx) {
  ctx->labels.pop_back();
}

Label* find_label_by_var(Context* ctx, Var* var) {
  if (var->type == VarType::Name) {
    for (int i = ctx->labels.size() - 1; i >= 0; --i) {
      Label* label = ctx->labels[i];
      if (string_slices_are_equal(label, &var->name))
        return label;
    }
    return nullptr;
  } else {
    if (var->index < 0 || static_cast<size_t>(var->index) >= ctx->labels.size())
      return nullptr;
    return ctx->labels[ctx->labels.size() - 1 - var->index];
  }
}

void use_name_for_var(StringSlice* name, Var* var) {
  if (var->type == VarType::Name) {
    assert(string_slices_are_equal(name, &var->name));
  }

  if (name && name->start) {
    var->type = VarType::Name;
    var->name = dup_string_slice(*name);
  }
}

Result use_name_for_func_type_var(Module* module, Var* var) {
  FuncType* func_type = get_func_type_by_var(module, var);
  if (!func_type)
    return Result::Error;
  use_name_for_var(&func_type->name, var);
  return Result::Ok;
}

Result use_name_for_func_var(Module* module, Var* var) {
  Func* func = get_func_by_var(module, var);
  if (!func)
    return Result::Error;
  use_name_for_var(&func->name, var);
  return Result::Ok;
}

Result use_name_for_global_var(Module* module, Var* var) {
  Global* global = get_global_by_var(module, var);
  if (!global)
    return Result::Error;
  use_name_for_var(&global->name, var);
  return Result::Ok;
}

Result use_name_for_table_var(Module* module, Var* var) {
  Table* table = get_table_by_var(module, var);
  if (!table)
    return Result::Error;
  use_name_for_var(&table->name, var);
  return Result::Ok;
}

Result use_name_for_memory_var(Module* module, Var* var) {
  Memory* memory = get_memory_by_var(module, var);
  if (!memory)
    return Result::Error;
  use_name_for_var(&memory->name, var);
  return Result::Ok;
}

Result use_name_for_param_and_local_var(Context* ctx, Func* func, Var* var) {
  int local_index = get_local_index_by_var(func, var);
  if (local_index < 0 ||
      static_cast<size_t>(local_index) >= get_num_params_and_locals(func))
    return Result::Error;

  uint32_t num_params = get_num_params(func);
  std::string* name;
  if (static_cast<uint32_t>(local_index) < num_params) {
    /* param */
    assert(static_cast<size_t>(local_index) < ctx->param_index_to_name.size());
    name = &ctx->param_index_to_name[local_index];
  } else {
    /* local */
    local_index -= num_params;
    assert(static_cast<size_t>(local_index) < ctx->local_index_to_name.size());
    name = &ctx->local_index_to_name[local_index];
  }

  if (var->type == VarType::Name) {
    assert(*name == string_slice_to_string(var->name));
    return Result::Ok;
  }

  if (!name->empty()) {
    var->type = VarType::Name;
    var->name = dup_string_slice(string_to_string_slice(*name));
    return var->name.start ? Result::Ok : Result::Error;
  }
  return Result::Ok;
}

Result begin_block_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  push_label(ctx, &expr->block->label);
  return Result::Ok;
}

Result end_block_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  pop_label(ctx);
  return Result::Ok;
}

Result begin_loop_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  push_label(ctx, &expr->loop->label);
  return Result::Ok;
}

Result end_loop_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  pop_label(ctx);
  return Result::Ok;
}

Result on_br_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Label* label = find_label_by_var(ctx, &expr->br.var);
  use_name_for_var(label, &expr->br.var);
  return Result::Ok;
}

Result on_br_if_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Label* label = find_label_by_var(ctx, &expr->br_if.var);
  use_name_for_var(label, &expr->br_if.var);
  return Result::Ok;
}

Result on_br_table_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  VarVector& targets = *expr->br_table.targets;
  for (Var& target : targets) {
    Label* label = find_label_by_var(ctx, &target);
    use_name_for_var(label, &target);
  }

  Label* label = find_label_by_var(ctx, &expr->br_table.default_target);
  use_name_for_var(label, &expr->br_table.default_target);
  return Result::Ok;
}

Result on_call_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_RESULT(use_name_for_func_var(ctx->module, &expr->call.var));
  return Result::Ok;
}

Result on_call_indirect_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_RESULT(
      use_name_for_func_type_var(ctx->module, &expr->call_indirect.var));
  return Result::Ok;
}

Result on_get_global_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_RESULT(use_name_for_global_var(ctx->module, &expr->get_global.var));
  return Result::Ok;
}

Result on_get_local_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_RESULT(use_name_for_param_and_local_var(ctx, ctx->current_func,
                                                &expr->get_local.var));
  return Result::Ok;
}

Result begin_if_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  push_label(ctx, &expr->if_.true_->label);
  return Result::Ok;
}

Result end_if_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  pop_label(ctx);
  return Result::Ok;
}

Result on_set_global_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_RESULT(use_name_for_global_var(ctx->module, &expr->set_global.var));
  return Result::Ok;
}

Result on_set_local_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_RESULT(use_name_for_param_and_local_var(ctx, ctx->current_func,
                                                &expr->set_local.var));
  return Result::Ok;
}

Result on_tee_local_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_RESULT(use_name_for_param_and_local_var(ctx, ctx->current_func,
                                                &expr->tee_local.var));
  return Result::Ok;
}

Result visit_func(Context* ctx, uint32_t func_index, Func* func) {
  ctx->current_func = func;
  if (decl_has_func_type(&func->decl)) {
    CHECK_RESULT(use_name_for_func_type_var(ctx->module, &func->decl.type_var));
  }

  make_type_binding_reverse_mapping(func->decl.sig.param_types,
                                    func->param_bindings,
                                    &ctx->param_index_to_name);

  make_type_binding_reverse_mapping(func->local_types, func->local_bindings,
                                    &ctx->local_index_to_name);

  CHECK_RESULT(visit_func(func, &ctx->visitor));
  ctx->current_func = nullptr;
  return Result::Ok;
}

Result visit_export(Context* ctx, uint32_t export_index, Export* export_) {
  if (export_->kind == ExternalKind::Func) {
    use_name_for_func_var(ctx->module, &export_->var);
  }
  return Result::Ok;
}

Result visit_elem_segment(Context* ctx,
                          uint32_t elem_segment_index,
                          ElemSegment* segment) {
  CHECK_RESULT(use_name_for_table_var(ctx->module, &segment->table_var));
  for (Var& var : segment->vars) {
    CHECK_RESULT(use_name_for_func_var(ctx->module, &var));
  }
  return Result::Ok;
}

Result visit_data_segment(Context* ctx,
                          uint32_t data_segment_index,
                          DataSegment* segment) {
  CHECK_RESULT(use_name_for_memory_var(ctx->module, &segment->memory_var));
  return Result::Ok;
}

Result visit_module(Context* ctx, Module* module) {
  for (size_t i = 0; i < module->funcs.size(); ++i)
    CHECK_RESULT(visit_func(ctx, i, module->funcs[i]));
  for (size_t i = 0; i < module->exports.size(); ++i)
    CHECK_RESULT(visit_export(ctx, i, module->exports[i]));
  for (size_t i = 0; i < module->elem_segments.size(); ++i)
    CHECK_RESULT(visit_elem_segment(ctx, i, module->elem_segments[i]));
  for (size_t i = 0; i < module->data_segments.size(); ++i)
    CHECK_RESULT(visit_data_segment(ctx, i, module->data_segments[i]));
  return Result::Ok;
}

}  // namespace

Result apply_names(Module* module) {
  Context ctx;
  ctx.module = module;
  ctx.visitor.user_data = &ctx;
  ctx.visitor.begin_block_expr = begin_block_expr;
  ctx.visitor.end_block_expr = end_block_expr;
  ctx.visitor.begin_loop_expr = begin_loop_expr;
  ctx.visitor.end_loop_expr = end_loop_expr;
  ctx.visitor.on_br_expr = on_br_expr;
  ctx.visitor.on_br_if_expr = on_br_if_expr;
  ctx.visitor.on_br_table_expr = on_br_table_expr;
  ctx.visitor.on_call_expr = on_call_expr;
  ctx.visitor.on_call_indirect_expr = on_call_indirect_expr;
  ctx.visitor.on_get_global_expr = on_get_global_expr;
  ctx.visitor.on_get_local_expr = on_get_local_expr;
  ctx.visitor.begin_if_expr = begin_if_expr;
  ctx.visitor.end_if_expr = end_if_expr;
  ctx.visitor.on_set_global_expr = on_set_global_expr;
  ctx.visitor.on_set_local_expr = on_set_local_expr;
  ctx.visitor.on_tee_local_expr = on_tee_local_expr;
  Result result = visit_module(&ctx, module);
  return result;
}

}  // namespace wabt
