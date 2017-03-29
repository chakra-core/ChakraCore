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

#include "validator.h"
#include "config.h"

#include <assert.h>
#include <inttypes.h>
#include <memory.h>
#include <stdarg.h>
#include <stdio.h>

#include "ast-parser-lexer-shared.h"
#include "binary-reader-ast.h"
#include "binary-reader.h"
#include "type-checker.h"

namespace wabt {

namespace {

enum class ActionResultKind {
  Error,
  Types,
  Type,
};

struct ActionResult {
  ActionResultKind kind;
  union {
    const TypeVector* types;
    Type type;
  };
};

struct Context {
  WABT_DISALLOW_COPY_AND_ASSIGN(Context);
  Context(SourceErrorHandler*, AstLexer*, const Script*);

  SourceErrorHandler* error_handler = nullptr;
  AstLexer* lexer = nullptr;
  const Script* script = nullptr;
  const Module* current_module = nullptr;
  const Func* current_func = nullptr;
  int current_table_index = 0;
  int current_memory_index = 0;
  int current_global_index = 0;
  int num_imported_globals = 0;
  TypeChecker typechecker;
  /* Cached for access by on_typechecker_error */
  const Location* expr_loc = nullptr;
  Result result = Result::Ok;
};

Context::Context(SourceErrorHandler* error_handler,
                 AstLexer* lexer,
                 const Script* script)
    : error_handler(error_handler), lexer(lexer), script(script) {}

}  // namespace

static void WABT_PRINTF_FORMAT(3, 4)
    print_error(Context* ctx, const Location* loc, const char* fmt, ...) {
  ctx->result = Result::Error;
  va_list args;
  va_start(args, fmt);
  ast_format_error(ctx->error_handler, loc, ctx->lexer, fmt, args);
  va_end(args);
}

static void on_typechecker_error(const char* msg, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  print_error(ctx, ctx->expr_loc, "%s", msg);
}

static bool is_power_of_two(uint32_t x) {
  return x && ((x & (x - 1)) == 0);
}

static uint32_t get_opcode_natural_alignment(Opcode opcode) {
  uint32_t memory_size = get_opcode_memory_size(opcode);
  assert(memory_size != 0);
  return memory_size;
}

static Result check_var(Context* ctx,
                        int max_index,
                        const Var* var,
                        const char* desc,
                        int* out_index) {
  assert(var->type == VarType::Index);
  if (var->index >= 0 && var->index < max_index) {
    if (out_index)
      *out_index = var->index;
    return Result::Ok;
  }
  print_error(ctx, &var->loc, "%s variable out of range (max %d)", desc,
              max_index);
  return Result::Error;
}

static Result check_func_var(Context* ctx,
                             const Var* var,
                             const Func** out_func) {
  int index;
  if (WABT_FAILED(check_var(ctx, ctx->current_module->funcs.size(), var,
                            "function", &index))) {
    return Result::Error;
  }

  if (out_func)
    *out_func = ctx->current_module->funcs[index];
  return Result::Ok;
}

static Result check_global_var(Context* ctx,
                               const Var* var,
                               const Global** out_global,
                               int* out_global_index) {
  int index;
  if (WABT_FAILED(check_var(ctx, ctx->current_module->globals.size(), var,
                            "global", &index))) {
    return Result::Error;
  }

  if (out_global)
    *out_global = ctx->current_module->globals[index];
  if (out_global_index)
    *out_global_index = index;
  return Result::Ok;
}

static Type get_global_var_type_or_any(Context* ctx, const Var* var) {
  const Global* global;
  if (WABT_SUCCEEDED(check_global_var(ctx, var, &global, nullptr)))
    return global->type;
  return Type::Any;
}

static Result check_func_type_var(Context* ctx,
                                  const Var* var,
                                  const FuncType** out_func_type) {
  int index;
  if (WABT_FAILED(check_var(ctx, ctx->current_module->func_types.size(), var,
                            "function type", &index))) {
    return Result::Error;
  }

  if (out_func_type)
    *out_func_type = ctx->current_module->func_types[index];
  return Result::Ok;
}

static Result check_table_var(Context* ctx,
                              const Var* var,
                              const Table** out_table) {
  int index;
  if (WABT_FAILED(check_var(ctx, ctx->current_module->tables.size(), var,
                            "table", &index))) {
    return Result::Error;
  }

  if (out_table)
    *out_table = ctx->current_module->tables[index];
  return Result::Ok;
}

static Result check_memory_var(Context* ctx,
                               const Var* var,
                               const Memory** out_memory) {
  int index;
  if (WABT_FAILED(check_var(ctx, ctx->current_module->memories.size(), var,
                            "memory", &index))) {
    return Result::Error;
  }

  if (out_memory)
    *out_memory = ctx->current_module->memories[index];
  return Result::Ok;
}

static Result check_local_var(Context* ctx, const Var* var, Type* out_type) {
  const Func* func = ctx->current_func;
  int max_index = get_num_params_and_locals(func);
  int index = get_local_index_by_var(func, var);
  if (index >= 0 && index < max_index) {
    if (out_type) {
      int num_params = get_num_params(func);
      if (index < num_params) {
        *out_type = get_param_type(func, index);
      } else {
        *out_type = ctx->current_func->local_types[index - num_params];
      }
    }
    return Result::Ok;
  }

  if (var->type == VarType::Name) {
    print_error(ctx, &var->loc,
                "undefined local variable \"" PRIstringslice "\"",
                WABT_PRINTF_STRING_SLICE_ARG(var->name));
  } else {
    print_error(ctx, &var->loc, "local variable out of range (max %d)",
                max_index);
  }
  return Result::Error;
}

static Type get_local_var_type_or_any(Context* ctx, const Var* var) {
  Type type = Type::Any;
  check_local_var(ctx, var, &type);
  return type;
}

static void check_align(Context* ctx,
                        const Location* loc,
                        uint32_t alignment,
                        uint32_t natural_alignment) {
  if (alignment != WABT_USE_NATURAL_ALIGNMENT) {
    if (!is_power_of_two(alignment))
      print_error(ctx, loc, "alignment must be power-of-two");
    if (alignment > natural_alignment) {
      print_error(ctx, loc,
                  "alignment must not be larger than natural alignment (%u)",
                  natural_alignment);
    }
  }
}

static void check_offset(Context* ctx, const Location* loc, uint64_t offset) {
  if (offset > UINT32_MAX) {
    print_error(ctx, loc, "offset must be less than or equal to 0xffffffff");
  }
}

static void check_type(Context* ctx,
                       const Location* loc,
                       Type actual,
                       Type expected,
                       const char* desc) {
  if (expected != actual) {
    print_error(ctx, loc, "type mismatch at %s. got %s, expected %s", desc,
                get_type_name(actual), get_type_name(expected));
  }
}

static void check_type_index(Context* ctx,
                             const Location* loc,
                             Type actual,
                             Type expected,
                             const char* desc,
                             int index,
                             const char* index_kind) {
  if (expected != actual && expected != Type::Any && actual != Type::Any) {
    print_error(ctx, loc, "type mismatch for %s %d of %s. got %s, expected %s",
                index_kind, index, desc, get_type_name(actual),
                get_type_name(expected));
  }
}

static void check_types(Context* ctx,
                        const Location* loc,
                        const TypeVector& actual,
                        const TypeVector& expected,
                        const char* desc,
                        const char* index_kind) {
  if (actual.size() == expected.size()) {
    for (size_t i = 0; i < actual.size(); ++i) {
      check_type_index(ctx, loc, actual[i], expected[i], desc, i, index_kind);
    }
  } else {
    print_error(ctx, loc, "expected %" PRIzd " %ss, got %" PRIzd,
                expected.size(), index_kind, actual.size());
  }
}

static void check_const_types(Context* ctx,
                              const Location* loc,
                              const TypeVector& actual,
                              const ConstVector& expected,
                              const char* desc) {
  if (actual.size() == expected.size()) {
    for (size_t i = 0; i < actual.size(); ++i) {
      check_type_index(ctx, loc, actual[i], expected[i].type, desc, i,
                       "result");
    }
  } else {
    print_error(ctx, loc, "expected %" PRIzd " results, got %" PRIzd,
                expected.size(), actual.size());
  }
}

static void check_const_type(Context* ctx,
                             const Location* loc,
                             Type actual,
                             const ConstVector& expected,
                             const char* desc) {
  TypeVector actual_types;
  if (actual != Type::Void)
    actual_types.push_back(actual);
  check_const_types(ctx, loc, actual_types, expected, desc);
}

static void check_assert_return_nan_type(Context* ctx,
                                         const Location* loc,
                                         Type actual,
                                         const char* desc) {
  /* when using assert_return_nan, the result can be either a f32 or f64 type
   * so we special case it here. */
  if (actual != Type::F32 && actual != Type::F64) {
    print_error(ctx, loc, "type mismatch at %s. got %s, expected f32 or f64",
                desc, get_type_name(actual));
  }
}

static void check_expr(Context* ctx, const Expr* expr);

static void check_expr_list(Context* ctx,
                            const Location* loc,
                            const Expr* first) {
  if (first) {
    for (const Expr* expr = first; expr; expr = expr->next)
      check_expr(ctx, expr);
  }
}

static void check_has_memory(Context* ctx, const Location* loc, Opcode opcode) {
  if (ctx->current_module->memories.size() == 0) {
    print_error(ctx, loc, "%s requires an imported or defined memory.",
                get_opcode_name(opcode));
  }
}

static void check_expr(Context* ctx, const Expr* expr) {
  ctx->expr_loc = &expr->loc;

  switch (expr->type) {
    case ExprType::Binary:
      typechecker_on_binary(&ctx->typechecker, expr->binary.opcode);
      break;

    case ExprType::Block:
      typechecker_on_block(&ctx->typechecker, &expr->block->sig);
      check_expr_list(ctx, &expr->loc, expr->block->first);
      typechecker_on_end(&ctx->typechecker);
      break;

    case ExprType::Br:
      typechecker_on_br(&ctx->typechecker, expr->br.var.index);
      break;

    case ExprType::BrIf:
      typechecker_on_br_if(&ctx->typechecker, expr->br_if.var.index);
      break;

    case ExprType::BrTable: {
      typechecker_begin_br_table(&ctx->typechecker);
      for (Var& var: *expr->br_table.targets) {
        typechecker_on_br_table_target(&ctx->typechecker, var.index);
      }
      typechecker_on_br_table_target(&ctx->typechecker,
                                     expr->br_table.default_target.index);
      typechecker_end_br_table(&ctx->typechecker);
      break;
    }

    case ExprType::Call: {
      const Func* callee;
      if (WABT_SUCCEEDED(check_func_var(ctx, &expr->call.var, &callee))) {
        typechecker_on_call(&ctx->typechecker, &callee->decl.sig.param_types,
                            &callee->decl.sig.result_types);
      }
      break;
    }

    case ExprType::CallIndirect: {
      const FuncType* func_type;
      if (ctx->current_module->tables.size() == 0) {
        print_error(ctx, &expr->loc,
                    "found call_indirect operator, but no table");
      }
      if (WABT_SUCCEEDED(
              check_func_type_var(ctx, &expr->call_indirect.var, &func_type))) {
        typechecker_on_call_indirect(&ctx->typechecker,
                                     &func_type->sig.param_types,
                                     &func_type->sig.result_types);
      }
      break;
    }

    case ExprType::Compare:
      typechecker_on_compare(&ctx->typechecker, expr->compare.opcode);
      break;

    case ExprType::Const:
      typechecker_on_const(&ctx->typechecker, expr->const_.type);
      break;

    case ExprType::Convert:
      typechecker_on_convert(&ctx->typechecker, expr->convert.opcode);
      break;

    case ExprType::Drop:
      typechecker_on_drop(&ctx->typechecker);
      break;

    case ExprType::GetGlobal:
      typechecker_on_get_global(
          &ctx->typechecker,
          get_global_var_type_or_any(ctx, &expr->get_global.var));
      break;

    case ExprType::GetLocal:
      typechecker_on_get_local(
          &ctx->typechecker,
          get_local_var_type_or_any(ctx, &expr->get_local.var));
      break;

    case ExprType::GrowMemory:
      check_has_memory(ctx, &expr->loc, Opcode::GrowMemory);
      typechecker_on_grow_memory(&ctx->typechecker);
      break;

    case ExprType::If:
      typechecker_on_if(&ctx->typechecker, &expr->if_.true_->sig);
      check_expr_list(ctx, &expr->loc, expr->if_.true_->first);
      if (expr->if_.false_) {
        typechecker_on_else(&ctx->typechecker);
        check_expr_list(ctx, &expr->loc, expr->if_.false_);
      }
      typechecker_on_end(&ctx->typechecker);
      break;

    case ExprType::Load:
      check_has_memory(ctx, &expr->loc, expr->load.opcode);
      check_align(ctx, &expr->loc, expr->load.align,
                  get_opcode_natural_alignment(expr->load.opcode));
      check_offset(ctx, &expr->loc, expr->load.offset);
      typechecker_on_load(&ctx->typechecker, expr->load.opcode);
      break;

    case ExprType::Loop:
      typechecker_on_loop(&ctx->typechecker, &expr->loop->sig);
      check_expr_list(ctx, &expr->loc, expr->loop->first);
      typechecker_on_end(&ctx->typechecker);
      break;

    case ExprType::CurrentMemory:
      check_has_memory(ctx, &expr->loc, Opcode::CurrentMemory);
      typechecker_on_current_memory(&ctx->typechecker);
      break;

    case ExprType::Nop:
      break;

    case ExprType::Return:
      typechecker_on_return(&ctx->typechecker);
      break;

    case ExprType::Select:
      typechecker_on_select(&ctx->typechecker);
      break;

    case ExprType::SetGlobal:
      typechecker_on_set_global(
          &ctx->typechecker,
          get_global_var_type_or_any(ctx, &expr->set_global.var));
      break;

    case ExprType::SetLocal:
      typechecker_on_set_local(
          &ctx->typechecker,
          get_local_var_type_or_any(ctx, &expr->set_local.var));
      break;

    case ExprType::Store:
      check_has_memory(ctx, &expr->loc, expr->store.opcode);
      check_align(ctx, &expr->loc, expr->store.align,
                  get_opcode_natural_alignment(expr->store.opcode));
      check_offset(ctx, &expr->loc, expr->store.offset);
      typechecker_on_store(&ctx->typechecker, expr->store.opcode);
      break;

    case ExprType::TeeLocal:
      typechecker_on_tee_local(
          &ctx->typechecker,
          get_local_var_type_or_any(ctx, &expr->tee_local.var));
      break;

    case ExprType::Unary:
      typechecker_on_unary(&ctx->typechecker, expr->unary.opcode);
      break;

    case ExprType::Unreachable:
      typechecker_on_unreachable(&ctx->typechecker);
      break;
  }
}

static void check_func_signature_matches_func_type(Context* ctx,
                                                   const Location* loc,
                                                   const FuncSignature& sig,
                                                   const FuncType* func_type) {
  check_types(ctx, loc, sig.result_types, func_type->sig.result_types,
              "function", "result");
  check_types(ctx, loc, sig.param_types, func_type->sig.param_types, "function",
              "argument");
}

static void check_func(Context* ctx, const Location* loc, const Func* func) {
  ctx->current_func = func;
  if (get_num_results(func) > 1) {
    print_error(ctx, loc, "multiple result values not currently supported.");
    /* don't run any other checks, the won't test the result_type properly */
    return;
  }
  if (decl_has_func_type(&func->decl)) {
    const FuncType* func_type;
    if (WABT_SUCCEEDED(
            check_func_type_var(ctx, &func->decl.type_var, &func_type))) {
      check_func_signature_matches_func_type(ctx, loc, func->decl.sig,
                                             func_type);
    }
  }

  ctx->expr_loc = loc;
  typechecker_begin_function(&ctx->typechecker, &func->decl.sig.result_types);
  check_expr_list(ctx, loc, func->first_expr);
  typechecker_end_function(&ctx->typechecker);
  ctx->current_func = nullptr;
}

static void print_const_expr_error(Context* ctx,
                                   const Location* loc,
                                   const char* desc) {
  print_error(ctx, loc,
              "invalid %s, must be a constant expression; either *.const or "
              "get_global.",
              desc);
}

static void check_const_init_expr(Context* ctx,
                                  const Location* loc,
                                  const Expr* expr,
                                  Type expected_type,
                                  const char* desc) {
  Type type = Type::Void;
  if (expr) {
    if (expr->next) {
      print_const_expr_error(ctx, loc, desc);
      return;
    }

    switch (expr->type) {
      case ExprType::Const:
        type = expr->const_.type;
        break;

      case ExprType::GetGlobal: {
        const Global* ref_global = nullptr;
        int ref_global_index;
        if (WABT_FAILED(check_global_var(ctx, &expr->get_global.var,
                                         &ref_global, &ref_global_index))) {
          return;
        }

        type = ref_global->type;
        /* globals can only reference previously defined, internal globals */
        if (ref_global_index >= ctx->current_global_index) {
          print_error(ctx, loc,
                      "initializer expression can only reference a previously "
                      "defined global");
        } else if (ref_global_index >= ctx->num_imported_globals) {
          print_error(
              ctx, loc,
              "initializer expression can only reference an imported global");
        }

        if (ref_global->mutable_) {
          print_error(
              ctx, loc,
              "initializer expression cannot reference a mutable global");
        }
        break;
      }

      default:
        print_const_expr_error(ctx, loc, desc);
        return;
    }
  }

  check_type(ctx, expr ? &expr->loc : loc, type, expected_type, desc);
}

static void check_global(Context* ctx,
                         const Location* loc,
                         const Global* global) {
  check_const_init_expr(ctx, loc, global->init_expr, global->type,
                        "global initializer expression");
}

static void check_limits(Context* ctx,
                         const Location* loc,
                         const Limits* limits,
                         uint64_t absolute_max,
                         const char* desc) {
  if (limits->initial > absolute_max) {
    print_error(ctx, loc, "initial %s (%" PRIu64 ") must be <= (%" PRIu64 ")",
                desc, limits->initial, absolute_max);
  }

  if (limits->has_max) {
    if (limits->max > absolute_max) {
      print_error(ctx, loc, "max %s (%" PRIu64 ") must be <= (%" PRIu64 ")",
                  desc, limits->max, absolute_max);
    }

    if (limits->max < limits->initial) {
      print_error(ctx, loc,
                  "max %s (%" PRIu64 ") must be >= initial %s (%" PRIu64 ")",
                  desc, limits->max, desc, limits->initial);
    }
  }
}

static void check_table(Context* ctx, const Location* loc, const Table* table) {
  if (ctx->current_table_index == 1)
    print_error(ctx, loc, "only one table allowed");
  check_limits(ctx, loc, &table->elem_limits, UINT32_MAX, "elems");
}

static void check_elem_segments(Context* ctx, const Module* module) {
  for (ModuleField* field = module->first_field; field; field = field->next) {
    if (field->type != ModuleFieldType::ElemSegment)
      continue;

    ElemSegment* elem_segment = field->elem_segment;
    const Table* table;
    if (!WABT_SUCCEEDED(check_table_var(ctx, &elem_segment->table_var, &table)))
      continue;

    for (const Var& var: elem_segment->vars) {
      if (!WABT_SUCCEEDED(check_func_var(ctx, &var, nullptr)))
        continue;
    }

    check_const_init_expr(ctx, &field->loc, elem_segment->offset, Type::I32,
                          "elem segment offset");
  }
}

static void check_memory(Context* ctx,
                         const Location* loc,
                         const Memory* memory) {
  if (ctx->current_memory_index == 1)
    print_error(ctx, loc, "only one memory block allowed");
  check_limits(ctx, loc, &memory->page_limits, WABT_MAX_PAGES, "pages");
}

static void check_data_segments(Context* ctx, const Module* module) {
  for (ModuleField* field = module->first_field; field; field = field->next) {
    if (field->type != ModuleFieldType::DataSegment)
      continue;

    DataSegment* data_segment = field->data_segment;
    const Memory* memory;
    if (!WABT_SUCCEEDED(
            check_memory_var(ctx, &data_segment->memory_var, &memory)))
      continue;

    check_const_init_expr(ctx, &field->loc, data_segment->offset, Type::I32,
                          "data segment offset");
  }
}

static void check_import(Context* ctx,
                         const Location* loc,
                         const Import* import) {
  switch (import->kind) {
    case ExternalKind::Func:
      if (decl_has_func_type(&import->func->decl))
        check_func_type_var(ctx, &import->func->decl.type_var, nullptr);
      break;
    case ExternalKind::Table:
      check_table(ctx, loc, import->table);
      ctx->current_table_index++;
      break;
    case ExternalKind::Memory:
      check_memory(ctx, loc, import->memory);
      ctx->current_memory_index++;
      break;
    case ExternalKind::Global:
      if (import->global->mutable_) {
        print_error(ctx, loc, "mutable globals cannot be imported");
      }
      ctx->num_imported_globals++;
      ctx->current_global_index++;
      break;
  }
}

static void check_export(Context* ctx, const Export* export_) {
  switch (export_->kind) {
    case ExternalKind::Func:
      check_func_var(ctx, &export_->var, nullptr);
      break;
    case ExternalKind::Table:
      check_table_var(ctx, &export_->var, nullptr);
      break;
    case ExternalKind::Memory:
      check_memory_var(ctx, &export_->var, nullptr);
      break;
    case ExternalKind::Global: {
      const Global* global;
      if (WABT_SUCCEEDED(
              check_global_var(ctx, &export_->var, &global, nullptr))) {
        if (global->mutable_) {
          print_error(ctx, &export_->var.loc,
                      "mutable globals cannot be exported");
        }
      }
      break;
    }
  }
}

static void on_duplicate_binding(const BindingHash::value_type& a,
                                 const BindingHash::value_type& b,
                                 void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  /* choose the location that is later in the file */
  const Location& a_loc = a.second.loc;
  const Location& b_loc = b.second.loc;
  const Location& loc = a_loc.line > b_loc.line ? a_loc : b_loc;
  print_error(ctx, &loc, "redefinition of export \"%s\"", a.first.c_str());
}

static void check_duplicate_export_bindings(Context* ctx,
                                            const Module* module) {
  module->export_bindings.find_duplicates(on_duplicate_binding, ctx);
}

static void check_module(Context* ctx, const Module* module) {
  bool seen_start = false;

  ctx->current_module = module;
  ctx->current_table_index = 0;
  ctx->current_memory_index = 0;
  ctx->current_global_index = 0;
  ctx->num_imported_globals = 0;

  for (ModuleField* field = module->first_field; field; field = field->next) {
    switch (field->type) {
      case ModuleFieldType::Func:
        check_func(ctx, &field->loc, field->func);
        break;

      case ModuleFieldType::Global:
        check_global(ctx, &field->loc, field->global);
        ctx->current_global_index++;
        break;

      case ModuleFieldType::Import:
        check_import(ctx, &field->loc, field->import);
        break;

      case ModuleFieldType::Export:
        check_export(ctx, field->export_);
        break;

      case ModuleFieldType::Table:
        check_table(ctx, &field->loc, field->table);
        ctx->current_table_index++;
        break;

      case ModuleFieldType::ElemSegment:
        /* checked below */
        break;

      case ModuleFieldType::Memory:
        check_memory(ctx, &field->loc, field->memory);
        ctx->current_memory_index++;
        break;

      case ModuleFieldType::DataSegment:
        /* checked below */
        break;

      case ModuleFieldType::FuncType:
        break;

      case ModuleFieldType::Start: {
        if (seen_start) {
          print_error(ctx, &field->loc, "only one start function allowed");
        }

        const Func* start_func = nullptr;
        check_func_var(ctx, &field->start, &start_func);
        if (start_func) {
          if (get_num_params(start_func) != 0) {
            print_error(ctx, &field->loc, "start function must be nullary");
          }

          if (get_num_results(start_func) != 0) {
            print_error(ctx, &field->loc,
                        "start function must not return anything");
          }
        }
        seen_start = true;
        break;
      }
    }
  }

  check_elem_segments(ctx, module);
  check_data_segments(ctx, module);
  check_duplicate_export_bindings(ctx, module);
}

/* returns the result type of the invoked function, checked by the caller;
 * returning nullptr means that another error occured first, so the result type
 * should be ignored. */
static const TypeVector* check_invoke(Context* ctx, const Action* action) {
  const ActionInvoke* invoke = action->invoke;
  const Module* module = get_module_by_var(ctx->script, &action->module_var);
  if (!module) {
    print_error(ctx, &action->loc, "unknown module");
    return nullptr;
  }

  Export* export_ = get_export_by_name(module, &action->name);
  if (!export_) {
    print_error(ctx, &action->loc,
                "unknown function export \"" PRIstringslice "\"",
                WABT_PRINTF_STRING_SLICE_ARG(action->name));
    return nullptr;
  }

  Func* func = get_func_by_var(module, &export_->var);
  if (!func) {
    /* this error will have already been reported, just skip it */
    return nullptr;
  }

  size_t actual_args = invoke->args.size();
  size_t expected_args = get_num_params(func);
  if (expected_args != actual_args) {
    print_error(ctx, &action->loc, "too %s parameters to function. got %" PRIzd
                                   ", expected %" PRIzd,
                actual_args > expected_args ? "many" : "few", actual_args,
                expected_args);
    return nullptr;
  }
  for (size_t i = 0; i < actual_args; ++i) {
    const Const* const_ = &invoke->args[i];
    check_type_index(ctx, &const_->loc, const_->type, get_param_type(func, i),
                     "invoke", i, "argument");
  }

  return &func->decl.sig.result_types;
}

static Result check_get(Context* ctx, const Action* action, Type* out_type) {
  const Module* module = get_module_by_var(ctx->script, &action->module_var);
  if (!module) {
    print_error(ctx, &action->loc, "unknown module");
    return Result::Error;
  }

  Export* export_ = get_export_by_name(module, &action->name);
  if (!export_) {
    print_error(ctx, &action->loc,
                "unknown global export \"" PRIstringslice "\"",
                WABT_PRINTF_STRING_SLICE_ARG(action->name));
    return Result::Error;
  }

  Global* global = get_global_by_var(module, &export_->var);
  if (!global) {
    /* this error will have already been reported, just skip it */
    return Result::Error;
  }

  *out_type = global->type;
  return Result::Ok;
}

static ActionResult check_action(Context* ctx, const Action* action) {
  ActionResult result;
  WABT_ZERO_MEMORY(result);

  switch (action->type) {
    case ActionType::Invoke:
      result.types = check_invoke(ctx, action);
      result.kind =
          result.types ? ActionResultKind::Types : ActionResultKind::Error;
      break;

    case ActionType::Get:
      if (WABT_SUCCEEDED(check_get(ctx, action, &result.type)))
        result.kind = ActionResultKind::Type;
      else
        result.kind = ActionResultKind::Error;
      break;
  }

  return result;
}

static void check_assert_return_nan_command(Context* ctx,
                                            const Action* action) {
  ActionResult result = check_action(ctx, action);

  /* a valid result type will either be f32 or f64; convert a TYPES result
   * into a TYPE result, so it is easier to check below. Type::Any is
   * used to specify a type that should not be checked (because an earlier
   * error occurred). */
  if (result.kind == ActionResultKind::Types) {
    if (result.types->size() == 1) {
      result.kind = ActionResultKind::Type;
      result.type = (*result.types)[0];
    } else {
      print_error(ctx, &action->loc, "expected 1 result, got %" PRIzd,
                  result.types->size());
      result.type = Type::Any;
    }
  }

  if (result.kind == ActionResultKind::Type && result.type != Type::Any)
    check_assert_return_nan_type(ctx, &action->loc, result.type, "action");
}

static void check_command(Context* ctx, const Command* command) {
  switch (command->type) {
    case CommandType::Module:
      check_module(ctx, command->module);
      break;

    case CommandType::Action:
      /* ignore result type */
      check_action(ctx, command->action);
      break;

    case CommandType::Register:
    case CommandType::AssertMalformed:
    case CommandType::AssertInvalid:
    case CommandType::AssertInvalidNonBinary:
    case CommandType::AssertUnlinkable:
    case CommandType::AssertUninstantiable:
      /* ignore */
      break;

    case CommandType::AssertReturn: {
      const Action* action = command->assert_return.action;
      ActionResult result = check_action(ctx, action);
      switch (result.kind) {
        case ActionResultKind::Types:
          check_const_types(ctx, &action->loc, *result.types,
                            *command->assert_return.expected, "action");
          break;

        case ActionResultKind::Type:
          check_const_type(ctx, &action->loc, result.type,
                           *command->assert_return.expected, "action");
          break;

        case ActionResultKind::Error:
          /* error occurred, don't do any further checks */
          break;
      }
      break;
    }

    case CommandType::AssertReturnCanonicalNan:
      check_assert_return_nan_command(
          ctx, command->assert_return_canonical_nan.action);
      break;

    case CommandType::AssertReturnArithmeticNan:
      check_assert_return_nan_command(
          ctx, command->assert_return_arithmetic_nan.action);
      break;

    case CommandType::AssertTrap:
    case CommandType::AssertExhaustion:
      /* ignore result type */
      check_action(ctx, command->assert_trap.action);
      break;
  }
}

Result validate_script(AstLexer* lexer,
                       const struct Script* script,
                       SourceErrorHandler* error_handler) {
  Context ctx(error_handler, lexer, script);

  TypeCheckerErrorHandler tc_error_handler;
  tc_error_handler.on_error = on_typechecker_error;
  tc_error_handler.user_data = &ctx;
  ctx.typechecker.error_handler = &tc_error_handler;

  for (const std::unique_ptr<Command>& command : script->commands)
    check_command(&ctx, command.get());
  return ctx.result;
}

}  // namespace wabt
