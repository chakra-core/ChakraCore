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

#include "src/generate-names.h"

#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

#include "src/cast.h"
#include "src/expr-visitor.h"
#include "src/ir.h"

namespace wabt {

namespace {

class NameGenerator : public ExprVisitor::DelegateNop {
 public:
  NameGenerator();

  Result VisitModule(Module* module);

  // Implementation of ExprVisitor::DelegateNop.
  Result BeginBlockExpr(BlockExpr* expr) override;
  Result BeginLoopExpr(LoopExpr* expr) override;
  Result BeginIfExpr(IfExpr* expr) override;
  Result BeginIfExceptExpr(IfExceptExpr* expr) override;

 private:
  static bool HasName(const std::string& str);

  // Generate a name with the given prefix, followed by the index and
  // optionally a disambiguating number. If index == kInvalidIndex, the index
  // is not appended.
  static void GenerateName(const char* prefix,
                           Index index,
                           unsigned disambiguator,
                           std::string* out_str);

  // Like GenerateName, but only generates a name if |out_str| is empty.
  static void MaybeGenerateName(const char* prefix,
                                Index index,
                                std::string* out_str);

  // Generate a name via GenerateName and bind it to the given binding hash. If
  // the name already exists, the name will be disambiguated until it can be
  // added.
  static void GenerateAndBindName(BindingHash* bindings,
                                  const char* prefix,
                                  Index index,
                                  std::string* out_str);

  // Like GenerateAndBindName, but only  generates a name if |out_str| is empty.
  static void MaybeGenerateAndBindName(BindingHash* bindings,
                                       const char* prefix,
                                       Index index,
                                       std::string* out_str);

  // Like MaybeGenerateAndBindName but uses the name directly, without
  // appending the index. If the name already exists, a disambiguating suffix
  // is added.
  static void MaybeUseAndBindName(BindingHash* bindings,
                                  const char* name,
                                  Index index,
                                  std::string* out_str);

  void GenerateAndBindLocalNames(BindingHash* bindings, const char* prefix);
  Result VisitFunc(Index func_index, Func* func);
  Result VisitGlobal(Index global_index, Global* global);
  Result VisitFuncType(Index func_type_index, FuncType* func_type);
  Result VisitTable(Index table_index, Table* table);
  Result VisitMemory(Index memory_index, Memory* memory);
  Result VisitExcept(Index except_index, Exception* except);
  Result VisitImport(Import* import);
  Result VisitExport(Export* export_);

  Module* module_ = nullptr;
  ExprVisitor visitor_;
  std::vector<std::string> index_to_name_;
  Index label_count_ = 0;

  Index num_func_imports_ = 0;
  Index num_table_imports_ = 0;
  Index num_memory_imports_ = 0;
  Index num_global_imports_ = 0;
  Index num_exception_imports_ = 0;
};

NameGenerator::NameGenerator() : visitor_(this) {}

// static
bool NameGenerator::HasName(const std::string& str) {
  return !str.empty();
}

// static
void NameGenerator::GenerateName(const char* prefix,
                                 Index index,
                                 unsigned disambiguator,
                                 std::string* str) {
  *str = prefix;
  if (index != kInvalidIndex) {
    *str += std::to_string(index);
  }
  if (disambiguator != 0) {
    *str += '_' + std::to_string(disambiguator);
  }
}

// static
void NameGenerator::MaybeGenerateName(const char* prefix,
                                      Index index,
                                      std::string* str) {
  if (!HasName(*str)) {
    // There's no bindings hash, so the name can't be a duplicate. Therefore it
    // doesn't need a disambiguating number.
    GenerateName(prefix, index, 0, str);
  }
}

// static
void NameGenerator::GenerateAndBindName(BindingHash* bindings,
                                        const char* prefix,
                                        Index index,
                                        std::string* str) {
  unsigned disambiguator = 0;
  while (true) {
    GenerateName(prefix, index, disambiguator, str);
    if (bindings->find(*str) == bindings->end()) {
      bindings->emplace(*str, Binding(index));
      break;
    }

    disambiguator++;
  }
}

// static
void NameGenerator::MaybeGenerateAndBindName(BindingHash* bindings,
                                             const char* prefix,
                                             Index index,
                                             std::string* str) {
  if (!HasName(*str)) {
    GenerateAndBindName(bindings, prefix, index, str);
  }
}

// static
void NameGenerator::MaybeUseAndBindName(BindingHash* bindings,
                                        const char* name,
                                        Index index,
                                        std::string* str) {
  if (!HasName(*str)) {
    unsigned disambiguator = 0;
    while (true) {
      GenerateName(name, kInvalidIndex, disambiguator, str);
      if (bindings->find(*str) == bindings->end()) {
        bindings->emplace(*str, Binding(index));
        break;
      }

      disambiguator++;
    }
  }
}

void NameGenerator::GenerateAndBindLocalNames(BindingHash* bindings,
                                              const char* prefix) {
  for (size_t i = 0; i < index_to_name_.size(); ++i) {
    const std::string& old_name = index_to_name_[i];
    if (!old_name.empty()) {
      continue;
    }

    std::string new_name;
    GenerateAndBindName(bindings, prefix, i, &new_name);
    index_to_name_[i] = new_name;
  }
}

Result NameGenerator::BeginBlockExpr(BlockExpr* expr) {
  MaybeGenerateName("$B", label_count_++, &expr->block.label);
  return Result::Ok;
}

Result NameGenerator::BeginLoopExpr(LoopExpr* expr) {
  MaybeGenerateName("$L", label_count_++, &expr->block.label);
  return Result::Ok;
}

Result NameGenerator::BeginIfExpr(IfExpr* expr) {
  MaybeGenerateName("$I", label_count_++, &expr->true_.label);
  return Result::Ok;
}

Result NameGenerator::BeginIfExceptExpr(IfExceptExpr* expr) {
  MaybeGenerateName("$E", label_count_++, &expr->true_.label);
  return Result::Ok;
}

Result NameGenerator::VisitFunc(Index func_index, Func* func) {
  MaybeGenerateAndBindName(&module_->func_bindings, "$f", func_index,
                           &func->name);

  MakeTypeBindingReverseMapping(func->decl.sig.param_types.size(),
                                func->param_bindings, &index_to_name_);
  GenerateAndBindLocalNames(&func->param_bindings, "$p");

  MakeTypeBindingReverseMapping(func->local_types.size(), func->local_bindings,
                                &index_to_name_);
  GenerateAndBindLocalNames(&func->local_bindings, "$l");

  label_count_ = 0;
  CHECK_RESULT(visitor_.VisitFunc(func));
  return Result::Ok;
}

Result NameGenerator::VisitGlobal(Index global_index, Global* global) {
  MaybeGenerateAndBindName(&module_->global_bindings, "$g", global_index,
                           &global->name);
  return Result::Ok;
}

Result NameGenerator::VisitFuncType(Index func_type_index,
                                    FuncType* func_type) {
  MaybeGenerateAndBindName(&module_->func_type_bindings, "$t", func_type_index,
                           &func_type->name);
  return Result::Ok;
}

Result NameGenerator::VisitTable(Index table_index, Table* table) {
  MaybeGenerateAndBindName(&module_->table_bindings, "$T", table_index,
                           &table->name);
  return Result::Ok;
}

Result NameGenerator::VisitMemory(Index memory_index, Memory* memory) {
  MaybeGenerateAndBindName(&module_->memory_bindings, "$M", memory_index,
                           &memory->name);
  return Result::Ok;
}

Result NameGenerator::VisitExcept(Index except_index, Exception* except) {
  MaybeGenerateAndBindName(&module_->except_bindings, "$e", except_index,
                           &except->name);
  return Result::Ok;
}

Result NameGenerator::VisitImport(Import* import) {
  BindingHash* bindings = nullptr;
  std::string* name = nullptr;
  Index index = kInvalidIndex;

  switch (import->kind()) {
    case ExternalKind::Func:
      if (auto* func_import = cast<FuncImport>(import)) {
        bindings = &module_->func_bindings;
        name = &func_import->func.name;
        index = num_func_imports_++;
      }
      break;

    case ExternalKind::Table:
      if (auto* table_import = cast<TableImport>(import)) {
        bindings = &module_->table_bindings;
        name = &table_import->table.name;
        index = num_table_imports_++;
      }
      break;

    case ExternalKind::Memory:
      if (auto* memory_import = cast<MemoryImport>(import)) {
        bindings = &module_->memory_bindings;
        name = &memory_import->memory.name;
        index = num_memory_imports_++;
      }
      break;

    case ExternalKind::Global:
      if (auto* global_import = cast<GlobalImport>(import)) {
        bindings = &module_->global_bindings;
        name = &global_import->global.name;
        index = num_global_imports_++;
      }
      break;

    case ExternalKind::Except:
      if (auto* except_import = cast<ExceptionImport>(import)) {
        bindings = &module_->except_bindings;
        name = &except_import->except.name;
        index = num_exception_imports_++;
      }
      break;
  }

  if (bindings && name) {
    assert(index != kInvalidIndex);
    std::string new_name = '$' + import->module_name + '.' + import->field_name;
    MaybeUseAndBindName(bindings, new_name.c_str(), index, name);
  }

  return Result::Ok;
}

Result NameGenerator::VisitExport(Export* export_) {
  BindingHash* bindings = nullptr;
  std::string* name = nullptr;
  Index index = kInvalidIndex;

  switch (export_->kind) {
    case ExternalKind::Func:
      if (Func* func = module_->GetFunc(export_->var)) {
        index = module_->GetFuncIndex(export_->var);
        bindings = &module_->func_bindings;
        name = &func->name;
      }
      break;

    case ExternalKind::Table:
      if (Table* table = module_->GetTable(export_->var)) {
        index = module_->GetTableIndex(export_->var);
        bindings = &module_->table_bindings;
        name = &table->name;
      }
      break;

    case ExternalKind::Memory:
      if (Memory* memory = module_->GetMemory(export_->var)) {
        index = module_->GetMemoryIndex(export_->var);
        bindings = &module_->memory_bindings;
        name = &memory->name;
      }
      break;

    case ExternalKind::Global:
      if (Global* global = module_->GetGlobal(export_->var)) {
        index = module_->GetGlobalIndex(export_->var);
        bindings = &module_->global_bindings;
        name = &global->name;
      }
      break;

    case ExternalKind::Except:
      if (Exception* except = module_->GetExcept(export_->var)) {
        index = module_->GetExceptIndex(export_->var);
        bindings = &module_->except_bindings;
        name = &except->name;
      }
      break;
  }

  if (bindings && name) {
    std::string new_name = '$' + export_->name;
    MaybeUseAndBindName(bindings, new_name.c_str(), index, name);
  }

  return Result::Ok;
}

Result NameGenerator::VisitModule(Module* module) {
  module_ = module;
  // Visit imports and exports first to give better names, derived from the
  // import/export name.
  for (Index i = 0; i < module->imports.size(); ++i)
    CHECK_RESULT(VisitImport(module->imports[i]));
  for (Index i = 0; i < module->exports.size(); ++i)
    CHECK_RESULT(VisitExport(module->exports[i]));

  for (Index i = 0; i < module->globals.size(); ++i)
    CHECK_RESULT(VisitGlobal(i, module->globals[i]));
  for (Index i = 0; i < module->func_types.size(); ++i)
    CHECK_RESULT(VisitFuncType(i, module->func_types[i]));
  for (Index i = 0; i < module->funcs.size(); ++i)
    CHECK_RESULT(VisitFunc(i, module->funcs[i]));
  for (Index i = 0; i < module->tables.size(); ++i)
    CHECK_RESULT(VisitTable(i, module->tables[i]));
  for (Index i = 0; i < module->memories.size(); ++i)
    CHECK_RESULT(VisitMemory(i, module->memories[i]));
  for (Index i = 0; i < module->excepts.size(); ++i)
    CHECK_RESULT(VisitExcept(i, module->excepts[i]));
  module_ = nullptr;
  return Result::Ok;
}

}  // end anonymous namespace

Result GenerateNames(Module* module) {
  NameGenerator generator;
  return generator.VisitModule(module);
}

}  // namespace wabt
