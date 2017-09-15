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

#include "src/validator.h"

#include <cassert>
#include <cinttypes>
#include <cstdarg>
#include <cstdio>

#include "config.h"

#include "src/binary-reader.h"
#include "src/cast.h"
#include "src/error-handler.h"
#include "src/ir.h"
#include "src/type-checker.h"
#include "src/wast-parser-lexer-shared.h"

namespace wabt {

namespace {

class Validator {
 public:
  WABT_DISALLOW_COPY_AND_ASSIGN(Validator);
  Validator(ErrorHandler*, WastLexer*, const Script*);

  Result CheckModule(const Module* module);
  Result CheckScript(const Script* script);
  Result CheckAllFuncSignatures(const Module* module);

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
  void CheckExprList(const Location* loc, const ExprList& exprs);
  void CheckHasMemory(const Location* loc, Opcode opcode);
  void CheckBlockSig(const Location* loc,
                     Opcode opcode,
                     const BlockSignature* sig);
  void CheckExpr(const Expr* expr);
  void CheckFuncSignature(const Location* loc, const Func* func);
  void CheckFunc(const Location* loc, const Func* func);
  void PrintConstExprError(const Location* loc, const char* desc);
  void CheckConstInitExpr(const Location* loc,
                          const ExprList& expr,
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
  const TypeVector* CheckInvoke(const InvokeAction* action);
  Result CheckGet(const GetAction* action, Type* out_type);
  ActionResult CheckAction(const Action* action);
  void CheckAssertReturnNanCommand(const Action* action);
  void CheckCommand(const Command* command);

  void CheckExcept(const Location* loc, const Exception* Except);
  Result CheckExceptVar(const Var* var, const Exception** out_except);

  ErrorHandler* error_handler_ = nullptr;
  WastLexer* lexer_ = nullptr;
  const Script* script_ = nullptr;
  const Module* current_module_ = nullptr;
  const Func* current_func_ = nullptr;
  Index current_table_index_ = 0;
  Index current_memory_index_ = 0;
  Index current_global_index_ = 0;
  Index num_imported_globals_ = 0;
  Index current_except_index_ = 0;
  TypeChecker typechecker_;
  // Cached for access by OnTypecheckerError.
  const Location* expr_loc_ = nullptr;
  Result result_ = Result::Ok;
};

Validator::Validator(ErrorHandler* error_handler,
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
  WastFormatError(error_handler_, loc, lexer_, fmt, args);
  va_end(args);
}

void Validator::OnTypecheckerError(const char* msg) {
  PrintError(expr_loc_, "%s", msg);
}

static bool is_power_of_two(uint32_t x) {
  return x && ((x & (x - 1)) == 0);
}

static Address get_opcode_natural_alignment(Opcode opcode) {
  Address memory_size = opcode.GetMemorySize();
  assert(memory_size != 0);
  return memory_size;
}

Result Validator::CheckVar(Index max_index,
                           const Var* var,
                           const char* desc,
                           Index* out_index) {
  if (var->index() < max_index) {
    if (out_index)
      *out_index = var->index();
    return Result::Ok;
  }
  PrintError(&var->loc, "%s variable out of range (max %" PRIindex ")", desc,
             max_index);
  return Result::Error;
}

Result Validator::CheckFuncVar(const Var* var, const Func** out_func) {
  Index index;
  CHECK_RESULT(CheckVar(current_module_->funcs.size(), var, "function", &index));
  if (out_func)
    *out_func = current_module_->funcs[index];
  return Result::Ok;
}

Result Validator::CheckGlobalVar(const Var* var,
                                 const Global** out_global,
                                 Index* out_global_index) {
  Index index;
  CHECK_RESULT(
      CheckVar(current_module_->globals.size(), var, "global", &index));
  if (out_global)
    *out_global = current_module_->globals[index];
  if (out_global_index)
    *out_global_index = index;
  return Result::Ok;
}

Type Validator::GetGlobalVarTypeOrAny(const Var* var) {
  const Global* global;
  if (Succeeded(CheckGlobalVar(var, &global, nullptr)))
    return global->type;
  return Type::Any;
}

Result Validator::CheckFuncTypeVar(const Var* var,
                                   const FuncType** out_func_type) {
  Index index;
  CHECK_RESULT(CheckVar(current_module_->func_types.size(), var,
                        "function type", &index));
  if (out_func_type)
    *out_func_type = current_module_->func_types[index];
  return Result::Ok;
}

Result Validator::CheckTableVar(const Var* var, const Table** out_table) {
  Index index;
  CHECK_RESULT(CheckVar(current_module_->tables.size(), var, "table", &index));
  if (out_table)
    *out_table = current_module_->tables[index];
  return Result::Ok;
}

Result Validator::CheckMemoryVar(const Var* var, const Memory** out_memory) {
  Index index;
  CHECK_RESULT(
      CheckVar(current_module_->memories.size(), var, "memory", &index));
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

  if (var->is_name()) {
    PrintError(&var->loc, "undefined local variable \"%s\"",
               var->name().c_str());
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
               GetTypeName(actual), GetTypeName(expected));
  }
}

void Validator::CheckTypeIndex(const Location* loc,
                               Type actual,
                               Type expected,
                               const char* desc,
                               Index index,
                               const char* index_kind) {
  if (expected != actual && expected != Type::Any && actual != Type::Any) {
    PrintError(
        loc, "type mismatch for %s %" PRIindex " of %s. got %s, expected %s",
        index_kind, index, desc, GetTypeName(actual), GetTypeName(expected));
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
               GetTypeName(actual));
  }
}

void Validator::CheckExprList(const Location* loc, const ExprList& exprs) {
  for (const Expr& expr : exprs)
    CheckExpr(&expr);
}

void Validator::CheckHasMemory(const Location* loc, Opcode opcode) {
  if (current_module_->memories.size() == 0) {
    PrintError(loc, "%s requires an imported or defined memory.",
               opcode.GetName());
  }
}

void Validator::CheckBlockSig(const Location* loc,
                              Opcode opcode,
                              const BlockSignature* sig) {
  if (sig->size() > 1) {
    PrintError(loc,
               "multiple %s signature result types not currently supported.",
               opcode.GetName());
  }
}

void Validator::CheckExpr(const Expr* expr) {
  expr_loc_ = &expr->loc;

  switch (expr->type()) {
    case ExprType::Binary:
      typechecker_.OnBinary(cast<BinaryExpr>(expr)->opcode);
      break;

    case ExprType::Block: {
      auto block_expr = cast<BlockExpr>(expr);
      CheckBlockSig(&block_expr->loc, Opcode::Block, &block_expr->block.sig);
      typechecker_.OnBlock(&block_expr->block.sig);
      CheckExprList(&block_expr->loc, block_expr->block.exprs);
      typechecker_.OnEnd();
      break;
    }

    case ExprType::Br:
      typechecker_.OnBr(cast<BrExpr>(expr)->var.index());
      break;

    case ExprType::BrIf:
      typechecker_.OnBrIf(cast<BrIfExpr>(expr)->var.index());
      break;

    case ExprType::BrTable: {
      auto br_table_expr = cast<BrTableExpr>(expr);
      typechecker_.BeginBrTable();
      for (const Var& var : br_table_expr->targets) {
        typechecker_.OnBrTableTarget(var.index());
      }
      typechecker_.OnBrTableTarget(br_table_expr->default_target.index());
      typechecker_.EndBrTable();
      break;
    }

    case ExprType::Call: {
      const Func* callee;
      if (Succeeded(CheckFuncVar(&cast<CallExpr>(expr)->var, &callee))) {
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
      if (Succeeded(CheckFuncTypeVar(&cast<CallIndirectExpr>(expr)->var,
                                     &func_type))) {
        typechecker_.OnCallIndirect(&func_type->sig.param_types,
                                    &func_type->sig.result_types);
      }
      break;
    }

    case ExprType::Compare:
      typechecker_.OnCompare(cast<CompareExpr>(expr)->opcode);
      break;

    case ExprType::Const:
      typechecker_.OnConst(cast<ConstExpr>(expr)->const_.type);
      break;

    case ExprType::Convert:
      typechecker_.OnConvert(cast<ConvertExpr>(expr)->opcode);
      break;

    case ExprType::Drop:
      typechecker_.OnDrop();
      break;

    case ExprType::GetGlobal:
      typechecker_.OnGetGlobal(
          GetGlobalVarTypeOrAny(&cast<GetGlobalExpr>(expr)->var));
      break;

    case ExprType::GetLocal:
      typechecker_.OnGetLocal(
          GetLocalVarTypeOrAny(&cast<GetLocalExpr>(expr)->var));
      break;

    case ExprType::GrowMemory:
      CheckHasMemory(&expr->loc, Opcode::GrowMemory);
      typechecker_.OnGrowMemory();
      break;

    case ExprType::If: {
      auto if_expr = cast<IfExpr>(expr);
      CheckBlockSig(&if_expr->loc, Opcode::If, &if_expr->true_.sig);
      typechecker_.OnIf(&if_expr->true_.sig);
      CheckExprList(&if_expr->loc, if_expr->true_.exprs);
      if (!if_expr->false_.empty()) {
        typechecker_.OnElse();
        CheckExprList(&if_expr->loc, if_expr->false_);
      }
      typechecker_.OnEnd();
      break;
    }

    case ExprType::Load: {
      auto load_expr = cast<LoadExpr>(expr);
      CheckHasMemory(&load_expr->loc, load_expr->opcode);
      CheckAlign(&load_expr->loc, load_expr->align,
                 get_opcode_natural_alignment(load_expr->opcode));
      typechecker_.OnLoad(load_expr->opcode);
      break;
    }

    case ExprType::Loop: {
      auto loop_expr = cast<LoopExpr>(expr);
      CheckBlockSig(&loop_expr->loc, Opcode::Loop, &loop_expr->block.sig);
      typechecker_.OnLoop(&loop_expr->block.sig);
      CheckExprList(&loop_expr->loc, loop_expr->block.exprs);
      typechecker_.OnEnd();
      break;
    }

    case ExprType::CurrentMemory:
      CheckHasMemory(&expr->loc, Opcode::CurrentMemory);
      typechecker_.OnCurrentMemory();
      break;

    case ExprType::Nop:
      break;

    case ExprType::Rethrow:
      typechecker_.OnRethrow(cast<RethrowExpr>(expr)->var.index());
      break;

    case ExprType::Return:
      typechecker_.OnReturn();
      break;

    case ExprType::Select:
      typechecker_.OnSelect();
      break;

    case ExprType::SetGlobal:
      typechecker_.OnSetGlobal(
          GetGlobalVarTypeOrAny(&cast<SetGlobalExpr>(expr)->var));
      break;

    case ExprType::SetLocal:
      typechecker_.OnSetLocal(
          GetLocalVarTypeOrAny(&cast<SetLocalExpr>(expr)->var));
      break;

    case ExprType::Store: {
      auto store_expr = cast<StoreExpr>(expr);
      CheckHasMemory(&store_expr->loc, store_expr->opcode);
      CheckAlign(&store_expr->loc, store_expr->align,
                 get_opcode_natural_alignment(store_expr->opcode));
      typechecker_.OnStore(store_expr->opcode);
      break;
    }

    case ExprType::TeeLocal:
      typechecker_.OnTeeLocal(
          GetLocalVarTypeOrAny(&cast<TeeLocalExpr>(expr)->var));
      break;

    case ExprType::Throw:
      const Exception* except;
      if (Succeeded(CheckExceptVar(&cast<ThrowExpr>(expr)->var, &except))) {
        typechecker_.OnThrow(&except->sig);
      }
      break;

    case ExprType::TryBlock: {
      auto try_expr = cast<TryExpr>(expr);
      CheckBlockSig(&try_expr->loc, Opcode::Try, &try_expr->block.sig);

      typechecker_.OnTryBlock(&try_expr->block.sig);
      CheckExprList(&try_expr->loc, try_expr->block.exprs);

      if (try_expr->catches.empty())
        PrintError(&try_expr->loc, "TryBlock: doesn't have any catch clauses");
      bool found_catch_all = false;
      for (const Catch& catch_ : try_expr->catches) {
        typechecker_.OnCatchBlock(&try_expr->block.sig);
        if (catch_.IsCatchAll()) {
          found_catch_all = true;
        } else {
          if (found_catch_all)
            PrintError(&catch_.loc, "Appears after catch all block");
          const Exception* except = nullptr;
          if (Succeeded(CheckExceptVar(&catch_.var, &except))) {
            typechecker_.OnCatch(&except->sig);
          }
        }
        CheckExprList(&catch_.loc, catch_.exprs);
      }
      typechecker_.OnEnd();
      break;
    }

    case ExprType::Unary:
      typechecker_.OnUnary(cast<UnaryExpr>(expr)->opcode);
      break;

    case ExprType::Unreachable:
      typechecker_.OnUnreachable();
      break;
  }
}

void Validator::CheckFuncSignature(const Location* loc, const Func* func) {
  if (func->decl.has_func_type) {
    const FuncType* func_type;
    if (Succeeded(CheckFuncTypeVar(&func->decl.type_var, &func_type))) {
      CheckTypes(loc, func->decl.sig.result_types, func_type->sig.result_types,
                 "function", "result");
      CheckTypes(loc, func->decl.sig.param_types, func_type->sig.param_types,
                 "function", "argument");
    }
  }
}

void Validator::CheckFunc(const Location* loc, const Func* func) {
  current_func_ = func;
  CheckFuncSignature(loc, func);
  if (func->GetNumResults() > 1) {
    PrintError(loc, "multiple result values not currently supported.");
    // Don't run any other checks, the won't test the result_type properly.
    return;
  }

  expr_loc_ = loc;
  typechecker_.BeginFunction(&func->decl.sig.result_types);
  CheckExprList(loc, func->exprs);
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
                                   const ExprList& exprs,
                                   Type expected_type,
                                   const char* desc) {
  Type type = Type::Void;
  if (!exprs.empty()) {
    if (exprs.size() > 1) {
      PrintConstExprError(loc, desc);
      return;
    }

    const Expr* expr = &exprs.front();
    loc = &expr->loc;

    switch (expr->type()) {
      case ExprType::Const:
        type = cast<ConstExpr>(expr)->const_.type;
        break;

      case ExprType::GetGlobal: {
        const Global* ref_global = nullptr;
        Index ref_global_index;
        if (Failed(CheckGlobalVar(&cast<GetGlobalExpr>(expr)->var,
                                  &ref_global, &ref_global_index))) {
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

  CheckType(loc, type, expected_type, desc);
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
  for (const ModuleField& field : module->fields) {
    if (auto elem_segment_field = dyn_cast<ElemSegmentModuleField>(&field)) {
      auto&& elem_segment = elem_segment_field->elem_segment;
      const Table* table;
      if (Failed(CheckTableVar(&elem_segment.table_var, &table)))
        continue;

      for (const Var& var : elem_segment.vars) {
        CheckFuncVar(&var, nullptr);
      }

      CheckConstInitExpr(&field.loc, elem_segment.offset, Type::I32,
                         "elem segment offset");
    }
  }
}

void Validator::CheckMemory(const Location* loc, const Memory* memory) {
  if (current_memory_index_ == 1)
    PrintError(loc, "only one memory block allowed");
  CheckLimits(loc, &memory->page_limits, WABT_MAX_PAGES, "pages");
}

void Validator::CheckDataSegments(const Module* module) {
  for (const ModuleField& field : module->fields) {
    if (auto data_segment_field = dyn_cast<DataSegmentModuleField>(&field)) {
      auto&& data_segment = data_segment_field->data_segment;
      const Memory* memory;
      if (Failed(CheckMemoryVar(&data_segment.memory_var, &memory)))
        continue;

      CheckConstInitExpr(&field.loc, data_segment.offset, Type::I32,
                         "data segment offset");
    }
  }
}

void Validator::CheckImport(const Location* loc, const Import* import) {
  switch (import->kind()) {
    case ExternalKind::Except:
      ++current_except_index_;
      CheckExcept(loc, &cast<ExceptionImport>(import)->except);
      break;

    case ExternalKind::Func: {
      auto* func_import = cast<FuncImport>(import);
      if (func_import->func.decl.has_func_type)
        CheckFuncTypeVar(&func_import->func.decl.type_var, nullptr);
      break;
    }

    case ExternalKind::Table:
      CheckTable(loc, &cast<TableImport>(import)->table);
      ++current_table_index_;
      break;

    case ExternalKind::Memory:
      CheckMemory(loc, &cast<MemoryImport>(import)->memory);
      ++current_memory_index_;
      break;

    case ExternalKind::Global: {
      auto* global_import = cast<GlobalImport>(import);
      if (global_import->global.mutable_) {
        PrintError(loc, "mutable globals cannot be imported");
      }
      ++num_imported_globals_;
      ++current_global_index_;
      break;
    }
  }
}

void Validator::CheckExport(const Location* loc, const Export* export_) {
  switch (export_->kind) {
    case ExternalKind::Except:
      CheckExceptVar(&export_->var, nullptr);
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
      if (Succeeded(CheckGlobalVar(&export_->var, &global, nullptr))) {
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

Result Validator::CheckModule(const Module* module) {
  bool seen_start = false;

  current_module_ = module;
  current_table_index_ = 0;
  current_memory_index_ = 0;
  current_global_index_ = 0;
  num_imported_globals_ = 0;
  current_except_index_ = 0;

  for (const ModuleField& field : module->fields) {
    switch (field.type()) {
      case ModuleFieldType::Except:
        ++current_except_index_;
        CheckExcept(&field.loc, &cast<ExceptionModuleField>(&field)->except);
        break;

      case ModuleFieldType::Func:
        CheckFunc(&field.loc, &cast<FuncModuleField>(&field)->func);
        break;

      case ModuleFieldType::Global:
        CheckGlobal(&field.loc, &cast<GlobalModuleField>(&field)->global);
        current_global_index_++;
        break;

      case ModuleFieldType::Import:
        CheckImport(&field.loc, cast<ImportModuleField>(&field)->import.get());
        break;

      case ModuleFieldType::Export:
        CheckExport(&field.loc, &cast<ExportModuleField>(&field)->export_);
        break;

      case ModuleFieldType::Table:
        CheckTable(&field.loc, &cast<TableModuleField>(&field)->table);
        current_table_index_++;
        break;

      case ModuleFieldType::ElemSegment:
        // Checked below.
        break;

      case ModuleFieldType::Memory:
        CheckMemory(&field.loc, &cast<MemoryModuleField>(&field)->memory);
        current_memory_index_++;
        break;

      case ModuleFieldType::DataSegment:
        // Checked below.
        break;

      case ModuleFieldType::FuncType:
        break;

      case ModuleFieldType::Start: {
        if (seen_start) {
          PrintError(&field.loc, "only one start function allowed");
        }

        const Func* start_func = nullptr;
        CheckFuncVar(&cast<StartModuleField>(&field)->start, &start_func);
        if (start_func) {
          if (start_func->GetNumParams() != 0) {
            PrintError(&field.loc, "start function must be nullary");
          }

          if (start_func->GetNumResults() != 0) {
            PrintError(&field.loc, "start function must not return anything");
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

  return result_;
}

// Returns the result type of the invoked function, checked by the caller;
// returning nullptr means that another error occured first, so the result type
// should be ignored.
const TypeVector* Validator::CheckInvoke(const InvokeAction* action) {
  const Module* module = script_->GetModule(action->module_var);
  if (!module) {
    PrintError(&action->loc, "unknown module");
    return nullptr;
  }

  const Export* export_ = module->GetExport(action->name);
  if (!export_) {
    PrintError(&action->loc, "unknown function export \"%s\"",
               action->name.c_str());
    return nullptr;
  }

  const Func* func = module->GetFunc(export_->var);
  if (!func) {
    // This error will have already been reported, just skip it.
    return nullptr;
  }

  size_t actual_args = action->args.size();
  size_t expected_args = func->GetNumParams();
  if (expected_args != actual_args) {
    PrintError(&action->loc, "too %s parameters to function. got %" PRIzd
                             ", expected %" PRIzd,
               actual_args > expected_args ? "many" : "few", actual_args,
               expected_args);
    return nullptr;
  }
  for (size_t i = 0; i < actual_args; ++i) {
    const Const* const_ = &action->args[i];
    CheckTypeIndex(&const_->loc, const_->type, func->GetParamType(i), "invoke",
                   i, "argument");
  }

  return &func->decl.sig.result_types;
}

Result Validator::CheckGet(const GetAction* action, Type* out_type) {
  const Module* module = script_->GetModule(action->module_var);
  if (!module) {
    PrintError(&action->loc, "unknown module");
    return Result::Error;
  }

  const Export* export_ = module->GetExport(action->name);
  if (!export_) {
    PrintError(&action->loc, "unknown global export \"%s\"",
               action->name.c_str());
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

Result Validator::CheckExceptVar(const Var* var, const Exception** out_except) {
  Index index;
  CHECK_RESULT(
      CheckVar(current_module_->excepts.size(), var, "except", &index));
  if (out_except)
    *out_except = current_module_->excepts[index];
  return Result::Ok;
}

void Validator::CheckExcept(const Location* loc, const Exception* except) {
  for (Type ty : except->sig) {
    switch (ty) {
      case Type::I32:
      case Type::I64:
      case Type::F32:
      case Type::F64:
        break;
      default:
        PrintError(loc, "Invalid exception type: %s", GetTypeName(ty));
        break;
    }
  }
}

Validator::ActionResult Validator::CheckAction(const Action* action) {
  ActionResult result;
  ZeroMemory(result);

  switch (action->type()) {
    case ActionType::Invoke:
      result.types = CheckInvoke(cast<InvokeAction>(action));
      result.kind =
          result.types ? ActionResult::Kind::Types : ActionResult::Kind::Error;
      break;

    case ActionType::Get:
      if (Succeeded(CheckGet(cast<GetAction>(action), &result.type)))
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
      CheckModule(&cast<ModuleCommand>(command)->module);
      break;

    case CommandType::Action:
      // Ignore result type.
      CheckAction(cast<ActionCommand>(command)->action.get());
      break;

    case CommandType::Register:
    case CommandType::AssertMalformed:
    case CommandType::AssertInvalid:
    case CommandType::AssertUnlinkable:
    case CommandType::AssertUninstantiable:
      // Ignore.
      break;

    case CommandType::AssertReturn: {
      auto* assert_return_command = cast<AssertReturnCommand>(command);
      const Action* action = assert_return_command->action.get();
      ActionResult result = CheckAction(action);
      switch (result.kind) {
        case ActionResult::Kind::Types:
          CheckConstTypes(&action->loc, *result.types,
                          assert_return_command->expected, "action");
          break;

        case ActionResult::Kind::Type:
          CheckConstType(&action->loc, result.type,
                         assert_return_command->expected, "action");
          break;

        case ActionResult::Kind::Error:
          // Error occurred, don't do any further checks.
          break;
      }
      break;
    }

    case CommandType::AssertReturnCanonicalNan:
      CheckAssertReturnNanCommand(
          cast<AssertReturnCanonicalNanCommand>(command)->action.get());
      break;

    case CommandType::AssertReturnArithmeticNan:
      CheckAssertReturnNanCommand(
          cast<AssertReturnArithmeticNanCommand>(command)->action.get());
      break;

    case CommandType::AssertTrap:
      // ignore result type.
      CheckAction(cast<AssertTrapCommand>(command)->action.get());
      break;
    case CommandType::AssertExhaustion:
      // ignore result type.
      CheckAction(cast<AssertExhaustionCommand>(command)->action.get());
      break;
  }
}

Result Validator::CheckScript(const Script* script) {
  for (const std::unique_ptr<Command>& command : script->commands)
    CheckCommand(command.get());
  return result_;
}

Result Validator::CheckAllFuncSignatures(const Module* module) {
  current_module_ = module;
  for (const ModuleField& field : module->fields) {
    switch (field.type()) {
      case ModuleFieldType::Func:
        CheckFuncSignature(&field.loc, &cast<FuncModuleField>(&field)->func);
        break;

      default:
        break;
    }
  }
  return result_;
}

}  // end anonymous namespace

Result ValidateScript(WastLexer* lexer,
                      const Script* script,
                      ErrorHandler* error_handler) {
  Validator validator(error_handler, lexer, script);

  return validator.CheckScript(script);
}

Result ValidateModule(WastLexer* lexer,
                      const Module* module,
                      ErrorHandler* error_handler) {
  Validator validator(error_handler, lexer, nullptr);

  return validator.CheckModule(module);
}

Result ValidateFuncSignatures(WastLexer* lexer,
                              const Module* module,
                              ErrorHandler* error_handler) {
  Validator validator(error_handler, lexer, nullptr);

  return validator.CheckAllFuncSignatures(module);
}

}  // namespace wabt
