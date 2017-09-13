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

#include "src/ir.h"

#include <cassert>
#include <cstddef>

#include "src/cast.h"

namespace {

const char* ExprTypeName[] = {
  "Binary",
  "Block",
  "Br",
  "BrIf",
  "BrTable",
  "Call",
  "CallIndirect",
  "Compare",
  "Const",
  "Convert",
  "CurrentMemory",
  "Drop",
  "GetGlobal",
  "GetLocal",
  "GrowMemory",
  "If",
  "Load",
  "Loop",
  "Nop",
  "Rethrow",
  "Return",
  "Select",
  "SetGlobal",
  "SetLocal",
  "Store",
  "TeeLocal",
  "Throw",
  "TryBlock",
  "Unary",
  "Unreachable"
};

}  // end of anonymous namespace

namespace wabt {

const char* GetExprTypeName(ExprType type) {
  static_assert(WABT_ENUM_COUNT(ExprType) == WABT_ARRAY_SIZE(ExprTypeName),
                "Malformed ExprTypeName array");
  return ExprTypeName[size_t(type)];
}

const char* GetExprTypeName(const Expr& expr) {
  return GetExprTypeName(expr.type());
}

bool FuncSignature::operator==(const FuncSignature& rhs) const {
  return param_types == rhs.param_types && result_types == rhs.result_types;
}

const Export* Module::GetExport(string_view name) const {
  Index index = export_bindings.FindIndex(name);
  if (index >= exports.size())
    return nullptr;
  return exports[index];
}

Index Module::GetFuncIndex(const Var& var) const {
  return func_bindings.FindIndex(var);
}

Index Module::GetGlobalIndex(const Var& var) const {
  return global_bindings.FindIndex(var);
}

Index Module::GetTableIndex(const Var& var) const {
  return table_bindings.FindIndex(var);
}

Index Module::GetMemoryIndex(const Var& var) const {
  return memory_bindings.FindIndex(var);
}

Index Module::GetFuncTypeIndex(const Var& var) const {
  return func_type_bindings.FindIndex(var);
}

Index Module::GetExceptIndex(const Var& var) const {
  return except_bindings.FindIndex(var);
}

Index Func::GetLocalIndex(const Var& var) const {
  if (var.is_index())
    return var.index();

  Index result = param_bindings.FindIndex(var);
  if (result != kInvalidIndex)
    return result;

  result = local_bindings.FindIndex(var);
  if (result == kInvalidIndex)
    return result;

  // The locals start after all the params.
  return decl.GetNumParams() + result;
}

const Func* Module::GetFunc(const Var& var) const {
  return const_cast<Module*>(this)->GetFunc(var);
}

Func* Module::GetFunc(const Var& var) {
  Index index = func_bindings.FindIndex(var);
  if (index >= funcs.size())
    return nullptr;
  return funcs[index];
}

const Global* Module::GetGlobal(const Var& var) const {
  return const_cast<Module*>(this)->GetGlobal(var);
}

Global* Module::GetGlobal(const Var& var) {
  Index index = global_bindings.FindIndex(var);
  if (index >= globals.size())
    return nullptr;
  return globals[index];
}

Table* Module::GetTable(const Var& var) {
  Index index = table_bindings.FindIndex(var);
  if (index >= tables.size())
    return nullptr;
  return tables[index];
}

Memory* Module::GetMemory(const Var& var) {
  Index index = memory_bindings.FindIndex(var);
  if (index >= memories.size())
    return nullptr;
  return memories[index];
}

Exception* Module::GetExcept(const Var& var) const {
  Index index = GetExceptIndex(var);
  if (index >= excepts.size())
    return nullptr;
  return excepts[index];
}

const FuncType* Module::GetFuncType(const Var& var) const {
  return const_cast<Module*>(this)->GetFuncType(var);
}

FuncType* Module::GetFuncType(const Var& var) {
  Index index = func_type_bindings.FindIndex(var);
  if (index >= func_types.size())
    return nullptr;
  return func_types[index];
}


Index Module::GetFuncTypeIndex(const FuncSignature& sig) const {
  for (size_t i = 0; i < func_types.size(); ++i)
    if (func_types[i]->sig == sig)
      return i;
  return kInvalidIndex;
}

Index Module::GetFuncTypeIndex(const FuncDeclaration& decl) const {
  if (decl.has_func_type) {
    return GetFuncTypeIndex(decl.type_var);
  } else {
    return GetFuncTypeIndex(decl.sig);
  }
}

void Module::AppendField(std::unique_ptr<DataSegmentModuleField> field) {
  data_segments.push_back(&field->data_segment);
  fields.push_back(std::move(field));
}

void Module::AppendField(std::unique_ptr<ElemSegmentModuleField> field) {
  elem_segments.push_back(&field->elem_segment);
  fields.push_back(std::move(field));
}

void Module::AppendField(std::unique_ptr<ExceptionModuleField> field) {
  Exception& except = field->except;
  if (!except.name.empty())
    except_bindings.emplace(except.name, Binding(field->loc, excepts.size()));
  excepts.push_back(&except);
  fields.push_back(std::move(field));
}

void Module::AppendField(std::unique_ptr<ExportModuleField> field) {
  // Exported names are allowed to be empty.
  Export& export_ = field->export_;
  export_bindings.emplace(export_.name, Binding(field->loc, exports.size()));
  exports.push_back(&export_);
  fields.push_back(std::move(field));
}

void Module::AppendField(std::unique_ptr<FuncModuleField> field) {
  Func& func = field->func;
  if (!func.name.empty())
    func_bindings.emplace(func.name, Binding(field->loc, funcs.size()));
  funcs.push_back(&func);
  fields.push_back(std::move(field));
}

void Module::AppendField(std::unique_ptr<FuncTypeModuleField> field) {
  FuncType& func_type = field->func_type;
  if (!func_type.name.empty()) {
    func_type_bindings.emplace(func_type.name,
                               Binding(field->loc, func_types.size()));
  }
  func_types.push_back(&func_type);
  fields.push_back(std::move(field));
}

void Module::AppendField(std::unique_ptr<GlobalModuleField> field) {
  Global& global = field->global;
  if (!global.name.empty())
    global_bindings.emplace(global.name, Binding(field->loc, globals.size()));
  globals.push_back(&global);
  fields.push_back(std::move(field));
}

void Module::AppendField(std::unique_ptr<ImportModuleField> field) {
  Import* import = field->import.get();
  const std::string* name = nullptr;
  BindingHash* bindings = nullptr;
  Index index = kInvalidIndex;

  switch (import->kind()) {
    case ExternalKind::Func: {
      Func& func = cast<FuncImport>(import)->func;
      name = &func.name;
      bindings = &func_bindings;
      index = funcs.size();
      funcs.push_back(&func);
      ++num_func_imports;
      break;
    }

    case ExternalKind::Table: {
      Table& table = cast<TableImport>(import)->table;
      name = &table.name;
      bindings = &table_bindings;
      index = tables.size();
      tables.push_back(&table);
      ++num_table_imports;
      break;
    }

    case ExternalKind::Memory: {
      Memory& memory = cast<MemoryImport>(import)->memory;
      name = &memory.name;
      bindings = &memory_bindings;
      index = memories.size();
      memories.push_back(&memory);
      ++num_memory_imports;
      break;
    }

    case ExternalKind::Global: {
      Global& global = cast<GlobalImport>(import)->global;
      name = &global.name;
      bindings = &global_bindings;
      index = globals.size();
      globals.push_back(&global);
      ++num_global_imports;
      break;
    }

    case ExternalKind::Except: {
      Exception& except = cast<ExceptionImport>(import)->except;
      name = &except.name;
      bindings = &except_bindings;
      index = excepts.size();
      excepts.push_back(&except);
      ++num_except_imports;
      break;
    }
  }

  assert(name && bindings && index != kInvalidIndex);
  if (!name->empty())
    bindings->emplace(*name, Binding(field->loc, index));
  imports.push_back(import);
  fields.push_back(std::move(field));
}

void Module::AppendField(std::unique_ptr<MemoryModuleField> field) {
  Memory& memory = field->memory;
  if (!memory.name.empty())
    memory_bindings.emplace(memory.name, Binding(field->loc, memories.size()));
  memories.push_back(&memory);
  fields.push_back(std::move(field));
}

void Module::AppendField(std::unique_ptr<StartModuleField> field) {
  start = &field->start;
  fields.push_back(std::move(field));
}

void Module::AppendField(std::unique_ptr<TableModuleField> field) {
  Table& table = field->table;
  if (!table.name.empty())
    table_bindings.emplace(table.name, Binding(field->loc, tables.size()));
  tables.push_back(&table);
  fields.push_back(std::move(field));
}

void Module::AppendField(std::unique_ptr<ModuleField> field) {
  switch (field->type()) {
    case ModuleFieldType::Func:
      AppendField(cast<FuncModuleField>(std::move(field)));
      break;

    case ModuleFieldType::Global:
      AppendField(cast<GlobalModuleField>(std::move(field)));
      break;

    case ModuleFieldType::Import:
      AppendField(cast<ImportModuleField>(std::move(field)));
      break;

    case ModuleFieldType::Export:
      AppendField(cast<ExportModuleField>(std::move(field)));
      break;

    case ModuleFieldType::FuncType:
      AppendField(cast<FuncTypeModuleField>(std::move(field)));
      break;

    case ModuleFieldType::Table:
      AppendField(cast<TableModuleField>(std::move(field)));
      break;

    case ModuleFieldType::ElemSegment:
      AppendField(cast<ElemSegmentModuleField>(std::move(field)));
      break;

    case ModuleFieldType::Memory:
      AppendField(cast<MemoryModuleField>(std::move(field)));
      break;

    case ModuleFieldType::DataSegment:
      AppendField(cast<DataSegmentModuleField>(std::move(field)));
      break;

    case ModuleFieldType::Start:
      AppendField(cast<StartModuleField>(std::move(field)));
      break;

    case ModuleFieldType::Except:
      AppendField(cast<ExceptionModuleField>(std::move(field)));
      break;
  }
}

void Module::AppendFields(ModuleFieldList* fields) {
  while (!fields->empty())
    AppendField(std::unique_ptr<ModuleField>(fields->extract_front()));
}

const Module* Script::GetFirstModule() const {
  return const_cast<Script*>(this)->GetFirstModule();
}

Module* Script::GetFirstModule() {
  for (const std::unique_ptr<Command>& command : commands) {
    if (auto* module_command = dyn_cast<ModuleCommand>(command.get()))
      return &module_command->module;
  }
  return nullptr;
}

const Module* Script::GetModule(const Var& var) const {
  Index index = module_bindings.FindIndex(var);
  if (index >= commands.size())
    return nullptr;
  auto* command = cast<ModuleCommand>(commands[index].get());
  return &command->module;
}

void MakeTypeBindingReverseMapping(
    const TypeVector& types,
    const BindingHash& bindings,
    std::vector<std::string>* out_reverse_mapping) {
  out_reverse_mapping->clear();
  out_reverse_mapping->resize(types.size());
  for (const auto& pair : bindings) {
    assert(static_cast<size_t>(pair.second.index) <
           out_reverse_mapping->size());
    (*out_reverse_mapping)[pair.second.index] = pair.first;
  }
}

Var::Var(Index index, const Location& loc)
    : loc(loc), type_(VarType::Index), index_(index) {}

Var::Var(string_view name, const Location& loc)
    : loc(loc), type_(VarType::Name), name_(name) {}

Var::Var(Var&& rhs) : Var(kInvalidIndex) {
  *this = std::move(rhs);
}

Var::Var(const Var& rhs) : Var(kInvalidIndex) {
  *this = rhs;
}

Var& Var::operator=(Var&& rhs) {
  loc = rhs.loc;
  if (rhs.is_index()) {
    set_index(rhs.index_);
  } else {
    set_name(rhs.name_);
  }
  return *this;
}

Var& Var::operator=(const Var& rhs) {
  loc = rhs.loc;
  if (rhs.is_index()) {
    set_index(rhs.index_);
  } else {
    set_name(rhs.name_);
  }
  return *this;
}

Var::~Var() {
  Destroy();
}

void Var::set_index(Index index) {
  Destroy();
  type_ = VarType::Index;
  index_ = index;
}

void Var::set_name(std::string&& name) {
  Destroy();
  type_ = VarType::Name;
  Construct(name_, std::move(name));
}

void Var::set_name(string_view name) {
  set_name(name.to_string());
}

void Var::Destroy() {
  if (is_name())
    Destruct(name_);
}

Const::Const(I32Tag, uint32_t value, const Location& loc_)
    : loc(loc_), type(Type::I32), u32(value) {
}

Const::Const(I64Tag, uint64_t value, const Location& loc_)
    : loc(loc_), type(Type::I64), u64(value) {
}

Const::Const(F32Tag, uint32_t value, const Location& loc_)
    : loc(loc_), type(Type::F32), f32_bits(value) {
}

Const::Const(F64Tag, uint64_t value, const Location& loc_)
    : loc(loc_), type(Type::F64), f64_bits(value) {
}

}  // namespace wabt
