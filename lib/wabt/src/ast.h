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

#ifndef WABT_AST_H_
#define WABT_AST_H_

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "binding-hash.h"
#include "common.h"

namespace wabt {

enum class VarType {
  Index,
  Name,
};

struct Var {
  // Keep the default constructor trivial so it can be used as a union member.
  Var() = default;
  explicit Var(int64_t index);
  explicit Var(const StringSlice& name);

  Location loc;
  VarType type;
  union {
    int64_t index;
    StringSlice name;
  };
};
typedef std::vector<Var> VarVector;

typedef StringSlice Label;

struct Const {
  // Struct tags to differentiate constructors.
  struct I32 {};
  struct I64 {};
  struct F32 {};
  struct F64 {};

  // Keep the default constructor trivial so it can be used as a union member.
  Const() = default;
  Const(I32, uint32_t);
  Const(I64, uint64_t);
  Const(F32, uint32_t);
  Const(F64, uint64_t);

  Location loc;
  Type type;
  union {
    uint32_t u32;
    uint64_t u64;
    uint32_t f32_bits;
    uint64_t f64_bits;
  };
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
  Return,
  Select,
  SetGlobal,
  SetLocal,
  Store,
  TeeLocal,
  Unary,
  Unreachable,
};

typedef TypeVector BlockSignature;

struct Block {
  WABT_DISALLOW_COPY_AND_ASSIGN(Block);
  Block();
  explicit Block(struct Expr* first);
  ~Block();

  Label label;
  BlockSignature sig;
  struct Expr* first;
};

struct Expr {
  WABT_DISALLOW_COPY_AND_ASSIGN(Expr);
  Expr();
  explicit Expr(ExprType);
  ~Expr();

  static Expr* CreateBinary(Opcode);
  static Expr* CreateBlock(Block*);
  static Expr* CreateBr(Var);
  static Expr* CreateBrIf(Var);
  static Expr* CreateBrTable(VarVector* targets, Var default_target);
  static Expr* CreateCall(Var);
  static Expr* CreateCallIndirect(Var);
  static Expr* CreateCompare(Opcode);
  static Expr* CreateConst(const Const&);
  static Expr* CreateConvert(Opcode);
  static Expr* CreateCurrentMemory();
  static Expr* CreateDrop();
  static Expr* CreateGetGlobal(Var);
  static Expr* CreateGetLocal(Var);
  static Expr* CreateGrowMemory();
  static Expr* CreateIf(struct Block* true_, struct Expr* false_ = nullptr);
  static Expr* CreateLoad(Opcode, uint32_t align, uint64_t offset);
  static Expr* CreateLoop(struct Block*);
  static Expr* CreateNop();
  static Expr* CreateReturn();
  static Expr* CreateSelect();
  static Expr* CreateSetGlobal(Var);
  static Expr* CreateSetLocal(Var);
  static Expr* CreateStore(Opcode, uint32_t align, uint64_t offset);
  static Expr* CreateTeeLocal(Var);
  static Expr* CreateUnary(Opcode);
  static Expr* CreateUnreachable();

  Location loc;
  ExprType type;
  Expr* next;
  union {
    struct { Opcode opcode; } binary, compare, convert, unary;
    struct Block *block, *loop;
    struct { Var var; } br, br_if;
    struct { VarVector* targets; Var default_target; } br_table;
    struct { Var var; } call, call_indirect;
    struct Const const_;
    struct { Var var; } get_global, set_global;
    struct { Var var; } get_local, set_local, tee_local;
    struct { struct Block* true_; struct Expr* false_; } if_;
    struct { Opcode opcode; uint32_t align; uint64_t offset; } load, store;
  };
};

struct FuncSignature {
  TypeVector param_types;
  TypeVector result_types;
};

struct FuncType {
  WABT_DISALLOW_COPY_AND_ASSIGN(FuncType);
  FuncType();
  ~FuncType();

  StringSlice name;
  FuncSignature sig;
};

struct FuncDeclaration {
  WABT_DISALLOW_COPY_AND_ASSIGN(FuncDeclaration);
  FuncDeclaration();
  ~FuncDeclaration();

  bool has_func_type;
  Var type_var;
  FuncSignature sig;
};

struct Func {
  WABT_DISALLOW_COPY_AND_ASSIGN(Func);
  Func();
  ~Func();

  StringSlice name;
  FuncDeclaration decl;
  TypeVector local_types;
  BindingHash param_bindings;
  BindingHash local_bindings;
  Expr* first_expr;
};

struct Global {
  WABT_DISALLOW_COPY_AND_ASSIGN(Global);
  Global();
  ~Global();

  StringSlice name;
  Type type;
  bool mutable_;
  Expr* init_expr;
};

struct Table {
  WABT_DISALLOW_COPY_AND_ASSIGN(Table);
  Table();
  ~Table();

  StringSlice name;
  Limits elem_limits;
};

struct ElemSegment {
  WABT_DISALLOW_COPY_AND_ASSIGN(ElemSegment);
  ElemSegment();
  ~ElemSegment();

  Var table_var;
  Expr* offset;
  VarVector vars;
};

struct Memory {
  WABT_DISALLOW_COPY_AND_ASSIGN(Memory);
  Memory();
  ~Memory();

  StringSlice name;
  Limits page_limits;
};

struct DataSegment {
  WABT_DISALLOW_COPY_AND_ASSIGN(DataSegment);
  DataSegment();
  ~DataSegment();

  Var memory_var;
  Expr* offset;
  char* data;
  size_t size;
};

struct Import {
  WABT_DISALLOW_COPY_AND_ASSIGN(Import);
  Import();
  ~Import();

  StringSlice module_name;
  StringSlice field_name;
  ExternalKind kind;
  union {
    /* an imported func is has the type Func so it can be more easily
     * included in the Module's vector of funcs; but only the
     * FuncDeclaration will have any useful information */
    Func* func;
    Table* table;
    Memory* memory;
    Global* global;
  };
};

struct Export {
  WABT_DISALLOW_COPY_AND_ASSIGN(Export);
  Export();
  ~Export();

  StringSlice name;
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
};

struct ModuleField {
  WABT_DISALLOW_COPY_AND_ASSIGN(ModuleField);
  ModuleField();
  ~ModuleField();

  Location loc;
  ModuleFieldType type;
  struct ModuleField* next;
  union {
    Func* func;
    Global* global;
    Import* import;
    Export* export_;
    FuncType* func_type;
    Table* table;
    ElemSegment* elem_segment;
    Memory* memory;
    DataSegment* data_segment;
    Var start;
  };
};

struct Module {
  WABT_DISALLOW_COPY_AND_ASSIGN(Module);
  Module();
  ~Module();

  Location loc;
  StringSlice name;
  ModuleField* first_field;
  ModuleField* last_field;

  uint32_t num_func_imports;
  uint32_t num_table_imports;
  uint32_t num_memory_imports;
  uint32_t num_global_imports;

  /* cached for convenience; the pointers are shared with values that are
   * stored in either ModuleField or Import. */
  std::vector<Func*> funcs;
  std::vector<Global*> globals;
  std::vector<Import*> imports;
  std::vector<Export*> exports;
  std::vector<FuncType*> func_types;
  std::vector<Table*> tables;
  std::vector<ElemSegment*> elem_segments;
  std::vector<Memory*> memories;
  std::vector<DataSegment*> data_segments;
  Var* start;

  BindingHash func_bindings;
  BindingHash global_bindings;
  BindingHash export_bindings;
  BindingHash func_type_bindings;
  BindingHash table_bindings;
  BindingHash memory_bindings;
};

enum class RawModuleType {
  Text,
  Binary,
};

/* "raw" means that the binary module has not yet been decoded. This is only
 * necessary when embedded in assert_invalid. In that case we want to defer
 * decoding errors until wabt_check_assert_invalid is called. This isn't needed
 * when parsing text, as assert_invalid always assumes that text parsing
 * succeeds. */
struct RawModule {
  WABT_DISALLOW_COPY_AND_ASSIGN(RawModule);
  RawModule();
  ~RawModule();

  RawModuleType type;
  union {
    Module* text;
    struct {
      Location loc;
      StringSlice name;
      char* data;
      size_t size;
    } binary;
  };
};

enum class ActionType {
  Invoke,
  Get,
};

struct ActionInvoke {
  WABT_DISALLOW_COPY_AND_ASSIGN(ActionInvoke);
  ActionInvoke();

  ConstVector args;
};

struct Action {
  WABT_DISALLOW_COPY_AND_ASSIGN(Action);
  Action();
  ~Action();

  Location loc;
  ActionType type;
  Var module_var;
  StringSlice name;
  union {
    ActionInvoke* invoke;
    struct {} get;
  };
};

enum class CommandType {
  Module,
  Action,
  Register,
  AssertMalformed,
  AssertInvalid,
  /* This is a module that is invalid, but cannot be written as a binary module
   * (e.g. it has unresolvable names.) */
  AssertInvalidNonBinary,
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

struct Command {
  WABT_DISALLOW_COPY_AND_ASSIGN(Command);
  Command();
  ~Command();

  CommandType type;
  union {
    Module* module;
    Action* action;
    struct { StringSlice module_name; Var var; } register_;
    struct { Action* action; ConstVector* expected; } assert_return;
    struct {
      Action* action;
    } assert_return_canonical_nan, assert_return_arithmetic_nan;
    struct { Action* action; StringSlice text; } assert_trap;
    struct {
      RawModule* module;
      StringSlice text;
    } assert_malformed, assert_invalid, assert_unlinkable,
        assert_uninstantiable;
  };
};
typedef std::vector<std::unique_ptr<Command>> CommandPtrVector;

struct Script {
  WABT_DISALLOW_COPY_AND_ASSIGN(Script);
  Script();

  CommandPtrVector commands;
  BindingHash module_bindings;
};

struct ExprVisitor {
  void* user_data;
  Result (*on_binary_expr)(Expr*, void* user_data);
  Result (*begin_block_expr)(Expr*, void* user_data);
  Result (*end_block_expr)(Expr*, void* user_data);
  Result (*on_br_expr)(Expr*, void* user_data);
  Result (*on_br_if_expr)(Expr*, void* user_data);
  Result (*on_br_table_expr)(Expr*, void* user_data);
  Result (*on_call_expr)(Expr*, void* user_data);
  Result (*on_call_indirect_expr)(Expr*, void* user_data);
  Result (*on_compare_expr)(Expr*, void* user_data);
  Result (*on_const_expr)(Expr*, void* user_data);
  Result (*on_convert_expr)(Expr*, void* user_data);
  Result (*on_current_memory_expr)(Expr*, void* user_data);
  Result (*on_drop_expr)(Expr*, void* user_data);
  Result (*on_get_global_expr)(Expr*, void* user_data);
  Result (*on_get_local_expr)(Expr*, void* user_data);
  Result (*on_grow_memory_expr)(Expr*, void* user_data);
  Result (*begin_if_expr)(Expr*, void* user_data);
  Result (*after_if_true_expr)(Expr*, void* user_data);
  Result (*end_if_expr)(Expr*, void* user_data);
  Result (*on_load_expr)(Expr*, void* user_data);
  Result (*begin_loop_expr)(Expr*, void* user_data);
  Result (*end_loop_expr)(Expr*, void* user_data);
  Result (*on_nop_expr)(Expr*, void* user_data);
  Result (*on_return_expr)(Expr*, void* user_data);
  Result (*on_select_expr)(Expr*, void* user_data);
  Result (*on_set_global_expr)(Expr*, void* user_data);
  Result (*on_set_local_expr)(Expr*, void* user_data);
  Result (*on_store_expr)(Expr*, void* user_data);
  Result (*on_tee_local_expr)(Expr*, void* user_data);
  Result (*on_unary_expr)(Expr*, void* user_data);
  Result (*on_unreachable_expr)(Expr*, void* user_data);
};

ModuleField* append_module_field(Module*);
/* ownership of the function signature is passed to the module */
FuncType* append_implicit_func_type(Location*, Module*, FuncSignature*);

/* destruction functions. not needed unless you're creating your own AST
 elements */
void destroy_expr_list(Expr*);
void destroy_var(Var*);

/* traversal functions */
Result visit_func(Func* func, ExprVisitor*);
Result visit_expr_list(Expr* expr, ExprVisitor*);

/* convenience functions for looking through the AST */
int get_index_from_var(const BindingHash* bindings, const Var* var);
int get_func_index_by_var(const Module* module, const Var* var);
int get_global_index_by_var(const Module* func, const Var* var);
int get_func_type_index_by_var(const Module* module, const Var* var);
int get_func_type_index_by_sig(const Module* module, const FuncSignature* sig);
int get_func_type_index_by_decl(const Module* module,
                                const FuncDeclaration* decl);
int get_table_index_by_var(const Module* module, const Var* var);
int get_memory_index_by_var(const Module* module, const Var* var);
int get_import_index_by_var(const Module* module, const Var* var);
int get_local_index_by_var(const Func* func, const Var* var);
int get_module_index_by_var(const Script* script, const Var* var);

Func* get_func_by_var(const Module* module, const Var* var);
Global* get_global_by_var(const Module* func, const Var* var);
FuncType* get_func_type_by_var(const Module* module, const Var* var);
Table* get_table_by_var(const Module* module, const Var* var);
Memory* get_memory_by_var(const Module* module, const Var* var);
Import* get_import_by_var(const Module* module, const Var* var);
Export* get_export_by_name(const Module* module, const StringSlice* name);
Module* get_first_module(const Script* script);
Module* get_module_by_var(const Script* script, const Var* var);

void make_type_binding_reverse_mapping(
    const TypeVector&,
    const BindingHash&,
    std::vector<std::string>* out_reverse_mapping);

static WABT_INLINE bool decl_has_func_type(const FuncDeclaration* decl) {
  return decl->has_func_type;
}

static WABT_INLINE bool signatures_are_equal(const FuncSignature* sig1,
                                             const FuncSignature* sig2) {
  return sig1->param_types == sig2->param_types &&
         sig1->result_types == sig2->result_types;
}

static WABT_INLINE size_t get_num_params(const Func* func) {
  return func->decl.sig.param_types.size();
}

static WABT_INLINE size_t get_num_results(const Func* func) {
  return func->decl.sig.result_types.size();
}

static WABT_INLINE size_t get_num_locals(const Func* func) {
  return func->local_types.size();
}

static WABT_INLINE size_t get_num_params_and_locals(const Func* func) {
  return get_num_params(func) + get_num_locals(func);
}

static WABT_INLINE Type get_param_type(const Func* func, int index) {
  assert(static_cast<size_t>(index) < func->decl.sig.param_types.size());
  return func->decl.sig.param_types[index];
}

static WABT_INLINE Type get_local_type(const Func* func, int index) {
  assert(static_cast<size_t>(index) < get_num_locals(func));
  return func->local_types[index];
}

static WABT_INLINE Type get_result_type(const Func* func, int index) {
  assert(static_cast<size_t>(index) < func->decl.sig.result_types.size());
  return func->decl.sig.result_types[index];
}

static WABT_INLINE Type get_func_type_param_type(const FuncType* func_type,
                                                 int index) {
  return func_type->sig.param_types[index];
}

static WABT_INLINE size_t get_func_type_num_params(const FuncType* func_type) {
  return func_type->sig.param_types.size();
}

static WABT_INLINE Type get_func_type_result_type(const FuncType* func_type,
                                                  int index) {
  return func_type->sig.result_types[index];
}

static WABT_INLINE size_t get_func_type_num_results(const FuncType* func_type) {
  return func_type->sig.result_types.size();
}

static WABT_INLINE const Location* get_raw_module_location(
    const RawModule* raw) {
  switch (raw->type) {
    case RawModuleType::Binary:
      return &raw->binary.loc;
    case RawModuleType::Text:
      return &raw->text->loc;
    default:
      assert(0);
      return nullptr;
  }
}

}  // namespace wabt

#endif /* WABT_AST_H_ */
