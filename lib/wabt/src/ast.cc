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

#include "ast.h"

#include <assert.h>
#include <stddef.h>

namespace wabt {

int get_index_from_var(const BindingHash* hash, const Var* var) {
  if (var->type == VarType::Name)
    return find_binding_index_by_name(hash, &var->name);
  return static_cast<int>(var->index);
}

ExportPtr get_export_by_name(const Module* module, const StringSlice* name) {
  int index = find_binding_index_by_name(&module->export_bindings, name);
  if (index == -1)
    return nullptr;
  return module->exports.data[index];
}

int get_func_index_by_var(const Module* module, const Var* var) {
  return get_index_from_var(&module->func_bindings, var);
}

int get_global_index_by_var(const Module* module, const Var* var) {
  return get_index_from_var(&module->global_bindings, var);
}

int get_table_index_by_var(const Module* module, const Var* var) {
  return get_index_from_var(&module->table_bindings, var);
}

int get_memory_index_by_var(const Module* module, const Var* var) {
  return get_index_from_var(&module->memory_bindings, var);
}

int get_func_type_index_by_var(const Module* module, const Var* var) {
  return get_index_from_var(&module->func_type_bindings, var);
}

int get_local_index_by_var(const Func* func, const Var* var) {
  if (var->type == VarType::Index)
    return static_cast<int>(var->index);

  int result = find_binding_index_by_name(&func->param_bindings, &var->name);
  if (result != -1)
    return result;

  result = find_binding_index_by_name(&func->local_bindings, &var->name);
  if (result == -1)
    return result;

  /* the locals start after all the params */
  return func->decl.sig.param_types.size + result;
}

int get_module_index_by_var(const Script* script, const Var* var) {
  return get_index_from_var(&script->module_bindings, var);
}

FuncPtr get_func_by_var(const Module* module, const Var* var) {
  int index = get_index_from_var(&module->func_bindings, var);
  if (index < 0 || static_cast<size_t>(index) >= module->funcs.size)
    return nullptr;
  return module->funcs.data[index];
}

GlobalPtr get_global_by_var(const Module* module, const Var* var) {
  int index = get_index_from_var(&module->global_bindings, var);
  if (index < 0 || static_cast<size_t>(index) >= module->globals.size)
    return nullptr;
  return module->globals.data[index];
}

TablePtr get_table_by_var(const Module* module, const Var* var) {
  int index = get_index_from_var(&module->table_bindings, var);
  if (index < 0 || static_cast<size_t>(index) >= module->tables.size)
    return nullptr;
  return module->tables.data[index];
}

MemoryPtr get_memory_by_var(const Module* module, const Var* var) {
  int index = get_index_from_var(&module->memory_bindings, var);
  if (index < 0 || static_cast<size_t>(index) >= module->memories.size)
    return nullptr;
  return module->memories.data[index];
}

FuncTypePtr get_func_type_by_var(const Module* module, const Var* var) {
  int index = get_index_from_var(&module->func_type_bindings, var);
  if (index < 0 || static_cast<size_t>(index) >= module->func_types.size)
    return nullptr;
  return module->func_types.data[index];
}

int get_func_type_index_by_sig(const Module* module, const FuncSignature* sig) {
  size_t i;
  for (i = 0; i < module->func_types.size; ++i)
    if (signatures_are_equal(&module->func_types.data[i]->sig, sig))
      return i;
  return -1;
}

int get_func_type_index_by_decl(const Module* module,
                                const FuncDeclaration* decl) {
  if (decl_has_func_type(decl)) {
    return get_func_type_index_by_var(module, &decl->type_var);
  } else {
    return get_func_type_index_by_sig(module, &decl->sig);
  }
}

Module* get_first_module(const Script* script) {
  size_t i;
  for (i = 0; i < script->commands.size; ++i) {
    Command* command = &script->commands.data[i];
    if (command->type == CommandType::Module)
      return &command->module;
  }
  return nullptr;
}

Module* get_module_by_var(const Script* script, const Var* var) {
  int index = get_index_from_var(&script->module_bindings, var);
  if (index < 0 || static_cast<size_t>(index) >= script->commands.size)
    return nullptr;
  Command* command = &script->commands.data[index];
  assert(command->type == CommandType::Module);
  return &command->module;
}

void make_type_binding_reverse_mapping(const TypeVector* types,
                                       const BindingHash* bindings,
                                       StringSliceVector* out_reverse_mapping) {
  uint32_t num_names = types->size;
  reserve_string_slices(out_reverse_mapping, num_names);
  out_reverse_mapping->size = num_names;
  memset(out_reverse_mapping->data, 0, num_names * sizeof(StringSlice));

  /* map index to name */
  size_t i;
  for (i = 0; i < bindings->entries.capacity; ++i) {
    const BindingHashEntry* entry = &bindings->entries.data[i];
    if (hash_entry_is_free(entry))
      continue;

    uint32_t index = entry->binding.index;
    assert(index < out_reverse_mapping->size);
    out_reverse_mapping->data[index] = entry->binding.name;
  }
}

void find_duplicate_bindings(const BindingHash* bindings,
                             DuplicateBindingCallback callback,
                             void* user_data) {
  size_t i;
  for (i = 0; i < bindings->entries.capacity; ++i) {
    BindingHashEntry* entry = &bindings->entries.data[i];
    if (hash_entry_is_free(entry))
      continue;

    /* only follow the chain if this is the first entry in the chain */
    if (entry->prev)
      continue;

    BindingHashEntry* a = entry;
    for (; a; a = a->next) {
      BindingHashEntry* b = a->next;
      for (; b; b = b->next) {
        if (string_slices_are_equal(&a->binding.name, &b->binding.name))
          callback(a, b, user_data);
      }
    }
  }
}

ModuleField* append_module_field(Module* module) {
  ModuleField* result = new ModuleField();
  if (!module->first_field)
    module->first_field = result;
  else if (module->last_field)
    module->last_field->next = result;
  module->last_field = result;
  return result;
}

FuncType* append_implicit_func_type(Location* loc,
                                    Module* module,
                                    FuncSignature* sig) {
  ModuleField* field = append_module_field(module);
  field->loc = *loc;
  field->type = ModuleFieldType::FuncType;
  field->func_type.sig = *sig;

  FuncType* func_type_ptr = &field->func_type;
  append_func_type_ptr_value(&module->func_types, &func_type_ptr);
  return func_type_ptr;
}

#define FOREACH_EXPR_TYPE(V)                 \
  V(ExprType::Binary, binary)                \
  V(ExprType::Block, block)                  \
  V(ExprType::Br, br)                        \
  V(ExprType::BrIf, br_if)                   \
  V(ExprType::BrTable, br_table)             \
  V(ExprType::Call, call)                    \
  V(ExprType::CallIndirect, call_indirect)   \
  V(ExprType::Compare, compare)              \
  V(ExprType::Const, const)                  \
  V(ExprType::Convert, convert)              \
  V(ExprType::GetGlobal, get_global)         \
  V(ExprType::GetLocal, get_local)           \
  V(ExprType::If, if)                        \
  V(ExprType::Load, load)                    \
  V(ExprType::Loop, loop)                    \
  V(ExprType::SetGlobal, set_global)         \
  V(ExprType::SetLocal, set_local)           \
  V(ExprType::Store, store)                  \
  V(ExprType::TeeLocal, tee_local)           \
  V(ExprType::Unary, unary)                  \
  V(ExprType::CurrentMemory, current_memory) \
  V(ExprType::Drop, drop)                    \
  V(ExprType::GrowMemory, grow_memory)       \
  V(ExprType::Nop, nop)                      \
  V(ExprType::Return, return )               \
  V(ExprType::Select, select)                \
  V(ExprType::Unreachable, unreachable)

#define DEFINE_NEW_EXPR(type_, name) \
  Expr* new_##name##_expr(void) {    \
    Expr* result = new Expr();       \
    result->type = type_;            \
    return result;                   \
  }
FOREACH_EXPR_TYPE(DEFINE_NEW_EXPR)
#undef DEFINE_NEW_EXPR

void destroy_var(Var* var) {
  if (var->type == VarType::Name)
    destroy_string_slice(&var->name);
}

void destroy_var_vector_and_elements(VarVector* vars) {
  WABT_DESTROY_VECTOR_AND_ELEMENTS(*vars, var);
}

void destroy_func_signature(FuncSignature* sig) {
  destroy_type_vector(&sig->param_types);
  destroy_type_vector(&sig->result_types);
}

void destroy_expr_list(Expr* first) {
  Expr* expr = first;
  while (expr) {
    Expr* next = expr->next;
    destroy_expr(expr);
    expr = next;
  }
}

void destroy_block(Block* block) {
  destroy_string_slice(&block->label);
  destroy_type_vector(&block->sig);
  destroy_expr_list(block->first);
}

void destroy_expr(Expr* expr) {
  switch (expr->type) {
    case ExprType::Block:
      destroy_block(&expr->block);
      break;
    case ExprType::Br:
      destroy_var(&expr->br.var);
      break;
    case ExprType::BrIf:
      destroy_var(&expr->br_if.var);
      break;
    case ExprType::BrTable:
      WABT_DESTROY_VECTOR_AND_ELEMENTS(expr->br_table.targets, var);
      destroy_var(&expr->br_table.default_target);
      break;
    case ExprType::Call:
      destroy_var(&expr->call.var);
      break;
    case ExprType::CallIndirect:
      destroy_var(&expr->call_indirect.var);
      break;
    case ExprType::GetGlobal:
      destroy_var(&expr->get_global.var);
      break;
    case ExprType::GetLocal:
      destroy_var(&expr->get_local.var);
      break;
    case ExprType::If:
      destroy_block(&expr->if_.true_);
      destroy_expr_list(expr->if_.false_);
      break;
    case ExprType::Loop:
      destroy_block(&expr->loop);
      break;
    case ExprType::SetGlobal:
      destroy_var(&expr->set_global.var);
      break;
    case ExprType::SetLocal:
      destroy_var(&expr->set_local.var);
      break;
    case ExprType::TeeLocal:
      destroy_var(&expr->tee_local.var);
      break;

    case ExprType::Binary:
    case ExprType::Compare:
    case ExprType::Const:
    case ExprType::Convert:
    case ExprType::Drop:
    case ExprType::CurrentMemory:
    case ExprType::GrowMemory:
    case ExprType::Load:
    case ExprType::Nop:
    case ExprType::Return:
    case ExprType::Select:
    case ExprType::Store:
    case ExprType::Unary:
    case ExprType::Unreachable:
      break;
  }
  delete expr;
}

void destroy_func_declaration(FuncDeclaration* decl) {
  destroy_var(&decl->type_var);
  if (!(decl->flags & WABT_FUNC_DECLARATION_FLAG_SHARED_SIGNATURE))
    destroy_func_signature(&decl->sig);
}

void destroy_func(Func* func) {
  destroy_string_slice(&func->name);
  destroy_func_declaration(&func->decl);
  destroy_type_vector(&func->local_types);
  destroy_binding_hash(&func->param_bindings);
  destroy_binding_hash(&func->local_bindings);
  destroy_expr_list(func->first_expr);
}

void destroy_global(Global* global) {
  destroy_string_slice(&global->name);
  destroy_expr_list(global->init_expr);
}

void destroy_import(Import* import) {
  destroy_string_slice(&import->module_name);
  destroy_string_slice(&import->field_name);
  switch (import->kind) {
    case ExternalKind::Func:
      destroy_func(&import->func);
      break;
    case ExternalKind::Table:
      destroy_table(&import->table);
      break;
    case ExternalKind::Memory:
      destroy_memory(&import->memory);
      break;
    case ExternalKind::Global:
      destroy_global(&import->global);
      break;
  }
}

void destroy_export(Export* export_) {
  destroy_string_slice(&export_->name);
  destroy_var(&export_->var);
}

void destroy_func_type(FuncType* func_type) {
  destroy_string_slice(&func_type->name);
  destroy_func_signature(&func_type->sig);
}

void destroy_data_segment(DataSegment* data) {
  destroy_var(&data->memory_var);
  destroy_expr_list(data->offset);
  delete [] data->data;
}

void destroy_memory(Memory* memory) {
  destroy_string_slice(&memory->name);
}

void destroy_table(Table* table) {
  destroy_string_slice(&table->name);
}

static void destroy_module_field(ModuleField* field) {
  switch (field->type) {
    case ModuleFieldType::Func:
      destroy_func(&field->func);
      break;
    case ModuleFieldType::Global:
      destroy_global(&field->global);
      break;
    case ModuleFieldType::Import:
      destroy_import(&field->import);
      break;
    case ModuleFieldType::Export:
      destroy_export(&field->export_);
      break;
    case ModuleFieldType::FuncType:
      destroy_func_type(&field->func_type);
      break;
    case ModuleFieldType::Table:
      destroy_table(&field->table);
      break;
    case ModuleFieldType::ElemSegment:
      destroy_elem_segment(&field->elem_segment);
      break;
    case ModuleFieldType::Memory:
      destroy_memory(&field->memory);
      break;
    case ModuleFieldType::DataSegment:
      destroy_data_segment(&field->data_segment);
      break;
    case ModuleFieldType::Start:
      destroy_var(&field->start);
      break;
  }
}

void destroy_module(Module* module) {
  destroy_string_slice(&module->name);

  ModuleField* field = module->first_field;
  while (field) {
    ModuleField* next_field = field->next;
    destroy_module_field(field);
    delete field;
    field = next_field;
  }

  /* everything that follows shares data with the module fields above, so we
   only need to destroy the containing vectors */
  destroy_func_ptr_vector(&module->funcs);
  destroy_global_ptr_vector(&module->globals);
  destroy_import_ptr_vector(&module->imports);
  destroy_export_ptr_vector(&module->exports);
  destroy_func_type_ptr_vector(&module->func_types);
  destroy_table_ptr_vector(&module->tables);
  destroy_elem_segment_ptr_vector(&module->elem_segments);
  destroy_memory_ptr_vector(&module->memories);
  destroy_data_segment_ptr_vector(&module->data_segments);
  destroy_binding_hash_entry_vector(&module->func_bindings.entries);
  destroy_binding_hash_entry_vector(&module->global_bindings.entries);
  destroy_binding_hash_entry_vector(&module->export_bindings.entries);
  destroy_binding_hash_entry_vector(&module->func_type_bindings.entries);
  destroy_binding_hash_entry_vector(&module->table_bindings.entries);
  destroy_binding_hash_entry_vector(&module->memory_bindings.entries);
}

void destroy_raw_module(RawModule* raw) {
  if (raw->type == RawModuleType::Text) {
    destroy_module(raw->text);
    delete raw->text;
  } else {
    destroy_string_slice(&raw->binary.name);
    delete [] raw->binary.data;
  }
}

void destroy_action(Action* action) {
  destroy_var(&action->module_var);
  switch (action->type) {
    case ActionType::Invoke:
      destroy_string_slice(&action->invoke.name);
      destroy_const_vector(&action->invoke.args);
      break;
    case ActionType::Get:
      destroy_string_slice(&action->get.name);
      break;
  }
}

void destroy_command(Command* command) {
  switch (command->type) {
    case CommandType::Module:
      destroy_module(&command->module);
      break;
    case CommandType::Action:
      destroy_action(&command->action);
      break;
    case CommandType::Register:
      destroy_string_slice(&command->register_.module_name);
      destroy_var(&command->register_.var);
      break;
    case CommandType::AssertMalformed:
      destroy_raw_module(&command->assert_malformed.module);
      destroy_string_slice(&command->assert_malformed.text);
      break;
    case CommandType::AssertInvalid:
    case CommandType::AssertInvalidNonBinary:
      destroy_raw_module(&command->assert_invalid.module);
      destroy_string_slice(&command->assert_invalid.text);
      break;
    case CommandType::AssertUnlinkable:
      destroy_raw_module(&command->assert_unlinkable.module);
      destroy_string_slice(&command->assert_unlinkable.text);
      break;
    case CommandType::AssertUninstantiable:
      destroy_raw_module(&command->assert_uninstantiable.module);
      destroy_string_slice(&command->assert_uninstantiable.text);
      break;
    case CommandType::AssertReturn:
      destroy_action(&command->assert_return.action);
      destroy_const_vector(&command->assert_return.expected);
      break;
    case CommandType::AssertReturnNan:
      destroy_action(&command->assert_return_nan.action);
      break;
    case CommandType::AssertTrap:
    case CommandType::AssertExhaustion:
      destroy_action(&command->assert_trap.action);
      destroy_string_slice(&command->assert_trap.text);
      break;
  }
}

void destroy_command_vector_and_elements(CommandVector* commands) {
  WABT_DESTROY_VECTOR_AND_ELEMENTS(*commands, command);
}

void destroy_elem_segment(ElemSegment* elem) {
  destroy_var(&elem->table_var);
  destroy_expr_list(elem->offset);
  WABT_DESTROY_VECTOR_AND_ELEMENTS(elem->vars, var);
}

void destroy_script(Script* script) {
  WABT_DESTROY_VECTOR_AND_ELEMENTS(script->commands, command);
  destroy_binding_hash(&script->module_bindings);
}

#define CHECK_RESULT(expr)   \
  do {                       \
    if (WABT_FAILED((expr))) \
      return Result::Error;  \
  } while (0)

#define CALLBACK(member)                                           \
  CHECK_RESULT((visitor)->member                                   \
                   ? (visitor)->member(expr, (visitor)->user_data) \
                   : Result::Ok)

static Result visit_expr(Expr* expr, ExprVisitor* visitor);

Result visit_expr_list(Expr* first, ExprVisitor* visitor) {
  Expr* expr;
  for (expr = first; expr; expr = expr->next)
    CHECK_RESULT(visit_expr(expr, visitor));
  return Result::Ok;
}

static Result visit_expr(Expr* expr, ExprVisitor* visitor) {
  switch (expr->type) {
    case ExprType::Binary:
      CALLBACK(on_binary_expr);
      break;

    case ExprType::Block:
      CALLBACK(begin_block_expr);
      CHECK_RESULT(visit_expr_list(expr->block.first, visitor));
      CALLBACK(end_block_expr);
      break;

    case ExprType::Br:
      CALLBACK(on_br_expr);
      break;

    case ExprType::BrIf:
      CALLBACK(on_br_if_expr);
      break;

    case ExprType::BrTable:
      CALLBACK(on_br_table_expr);
      break;

    case ExprType::Call:
      CALLBACK(on_call_expr);
      break;

    case ExprType::CallIndirect:
      CALLBACK(on_call_indirect_expr);
      break;

    case ExprType::Compare:
      CALLBACK(on_compare_expr);
      break;

    case ExprType::Const:
      CALLBACK(on_const_expr);
      break;

    case ExprType::Convert:
      CALLBACK(on_convert_expr);
      break;

    case ExprType::CurrentMemory:
      CALLBACK(on_current_memory_expr);
      break;

    case ExprType::Drop:
      CALLBACK(on_drop_expr);
      break;

    case ExprType::GetGlobal:
      CALLBACK(on_get_global_expr);
      break;

    case ExprType::GetLocal:
      CALLBACK(on_get_local_expr);
      break;

    case ExprType::GrowMemory:
      CALLBACK(on_grow_memory_expr);
      break;

    case ExprType::If:
      CALLBACK(begin_if_expr);
      CHECK_RESULT(visit_expr_list(expr->if_.true_.first, visitor));
      CALLBACK(after_if_true_expr);
      CHECK_RESULT(visit_expr_list(expr->if_.false_, visitor));
      CALLBACK(end_if_expr);
      break;

    case ExprType::Load:
      CALLBACK(on_load_expr);
      break;

    case ExprType::Loop:
      CALLBACK(begin_loop_expr);
      CHECK_RESULT(visit_expr_list(expr->loop.first, visitor));
      CALLBACK(end_loop_expr);
      break;

    case ExprType::Nop:
      CALLBACK(on_nop_expr);
      break;

    case ExprType::Return:
      CALLBACK(on_return_expr);
      break;

    case ExprType::Select:
      CALLBACK(on_select_expr);
      break;

    case ExprType::SetGlobal:
      CALLBACK(on_set_global_expr);
      break;

    case ExprType::SetLocal:
      CALLBACK(on_set_local_expr);
      break;

    case ExprType::Store:
      CALLBACK(on_store_expr);
      break;

    case ExprType::TeeLocal:
      CALLBACK(on_tee_local_expr);
      break;

    case ExprType::Unary:
      CALLBACK(on_unary_expr);
      break;

    case ExprType::Unreachable:
      CALLBACK(on_unreachable_expr);
      break;
  }

  return Result::Ok;
}

/* TODO(binji): make the visitor non-recursive */
Result visit_func(Func* func, ExprVisitor* visitor) {
  return visit_expr_list(func->first_expr, visitor);
}

}  // namespace wabt
