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

#include "src/resolve-names.h"

#include <cassert>
#include <cstdio>

#include "src/cast.h"
#include "src/error-handler.h"
#include "src/expr-visitor.h"
#include "src/ir.h"
#include "src/wast-lexer.h"
#include "src/wast-parser-lexer-shared.h"

namespace wabt {

namespace {

class NameResolver : public ExprVisitor::DelegateNop {
 public:
  NameResolver(WastLexer* lexer, Script* script, ErrorHandler* error_handler);

  Result VisitModule(Module* module);
  Result VisitScript(Script* script);

  // Implementation of ExprVisitor::DelegateNop.
  Result BeginBlockExpr(BlockExpr*) override;
  Result EndBlockExpr(BlockExpr*) override;
  Result OnBrExpr(BrExpr*) override;
  Result OnBrIfExpr(BrIfExpr*) override;
  Result OnBrTableExpr(BrTableExpr*) override;
  Result OnCallExpr(CallExpr*) override;
  Result OnCallIndirectExpr(CallIndirectExpr*) override;
  Result OnGetGlobalExpr(GetGlobalExpr*) override;
  Result OnGetLocalExpr(GetLocalExpr*) override;
  Result BeginIfExpr(IfExpr*) override;
  Result EndIfExpr(IfExpr*) override;
  Result BeginIfExceptExpr(IfExceptExpr*) override;
  Result EndIfExceptExpr(IfExceptExpr*) override;
  Result BeginLoopExpr(LoopExpr*) override;
  Result EndLoopExpr(LoopExpr*) override;
  Result OnSetGlobalExpr(SetGlobalExpr*) override;
  Result OnSetLocalExpr(SetLocalExpr*) override;
  Result OnTeeLocalExpr(TeeLocalExpr*) override;
  Result BeginTryExpr(TryExpr*) override;
  Result EndTryExpr(TryExpr*) override;
  Result OnThrowExpr(ThrowExpr*) override;

 private:
  void PrintError(const Location* loc, const char* fmt, ...);
  void PushLabel(const std::string& label);
  void PopLabel();
  void CheckDuplicateBindings(const BindingHash* bindings, const char* desc);
  void ResolveLabelVar(Var* var);
  void ResolveVar(const BindingHash* bindings, Var* var, const char* desc);
  void ResolveFuncVar(Var* var);
  void ResolveGlobalVar(Var* var);
  void ResolveFuncTypeVar(Var* var);
  void ResolveTableVar(Var* var);
  void ResolveMemoryVar(Var* var);
  void ResolveExceptionVar(Var* var);
  void ResolveLocalVar(Var* var);
  void VisitFunc(Func* func);
  void VisitExport(Export* export_);
  void VisitGlobal(Global* global);
  void VisitElemSegment(ElemSegment* segment);
  void VisitDataSegment(DataSegment* segment);
  void VisitScriptModule(ScriptModule* script_module);
  void VisitCommand(Command* command);

  ErrorHandler* error_handler_ = nullptr;
  WastLexer* lexer_ = nullptr;
  Script* script_ = nullptr;
  Module* current_module_ = nullptr;
  Func* current_func_ = nullptr;
  ExprVisitor visitor_;
  std::vector<std::string> labels_;
  Result result_ = Result::Ok;
};

NameResolver::NameResolver(WastLexer* lexer,
                           Script* script,
                           ErrorHandler* error_handler)
    : error_handler_(error_handler),
      lexer_(lexer),
      script_(script),
      visitor_(this) {}

}  // end anonymous namespace

void WABT_PRINTF_FORMAT(3, 4) NameResolver::PrintError(const Location* loc,
                                                       const char* fmt,
                                                       ...) {
  result_ = Result::Error;
  va_list args;
  va_start(args, fmt);
  WastFormatError(error_handler_, loc, lexer_, fmt, args);
  va_end(args);
}

void NameResolver::PushLabel(const std::string& label) {
  labels_.push_back(label);
}

void NameResolver::PopLabel() {
  labels_.pop_back();
}

void NameResolver::CheckDuplicateBindings(const BindingHash* bindings,
                                          const char* desc) {
  bindings->FindDuplicates([this, desc](const BindingHash::value_type& a,
                                        const BindingHash::value_type& b) {
    // Choose the location that is later in the file.
    const Location& a_loc = a.second.loc;
    const Location& b_loc = b.second.loc;
    const Location& loc = a_loc.line > b_loc.line ? a_loc : b_loc;
    PrintError(&loc, "redefinition of %s \"%s\"", desc, a.first.c_str());

  });
}

void NameResolver::ResolveLabelVar(Var* var) {
  if (var->is_name()) {
    for (int i = labels_.size() - 1; i >= 0; --i) {
      const std::string& label = labels_[i];
      if (label == var->name()) {
        var->set_index(labels_.size() - i - 1);
        return;
      }
    }
    PrintError(&var->loc, "undefined label variable \"%s\"",
               var->name().c_str());
  }
}

void NameResolver::ResolveVar(const BindingHash* bindings,
                              Var* var,
                              const char* desc) {
  if (var->is_name()) {
    Index index = bindings->FindIndex(*var);
    if (index == kInvalidIndex) {
      PrintError(&var->loc, "undefined %s variable \"%s\"", desc,
                 var->name().c_str());
      return;
    }

    var->set_index(index);
  }
}

void NameResolver::ResolveFuncVar(Var* var) {
  ResolveVar(&current_module_->func_bindings, var, "function");
}

void NameResolver::ResolveGlobalVar(Var* var) {
  ResolveVar(&current_module_->global_bindings, var, "global");
}

void NameResolver::ResolveFuncTypeVar(Var* var) {
  ResolveVar(&current_module_->func_type_bindings, var, "function type");
}

void NameResolver::ResolveTableVar(Var* var) {
  ResolveVar(&current_module_->table_bindings, var, "table");
}

void NameResolver::ResolveMemoryVar(Var* var) {
  ResolveVar(&current_module_->memory_bindings, var, "memory");
}

void NameResolver::ResolveExceptionVar(Var* var) {
  ResolveVar(&current_module_->except_bindings, var, "exception");
}

void NameResolver::ResolveLocalVar(Var* var) {
  if (var->is_name()) {
    if (!current_func_) {
      return;
    }

    Index index = current_func_->GetLocalIndex(*var);
    if (index == kInvalidIndex) {
      PrintError(&var->loc, "undefined local variable \"%s\"",
                 var->name().c_str());
      return;
    }

    var->set_index(index);
  }
}

Result NameResolver::BeginBlockExpr(BlockExpr* expr) {
  PushLabel(expr->block.label);
  return Result::Ok;
}

Result NameResolver::EndBlockExpr(BlockExpr* expr) {
  PopLabel();
  return Result::Ok;
}

Result NameResolver::BeginLoopExpr(LoopExpr* expr) {
  PushLabel(expr->block.label);
  return Result::Ok;
}

Result NameResolver::EndLoopExpr(LoopExpr* expr) {
  PopLabel();
  return Result::Ok;
}

Result NameResolver::OnBrExpr(BrExpr* expr) {
  ResolveLabelVar(&expr->var);
  return Result::Ok;
}

Result NameResolver::OnBrIfExpr(BrIfExpr* expr) {
  ResolveLabelVar(&expr->var);
  return Result::Ok;
}

Result NameResolver::OnBrTableExpr(BrTableExpr* expr) {
  for (Var& target : expr->targets)
    ResolveLabelVar(&target);
  ResolveLabelVar(&expr->default_target);
  return Result::Ok;
}

Result NameResolver::OnCallExpr(CallExpr* expr) {
  ResolveFuncVar(&expr->var);
  return Result::Ok;
}

Result NameResolver::OnCallIndirectExpr(CallIndirectExpr* expr) {
  if (expr->decl.has_func_type) {
    ResolveFuncTypeVar(&expr->decl.type_var);
  }
  return Result::Ok;
}

Result NameResolver::OnGetGlobalExpr(GetGlobalExpr* expr) {
  ResolveGlobalVar(&expr->var);
  return Result::Ok;
}

Result NameResolver::OnGetLocalExpr(GetLocalExpr* expr) {
  ResolveLocalVar(&expr->var);
  return Result::Ok;
}

Result NameResolver::BeginIfExpr(IfExpr* expr) {
  PushLabel(expr->true_.label);
  return Result::Ok;
}

Result NameResolver::EndIfExpr(IfExpr* expr) {
  PopLabel();
  return Result::Ok;
}

Result NameResolver::BeginIfExceptExpr(IfExceptExpr* expr) {
  PushLabel(expr->true_.label);
  ResolveExceptionVar(&expr->except_var);
  return Result::Ok;
}

Result NameResolver::EndIfExceptExpr(IfExceptExpr* expr) {
  PopLabel();
  return Result::Ok;
}

Result NameResolver::OnSetGlobalExpr(SetGlobalExpr* expr) {
  ResolveGlobalVar(&expr->var);
  return Result::Ok;
}

Result NameResolver::OnSetLocalExpr(SetLocalExpr* expr) {
  ResolveLocalVar(&expr->var);
  return Result::Ok;
}

Result NameResolver::OnTeeLocalExpr(TeeLocalExpr* expr) {
  ResolveLocalVar(&expr->var);
  return Result::Ok;
}

Result NameResolver::BeginTryExpr(TryExpr* expr) {
  PushLabel(expr->block.label);
  return Result::Ok;
}

Result NameResolver::EndTryExpr(TryExpr*) {
  PopLabel();
  return Result::Ok;
}

Result NameResolver::OnThrowExpr(ThrowExpr* expr) {
  ResolveExceptionVar(&expr->var);
  return Result::Ok;
}

void NameResolver::VisitFunc(Func* func) {
  current_func_ = func;
  if (func->decl.has_func_type) {
    ResolveFuncTypeVar(&func->decl.type_var);
  }

  CheckDuplicateBindings(&func->param_bindings, "parameter");
  CheckDuplicateBindings(&func->local_bindings, "local");

  visitor_.VisitFunc(func);
  current_func_ = nullptr;
}

void NameResolver::VisitExport(Export* export_) {
  switch (export_->kind) {
    case ExternalKind::Func:
      ResolveFuncVar(&export_->var);
      break;

    case ExternalKind::Table:
      ResolveTableVar(&export_->var);
      break;

    case ExternalKind::Memory:
      ResolveMemoryVar(&export_->var);
      break;

    case ExternalKind::Global:
      ResolveGlobalVar(&export_->var);
      break;

    case ExternalKind::Except:
      ResolveExceptionVar(&export_->var);
      break;
  }
}

void NameResolver::VisitGlobal(Global* global) {
  visitor_.VisitExprList(global->init_expr);
}

void NameResolver::VisitElemSegment(ElemSegment* segment) {
  ResolveTableVar(&segment->table_var);
  visitor_.VisitExprList(segment->offset);
  for (Var& var : segment->vars)
    ResolveFuncVar(&var);
}

void NameResolver::VisitDataSegment(DataSegment* segment) {
  ResolveMemoryVar(&segment->memory_var);
  visitor_.VisitExprList(segment->offset);
}

Result NameResolver::VisitModule(Module* module) {
  current_module_ = module;
  CheckDuplicateBindings(&module->func_bindings, "function");
  CheckDuplicateBindings(&module->global_bindings, "global");
  CheckDuplicateBindings(&module->func_type_bindings, "function type");
  CheckDuplicateBindings(&module->table_bindings, "table");
  CheckDuplicateBindings(&module->memory_bindings, "memory");
  CheckDuplicateBindings(&module->except_bindings, "except");

  for (Func* func : module->funcs)
    VisitFunc(func);
  for (Export* export_ : module->exports)
    VisitExport(export_);
  for (Global* global : module->globals)
    VisitGlobal(global);
  for (ElemSegment* elem_segment : module->elem_segments)
    VisitElemSegment(elem_segment);
  for (DataSegment* data_segment : module->data_segments)
    VisitDataSegment(data_segment);
  for (Var* start : module->starts)
    ResolveFuncVar(start);
  current_module_ = nullptr;
  return result_;
}

void NameResolver::VisitScriptModule(ScriptModule* script_module) {
  if (auto* tsm = dyn_cast<TextScriptModule>(script_module)) {
    VisitModule(&tsm->module);
  }
}

void NameResolver::VisitCommand(Command* command) {
  switch (command->type) {
    case CommandType::Module:
      VisitModule(&cast<ModuleCommand>(command)->module);
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
      auto* assert_invalid_command = cast<AssertInvalidCommand>(command);
      /* The module may be invalid because the names cannot be resolved; we
       * don't want to print errors or fail if that's the case, but we still
       * should try to resolve names when possible. */
      ErrorHandlerNop new_error_handler;
      NameResolver new_resolver(lexer_, script_, &new_error_handler);
      new_resolver.VisitScriptModule(assert_invalid_command->module.get());
      break;
    }

    case CommandType::AssertUnlinkable:
      VisitScriptModule(cast<AssertUnlinkableCommand>(command)->module.get());
      break;

    case CommandType::AssertUninstantiable:
      VisitScriptModule(
          cast<AssertUninstantiableCommand>(command)->module.get());
      break;
  }
}

Result NameResolver::VisitScript(Script* script) {
  for (const std::unique_ptr<Command>& command : script->commands)
    VisitCommand(command.get());
  return result_;
}

Result ResolveNamesModule(WastLexer* lexer,
                          Module* module,
                          ErrorHandler* error_handler) {
  NameResolver resolver(lexer, nullptr, error_handler);
  return resolver.VisitModule(module);
}

Result ResolveNamesScript(WastLexer* lexer,
                          Script* script,
                          ErrorHandler* error_handler) {
  NameResolver resolver(lexer, script, error_handler);
  return resolver.VisitScript(script);
}

}  // namespace wabt
