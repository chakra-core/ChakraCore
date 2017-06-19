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

#include <cassert>
#include <cstdio>
#include <string>
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

class NameGenerator : public ExprVisitor::DelegateNop {
 public:
  NameGenerator();

  Result VisitModule(Module* module);

  // Implementation of ExprVisitor::DelegateNop.
  Result BeginBlockExpr(Expr* expr) override;
  Result BeginLoopExpr(Expr* expr) override;
  Result BeginIfExpr(Expr* expr) override;

 private:
  static bool HasName(StringSlice* str);
  static void GenerateName(const char* prefix, Index index, StringSlice* str);
  static void MaybeGenerateName(const char* prefix,
                                Index index,
                                StringSlice* str);
  static void GenerateAndBindName(BindingHash* bindings,
                                  const char* prefix,
                                  Index index,
                                  StringSlice* str);
  static void MaybeGenerateAndBindName(BindingHash* bindings,
                                       const char* prefix,
                                       Index index,
                                       StringSlice* str);
  void GenerateAndBindLocalNames(BindingHash* bindings, const char* prefix);
  Result VisitFunc(Index func_index, Func* func);
  Result VisitGlobal(Index global_index, Global* global);
  Result VisitFuncType(Index func_type_index, FuncType* func_type);
  Result VisitTable(Index table_index, Table* table);
  Result VisitMemory(Index memory_index, Memory* memory);

  Module* module_ = nullptr;
  ExprVisitor visitor_;
  std::vector<std::string> index_to_name_;
  Index label_count_ = 0;
};

NameGenerator::NameGenerator() : visitor_(this) {}

// static
bool NameGenerator::HasName(StringSlice* str) {
  return str->length > 0;
}

// static
void NameGenerator::GenerateName(const char* prefix,
                                 Index index,
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

// static
void NameGenerator::MaybeGenerateName(const char* prefix,
                                      Index index,
                                      StringSlice* str) {
  if (!HasName(str))
    GenerateName(prefix, index, str);
}

// static
void NameGenerator::GenerateAndBindName(BindingHash* bindings,
                                        const char* prefix,
                                        Index index,
                                        StringSlice* str) {
  GenerateName(prefix, index, str);
  bindings->emplace(string_slice_to_string(*str), Binding(index));
}

// static
void NameGenerator::MaybeGenerateAndBindName(BindingHash* bindings,
                                             const char* prefix,
                                             Index index,
                                             StringSlice* str) {
  if (!HasName(str))
    GenerateAndBindName(bindings, prefix, index, str);
}

void NameGenerator::GenerateAndBindLocalNames(BindingHash* bindings,
                                              const char* prefix) {
  for (size_t i = 0; i < index_to_name_.size(); ++i) {
    const std::string& old_name = index_to_name_[i];
    if (!old_name.empty())
      continue;

    StringSlice new_name;
    GenerateAndBindName(bindings, prefix, i, &new_name);
    index_to_name_[i] = string_slice_to_string(new_name);
    destroy_string_slice(&new_name);
  }
}

Result NameGenerator::BeginBlockExpr(Expr* expr) {
  MaybeGenerateName("$B", label_count_++, &expr->block->label);
  return Result::Ok;
}

Result NameGenerator::BeginLoopExpr(Expr* expr) {
  MaybeGenerateName("$L", label_count_++, &expr->loop->label);
  return Result::Ok;
}

Result NameGenerator::BeginIfExpr(Expr* expr) {
  MaybeGenerateName("$I", label_count_++, &expr->if_.true_->label);
  return Result::Ok;
}

Result NameGenerator::VisitFunc(Index func_index, Func* func) {
  MaybeGenerateAndBindName(&module_->func_bindings, "$f", func_index,
                           &func->name);

  MakeTypeBindingReverseMapping(func->decl.sig.param_types,
                                func->param_bindings, &index_to_name_);
  GenerateAndBindLocalNames(&func->param_bindings, "$p");

  MakeTypeBindingReverseMapping(func->local_types, func->local_bindings,
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

Result NameGenerator::VisitModule(Module* module) {
  module_ = module;
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
  module_ = nullptr;
  return Result::Ok;
}

}  // namespace

Result generate_names(Module* module) {
  NameGenerator generator;
  return generator.VisitModule(module);
}

}  // namespace wabt
