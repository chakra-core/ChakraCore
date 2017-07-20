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

#include <cassert>
#include <cstdio>
#include <vector>

#include "expr-visitor.h"
#include "ir.h"

#define CHECK_RESULT(expr)  \
  do {                      \
    if (WABT_FAILED(expr))  \
      return Result::Error; \
  } while (0)

namespace wabt {

namespace {

class NameApplier : public ExprVisitor::DelegateNop {
 public:
  NameApplier();

  Result VisitModule(Module* module);

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
  void PushLabel(Label* label);
  void PopLabel();
  Label* FindLabelByVar(Var* var);
  void UseNameForVar(StringSlice* name, Var* var);
  Result UseNameForFuncTypeVar(Module* module, Var* var);
  Result UseNameForFuncVar(Module* module, Var* var);
  Result UseNameForGlobalVar(Module* module, Var* var);
  Result UseNameForTableVar(Module* module, Var* var);
  Result UseNameForMemoryVar(Module* module, Var* var);
  Result UseNameForParamAndLocalVar(Func* func, Var* var);
  Result VisitFunc(Index func_index, Func* func);
  Result VisitExport(Index export_index, Export* export_);
  Result VisitElemSegment(Index elem_segment_index, ElemSegment* segment);
  Result VisitDataSegment(Index data_segment_index, DataSegment* segment);

  Module* module_ = nullptr;
  Func* current_func_ = nullptr;
  ExprVisitor visitor_;
  /* mapping from param index to its name, if any, for the current func */
  std::vector<std::string> param_index_to_name_;
  std::vector<std::string> local_index_to_name_;
  std::vector<Label*> labels_;
};

NameApplier::NameApplier() : visitor_(this) {}

void NameApplier::PushLabel(Label* label) {
  labels_.push_back(label);
}

void NameApplier::PopLabel() {
  labels_.pop_back();
}

Label* NameApplier::FindLabelByVar(Var* var) {
  if (var->type == VarType::Name) {
    for (int i = labels_.size() - 1; i >= 0; --i) {
      Label* label = labels_[i];
      if (string_slices_are_equal(label, &var->name))
        return label;
    }
    return nullptr;
  } else {
    if (var->index >= labels_.size())
      return nullptr;
    return labels_[labels_.size() - 1 - var->index];
  }
}

void NameApplier::UseNameForVar(StringSlice* name, Var* var) {
  if (var->type == VarType::Name) {
    assert(string_slices_are_equal(name, &var->name));
  }

  if (name && name->start) {
    var->type = VarType::Name;
    var->name = dup_string_slice(*name);
  }
}

Result NameApplier::UseNameForFuncTypeVar(Module* module, Var* var) {
  FuncType* func_type = module->GetFuncType(*var);
  if (!func_type)
    return Result::Error;
  UseNameForVar(&func_type->name, var);
  return Result::Ok;
}

Result NameApplier::UseNameForFuncVar(Module* module, Var* var) {
  Func* func = module->GetFunc(*var);
  if (!func)
    return Result::Error;
  UseNameForVar(&func->name, var);
  return Result::Ok;
}

Result NameApplier::UseNameForGlobalVar(Module* module, Var* var) {
  Global* global = module->GetGlobal(*var);
  if (!global)
    return Result::Error;
  UseNameForVar(&global->name, var);
  return Result::Ok;
}

Result NameApplier::UseNameForTableVar(Module* module, Var* var) {
  Table* table = module->GetTable(*var);
  if (!table)
    return Result::Error;
  UseNameForVar(&table->name, var);
  return Result::Ok;
}

Result NameApplier::UseNameForMemoryVar(Module* module, Var* var) {
  Memory* memory = module->GetMemory(*var);
  if (!memory)
    return Result::Error;
  UseNameForVar(&memory->name, var);
  return Result::Ok;
}

Result NameApplier::UseNameForParamAndLocalVar(Func* func, Var* var) {
  Index local_index = func->GetLocalIndex(*var);
  if (local_index >= func->GetNumParamsAndLocals())
    return Result::Error;

  Index num_params = func->GetNumParams();
  std::string* name;
  if (local_index < num_params) {
    /* param */
    assert(local_index < param_index_to_name_.size());
    name = &param_index_to_name_[local_index];
  } else {
    /* local */
    local_index -= num_params;
    assert(local_index < local_index_to_name_.size());
    name = &local_index_to_name_[local_index];
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

Result NameApplier::BeginBlockExpr(Expr* expr) {
  PushLabel(&expr->block->label);
  return Result::Ok;
}

Result NameApplier::EndBlockExpr(Expr* expr) {
  PopLabel();
  return Result::Ok;
}

Result NameApplier::BeginLoopExpr(Expr* expr) {
  PushLabel(&expr->loop->label);
  return Result::Ok;
}

Result NameApplier::EndLoopExpr(Expr* expr) {
  PopLabel();
  return Result::Ok;
}

Result NameApplier::OnBrExpr(Expr* expr) {
  Label* label = FindLabelByVar(&expr->br.var);
  UseNameForVar(label, &expr->br.var);
  return Result::Ok;
}

Result NameApplier::OnBrIfExpr(Expr* expr) {
  Label* label = FindLabelByVar(&expr->br_if.var);
  UseNameForVar(label, &expr->br_if.var);
  return Result::Ok;
}

Result NameApplier::OnBrTableExpr(Expr* expr) {
  VarVector& targets = *expr->br_table.targets;
  for (Var& target : targets) {
    Label* label = FindLabelByVar(&target);
    UseNameForVar(label, &target);
  }

  Label* label = FindLabelByVar(&expr->br_table.default_target);
  UseNameForVar(label, &expr->br_table.default_target);
  return Result::Ok;
}

Result NameApplier::OnCallExpr(Expr* expr) {
  CHECK_RESULT(UseNameForFuncVar(module_, &expr->call.var));
  return Result::Ok;
}

Result NameApplier::OnCallIndirectExpr(Expr* expr) {
  CHECK_RESULT(UseNameForFuncTypeVar(module_, &expr->call_indirect.var));
  return Result::Ok;
}

Result NameApplier::OnGetGlobalExpr(Expr* expr) {
  CHECK_RESULT(UseNameForGlobalVar(module_, &expr->get_global.var));
  return Result::Ok;
}

Result NameApplier::OnGetLocalExpr(Expr* expr) {
  CHECK_RESULT(UseNameForParamAndLocalVar(current_func_, &expr->get_local.var));
  return Result::Ok;
}

Result NameApplier::BeginIfExpr(Expr* expr) {
  PushLabel(&expr->if_.true_->label);
  return Result::Ok;
}

Result NameApplier::EndIfExpr(Expr* expr) {
  PopLabel();
  return Result::Ok;
}

Result NameApplier::OnSetGlobalExpr(Expr* expr) {
  CHECK_RESULT(UseNameForGlobalVar(module_, &expr->set_global.var));
  return Result::Ok;
}

Result NameApplier::OnSetLocalExpr(Expr* expr) {
  CHECK_RESULT(UseNameForParamAndLocalVar(current_func_, &expr->set_local.var));
  return Result::Ok;
}

Result NameApplier::OnTeeLocalExpr(Expr* expr) {
  CHECK_RESULT(UseNameForParamAndLocalVar(current_func_, &expr->tee_local.var));
  return Result::Ok;
}

Result NameApplier::VisitFunc(Index func_index, Func* func) {
  current_func_ = func;
  if (func->decl.has_func_type) {
    CHECK_RESULT(UseNameForFuncTypeVar(module_, &func->decl.type_var));
  }

  MakeTypeBindingReverseMapping(func->decl.sig.param_types,
                                func->param_bindings, &param_index_to_name_);

  MakeTypeBindingReverseMapping(func->local_types, func->local_bindings,
                                &local_index_to_name_);

  CHECK_RESULT(visitor_.VisitFunc(func));
  current_func_ = nullptr;
  return Result::Ok;
}

Result NameApplier::VisitExport(Index export_index, Export* export_) {
  if (export_->kind == ExternalKind::Func) {
    UseNameForFuncVar(module_, &export_->var);
  }
  return Result::Ok;
}

Result NameApplier::VisitElemSegment(Index elem_segment_index,
                                     ElemSegment* segment) {
  CHECK_RESULT(UseNameForTableVar(module_, &segment->table_var));
  for (Var& var : segment->vars) {
    CHECK_RESULT(UseNameForFuncVar(module_, &var));
  }
  return Result::Ok;
}

Result NameApplier::VisitDataSegment(Index data_segment_index,
                                     DataSegment* segment) {
  CHECK_RESULT(UseNameForMemoryVar(module_, &segment->memory_var));
  return Result::Ok;
}

Result NameApplier::VisitModule(Module* module) {
  module_ = module;
  for (size_t i = 0; i < module->funcs.size(); ++i)
    CHECK_RESULT(VisitFunc(i, module->funcs[i]));
  for (size_t i = 0; i < module->exports.size(); ++i)
    CHECK_RESULT(VisitExport(i, module->exports[i]));
  for (size_t i = 0; i < module->elem_segments.size(); ++i)
    CHECK_RESULT(VisitElemSegment(i, module->elem_segments[i]));
  for (size_t i = 0; i < module->data_segments.size(); ++i)
    CHECK_RESULT(VisitDataSegment(i, module->data_segments[i]));
  module_ = nullptr;
  return Result::Ok;
}

}  // namespace

Result apply_names(Module* module) {
  NameApplier applier;
  return applier.VisitModule(module);
}

}  // namespace wabt
