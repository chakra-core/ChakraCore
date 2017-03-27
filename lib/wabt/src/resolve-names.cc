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

#include "resolve-names.h"

#include <assert.h>
#include <stdio.h>

#include "ast.h"
#include "ast-parser-lexer-shared.h"

namespace wabt {

namespace {

typedef Label* LabelPtr;

struct Context {
  Context();

  SourceErrorHandler* error_handler = nullptr;
  AstLexer* lexer = nullptr;
  Script* script = nullptr;
  Module* current_module = nullptr;
  Func* current_func = nullptr;
  ExprVisitor visitor;
  std::vector<Label*> labels;
  Result result = Result::Ok;
};

Context::Context() {
  WABT_ZERO_MEMORY(visitor);
}

}  // namespace

static void WABT_PRINTF_FORMAT(3, 4)
    print_error(Context* ctx, const Location* loc, const char* fmt, ...) {
  ctx->result = Result::Error;
  va_list args;
  va_start(args, fmt);
  ast_format_error(ctx->error_handler, loc, ctx->lexer, fmt, args);
  va_end(args);
}

static void push_label(Context* ctx, Label* label) {
  ctx->labels.push_back(label);
}

static void pop_label(Context* ctx) {
  ctx->labels.pop_back();
}

struct FindDuplicateBindingContext {
  Context* ctx;
  const char* desc;
};

static void on_duplicate_binding(const BindingHash::value_type& a,
                                 const BindingHash::value_type& b,
                                 void* user_data) {
  FindDuplicateBindingContext* fdbc =
      static_cast<FindDuplicateBindingContext*>(user_data);
  /* choose the location that is later in the file */
  const Location& a_loc = a.second.loc;
  const Location& b_loc = b.second.loc;
  const Location& loc = a_loc.line > b_loc.line ? a_loc : b_loc;
  print_error(fdbc->ctx, &loc, "redefinition of %s \"%s\"", fdbc->desc,
              a.first.c_str());
}

static void check_duplicate_bindings(Context* ctx,
                                     const BindingHash* bindings,
                                     const char* desc) {
  FindDuplicateBindingContext fdbc;
  fdbc.ctx = ctx;
  fdbc.desc = desc;
  bindings->find_duplicates(on_duplicate_binding, &fdbc);
}

static void resolve_label_var(Context* ctx, Var* var) {
  if (var->type == VarType::Name) {
    for (int i = ctx->labels.size() - 1; i >= 0; --i) {
      Label* label = ctx->labels[i];
      if (string_slices_are_equal(label, &var->name)) {
        destroy_string_slice(&var->name);
        var->type = VarType::Index;
        var->index = ctx->labels.size() - i - 1;
        return;
      }
    }
    print_error(ctx, &var->loc,
                "undefined label variable \"" PRIstringslice "\"",
                WABT_PRINTF_STRING_SLICE_ARG(var->name));
  }
}

static void resolve_var(Context* ctx,
                        const BindingHash* bindings,
                        Var* var,
                        const char* desc) {
  if (var->type == VarType::Name) {
    int index = get_index_from_var(bindings, var);
    if (index == -1) {
      print_error(ctx, &var->loc,
                  "undefined %s variable \"" PRIstringslice "\"", desc,
                  WABT_PRINTF_STRING_SLICE_ARG(var->name));
      return;
    }

    destroy_string_slice(&var->name);
    var->index = index;
    var->type = VarType::Index;
  }
}

static void resolve_func_var(Context* ctx, Var* var) {
  resolve_var(ctx, &ctx->current_module->func_bindings, var, "function");
}

static void resolve_global_var(Context* ctx, Var* var) {
  resolve_var(ctx, &ctx->current_module->global_bindings, var, "global");
}

static void resolve_func_type_var(Context* ctx, Var* var) {
  resolve_var(ctx, &ctx->current_module->func_type_bindings, var,
              "function type");
}

static void resolve_table_var(Context* ctx, Var* var) {
  resolve_var(ctx, &ctx->current_module->table_bindings, var, "table");
}

static void resolve_memory_var(Context* ctx, Var* var) {
  resolve_var(ctx, &ctx->current_module->memory_bindings, var, "memory");
}

static void resolve_local_var(Context* ctx, Var* var) {
  if (var->type == VarType::Name) {
    int index = get_local_index_by_var(ctx->current_func, var);
    if (index == -1) {
      print_error(ctx, &var->loc,
                  "undefined local variable \"" PRIstringslice "\"",
                  WABT_PRINTF_STRING_SLICE_ARG(var->name));
      return;
    }

    destroy_string_slice(&var->name);
    var->index = index;
    var->type = VarType::Index;
  }
}

static Result begin_block_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  push_label(ctx, &expr->block->label);
  return Result::Ok;
}

static Result end_block_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  pop_label(ctx);
  return Result::Ok;
}

static Result begin_loop_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  push_label(ctx, &expr->loop->label);
  return Result::Ok;
}

static Result end_loop_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  pop_label(ctx);
  return Result::Ok;
}

static Result on_br_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  resolve_label_var(ctx, &expr->br.var);
  return Result::Ok;
}

static Result on_br_if_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  resolve_label_var(ctx, &expr->br_if.var);
  return Result::Ok;
}

static Result on_br_table_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  for (Var& target: *expr->br_table.targets)
    resolve_label_var(ctx, &target);
  resolve_label_var(ctx, &expr->br_table.default_target);
  return Result::Ok;
}

static Result on_call_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  resolve_func_var(ctx, &expr->call.var);
  return Result::Ok;
}

static Result on_call_indirect_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  resolve_func_type_var(ctx, &expr->call_indirect.var);
  return Result::Ok;
}

static Result on_get_global_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  resolve_global_var(ctx, &expr->get_global.var);
  return Result::Ok;
}

static Result on_get_local_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  resolve_local_var(ctx, &expr->get_local.var);
  return Result::Ok;
}

static Result begin_if_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  push_label(ctx, &expr->if_.true_->label);
  return Result::Ok;
}

static Result end_if_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  pop_label(ctx);
  return Result::Ok;
}

static Result on_set_global_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  resolve_global_var(ctx, &expr->set_global.var);
  return Result::Ok;
}

static Result on_set_local_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  resolve_local_var(ctx, &expr->set_local.var);
  return Result::Ok;
}

static Result on_tee_local_expr(Expr* expr, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  resolve_local_var(ctx, &expr->tee_local.var);
  return Result::Ok;
}

static void visit_func(Context* ctx, Func* func) {
  ctx->current_func = func;
  if (decl_has_func_type(&func->decl))
    resolve_func_type_var(ctx, &func->decl.type_var);

  check_duplicate_bindings(ctx, &func->param_bindings, "parameter");
  check_duplicate_bindings(ctx, &func->local_bindings, "local");

  visit_func(func, &ctx->visitor);
  ctx->current_func = nullptr;
}

static void visit_export(Context* ctx, Export* export_) {
  switch (export_->kind) {
    case ExternalKind::Func:
      resolve_func_var(ctx, &export_->var);
      break;

    case ExternalKind::Table:
      resolve_table_var(ctx, &export_->var);
      break;

    case ExternalKind::Memory:
      resolve_memory_var(ctx, &export_->var);
      break;

    case ExternalKind::Global:
      resolve_global_var(ctx, &export_->var);
      break;
  }
}

static void visit_global(Context* ctx, Global* global) {
  visit_expr_list(global->init_expr, &ctx->visitor);
}

static void visit_elem_segment(Context* ctx, ElemSegment* segment) {
  resolve_table_var(ctx, &segment->table_var);
  visit_expr_list(segment->offset, &ctx->visitor);
  for (Var& var: segment->vars)
    resolve_func_var(ctx, &var);
}

static void visit_data_segment(Context* ctx, DataSegment* segment) {
  resolve_memory_var(ctx, &segment->memory_var);
  visit_expr_list(segment->offset, &ctx->visitor);
}

static void visit_module(Context* ctx, Module* module) {
  ctx->current_module = module;
  check_duplicate_bindings(ctx, &module->func_bindings, "function");
  check_duplicate_bindings(ctx, &module->global_bindings, "global");
  check_duplicate_bindings(ctx, &module->func_type_bindings, "function type");
  check_duplicate_bindings(ctx, &module->table_bindings, "table");
  check_duplicate_bindings(ctx, &module->memory_bindings, "memory");

  for (Func* func : module->funcs)
    visit_func(ctx, func);
  for (Export* export_ : module->exports)
    visit_export(ctx, export_);
  for (Global* global : module->globals)
    visit_global(ctx, global);
  for (ElemSegment* elem_segment : module->elem_segments)
    visit_elem_segment(ctx, elem_segment);
  for (DataSegment* data_segment : module->data_segments)
    visit_data_segment(ctx, data_segment);
  if (module->start)
    resolve_func_var(ctx, module->start);
  ctx->current_module = nullptr;
}

static void visit_raw_module(Context* ctx, RawModule* raw_module) {
  if (raw_module->type == RawModuleType::Text)
    visit_module(ctx, raw_module->text);
}

static bool dummy_source_error_callback(const Location* loc,
                                        const char* error,
                                        const char* source_line,
                                        size_t source_line_length,
                                        size_t source_line_column_offset,
                                        void* user_data) {
  return false;
}

static void visit_command(Context* ctx, Command* command) {
  switch (command->type) {
    case CommandType::Module:
      visit_module(ctx, command->module);
      break;

    case CommandType::Action:
    case CommandType::AssertReturn:
    case CommandType::AssertReturnCanonicalNan:
    case CommandType::AssertReturnArithmeticNan:
    case CommandType::AssertTrap:
    case CommandType::AssertExhaustion:
    case CommandType::Register:
      /* Don't resolve a module_var, since it doesn't really behave like other
       * vars. You can't reference a module by index. */
      break;

    case CommandType::AssertMalformed:
      /* Malformed modules should not be text; the whole point of this
       * assertion is to test for malformed binary modules. */
      break;

    case CommandType::AssertInvalid: {
      /* The module may be invalid because the names cannot be resolved; we
       * don't want to print errors or fail if that's the case, but we still
       * should try to resolve names when possible. */
      SourceErrorHandler new_error_handler;
      new_error_handler.on_error = dummy_source_error_callback;
      new_error_handler.source_line_max_length =
          ctx->error_handler->source_line_max_length;

      Context new_ctx;
      new_ctx.error_handler = &new_error_handler;
      new_ctx.lexer = ctx->lexer;
      new_ctx.visitor = ctx->visitor;
      new_ctx.visitor.user_data = &new_ctx;
      new_ctx.result = Result::Ok;

      visit_raw_module(&new_ctx, command->assert_invalid.module);
      if (WABT_FAILED(new_ctx.result)) {
        command->type = CommandType::AssertInvalidNonBinary;
      }
      break;
    }

    case CommandType::AssertInvalidNonBinary:
      /* The only reason a module would be "non-binary" is if the names cannot
       * be resolved. So we assume name resolution has already been tried and
       * failed, so skip it. */
      break;

    case CommandType::AssertUnlinkable:
      visit_raw_module(ctx, command->assert_unlinkable.module);
      break;

    case CommandType::AssertUninstantiable:
      visit_raw_module(ctx, command->assert_uninstantiable.module);
      break;
  }
}

static void visit_script(Context* ctx, Script* script) {
  for (const std::unique_ptr<Command>& command: script->commands)
    visit_command(ctx, command.get());
}

static void init_context(Context* ctx,
                         AstLexer* lexer,
                         Script* script,
                         SourceErrorHandler* error_handler) {
  ctx->lexer = lexer;
  ctx->error_handler = error_handler;
  ctx->result = Result::Ok;
  ctx->script = script;
  ctx->visitor.user_data = ctx;
  ctx->visitor.begin_block_expr = begin_block_expr;
  ctx->visitor.end_block_expr = end_block_expr;
  ctx->visitor.begin_loop_expr = begin_loop_expr;
  ctx->visitor.end_loop_expr = end_loop_expr;
  ctx->visitor.on_br_expr = on_br_expr;
  ctx->visitor.on_br_if_expr = on_br_if_expr;
  ctx->visitor.on_br_table_expr = on_br_table_expr;
  ctx->visitor.on_call_expr = on_call_expr;
  ctx->visitor.on_call_indirect_expr = on_call_indirect_expr;
  ctx->visitor.on_get_global_expr = on_get_global_expr;
  ctx->visitor.on_get_local_expr = on_get_local_expr;
  ctx->visitor.begin_if_expr = begin_if_expr;
  ctx->visitor.end_if_expr = end_if_expr;
  ctx->visitor.on_set_global_expr = on_set_global_expr;
  ctx->visitor.on_set_local_expr = on_set_local_expr;
  ctx->visitor.on_tee_local_expr = on_tee_local_expr;
}

Result resolve_names_module(AstLexer* lexer,
                            Module* module,
                            SourceErrorHandler* error_handler) {
  Context ctx;
  init_context(&ctx, lexer, nullptr, error_handler);
  visit_module(&ctx, module);
  return ctx.result;
}

Result resolve_names_script(AstLexer* lexer,
                            Script* script,
                            SourceErrorHandler* error_handler) {
  Context ctx;
  init_context(&ctx, lexer, script, error_handler);
  visit_script(&ctx, script);
  return ctx.result;
}

}  // namespace wabt
