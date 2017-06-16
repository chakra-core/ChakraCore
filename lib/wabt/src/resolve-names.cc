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

#include <cassert>
#include <cstdio>

#include "expr-visitor.h"
#include "ir.h"
#include "wast-parser-lexer-shared.h"

namespace wabt {

namespace {

typedef Label* LabelPtr;

class NameResolver : public ExprVisitor::DelegateNop {
 public:
  NameResolver(WastLexer* lexer,
               Script* script,
               SourceErrorHandler* error_handler);

  Result VisitModule(Module* module);
  Result VisitScript(Script* script);

  // Implementation of ExprVisitor::DelegateNop.
  Result BeginBlockExpr(Expr*) override;
  Result EndBlockExpr(Expr*) override;
  Result OnBrExpr(Expr*) override;
  Result OnBrIfExpr(Expr*) override;
  Result OnBrTableExpr(Expr*) override;
  Result OnCallExpr(Expr*) override;
  Result OnCallIndirectExpr(Expr*) override;
  Result OnGetGlobalExpr(Expr*) override;
  Result OnGetLocalExpr(Expr*) override;
  Result BeginIfExpr(Expr*) override;
  Result EndIfExpr(Expr*) override;
  Result BeginLoopExpr(Expr*) override;
  Result EndLoopExpr(Expr*) override;
  Result OnSetGlobalExpr(Expr*) override;
  Result OnSetLocalExpr(Expr*) override;
  Result OnTeeLocalExpr(Expr*) override;

 private:
  void PrintError(const Location* loc, const char* fmt, ...);
  void PushLabel(Label* label);
  void PopLabel();
  void CheckDuplicateBindings(const BindingHash* bindings, const char* desc);
  void ResolveLabelVar(Var* var);
  void ResolveVar(const BindingHash* bindings, Var* var, const char* desc);
  void ResolveFuncVar(Var* var);
  void ResolveGlobalVar(Var* var);
  void ResolveFuncTypeVar(Var* var);
  void ResolveTableVar(Var* var);
  void ResolveMemoryVar(Var* var);
  void ResolveLocalVar(Var* var);
  void VisitFunc(Func* func);
  void VisitExport(Export* export_);
  void VisitGlobal(Global* global);
  void VisitElemSegment(ElemSegment* segment);
  void VisitDataSegment(DataSegment* segment);
  void VisitScriptModule(ScriptModule* script_module);
  void VisitCommand(Command* command);

  SourceErrorHandler* error_handler_ = nullptr;
  WastLexer* lexer_ = nullptr;
  Script* script_ = nullptr;
  Module* current_module_ = nullptr;
  Func* current_func_ = nullptr;
  ExprVisitor visitor_;
  std::vector<Label*> labels_;
  Result result_ = Result::Ok;
};

NameResolver::NameResolver(WastLexer* lexer,
                           Script* script,
                           SourceErrorHandler* error_handler)
    : error_handler_(error_handler),
      lexer_(lexer),
      script_(script),
      visitor_(this) {}

}  // namespace

void WABT_PRINTF_FORMAT(3, 4) NameResolver::PrintError(const Location* loc,
                                                       const char* fmt,
                                                       ...) {
  result_ = Result::Error;
  va_list args;
  va_start(args, fmt);
  wast_format_error(error_handler_, loc, lexer_, fmt, args);
  va_end(args);
}

void NameResolver::PushLabel(Label* label) {
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
  if (var->type == VarType::Name) {
    for (int i = labels_.size() - 1; i >= 0; --i) {
      Label* label = labels_[i];
      if (string_slices_are_equal(label, &var->name)) {
        destroy_string_slice(&var->name);
        var->type = VarType::Index;
        var->index = labels_.size() - i - 1;
        return;
      }
    }
    PrintError(&var->loc, "undefined label variable \"" PRIstringslice "\"",
               WABT_PRINTF_STRING_SLICE_ARG(var->name));
  }
}

void NameResolver::ResolveVar(const BindingHash* bindings,
                              Var* var,
                              const char* desc) {
  if (var->type == VarType::Name) {
    Index index = bindings->FindIndex(*var);
    if (index == kInvalidIndex) {
      PrintError(&var->loc, "undefined %s variable \"" PRIstringslice "\"",
                 desc, WABT_PRINTF_STRING_SLICE_ARG(var->name));
      return;
    }

    destroy_string_slice(&var->name);
    var->index = index;
    var->type = VarType::Index;
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

void NameResolver::ResolveLocalVar(Var* var) {
  if (var->type == VarType::Name) {
    if (!current_func_)
      return;

    Index index = current_func_->GetLocalIndex(*var);
    if (index == kInvalidIndex) {
      PrintError(&var->loc, "undefined local variable \"" PRIstringslice "\"",
                 WABT_PRINTF_STRING_SLICE_ARG(var->name));
      return;
    }

    destroy_string_slice(&var->name);
    var->index = index;
    var->type = VarType::Index;
  }
}

Result NameResolver::BeginBlockExpr(Expr* expr) {
  PushLabel(&expr->block->label);
  return Result::Ok;
}

Result NameResolver::EndBlockExpr(Expr* expr) {
  PopLabel();
  return Result::Ok;
}

Result NameResolver::BeginLoopExpr(Expr* expr) {
  PushLabel(&expr->loop->label);
  return Result::Ok;
}

Result NameResolver::EndLoopExpr(Expr* expr) {
  PopLabel();
  return Result::Ok;
}

Result NameResolver::OnBrExpr(Expr* expr) {
  ResolveLabelVar(&expr->br.var);
  return Result::Ok;
}

Result NameResolver::OnBrIfExpr(Expr* expr) {
  ResolveLabelVar(&expr->br_if.var);
  return Result::Ok;
}

Result NameResolver::OnBrTableExpr(Expr* expr) {
  for (Var& target : *expr->br_table.targets)
    ResolveLabelVar(&target);
  ResolveLabelVar(&expr->br_table.default_target);
  return Result::Ok;
}

Result NameResolver::OnCallExpr(Expr* expr) {
  ResolveFuncVar(&expr->call.var);
  return Result::Ok;
}

Result NameResolver::OnCallIndirectExpr(Expr* expr) {
  ResolveFuncTypeVar(&expr->call_indirect.var);
  return Result::Ok;
}

Result NameResolver::OnGetGlobalExpr(Expr* expr) {
  ResolveGlobalVar(&expr->get_global.var);
  return Result::Ok;
}

Result NameResolver::OnGetLocalExpr(Expr* expr) {
  ResolveLocalVar(&expr->get_local.var);
  return Result::Ok;
}

Result NameResolver::BeginIfExpr(Expr* expr) {
  PushLabel(&expr->if_.true_->label);
  return Result::Ok;
}

Result NameResolver::EndIfExpr(Expr* expr) {
  PopLabel();
  return Result::Ok;
}

Result NameResolver::OnSetGlobalExpr(Expr* expr) {
  ResolveGlobalVar(&expr->set_global.var);
  return Result::Ok;
}

Result NameResolver::OnSetLocalExpr(Expr* expr) {
  ResolveLocalVar(&expr->set_local.var);
  return Result::Ok;
}

Result NameResolver::OnTeeLocalExpr(Expr* expr) {
  ResolveLocalVar(&expr->tee_local.var);
  return Result::Ok;
}

void NameResolver::VisitFunc(Func* func) {
  current_func_ = func;
  if (func->decl.has_func_type)
    ResolveFuncTypeVar(&func->decl.type_var);

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
      WABT_FATAL("NameResolver::VisitExport(except) not defined\n");
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
  if (module->start)
    ResolveFuncVar(module->start);
  current_module_ = nullptr;
  return result_;
}

void NameResolver::VisitScriptModule(ScriptModule* script_module) {
  if (script_module->type == ScriptModule::Type::Text)
    VisitModule(script_module->text);
}

void NameResolver::VisitCommand(Command* command) {
  switch (command->type) {
    case CommandType::Module:
      VisitModule(command->module);
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
      SourceErrorHandlerNop new_error_handler;

      NameResolver new_resolver(lexer_, script_, &new_error_handler);
      new_resolver.VisitScriptModule(command->assert_invalid.module);
      if (WABT_FAILED(new_resolver.result_)) {
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
      VisitScriptModule(command->assert_unlinkable.module);
      break;

    case CommandType::AssertUninstantiable:
      VisitScriptModule(command->assert_uninstantiable.module);
      break;
  }
}

Result NameResolver::VisitScript(Script* script) {
  for (const std::unique_ptr<Command>& command : script->commands)
    VisitCommand(command.get());
  return result_;
}

Result resolve_names_module(WastLexer* lexer,
                            Module* module,
                            SourceErrorHandler* error_handler) {
  NameResolver resolver(lexer, nullptr, error_handler);
  return resolver.VisitModule(module);
}

Result resolve_names_script(WastLexer* lexer,
                            Script* script,
                            SourceErrorHandler* error_handler) {
  NameResolver resolver(lexer, script, error_handler);
  return resolver.VisitScript(script);
}

}  // namespace wabt
