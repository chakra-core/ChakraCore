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

#ifndef WABT_IR_H_
#define WABT_IR_H_

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <type_traits>

#include "src/binding-hash.h"
#include "src/common.h"
#include "src/intrusive-list.h"
#include "src/opcode.h"
#include "src/string-view.h"

namespace wabt {

enum class VarType {
  Index,
  Name,
};

struct Var {
  explicit Var(Index index = kInvalidIndex, const Location& loc = Location());
  explicit Var(string_view name, const Location& loc = Location());
  Var(Var&&);
  Var(const Var&);
  Var& operator =(const Var&);
  Var& operator =(Var&&);
  ~Var();

  VarType type() const { return type_; }
  bool is_index() const { return type_ == VarType::Index; }
  bool is_name() const { return type_ == VarType::Name; }

  Index index() const { assert(is_index()); return index_; }
  const std::string& name() const { assert(is_name()); return name_; }

  void set_index(Index);
  void set_name(std::string&&);
  void set_name(string_view);

  Location loc;

 private:
  void Destroy();

  VarType type_;
  union {
    Index index_;
    std::string name_;
  };
};
typedef std::vector<Var> VarVector;

struct Const {
  Const() : Const(I32Tag(), 0, Location()) {}

  static Const I32(uint32_t val = 0, const Location& loc = Location()) {
    return Const(I32Tag(), val, loc);
  }

  static Const I64(uint64_t val = 0, const Location& loc = Location()) {
    return Const(I64Tag(), val, loc);
  }

  static Const F32(uint32_t val = 0, const Location& loc = Location()) {
    return Const(F32Tag(), val, loc);
  }

  static Const F64(uint64_t val = 0, const Location& loc = Location()) {
    return Const(F64Tag(), val, loc);
  }

  Location loc;
  Type type;
  union {
    uint32_t u32;
    uint64_t u64;
    uint32_t f32_bits;
    uint64_t f64_bits;
  };

 private:
  // Struct tags to differentiate constructors.
  struct I32Tag {};
  struct I64Tag {};
  struct F32Tag {};
  struct F64Tag {};

  Const(I32Tag, uint32_t val = 0, const Location& loc = Location());
  Const(I64Tag, uint64_t val = 0, const Location& loc = Location());
  Const(F32Tag, uint32_t val = 0, const Location& loc = Location());
  Const(F64Tag, uint64_t val = 0, const Location& loc = Location());
};
typedef std::vector<Const> ConstVector;

enum class ExprType {
  Binary,
  Block,
  Br,
  BrIf,
  BrTable,
  Call,
  CallIndirect,
  Compare,
  Const,
  Convert,
  CurrentMemory,
  Drop,
  GetGlobal,
  GetLocal,
  GrowMemory,
  If,
  Load,
  Loop,
  Nop,
  Rethrow,
  Return,
  Select,
  SetGlobal,
  SetLocal,
  Store,
  TeeLocal,
  Throw,
  TryBlock,
  Unary,
  Unreachable,

  First = Binary,
  Last = Unreachable
};

const char* GetExprTypeName(ExprType type);

typedef TypeVector BlockSignature;

class Expr;
typedef intrusive_list<Expr> ExprList;

struct Block {
  Block() = default;
  explicit Block(ExprList exprs) : exprs(std::move(exprs)) {}

  std::string label;
  BlockSignature sig;
  ExprList exprs;
};

struct Catch {
  explicit Catch(const Location& loc = Location()) : loc(loc) {}
  explicit Catch(const Var& var, const Location& loc = Location())
      : loc(loc), var(var) {}
  Location loc;
  Var var;
  ExprList exprs;
  bool IsCatchAll() const {
    return var.is_index() && var.index() == kInvalidIndex;
  }
};

typedef std::vector<Catch> CatchVector;

class Expr : public intrusive_list_base<Expr> {
 public:
  WABT_DISALLOW_COPY_AND_ASSIGN(Expr);
  Expr() = delete;
  virtual ~Expr() = default;

  ExprType type() const { return type_; }

  Location loc;

 protected:
  explicit Expr(ExprType type, const Location& loc = Location())
      : loc(loc), type_(type) {}

  ExprType type_;
};

const char* GetExprTypeName(const Expr& expr);

template <ExprType TypeEnum>
class ExprMixin : public Expr {
 public:
  static bool classof(const Expr* expr) { return expr->type() == TypeEnum; }

  explicit ExprMixin(const Location& loc = Location()) : Expr(TypeEnum, loc) {}
};

typedef ExprMixin<ExprType::CurrentMemory> CurrentMemoryExpr;
typedef ExprMixin<ExprType::Drop> DropExpr;
typedef ExprMixin<ExprType::GrowMemory> GrowMemoryExpr;
typedef ExprMixin<ExprType::Nop> NopExpr;
typedef ExprMixin<ExprType::Return> ReturnExpr;
typedef ExprMixin<ExprType::Select> SelectExpr;
typedef ExprMixin<ExprType::Unreachable> UnreachableExpr;

template <ExprType TypeEnum>
class OpcodeExpr : public ExprMixin<TypeEnum> {
 public:
  OpcodeExpr(Opcode opcode, const Location& loc = Location())
      : ExprMixin<TypeEnum>(loc), opcode(opcode) {}

  Opcode opcode;
};

typedef OpcodeExpr<ExprType::Binary> BinaryExpr;
typedef OpcodeExpr<ExprType::Compare> CompareExpr;
typedef OpcodeExpr<ExprType::Convert> ConvertExpr;
typedef OpcodeExpr<ExprType::Unary> UnaryExpr;

template <ExprType TypeEnum>
class VarExpr : public ExprMixin<TypeEnum> {
 public:
  VarExpr(const Var& var, const Location& loc = Location())
      : ExprMixin<TypeEnum>(loc), var(var) {}

  Var var;
};

typedef VarExpr<ExprType::Br> BrExpr;
typedef VarExpr<ExprType::BrIf> BrIfExpr;
typedef VarExpr<ExprType::Call> CallExpr;
typedef VarExpr<ExprType::CallIndirect> CallIndirectExpr;
typedef VarExpr<ExprType::GetGlobal> GetGlobalExpr;
typedef VarExpr<ExprType::GetLocal> GetLocalExpr;
typedef VarExpr<ExprType::Rethrow> RethrowExpr;
typedef VarExpr<ExprType::SetGlobal> SetGlobalExpr;
typedef VarExpr<ExprType::SetLocal> SetLocalExpr;
typedef VarExpr<ExprType::TeeLocal> TeeLocalExpr;
typedef VarExpr<ExprType::Throw> ThrowExpr;

template <ExprType TypeEnum>
class BlockExprBase : public ExprMixin<TypeEnum> {
 public:
  explicit BlockExprBase(const Location& loc = Location())
      : ExprMixin<TypeEnum>(loc) {}

  Block block;
};

typedef BlockExprBase<ExprType::Block> BlockExpr;
typedef BlockExprBase<ExprType::Loop> LoopExpr;

class IfExpr : public ExprMixin<ExprType::If> {
 public:
  explicit IfExpr(const Location& loc = Location())
      : ExprMixin<ExprType::If>(loc) {}

  Block true_;
  ExprList false_;
};

class TryExpr : public ExprMixin<ExprType::TryBlock> {
 public:
  explicit TryExpr(const Location& loc = Location())
      : ExprMixin<ExprType::TryBlock>(loc) {}

  Block block;
  CatchVector catches;
};

class BrTableExpr : public ExprMixin<ExprType::BrTable> {
 public:
  BrTableExpr(const Location& loc = Location())
      : ExprMixin<ExprType::BrTable>(loc) {}

  VarVector targets;
  Var default_target;
};

class ConstExpr : public ExprMixin<ExprType::Const> {
 public:
  ConstExpr(const Const& c, const Location& loc = Location())
      : ExprMixin<ExprType::Const>(loc), const_(c) {}

  Const const_;
};

template <ExprType TypeEnum>
class LoadStoreExpr : public ExprMixin<TypeEnum> {
 public:
  LoadStoreExpr(Opcode opcode,
                Address align,
                uint32_t offset,
                const Location& loc = Location())
      : ExprMixin<TypeEnum>(loc),
        opcode(opcode),
        align(align),
        offset(offset) {}

  Opcode opcode;
  Address align;
  uint32_t offset;
};

typedef LoadStoreExpr<ExprType::Load> LoadExpr;
typedef LoadStoreExpr<ExprType::Store> StoreExpr;

struct Exception {
  Exception() = default;
  explicit Exception(string_view name) : name(name.to_string()) {}

  std::string name;
  TypeVector sig;
};

struct FuncSignature {
  TypeVector param_types;
  TypeVector result_types;

  Index GetNumParams() const { return param_types.size(); }
  Index GetNumResults() const { return result_types.size(); }
  Type GetParamType(Index index) const { return param_types[index]; }
  Type GetResultType(Index index) const { return result_types[index]; }

  bool operator==(const FuncSignature&) const;
};

struct FuncType {
  FuncType() = default;
  explicit FuncType(string_view name) : name(name.to_string()) {}

  Index GetNumParams() const { return sig.GetNumParams(); }
  Index GetNumResults() const { return sig.GetNumResults(); }
  Type GetParamType(Index index) const { return sig.GetParamType(index); }
  Type GetResultType(Index index) const { return sig.GetResultType(index); }

  std::string name;
  FuncSignature sig;
};

struct FuncDeclaration {
  Index GetNumParams() const { return sig.GetNumParams(); }
  Index GetNumResults() const { return sig.GetNumResults(); }
  Type GetParamType(Index index) const { return sig.GetParamType(index); }
  Type GetResultType(Index index) const { return sig.GetResultType(index); }

  bool has_func_type = false;
  Var type_var;
  FuncSignature sig;
};

struct Func {
  Func() = default;
  explicit Func(string_view name) : name(name.to_string()) {}

  Type GetParamType(Index index) const { return decl.GetParamType(index); }
  Type GetResultType(Index index) const { return decl.GetResultType(index); }
  Index GetNumParams() const { return decl.GetNumParams(); }
  Index GetNumLocals() const { return local_types.size(); }
  Index GetNumParamsAndLocals() const {
    return GetNumParams() + GetNumLocals();
  }
  Index GetNumResults() const { return decl.GetNumResults(); }
  Index GetLocalIndex(const Var&) const;

  std::string name;
  FuncDeclaration decl;
  TypeVector local_types;
  BindingHash param_bindings;
  BindingHash local_bindings;
  ExprList exprs;
};

struct Global {
  Global() = default;
  explicit Global(string_view name) : name(name.to_string()) {}

  std::string name;
  Type type = Type::Void;
  bool mutable_ = false;
  ExprList init_expr;
};

struct Table {
  Table() = default;
  explicit Table(string_view name) : name(name.to_string()) {}

  std::string name;
  Limits elem_limits;
};

struct ElemSegment {
  Var table_var;
  ExprList offset;
  VarVector vars;
};

struct Memory {
  Memory() = default;
  explicit Memory(string_view name) : name(name.to_string()) {}

  std::string name;
  Limits page_limits;
};

struct DataSegment {
  Var memory_var;
  ExprList offset;
  std::vector<uint8_t> data;
};

class Import {
 public:
  WABT_DISALLOW_COPY_AND_ASSIGN(Import);
  Import() = delete;
  virtual ~Import() = default;

  ExternalKind kind() const { return kind_; }

  std::string module_name;
  std::string field_name;

 protected:
  Import(ExternalKind kind) : kind_(kind) {}

  ExternalKind kind_;
};

template <ExternalKind TypeEnum>
class ImportMixin : public Import {
 public:
  static bool classof(const Import* import) {
    return import->kind() == TypeEnum;
  }

  ImportMixin() : Import(TypeEnum) {}
};

class FuncImport : public ImportMixin<ExternalKind::Func> {
 public:
  explicit FuncImport(string_view name = string_view())
      : ImportMixin<ExternalKind::Func>(), func(name) {}

  Func func;
};

class TableImport : public ImportMixin<ExternalKind::Table> {
 public:
  explicit TableImport(string_view name = string_view())
      : ImportMixin<ExternalKind::Table>(), table(name) {}

  Table table;
};

class MemoryImport : public ImportMixin<ExternalKind::Memory> {
 public:
  explicit MemoryImport(string_view name = string_view())
      : ImportMixin<ExternalKind::Memory>(), memory(name) {}

  Memory memory;
};

class GlobalImport : public ImportMixin<ExternalKind::Global> {
 public:
  explicit GlobalImport(string_view name = string_view())
      : ImportMixin<ExternalKind::Global>(), global(name) {}

  Global global;
};

class ExceptionImport : public ImportMixin<ExternalKind::Except> {
 public:
  explicit ExceptionImport(string_view name = string_view())
      : ImportMixin<ExternalKind::Except>(), except(name) {}

  Exception except;
};

struct Export {
  std::string name;
  ExternalKind kind;
  Var var;
};

enum class ModuleFieldType {
  Func,
  Global,
  Import,
  Export,
  FuncType,
  Table,
  ElemSegment,
  Memory,
  DataSegment,
  Start,
  Except
};

class ModuleField : public intrusive_list_base<ModuleField> {
 public:
  WABT_DISALLOW_COPY_AND_ASSIGN(ModuleField);
  ModuleField() = delete;
  virtual ~ModuleField() = default;

  ModuleFieldType type() const { return type_; }

  Location loc;

 protected:
  ModuleField(ModuleFieldType type, const Location& loc)
      : loc(loc), type_(type) {}

  ModuleFieldType type_;
};

typedef intrusive_list<ModuleField> ModuleFieldList;

template <ModuleFieldType TypeEnum>
class ModuleFieldMixin : public ModuleField {
 public:
  static bool classof(const ModuleField* field) {
    return field->type() == TypeEnum;
  }

  explicit ModuleFieldMixin(const Location& loc) : ModuleField(TypeEnum, loc) {}
};

class FuncModuleField : public ModuleFieldMixin<ModuleFieldType::Func> {
 public:
  explicit FuncModuleField(const Location& loc = Location(),
                           string_view name = string_view())
      : ModuleFieldMixin<ModuleFieldType::Func>(loc), func(name) {}

  Func func;
};

class GlobalModuleField : public ModuleFieldMixin<ModuleFieldType::Global> {
 public:
  explicit GlobalModuleField(const Location& loc = Location(),
                             string_view name = string_view())
      : ModuleFieldMixin<ModuleFieldType::Global>(loc), global(name) {}

  Global global;
};

class ImportModuleField : public ModuleFieldMixin<ModuleFieldType::Import> {
 public:
  explicit ImportModuleField(const Location& loc = Location())
      : ModuleFieldMixin<ModuleFieldType::Import>(loc) {}
  explicit ImportModuleField(std::unique_ptr<Import> import,
                             const Location& loc = Location())
      : ModuleFieldMixin<ModuleFieldType::Import>(loc),
        import(std::move(import)) {}

  std::unique_ptr<Import> import;
};

class ExportModuleField : public ModuleFieldMixin<ModuleFieldType::Export> {
 public:
  explicit ExportModuleField(const Location& loc = Location())
      : ModuleFieldMixin<ModuleFieldType::Export>(loc) {}

  Export export_;
};

class FuncTypeModuleField : public ModuleFieldMixin<ModuleFieldType::FuncType> {
 public:
  explicit FuncTypeModuleField(const Location& loc = Location(),
                               string_view name = string_view())
      : ModuleFieldMixin<ModuleFieldType::FuncType>(loc), func_type(name) {}

  FuncType func_type;
};

class TableModuleField : public ModuleFieldMixin<ModuleFieldType::Table> {
 public:
  explicit TableModuleField(const Location& loc = Location(),
                            string_view name = string_view())
      : ModuleFieldMixin<ModuleFieldType::Table>(loc), table(name) {}

  Table table;
};

class ElemSegmentModuleField
    : public ModuleFieldMixin<ModuleFieldType::ElemSegment> {
 public:
  explicit ElemSegmentModuleField(const Location& loc = Location())
      : ModuleFieldMixin<ModuleFieldType::ElemSegment>(loc) {}

  ElemSegment elem_segment;
};

class MemoryModuleField : public ModuleFieldMixin<ModuleFieldType::Memory> {
 public:
  explicit MemoryModuleField(const Location& loc = Location(),
                             string_view name = string_view())
      : ModuleFieldMixin<ModuleFieldType::Memory>(loc), memory(name) {}

  Memory memory;
};

class DataSegmentModuleField
    : public ModuleFieldMixin<ModuleFieldType::DataSegment> {
 public:
  explicit DataSegmentModuleField( const Location& loc = Location())
      : ModuleFieldMixin<ModuleFieldType::DataSegment>(loc) {}

  DataSegment data_segment;
};

class ExceptionModuleField : public ModuleFieldMixin<ModuleFieldType::Except> {
 public:
  explicit ExceptionModuleField(const Location& loc = Location(),
                                string_view name = string_view())
      : ModuleFieldMixin<ModuleFieldType::Except>(loc), except(name) {}

  Exception except;
};

class StartModuleField : public ModuleFieldMixin<ModuleFieldType::Start> {
 public:
  explicit StartModuleField(Var start = Var(),
                            const Location& loc = Location())
      : ModuleFieldMixin<ModuleFieldType::Start>(loc), start(start) {}

  Var start;
};

struct Module {
  Index GetFuncTypeIndex(const Var&) const;
  Index GetFuncTypeIndex(const FuncDeclaration&) const;
  Index GetFuncTypeIndex(const FuncSignature&) const;
  const FuncType* GetFuncType(const Var&) const;
  FuncType* GetFuncType(const Var&);
  Index GetFuncIndex(const Var&) const;
  const Func* GetFunc(const Var&) const;
  Func* GetFunc(const Var&);
  Index GetTableIndex(const Var&) const;
  Table* GetTable(const Var&);
  Index GetMemoryIndex(const Var&) const;
  Memory* GetMemory(const Var&);
  Index GetGlobalIndex(const Var&) const;
  const Global* GetGlobal(const Var&) const;
  Global* GetGlobal(const Var&);
  const Export* GetExport(string_view) const;
  Exception* GetExcept(const Var&) const;
  Index GetExceptIndex(const Var&) const;

  // TODO(binji): move this into a builder class?
  void AppendField(std::unique_ptr<DataSegmentModuleField>);
  void AppendField(std::unique_ptr<ElemSegmentModuleField>);
  void AppendField(std::unique_ptr<ExceptionModuleField>);
  void AppendField(std::unique_ptr<ExportModuleField>);
  void AppendField(std::unique_ptr<FuncModuleField>);
  void AppendField(std::unique_ptr<FuncTypeModuleField>);
  void AppendField(std::unique_ptr<GlobalModuleField>);
  void AppendField(std::unique_ptr<ImportModuleField>);
  void AppendField(std::unique_ptr<MemoryModuleField>);
  void AppendField(std::unique_ptr<StartModuleField>);
  void AppendField(std::unique_ptr<TableModuleField>);
  void AppendField(std::unique_ptr<ModuleField>);
  void AppendFields(ModuleFieldList*);

  Location loc;
  std::string name;
  ModuleFieldList fields;

  Index num_except_imports = 0;
  Index num_func_imports = 0;
  Index num_table_imports = 0;
  Index num_memory_imports = 0;
  Index num_global_imports = 0;

  // Cached for convenience; the pointers are shared with values that are
  // stored in either ModuleField or Import.
  std::vector<Exception*> excepts;
  std::vector<Func*> funcs;
  std::vector<Global*> globals;
  std::vector<Import*> imports;
  std::vector<Export*> exports;
  std::vector<FuncType*> func_types;
  std::vector<Table*> tables;
  std::vector<ElemSegment*> elem_segments;
  std::vector<Memory*> memories;
  std::vector<DataSegment*> data_segments;
  Var* start = nullptr;

  BindingHash except_bindings;
  BindingHash func_bindings;
  BindingHash global_bindings;
  BindingHash export_bindings;
  BindingHash func_type_bindings;
  BindingHash table_bindings;
  BindingHash memory_bindings;
};

enum class ScriptModuleType {
  Text,
  Binary,
  Quoted,
};

// A ScriptModule is a module that may not yet be decoded. This allows for text
// and binary parsing errors to be deferred until validation time.
class ScriptModule {
 public:
  WABT_DISALLOW_COPY_AND_ASSIGN(ScriptModule);
  ScriptModule() = delete;
  virtual ~ScriptModule() = default;

  ScriptModuleType type() const { return type_; }
  virtual const Location& location() const = 0;

 protected:
  explicit ScriptModule(ScriptModuleType type) : type_(type) {}

  ScriptModuleType type_;
};

template <ScriptModuleType TypeEnum>
class ScriptModuleMixin : public ScriptModule {
 public:
  static bool classof(const ScriptModule* script_module) {
    return script_module->type() == TypeEnum;
  }

  ScriptModuleMixin() : ScriptModule(TypeEnum) {}
};

class TextScriptModule : public ScriptModuleMixin<ScriptModuleType::Text> {
 public:
  const Location& location() const override { return module.loc; }

  Module module;
};

template <ScriptModuleType TypeEnum>
class DataScriptModule : public ScriptModuleMixin<TypeEnum> {
 public:
  const Location& location() const override { return loc; }

  Location loc;
  std::string name;
  std::vector<uint8_t> data;
};

typedef DataScriptModule<ScriptModuleType::Binary> BinaryScriptModule;
typedef DataScriptModule<ScriptModuleType::Quoted> QuotedScriptModule;

enum class ActionType {
  Invoke,
  Get,
};

class Action {
 public:
  WABT_DISALLOW_COPY_AND_ASSIGN(Action);
  Action() = delete;
  virtual ~Action() = default;

  ActionType type() const { return type_; }

  Location loc;
  Var module_var;
  std::string name;

 protected:
  explicit Action(ActionType type, const Location& loc = Location())
      : loc(loc), type_(type) {}

  ActionType type_;
};

typedef std::unique_ptr<Action> ActionPtr;

template <ActionType TypeEnum>
class ActionMixin : public Action {
 public:
  static bool classof(const Action* action) {
    return action->type() == TypeEnum;
  }

  explicit ActionMixin(const Location& loc = Location())
      : Action(TypeEnum, loc) {}
};

class GetAction : public ActionMixin<ActionType::Get> {
 public:
  explicit GetAction(const Location& loc = Location())
      : ActionMixin<ActionType::Get>(loc) {}
};

class InvokeAction : public ActionMixin<ActionType::Invoke> {
 public:
  explicit InvokeAction(const Location& loc = Location())
      : ActionMixin<ActionType::Invoke>(loc) {}

  ConstVector args;
};

enum class CommandType {
  Module,
  Action,
  Register,
  AssertMalformed,
  AssertInvalid,
  AssertUnlinkable,
  AssertUninstantiable,
  AssertReturn,
  AssertReturnCanonicalNan,
  AssertReturnArithmeticNan,
  AssertTrap,
  AssertExhaustion,

  First = Module,
  Last = AssertExhaustion,
};
static const int kCommandTypeCount = WABT_ENUM_COUNT(CommandType);

class Command {
 public:
  WABT_DISALLOW_COPY_AND_ASSIGN(Command);
  Command() = delete;
  virtual ~Command() = default;

  CommandType type;

 protected:
  explicit Command(CommandType type) : type(type) {}
};

template <CommandType TypeEnum>
class CommandMixin : public Command {
 public:
  static bool classof(const Command* cmd) { return cmd->type == TypeEnum; }
  CommandMixin() : Command(TypeEnum) {}
};

class ModuleCommand : public CommandMixin<CommandType::Module> {
 public:
  Module module;
};

template <CommandType TypeEnum>
class ActionCommandBase : public CommandMixin<TypeEnum> {
 public:
  ActionPtr action;
};

typedef ActionCommandBase<CommandType::Action> ActionCommand;
typedef ActionCommandBase<CommandType::AssertReturnCanonicalNan>
    AssertReturnCanonicalNanCommand;
typedef ActionCommandBase<CommandType::AssertReturnArithmeticNan>
    AssertReturnArithmeticNanCommand;

class RegisterCommand : public CommandMixin<CommandType::Register> {
 public:
  RegisterCommand(string_view module_name, const Var& var)
      : module_name(module_name), var(var) {}

  std::string module_name;
  Var var;
};

class AssertReturnCommand : public CommandMixin<CommandType::AssertReturn> {
 public:
  ActionPtr action;
  ConstVector expected;
};

template <CommandType TypeEnum>
class AssertTrapCommandBase : public CommandMixin<TypeEnum> {
 public:
  ActionPtr action;
  std::string text;
};

typedef AssertTrapCommandBase<CommandType::AssertTrap> AssertTrapCommand;
typedef AssertTrapCommandBase<CommandType::AssertExhaustion>
    AssertExhaustionCommand;

template <CommandType TypeEnum>
class AssertModuleCommand : public CommandMixin<TypeEnum> {
 public:
  std::unique_ptr<ScriptModule> module;
  std::string text;
};

typedef AssertModuleCommand<CommandType::AssertMalformed>
    AssertMalformedCommand;
typedef AssertModuleCommand<CommandType::AssertInvalid> AssertInvalidCommand;
typedef AssertModuleCommand<CommandType::AssertUnlinkable>
    AssertUnlinkableCommand;
typedef AssertModuleCommand<CommandType::AssertUninstantiable>
    AssertUninstantiableCommand;

typedef std::unique_ptr<Command> CommandPtr;
typedef std::vector<CommandPtr> CommandPtrVector;

struct Script {
  WABT_DISALLOW_COPY_AND_ASSIGN(Script);
  Script() = default;

  const Module* GetFirstModule() const;
  Module* GetFirstModule();
  const Module* GetModule(const Var&) const;

  CommandPtrVector commands;
  BindingHash module_bindings;
};

void MakeTypeBindingReverseMapping(
    const TypeVector& types,
    const BindingHash&  bindings,
    std::vector<std::string>* out_reverse_mapping);

}  // namespace wabt

#endif /* WABT_IR_H_ */
