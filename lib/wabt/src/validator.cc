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

#include <cassert>
#include <cinttypes>
#include <cstdarg>
#include <cstdio>

#include "binary-reader.h"
#include "source-error-handler.h"
#include "type-checker.h"
#include "wast-parser-lexer-shared.h"

namespace wabt {

namespace {

class Validator {
 public:
  WABT_DISALLOW_COPY_AND_ASSIGN(Validator);
  Validator(SourceErrorHandler*, WastLexer*, const Script*);

  Result CheckScript(const Script* script);

 private:
  struct ActionResult {
    enum class Kind {
      Error,
      Types,
      Type,
    } kind;

    union {
      const TypeVector* types;
      Type type;
    };
  };

  void WABT_PRINTF_FORMAT(3, 4)
      PrintError(const Location* loc, const char* fmt, ...);
  void OnTypecheckerError(const char* msg);
  Result CheckVar(Index max_index,
                  const Var* var,
                  const char* desc,
                  Index* out_index);
  Result CheckFuncVar(const Var* var, const Func** out_func);
  Result CheckGlobalVar(const Var* var,
                        const Global** out_global,
                        Index* out_global_index);
  Type GetGlobalVarTypeOrAny(const Var* var);
  Result CheckFuncTypeVar(const Var* var, const FuncType** out_func_type);
  Result CheckTableVar(const Var* var, const Table** out_table);
  Result CheckMemoryVar(const Var* var, const Memory** out_memory);
  Result CheckLocalVar(const Var* var, Type* out_type);
  Type GetLocalVarTypeOrAny(const Var* var);
  void CheckAlign(const Location* loc,
                  Address alignment,
                  Address natural_alignment);
  void CheckType(const Location* loc,
                 Type actual,
                 Type expected,
                 const char* desc);
  void CheckTypeIndex(const Location* loc,
                      Type actual,
                      Type expected,
                      const char* desc,
                      Index index,
                      const char* index_kind);
  void CheckTypes(const Location* loc,
                  const TypeVector& actual,
                  const TypeVector& expected,
                  const char* desc,
                  const char* index_kind);
  void CheckConstTypes(const Location* loc,
                       const TypeVector& actual,
                       const ConstVector& expected,
                       const char* desc);
  void CheckConstType(const Location* loc,
                      Type actual,
                      const ConstVector& expected,
                      const char* desc);
  void CheckAssertReturnNanType(const Location* loc,
                                Type actual,
                                const char* desc);
  void CheckExprList(const Location* loc, const Expr* first);
  void CheckHasMemory(const Location* loc, Opcode opcode);
  void CheckBlockSig(const Location* loc,
                     Opcode opcode,
                     const BlockSignature* sig);
  void CheckExpr(const Expr* expr);
  void CheckFuncSignatureMatchesFuncType(const Location* loc,
                                         const FuncSignature& sig,
                                         const FuncType* func_type);
  void CheckFunc(const Location* loc, const Func* func);
  void PrintConstExprError(const Location* loc, const char* desc);
  void CheckConstInitExpr(const Location* loc,
                          const Expr* expr,
                          Type expected_type,
                          const char* desc);
  void CheckGlobal(const Location* loc, const Global* global);
  void CheckLimits(const Location* loc,
                   const Limits* limits,
                   uint64_t absolute_max,
                   const char* desc);
  void CheckTable(const Location* loc, const Table* table);
  void CheckElemSegments(const Module* module);
  void CheckMemory(const Location* loc, const Memory* memory);
  void CheckDataSegments(const Module* module);
  void CheckImport(const Location* loc, const Import* import);
  void CheckExport(const Location* loc, const Export* export_);

  void CheckDuplicateExportBindings(const Module* module);
  void CheckModule(const Module* module);
  const TypeVector* CheckInvoke(const Action* action);
  Result CheckGet(const Action* action, Type* out_type);
  ActionResult CheckAction(const Action* action);
  void CheckAssertReturnNanCommand(const Action* action);
  void CheckCommand(const Command* command);

  SourceErrorHandler* error_handler_ = nullptr;
  WastLexer* lexer_ = nullptr;
  const Script* script_ = nullptr;
  const Module* current_module_ = nullptr;
  const Func* current_func_ = nullptr;
  Index current_table_index_ = 0;
  Index current_memory_index_ = 0;
  Index current_global_index_ = 0;
  Index num_imported_globals_ = 0;
  TypeChecker typechecker_;
  // Cached for access by OnTypecheckerError.
  const Location* expr_loc_ = nullptr;
  Result result_ = Result::Ok;
};

Validator::Validator(SourceErrorHandler* error_handler,
                     WastLexer* lexer,
                     const Script* script)
    : error_handler_(error_handler), lexer_(lexer), script_(script) {
  typechecker_.set_error_callback(
      [this](const char* msg) { OnTypecheckerError(msg); });
}

void Validator::PrintError(const Location* loc, const char* fmt, ...) {
  result_ = Result::Error;
  va_list args;
  va_start(args, fmt);
  wast_format_error(error_handler_, loc, lexer_, fmt, args);
  va_end(args);
}

void Validator::OnTypecheckerError(const char* msg) {
  PrintError(expr_loc_, "%s", msg);
}

static bool is_power_of_two(uint32_t x) {
  return x && ((x & (x - 1)) == 0);
}

static Address get_opcode_natural_alignment(Opcode opcode) {
  Address memory_size = get_opcode_memory_size(opcode);
  assert(memory_size != 0);
  return memory_size;
}

Result Validator::CheckVar(Index max_index,
                           const Var* var,
                           const char* desc,
                           Index* out_index) {
  assert(var->type == VarType::Index);
  if (var->index < max_index) {
    if (out_index)
      *out_index = var->index;
    return Result::Ok;
  }
  PrintError(&var->loc, "%s variable out of range (max %" PRIindex ")", desc,
             max_index);
  return Result::Error;
}

Result Validator::CheckFuncVar(const Var* var, const Func** out_func) {
  Index index;
  if (WABT_FAILED(
          CheckVar(current_module_->funcs.size(), var, "function", &index))) {
    return Result::Error;
  }

  if (out_func)
    *out_func = current_module_->funcs[index];
  return Result::Ok;
}

Result Validator::CheckGlobalVar(const Var* var,
                                 const Global** out_global,
                                 Index* out_global_index) {
  Index index;
  if (WABT_FAILED(
          CheckVar(current_module_->globals.size(), var, "global", &index))) {
    return Result::Error;
  }

  if (out_global)
    *out_global = current_module_->globals[index];
  if (out_global_index)
    *out_global_index = index;
  return Result::Ok;
}

Type Validator::GetGlobalVarTypeOrAny(const Var* var) {
  const Global* global;
  if (WABT_SUCCEEDED(CheckGlobalVar(var, &global, nullptr)))
    return global->type;
  return Type::Any;
}

Result Validator::CheckFuncTypeVar(const Var* var,
                                   const FuncType** out_func_type) {
  Index index;
  if (WABT_FAILED(CheckVar(current_module_->func_types.size(), var,
                           "function type", &index))) {
    return Result::Error;
  }

  if (out_func_type)
    *out_func_type = current_module_->func_types[index];
  return Result::Ok;
}

Result Validator::CheckTableVar(const Var* var, const Table** out_table) {
  Index index;
  if (WABT_FAILED(
          CheckVar(current_module_->tables.size(), var, "table", &index))) {
    return Result::Error;
  }

  if (out_table)
    *out_table = current_module_->tables[index];
  return Result::Ok;
}

Result Validator::CheckMemoryVar(const Var* var, const Memory** out_memory) {
  Index index;
  if (WABT_FAILED(
          CheckVar(current_module_->memories.size(), var, "memory", &index))) {
    return Result::Error;
  }

  if (out_memory)
    *out_memory = current_module_->memories[index];
  return Result::Ok;
}

Result Validator::CheckLocalVar(const Var* var, Type* out_type) {
  const Func* func = current_func_;
  Index max_index = func->GetNumParamsAndLocals();
  Index index = func->GetLocalIndex(*var);
  if (index < max_index) {
    if (out_type) {
      Index num_params = func->GetNumParams();
      if (index < num_params) {
        *out_type = func->GetParamType(index);
      } else {
        *out_type = current_func_->local_types[index - num_params];
      }
    }
    return Result::Ok;
  }

  if (var->type == VarType::Name) {
    PrintError(&var->loc, "undefined local variable \"" PRIstringslice "\"",
               WABT_PRINTF_STRING_SLICE_ARG(var->name));
  } else {
    PrintError(&var->loc, "local variable out of range (max %" PRIindex ")",
               max_index);
  }
  return Result::Error;
}

Type Validator::GetLocalVarTypeOrAny(const Var* var) {
  Type type = Type::Any;
  CheckLocalVar(var, &type);
  return type;
}

void Validator::CheckAlign(const Location* loc,
                           Address alignment,
                           Address natural_alignment) {
  if (alignment != WABT_USE_NATURAL_ALIGNMENT) {
    if (!is_power_of_two(alignment))
      PrintError(loc, "alignment must be power-of-two");
    if (alignment > natural_alignment) {
      PrintError(loc,
                 "alignment must not be larger than natural alignment (%u)",
                 natural_alignment);
    }
  }
}

void Validator::CheckType(const Location* loc,
                          Type actual,
                          Type expected,
                          const char* desc) {
  if (expected != actual) {
    PrintError(loc, "type mismatch at %s. got %s, expected %s", desc,
               get_type_name(actual), get_type_name(expected));
  }
}

void Validator::CheckTypeIndex(const Location* loc,
                               Type actual,
                               Type expected,
                               const char* desc,
                               Index index,
                               const char* index_kind) {
  if (expected != actual && expected != Type::Any && actual != Type::Any) {
    PrintError(loc,
               "type mismatch for %s %" PRIindex " of %s. got %s, expected %s",
               index_kind, index, desc, get_type_name(actual),
               get_type_name(expected));
  }
}

void Validator::CheckTypes(const Location* loc,
                           const TypeVector& actual,
                           const TypeVector& expected,
                           const char* desc,
                           const char* index_kind) {
  if (actual.size() == expected.size()) {
    for (size_t i = 0; i < actual.size(); ++i) {
      CheckTypeIndex(loc, actual[i], expected[i], desc, i, index_kind);
    }
  } else {
    PrintError(loc, "expected %" PRIzd " %ss, got %" PRIzd, expected.size(),
               index_kind, actual.size());
  }
}

void Validator::CheckConstTypes(const Location* loc,
                                const TypeVector& actual,
                                const ConstVector& expected,
                                const char* desc) {
  if (actual.size() == expected.size()) {
    for (size_t i = 0; i < actual.size(); ++i) {
      CheckTypeIndex(loc, actual[i], expected[i].type, desc, i, "result");
    }
  } else {
    PrintError(loc, "expected %" PRIzd " results, got %" PRIzd, expected.size(),
               actual.size());
  }
}

void Validator::CheckConstType(const Location* loc,
                               Type actual,
                               const ConstVector& expected,
                               const char* desc) {
  TypeVector actual_types;
  if (actual != Type::Void)
    actual_types.push_back(actual);
  CheckConstTypes(loc, actual_types, expected, desc);
}

void Validator::CheckAssertReturnNanType(const Location* loc,
                                         Type actual,
                                         const char* desc) {
  // When using assert_return_nan, the result can be either a f32 or f64 type
  // so we special case it here.
  if (actual != Type::F32 && actual != Type::F64) {
    PrintError(loc, "type mismatch at %s. got %s, expected f32 or f64", desc,
               get_type_name(actual));
  }
}

void Validator::CheckExprList(const Location* loc, const Expr* first) {
  if (first) {
    for (const Expr* expr = first; expr; expr = expr->next)
      CheckExpr(expr);
  }
}

void Validator::CheckHasMemory(const Location* loc, Opcode opcode) {
  if (current_module_->memories.size() == 0) {
    PrintError(loc, "%s requires an imported or defined memory.",
               get_opcode_name(opcode));
  }
}

void Validator::CheckBlockSig(const Location* loc,
                              Opcode opcode,
                              const BlockSignature* sig) {
  if (sig->size() > 1) {
    PrintError(loc,
               "multiple %s signature result types not currently supported.",
               get_opcode_name(opcode));
  }
}

void Validator::CheckExpr(const Expr* expr) {
  expr_loc_ = &expr->loc;

  switch (expr->type) {
    case ExprType::Binary:
      typechecker_.OnBinary(expr->binary.opcode);
      break;

    case ExprType::Block:
      CheckBlockSig(&expr->loc, Opcode::Block, &expr->block->sig);
      typechecker_.OnBlock(&expr->block->sig);
      CheckExprList(&expr->loc, expr->block->first);
      typechecker_.OnEnd();
      break;

    case ExprType::Br:
      typechecker_.OnBr(expr->br.var.index);
      break;

    case ExprType::BrIf:
      typechecker_.OnBrIf(expr->br_if.var.index);
      break;

    case ExprType::BrTable: {
      typechecker_.BeginBrTable();
      for (Var& var : *expr->br_table.targets) {
        typechecker_.OnBrTableTarget(var.index);
      }
      typechecker_.OnBrTableTarget(expr->br_table.default_target.index);
      typechecker_.EndBrTable();
      break;
    }

    case ExprType::Call: {
      const Func* callee;
      if (WABT_SUCCEEDED(CheckFuncVar(&expr->call.var, &callee))) {
        typechecker_.OnCall(&callee->decl.sig.param_types,
                            &callee->decl.sig.result_types);
      }
      break;
    }

    case ExprType::CallIndirect: {
      const FuncType* func_type;
      if (current_module_->tables.size() == 0) {
        PrintError(&expr->loc, "found call_indirect operator, but no table");
      }
      if (WABT_SUCCEEDED(
              CheckFuncTypeVar(&expr->call_indirect.var, &func_type))) {
        typechecker_.OnCallIndirect(&func_type->sig.param_types,
                                    &func_type->sig.result_types);
      }
      break;
    }

    case ExprType::Catch:
      // TODO(karlschimpf) Define.
      PrintError(&expr->loc, "Catch: don't know how to validate");
      break;

    case ExprType::CatchAll:
      // TODO(karlschimpf) Define.
      PrintError(&expr->loc, "CatchAll: don't know how to validate");
      break;

    case ExprType::Compare:
      typechecker_.OnCompare(expr->compare.opcode);
      break;

    case ExprType::Const:
      typechecker_.OnConst(expr->const_.type);
      break;

    case ExprType::Convert:
      typechecker_.OnConvert(expr->convert.opcode);
      break;

    case ExprType::Drop:
      typechecker_.OnDrop();
      break;

    case ExprType::GetGlobal:
      typechecker_.OnGetGlobal(GetGlobalVarTypeOrAny(&expr->get_global.var));
      break;

    case ExprType::GetLocal:
      typechecker_.OnGetLocal(GetLocalVarTypeOrAny(&expr->get_local.var));
      break;

    case ExprType::GrowMemory:
      CheckHasMemory(&expr->loc, Opcode::GrowMemory);
      typechecker_.OnGrowMemory();
      break;

    case ExprType::If:
      CheckBlockSig(&expr->loc, Opcode::If, &expr->if_.true_->sig);
      typechecker_.OnIf(&expr->if_.true_->sig);
      CheckExprList(&expr->loc, expr->if_.true_->first);
      if (expr->if_.false_) {
        typechecker_.OnElse();
        CheckExprList(&expr->loc, expr->if_.false_);
      }
      typechecker_.OnEnd();
      break;

    case ExprType::Load:
      CheckHasMemory(&expr->loc, expr->load.opcode);
      CheckAlign(&expr->loc, expr->load.align,
                 get_opcode_natural_alignment(expr->load.opcode));
      typechecker_.OnLoad(expr->load.opcode);
      break;

    case ExprType::Loop:
      CheckBlockSig(&expr->loc, Opcode::Loop, &expr->loop->sig);
      typechecker_.OnLoop(&expr->loop->sig);
      CheckExprList(&expr->loc, expr->loop->first);
      typechecker_.OnEnd();
      break;

    case ExprType::CurrentMemory:
      CheckHasMemory(&expr->loc, Opcode::CurrentMemory);
      typechecker_.OnCurrentMemory();
      break;

    case ExprType::Nop:
      break;

    case ExprType::Rethrow:
      // TODO(karlschimpf) Define.
      PrintError(&expr->loc, "Rethrow: don't know how to validate");
      break;

    case ExprType::Return:
      typechecker_.OnReturn();
      break;

    case ExprType::Select:
      typechecker_.OnSelect();
      break;

    case ExprType::SetGlobal:
      typechecker_.OnSetGlobal(GetGlobalVarTypeOrAny(&expr->set_global.var));
      break;

    case ExprType::SetLocal:
      typechecker_.OnSetLocal(GetLocalVarTypeOrAny(&expr->set_local.var));
      break;

    case ExprType::Store:
      CheckHasMemory(&expr->loc, expr->store.opcode);
      CheckAlign(&expr->loc, expr->store.align,
                 get_opcode_natural_alignment(expr->store.opcode));
      typechecker_.OnStore(expr->store.opcode);
      break;

    case ExprType::TeeLocal:
      typechecker_.OnTeeLocal(GetLocalVarTypeOrAny(&expr->tee_local.var));
      break;

    case ExprType::Throw:
      // TODO(karlschimpf) Define.
      PrintError(&expr->loc, "Throw: don't know how to validate");
      break;

    case ExprType::TryBlock:
      // TODO(karlschimpf) Define.
      PrintError(&expr->loc, "TryBlock: don't know how to validate");
      break;

    case ExprType::Unary:
      typechecker_.OnUnary(expr->unary.opcode);
      break;

    case ExprType::Unreachable:
      typechecker_.OnUnreachable();
      break;
  }
}

void Validator::CheckFuncSignatureMatchesFuncType(const Location* loc,
                                                  const FuncSignature& sig,
                                                  const FuncType* func_type) {
  CheckTypes(loc, sig.result_types, func_type->sig.result_types, "function",
             "result");
  CheckTypes(loc, sig.param_types, func_type->sig.param_types, "function",
             "argument");
}

void Validator::CheckFunc(const Location* loc, const Func* func) {
  current_func_ = func;
  if (func->GetNumResults() > 1) {
    PrintError(loc, "multiple result values not currently supported.");
    // Don't run any other checks, the won't test the result_type properly.
    return;
  }
  if (func->decl.has_func_type) {
    const FuncType* func_type;
    if (WABT_SUCCEEDED(CheckFuncTypeVar(&func->decl.type_var, &func_type))) {
      CheckFuncSignatureMatchesFuncType(loc, func->decl.sig, func_type);
    }
  }

  expr_loc_ = loc;
  typechecker_.BeginFunction(&func->decl.sig.result_types);
  CheckExprList(loc, func->first_expr);
  typechecker_.EndFunction();
  current_func_ = nullptr;
}

void Validator::PrintConstExprError(const Location* loc, const char* desc) {
  PrintError(loc,
             "invalid %s, must be a constant expression; either *.const or "
             "get_global.",
             desc);
}

void Validator::CheckConstInitExpr(const Location* loc,
                                   const Expr* expr,
                                   Type expected_type,
                                   const char* desc) {
  Type type = Type::Void;
  if (expr) {
    if (expr->next) {
      PrintConstExprError(loc, desc);
      return;
    }

    switch (expr->type) {
      case ExprType::Const:
        type = expr->const_.type;
        break;

      case ExprType::GetGlobal: {
        const Global* ref_global = nullptr;
        Index ref_global_index;
        if (WABT_FAILED(CheckGlobalVar(&expr->get_global.var, &ref_global,
                                       &ref_global_index))) {
          return;
        }

        type = ref_global->type;
        if (ref_global_index >= num_imported_globals_) {
          PrintError(
              loc,
              "initializer expression can only reference an imported global");
        }

        if (ref_global->mutable_) {
          PrintError(
              loc, "initializer expression cannot reference a mutable global");
        }
        break;
      }

      default:
        PrintConstExprError(loc, desc);
        return;
    }
  }

  CheckType(expr ? &expr->loc : loc, type, expected_type, desc);
}

void Validator::CheckGlobal(const Location* loc, const Global* global) {
  CheckConstInitExpr(loc, global->init_expr, global->type,
                     "global initializer expression");
}

void Validator::CheckLimits(const Location* loc,
                            const Limits* limits,
                            uint64_t absolute_max,
                            const char* desc) {
  if (limits->initial > absolute_max) {
    PrintError(loc, "initial %s (%" PRIu64 ") must be <= (%" PRIu64 ")", desc,
               limits->initial, absolute_max);
  }

  if (limits->has_max) {
    if (limits->max > absolute_max) {
      PrintError(loc, "max %s (%" PRIu64 ") must be <= (%" PRIu64 ")", desc,
                 limits->max, absolute_max);
    }

    if (limits->max < limits->initial) {
      PrintError(loc,
                 "max %s (%" PRIu64 ") must be >= initial %s (%" PRIu64 ")",
                 desc, limits->max, desc, limits->initial);
    }
  }
}

void Validator::CheckTable(const Location* loc, const Table* table) {
  if (current_table_index_ == 1)
    PrintError(loc, "only one table allowed");
  CheckLimits(loc, &table->elem_limits, UINT32_MAX, "elems");
}

void Validator::CheckElemSegments(const Module* module) {
  for (ModuleField* field = module->first_field; field; field = field->next) {
    if (field->type != ModuleFieldType::ElemSegment)
      continue;

    ElemSegment* elem_segment = field->elem_segment;
    const Table* table;
    if (!WABT_SUCCEEDED(CheckTableVar(&elem_segment->table_var, &table)))
      continue;

    for (const Var& var : elem_segment->vars) {
      if (!WABT_SUCCEEDED(CheckFuncVar(&var, nullptr)))
        continue;
    }

    CheckConstInitExpr(&field->loc, elem_segment->offset, Type::I32,
                       "elem segment offset");
  }
}

void Validator::CheckMemory(const Location* loc, const Memory* memory) {
  if (current_memory_index_ == 1)
    PrintError(loc, "only one memory block allowed");
  CheckLimits(loc, &memory->page_limits, WABT_MAX_PAGES, "pages");
}

void Validator::CheckDataSegments(const Module* module) {
  for (ModuleField* field = module->first_field; field; field = field->next) {
    if (field->type != ModuleFieldType::DataSegment)
      continue;

    DataSegment* data_segment = field->data_segment;
    const Memory* memory;
    if (!WABT_SUCCEEDED(CheckMemoryVar(&data_segment->memory_var, &memory)))
      continue;

    CheckConstInitExpr(&field->loc, data_segment->offset, Type::I32,
                       "data segment offset");
  }
}

void Validator::CheckImport(const Location* loc, const Import* import) {
  switch (import->kind) {
    case ExternalKind::Except:
      // TODO(karlschimpf) Define.
      PrintError(loc, "import except: don't know how to validate");
      break;
    case ExternalKind::Func:
      if (import->func->decl.has_func_type)
        CheckFuncTypeVar(&import->func->decl.type_var, nullptr);
      break;
    case ExternalKind::Table:
      CheckTable(loc, import->table);
      current_table_index_++;
      break;
    case ExternalKind::Memory:
      CheckMemory(loc, import->memory);
      current_memory_index_++;
      break;
    case ExternalKind::Global:
      if (import->global->mutable_) {
        PrintError(loc, "mutable globals cannot be imported");
      }
      num_imported_globals_++;
      current_global_index_++;
      break;
  }
}

void Validator::CheckExport(const Location* loc, const Export* export_) {
  switch (export_->kind) {
    case ExternalKind::Except:
      // TODO(karlschimpf) Define.
      PrintError(loc, "except: don't know how to validate export");
      break;
    case ExternalKind::Func:
      CheckFuncVar(&export_->var, nullptr);
      break;
    case ExternalKind::Table:
      CheckTableVar(&export_->var, nullptr);
      break;
    case ExternalKind::Memory:
      CheckMemoryVar(&export_->var, nullptr);
      break;
    case ExternalKind::Global: {
      const Global* global;
      if (WABT_SUCCEEDED(CheckGlobalVar(&export_->var, &global, nullptr))) {
        if (global->mutable_) {
          PrintError(&export_->var.loc, "mutable globals cannot be exported");
        }
      }
      break;
    }
  }
}

void Validator::CheckDuplicateExportBindings(const Module* module) {
  module->export_bindings.FindDuplicates([this](
      const BindingHash::value_type& a, const BindingHash::value_type& b) {
    // Choose the location that is later in the file.
    const Location& a_loc = a.second.loc;
    const Location& b_loc = b.second.loc;
    const Location& loc = a_loc.line > b_loc.line ? a_loc : b_loc;
    PrintError(&loc, "redefinition of export \"%s\"", a.first.c_str());
  });
}

void Validator::CheckModule(const Module* module) {
  bool seen_start = false;

  current_module_ = module;
  current_table_index_ = 0;
  current_memory_index_ = 0;
  current_global_index_ = 0;
  num_imported_globals_ = 0;

  for (ModuleField* field = module->first_field; field; field = field->next) {
    switch (field->type) {
      case ModuleFieldType::Except:
        // TODO(karlschimpf) Define.
        PrintError(&field->loc, "except clause: don't know how to validate");
        break;
      case ModuleFieldType::Func:
        CheckFunc(&field->loc, field->func);
        break;

      case ModuleFieldType::Global:
        CheckGlobal(&field->loc, field->global);
        current_global_index_++;
        break;

      case ModuleFieldType::Import:
        CheckImport(&field->loc, field->import);
        break;

      case ModuleFieldType::Export:
        CheckExport(&field->loc, field->export_);
        break;

      case ModuleFieldType::Table:
        CheckTable(&field->loc, field->table);
        current_table_index_++;
        break;

      case ModuleFieldType::ElemSegment:
        // Checked below.
        break;

      case ModuleFieldType::Memory:
        CheckMemory(&field->loc, field->memory);
        current_memory_index_++;
        break;

      case ModuleFieldType::DataSegment:
        // Checked below.
        break;

      case ModuleFieldType::FuncType:
        break;

      case ModuleFieldType::Start: {
        if (seen_start) {
          PrintError(&field->loc, "only one start function allowed");
        }

        const Func* start_func = nullptr;
        CheckFuncVar(&field->start, &start_func);
        if (start_func) {
          if (start_func->GetNumParams() != 0) {
            PrintError(&field->loc, "start function must be nullary");
          }

          if (start_func->GetNumResults() != 0) {
            PrintError(&field->loc, "start function must not return anything");
          }
        }
        seen_start = true;
        break;
      }
    }
  }

  CheckElemSegments(module);
  CheckDataSegments(module);
  CheckDuplicateExportBindings(module);
}

// Returns the result type of the invoked function, checked by the caller;
// returning nullptr means that another error occured first, so the result type
// should be ignored.
const TypeVector* Validator::CheckInvoke(const Action* action) {
  const ActionInvoke* invoke = action->invoke;
  const Module* module = script_->GetModule(action->module_var);
  if (!module) {
    PrintError(&action->loc, "unknown module");
    return nullptr;
  }

  const Export* export_ = module->GetExport(action->name);
  if (!export_) {
    PrintError(&action->loc, "unknown function export \"" PRIstringslice "\"",
               WABT_PRINTF_STRING_SLICE_ARG(action->name));
    return nullptr;
  }

  const Func* func = module->GetFunc(export_->var);
  if (!func) {
    // This error will have already been reported, just skip it.
    return nullptr;
  }

  size_t actual_args = invoke->args.size();
  size_t expected_args = func->GetNumParams();
  if (expected_args != actual_args) {
    PrintError(&action->loc, "too %s parameters to function. got %" PRIzd
                             ", expected %" PRIzd,
               actual_args > expected_args ? "many" : "few", actual_args,
               expected_args);
    return nullptr;
  }
  for (size_t i = 0; i < actual_args; ++i) {
    const Const* const_ = &invoke->args[i];
    CheckTypeIndex(&const_->loc, const_->type, func->GetParamType(i), "invoke",
                   i, "argument");
  }

  return &func->decl.sig.result_types;
}

Result Validator::CheckGet(const Action* action, Type* out_type) {
  const Module* module = script_->GetModule(action->module_var);
  if (!module) {
    PrintError(&action->loc, "unknown module");
    return Result::Error;
  }

  const Export* export_ = module->GetExport(action->name);
  if (!export_) {
    PrintError(&action->loc, "unknown global export \"" PRIstringslice "\"",
               WABT_PRINTF_STRING_SLICE_ARG(action->name));
    return Result::Error;
  }

  const Global* global = module->GetGlobal(export_->var);
  if (!global) {
    // This error will have already been reported, just skip it.
    return Result::Error;
  }

  *out_type = global->type;
  return Result::Ok;
}

Validator::ActionResult Validator::CheckAction(const Action* action) {
  ActionResult result;
  WABT_ZERO_MEMORY(result);

  switch (action->type) {
    case ActionType::Invoke:
      result.types = CheckInvoke(action);
      result.kind =
          result.types ? ActionResult::Kind::Types : ActionResult::Kind::Error;
      break;

    case ActionType::Get:
      if (WABT_SUCCEEDED(CheckGet(action, &result.type)))
        result.kind = ActionResult::Kind::Type;
      else
        result.kind = ActionResult::Kind::Error;
      break;
  }

  return result;
}

void Validator::CheckAssertReturnNanCommand(const Action* action) {
  ActionResult result = CheckAction(action);

  // A valid result type will either be f32 or f64; convert a Types result into
  // a Type result, so it is easier to check below. Type::Any is used to
  // specify a type that should not be checked (because an earlier error
  // occurred).
  if (result.kind == ActionResult::Kind::Types) {
    if (result.types->size() == 1) {
      result.kind = ActionResult::Kind::Type;
      result.type = (*result.types)[0];
    } else {
      PrintError(&action->loc, "expected 1 result, got %" PRIzd,
                 result.types->size());
      result.type = Type::Any;
    }
  }

  if (result.kind == ActionResult::Kind::Type && result.type != Type::Any)
    CheckAssertReturnNanType(&action->loc, result.type, "action");
}

void Validator::CheckCommand(const Command* command) {
  switch (command->type) {
    case CommandType::Module:
      CheckModule(command->module);
      break;

    case CommandType::Action:
      // Ignore result type.
      CheckAction(command->action);
      break;

    case CommandType::Register:
    case CommandType::AssertMalformed:
    case CommandType::AssertInvalid:
    case CommandType::AssertInvalidNonBinary:
    case CommandType::AssertUnlinkable:
    case CommandType::AssertUninstantiable:
      // Ignore.
      break;

    case CommandType::AssertReturn: {
      const Action* action = command->assert_return.action;
      ActionResult result = CheckAction(action);
      switch (result.kind) {
        case ActionResult::Kind::Types:
          CheckConstTypes(&action->loc, *result.types,
                          *command->assert_return.expected, "action");
          break;

        case ActionResult::Kind::Type:
          CheckConstType(&action->loc, result.type,
                         *command->assert_return.expected, "action");
          break;

        case ActionResult::Kind::Error:
          // Error occurred, don't do any further checks.
          break;
      }
      break;
    }

    case CommandType::AssertReturnCanonicalNan:
      CheckAssertReturnNanCommand(command->assert_return_canonical_nan.action);
      break;

    case CommandType::AssertReturnArithmeticNan:
      CheckAssertReturnNanCommand(command->assert_return_arithmetic_nan.action);
      break;

    case CommandType::AssertTrap:
    case CommandType::AssertExhaustion:
      // ignore result type.
      CheckAction(command->assert_trap.action);
      break;
  }
}

Result Validator::CheckScript(const Script* script) {
  for (const std::unique_ptr<Command>& command : script->commands)
    CheckCommand(command.get());
  return result_;
}

}  // namespace

Result validate_script(WastLexer* lexer,
                       const Script* script,
                       SourceErrorHandler* error_handler) {
  Validator validator(error_handler, lexer, script);

  return validator.CheckScript(script);
}

}  // namespace wabt
