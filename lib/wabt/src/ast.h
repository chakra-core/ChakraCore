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

#include "binding-hash.h"
#include "common.h"
#include "type-vector.h"
#include "vector.h"

namespace wabt {

enum class VarType {
  Index,
  Name,
};

struct Var {
  Location loc;
  VarType type;
  union {
    int64_t index;
    StringSlice name;
  };
};
WABT_DEFINE_VECTOR(var, Var);

typedef StringSlice Label;
WABT_DEFINE_VECTOR(string_slice, StringSlice);

struct Const {
  Location loc;
  Type type;
  union {
    uint32_t u32;
    uint64_t u64;
    uint32_t f32_bits;
    uint64_t f64_bits;
  };
};
WABT_DEFINE_VECTOR(const, Const);

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
  Label label;
  BlockSignature sig;
  struct Expr* first;
};

struct Expr {
  Location loc;
  ExprType type;
  Expr* next;
  union {
    struct { Opcode opcode; } binary, compare, convert, unary;
    Block block, loop;
    struct { Var var; } br, br_if;
    struct { VarVector targets; Var default_target; } br_table;
    struct { Var var; } call, call_indirect;
    Const const_;
    struct { Var var; } get_global, set_global;
    struct { Var var; } get_local, set_local, tee_local;
    struct { Block true_; struct Expr* false_; } if_;
    struct { Opcode opcode; uint32_t align; uint64_t offset; } load, store;
  };
};

struct FuncSignature {
  TypeVector param_types;
  TypeVector result_types;
};

struct FuncType {
  StringSlice name;
  FuncSignature sig;
};
typedef FuncType* FuncTypePtr;
WABT_DEFINE_VECTOR(func_type_ptr, FuncTypePtr);

enum {
  WABT_FUNC_DECLARATION_FLAG_HAS_FUNC_TYPE = 1,
  /* set if the signature is owned by module */
  WABT_FUNC_DECLARATION_FLAG_SHARED_SIGNATURE = 2,
};
typedef uint32_t FuncDeclarationFlags;

struct FuncDeclaration {
  FuncDeclarationFlags flags;
  Var type_var;
  FuncSignature sig;
};

struct Func {
  StringSlice name;
  FuncDeclaration decl;
  TypeVector local_types;
  BindingHash param_bindings;
  BindingHash local_bindings;
  Expr* first_expr;
};
typedef Func* FuncPtr;
WABT_DEFINE_VECTOR(func_ptr, FuncPtr);

struct Global {
  StringSlice name;
  Type type;
  bool mutable_;
  Expr* init_expr;
};
typedef Global* GlobalPtr;
WABT_DEFINE_VECTOR(global_ptr, GlobalPtr);

struct Table {
  StringSlice name;
  Limits elem_limits;
};
typedef Table* TablePtr;
WABT_DEFINE_VECTOR(table_ptr, TablePtr);

struct ElemSegment {
  Var table_var;
  Expr* offset;
  VarVector vars;
};
typedef ElemSegment* ElemSegmentPtr;
WABT_DEFINE_VECTOR(elem_segment_ptr, ElemSegmentPtr);

struct Memory {
  StringSlice name;
  Limits page_limits;
};
typedef Memory* MemoryPtr;
WABT_DEFINE_VECTOR(memory_ptr, MemoryPtr);

struct DataSegment {
  Var memory_var;
  Expr* offset;
  char* data;
  size_t size;
};
typedef DataSegment* DataSegmentPtr;
WABT_DEFINE_VECTOR(data_segment_ptr, DataSegmentPtr);

struct Import {
  StringSlice module_name;
  StringSlice field_name;
  ExternalKind kind;
  union {
    /* an imported func is has the type Func so it can be more easily
     * included in the Module's vector of funcs; but only the
     * FuncDeclaration will have any useful information */
    Func func;
    Table table;
    Memory memory;
    Global global;
  };
};
typedef Import* ImportPtr;
WABT_DEFINE_VECTOR(import_ptr, ImportPtr);

struct Export {
  StringSlice name;
  ExternalKind kind;
  Var var;
};
typedef Export* ExportPtr;
WABT_DEFINE_VECTOR(export_ptr, ExportPtr);

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
  Location loc;
  ModuleFieldType type;
  struct ModuleField* next;
  union {
    Func func;
    Global global;
    Import import;
    Export export_;
    FuncType func_type;
    Table table;
    ElemSegment elem_segment;
    Memory memory;
    DataSegment data_segment;
    Var start;
  };
};

struct Module {
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
  FuncPtrVector funcs;
  GlobalPtrVector globals;
  ImportPtrVector imports;
  ExportPtrVector exports;
  FuncTypePtrVector func_types;
  TablePtrVector tables;
  ElemSegmentPtrVector elem_segments;
  MemoryPtrVector memories;
  DataSegmentPtrVector data_segments;
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
  StringSlice name;
  ConstVector args;
};

struct ActionGet {
  StringSlice name;
};

struct Action {
  Location loc;
  ActionType type;
  Var module_var;
  union {
    ActionInvoke invoke;
    ActionGet get;
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
  AssertReturnNan,
  AssertTrap,
  AssertExhaustion,

  First = Module,
  Last = AssertExhaustion,
};
static const int kCommandTypeCount = WABT_ENUM_COUNT(CommandType);

struct Command {
  CommandType type;
  union {
    Module module;
    Action action;
    struct { StringSlice module_name; Var var; } register_;
    struct { Action action; ConstVector expected; } assert_return;
    struct { Action action; } assert_return_nan;
    struct { Action action; StringSlice text; } assert_trap;
    struct {
      RawModule module;
      StringSlice text;
    } assert_malformed, assert_invalid, assert_unlinkable,
        assert_uninstantiable;
  };
};
WABT_DEFINE_VECTOR(command, Command);

struct Script {
  CommandVector commands;
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

/* Expr creation functions */
Expr* new_binary_expr(void);
Expr* new_block_expr(void);
Expr* new_br_expr(void);
Expr* new_br_if_expr(void);
Expr* new_br_table_expr(void);
Expr* new_call_expr(void);
Expr* new_call_indirect_expr(void);
Expr* new_compare_expr(void);
Expr* new_const_expr(void);
Expr* new_convert_expr(void);
Expr* new_current_memory_expr(void);
Expr* new_drop_expr(void);
Expr* new_get_global_expr(void);
Expr* new_get_local_expr(void);
Expr* new_grow_memory_expr(void);
Expr* new_if_expr(void);
Expr* new_load_expr(void);
Expr* new_loop_expr(void);
Expr* new_nop_expr(void);
Expr* new_return_expr(void);
Expr* new_select_expr(void);
Expr* new_set_global_expr(void);
Expr* new_set_local_expr(void);
Expr* new_store_expr(void);
Expr* new_tee_local_expr(void);
Expr* new_unary_expr(void);
Expr* new_unreachable_expr(void);

/* destruction functions. not needed unless you're creating your own AST
 elements */
void destroy_script(struct Script*);
void destroy_action(struct Action*);
void destroy_block(struct Block*);
void destroy_command_vector_and_elements(CommandVector*);
void destroy_command(Command*);
void destroy_data_segment(DataSegment*);
void destroy_elem_segment(ElemSegment*);
void destroy_export(Export*);
void destroy_expr(Expr*);
void destroy_expr_list(Expr*);
void destroy_func_declaration(FuncDeclaration*);
void destroy_func_signature(FuncSignature*);
void destroy_func_type(FuncType*);
void destroy_func(Func*);
void destroy_import(Import*);
void destroy_memory(Memory*);
void destroy_module(Module*);
void destroy_raw_module(RawModule*);
void destroy_table(Table*);
void destroy_var_vector_and_elements(VarVector*);
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

FuncPtr get_func_by_var(const Module* module, const Var* var);
GlobalPtr get_global_by_var(const Module* func, const Var* var);
FuncTypePtr get_func_type_by_var(const Module* module, const Var* var);
TablePtr get_table_by_var(const Module* module, const Var* var);
MemoryPtr get_memory_by_var(const Module* module, const Var* var);
ImportPtr get_import_by_var(const Module* module, const Var* var);
ExportPtr get_export_by_name(const Module* module, const StringSlice* name);
Module* get_first_module(const Script* script);
Module* get_module_by_var(const Script* script, const Var* var);

void make_type_binding_reverse_mapping(const TypeVector*,
                                       const BindingHash*,
                                       StringSliceVector* out_reverse_mapping);

typedef void (*DuplicateBindingCallback)(BindingHashEntry* a,
                                         BindingHashEntry* b,
                                         void* user_data);
void find_duplicate_bindings(const BindingHash*,
                             DuplicateBindingCallback callback,
                             void* user_data);

static WABT_INLINE bool decl_has_func_type(const FuncDeclaration* decl) {
  return static_cast<bool>(
      (decl->flags & WABT_FUNC_DECLARATION_FLAG_HAS_FUNC_TYPE) != 0);
}

static WABT_INLINE bool signatures_are_equal(const FuncSignature* sig1,
                                             const FuncSignature* sig2) {
  return static_cast<bool>(
      type_vectors_are_equal(&sig1->param_types, &sig2->param_types) &&
      type_vectors_are_equal(&sig1->result_types, &sig2->result_types));
}

static WABT_INLINE size_t get_num_params(const Func* func) {
  return func->decl.sig.param_types.size;
}

static WABT_INLINE size_t get_num_results(const Func* func) {
  return func->decl.sig.result_types.size;
}

static WABT_INLINE size_t get_num_locals(const Func* func) {
  return func->local_types.size;
}

static WABT_INLINE size_t get_num_params_and_locals(const Func* func) {
  return get_num_params(func) + get_num_locals(func);
}

static WABT_INLINE Type get_param_type(const Func* func, int index) {
  assert(static_cast<size_t>(index) < func->decl.sig.param_types.size);
  return func->decl.sig.param_types.data[index];
}

static WABT_INLINE Type get_local_type(const Func* func, int index) {
  assert(static_cast<size_t>(index) < get_num_locals(func));
  return func->local_types.data[index];
}

static WABT_INLINE Type get_result_type(const Func* func, int index) {
  assert(static_cast<size_t>(index) < func->decl.sig.result_types.size);
  return func->decl.sig.result_types.data[index];
}

static WABT_INLINE Type get_func_type_param_type(const FuncType* func_type,
                                                 int index) {
  return func_type->sig.param_types.data[index];
}

static WABT_INLINE size_t get_func_type_num_params(const FuncType* func_type) {
  return func_type->sig.param_types.size;
}

static WABT_INLINE Type get_func_type_result_type(const FuncType* func_type,
                                                  int index) {
  return func_type->sig.result_types.data[index];
}

static WABT_INLINE size_t get_func_type_num_results(const FuncType* func_type) {
  return func_type->sig.result_types.size;
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
