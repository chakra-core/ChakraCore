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

#include "binary-reader-ast.h"

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include "ast.h"
#include "binary-reader.h"
#include "common.h"

#define CHECK_RESULT(expr) \
  do {                     \
    if (WABT_FAILED(expr)) \
      return Result::Error;   \
  } while (0)

namespace wabt {

struct LabelNode {
  LabelType label_type;
  Expr** first;
  Expr* last;
};
WABT_DEFINE_VECTOR(label_node, LabelNode);

struct Context {
  BinaryErrorHandler* error_handler;
  Module* module;

  Func* current_func;
  LabelNodeVector label_stack;
  uint32_t max_depth;
  Expr** current_init_expr;
};

static void handle_error(Context* ctx, uint32_t offset, const char* message);

static void WABT_PRINTF_FORMAT(2, 3)
    print_error(Context* ctx, const char* format, ...) {
  WABT_SNPRINTF_ALLOCA(buffer, length, format);
  handle_error(ctx, WABT_UNKNOWN_OFFSET, buffer);
}

static void push_label(Context* ctx, LabelType label_type, Expr** first) {
  LabelNode label;
  label.label_type = label_type;
  label.first = first;
  label.last = nullptr;
  ctx->max_depth++;
  append_label_node_value(&ctx->label_stack, &label);
}

static Result pop_label(Context* ctx) {
  if (ctx->label_stack.size == 0) {
    print_error(ctx, "popping empty label stack");
    return Result::Error;
  }

  ctx->max_depth--;
  ctx->label_stack.size--;
  return Result::Ok;
}

static Result get_label_at(Context* ctx, LabelNode** label, uint32_t depth) {
  if (depth >= ctx->label_stack.size) {
    print_error(ctx, "accessing stack depth: %u >= max: %" PRIzd, depth,
                ctx->label_stack.size);
    return Result::Error;
  }

  *label = &ctx->label_stack.data[ctx->label_stack.size - depth - 1];
  return Result::Ok;
}

static Result top_label(Context* ctx, LabelNode** label) {
  return get_label_at(ctx, label, 0);
}

static void dup_name(Context* ctx, StringSlice* name, StringSlice* out_name) {
  if (name->length > 0) {
    *out_name = dup_string_slice(*name);
  } else {
    WABT_ZERO_MEMORY(*out_name);
  }
}

static Result append_expr(Context* ctx, Expr* expr) {
  LabelNode* label;
  if (WABT_FAILED(top_label(ctx, &label))) {
    delete expr;
    return Result::Error;
  }
  if (*label->first) {
    label->last->next = expr;
    label->last = expr;
  } else {
    *label->first = label->last = expr;
  }
  return Result::Ok;
}

static void handle_error(Context* ctx, uint32_t offset, const char* message) {
  if (ctx->error_handler->on_error) {
    ctx->error_handler->on_error(offset, message,
                                 ctx->error_handler->user_data);
  }
}

static void on_error(BinaryReaderContext* reader_context, const char* message) {
  Context* ctx = static_cast<Context*>(reader_context->user_data);
  handle_error(ctx, reader_context->offset, message);
}

static Result on_signature_count(uint32_t count, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  reserve_func_type_ptrs(&ctx->module->func_types, count);
  return Result::Ok;
}

static Result on_signature(uint32_t index,
                           uint32_t param_count,
                           Type* param_types,
                           uint32_t result_count,
                           Type* result_types,
                           void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  ModuleField* field = append_module_field(ctx->module);
  field->type = ModuleFieldType::FuncType;

  FuncType* func_type = &field->func_type;
  WABT_ZERO_MEMORY(*func_type);

  reserve_types(&func_type->sig.param_types, param_count);
  func_type->sig.param_types.size = param_count;
  memcpy(func_type->sig.param_types.data, param_types,
         param_count * sizeof(Type));

  reserve_types(&func_type->sig.result_types, result_count);
  func_type->sig.result_types.size = result_count;
  memcpy(func_type->sig.result_types.data, result_types,
         result_count * sizeof(Type));

  assert(index < ctx->module->func_types.capacity);
  FuncTypePtr* func_type_ptr = append_func_type_ptr(&ctx->module->func_types);
  *func_type_ptr = func_type;
  return Result::Ok;
}

static Result on_import_count(uint32_t count, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  reserve_import_ptrs(&ctx->module->imports, count);
  return Result::Ok;
}

static Result on_import(uint32_t index,
                        StringSlice module_name,
                        StringSlice field_name,
                        void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  assert(index < ctx->module->imports.capacity);

  ModuleField* field = append_module_field(ctx->module);
  field->type = ModuleFieldType::Import;

  Import* import = &field->import;
  WABT_ZERO_MEMORY(*import);
  import->module_name = dup_string_slice(module_name);
  import->field_name = dup_string_slice(field_name);

  ImportPtr* import_ptr = append_import_ptr(&ctx->module->imports);
  *import_ptr = import;
  return Result::Ok;
}

static Result on_import_func(uint32_t import_index,
                             uint32_t func_index,
                             uint32_t sig_index,
                             void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  assert(import_index == ctx->module->imports.size - 1);
  assert(sig_index < ctx->module->func_types.size);
  Import* import = ctx->module->imports.data[import_index];

  import->kind = ExternalKind::Func;
  import->func.decl.flags = WABT_FUNC_DECLARATION_FLAG_HAS_FUNC_TYPE |
                            WABT_FUNC_DECLARATION_FLAG_SHARED_SIGNATURE;
  import->func.decl.type_var.type = VarType::Index;
  import->func.decl.type_var.index = sig_index;
  import->func.decl.sig = ctx->module->func_types.data[sig_index]->sig;

  FuncPtr func_ptr = &import->func;
  append_func_ptr_value(&ctx->module->funcs, &func_ptr);
  ctx->module->num_func_imports++;
  return Result::Ok;
}

static Result on_import_table(uint32_t import_index,
                              uint32_t table_index,
                              Type elem_type,
                              const Limits* elem_limits,
                              void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  assert(import_index == ctx->module->imports.size - 1);
  Import* import = ctx->module->imports.data[import_index];
  import->kind = ExternalKind::Table;
  import->table.elem_limits = *elem_limits;

  TablePtr table_ptr = &import->table;
  append_table_ptr_value(&ctx->module->tables, &table_ptr);
  ctx->module->num_table_imports++;
  return Result::Ok;
}

static Result on_import_memory(uint32_t import_index,
                               uint32_t memory_index,
                               const Limits* page_limits,
                               void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  assert(import_index == ctx->module->imports.size - 1);
  Import* import = ctx->module->imports.data[import_index];
  import->kind = ExternalKind::Memory;
  import->memory.page_limits = *page_limits;

  MemoryPtr memory_ptr = &import->memory;
  append_memory_ptr_value(&ctx->module->memories, &memory_ptr);
  ctx->module->num_memory_imports++;
  return Result::Ok;
}

static Result on_import_global(uint32_t import_index,
                               uint32_t global_index,
                               Type type,
                               bool mutable_,
                               void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  assert(import_index == ctx->module->imports.size - 1);
  Import* import = ctx->module->imports.data[import_index];
  import->kind = ExternalKind::Global;
  import->global.type = type;
  import->global.mutable_ = mutable_;

  GlobalPtr global_ptr = &import->global;
  append_global_ptr_value(&ctx->module->globals, &global_ptr);
  ctx->module->num_global_imports++;
  return Result::Ok;
}

static Result on_function_signatures_count(uint32_t count, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  reserve_func_ptrs(&ctx->module->funcs, ctx->module->num_func_imports + count);
  return Result::Ok;
}

static Result on_function_signature(uint32_t index,
                                    uint32_t sig_index,
                                    void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  assert(index < ctx->module->funcs.capacity);
  assert(sig_index < ctx->module->func_types.size);

  ModuleField* field = append_module_field(ctx->module);
  field->type = ModuleFieldType::Func;

  Func* func = &field->func;
  WABT_ZERO_MEMORY(*func);
  func->decl.flags = WABT_FUNC_DECLARATION_FLAG_HAS_FUNC_TYPE |
                     WABT_FUNC_DECLARATION_FLAG_SHARED_SIGNATURE;
  func->decl.type_var.type = VarType::Index;
  func->decl.type_var.index = sig_index;
  func->decl.sig = ctx->module->func_types.data[sig_index]->sig;

  FuncPtr* func_ptr = append_func_ptr(&ctx->module->funcs);
  *func_ptr = func;
  return Result::Ok;
}

static Result on_table_count(uint32_t count, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  reserve_table_ptrs(&ctx->module->tables,
                     ctx->module->num_table_imports + count);
  return Result::Ok;
}

static Result on_table(uint32_t index,
                       Type elem_type,
                       const Limits* elem_limits,
                       void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  assert(index < ctx->module->tables.capacity);

  ModuleField* field = append_module_field(ctx->module);
  field->type = ModuleFieldType::Table;

  Table* table = &field->table;
  WABT_ZERO_MEMORY(*table);
  table->elem_limits = *elem_limits;

  TablePtr* table_ptr = append_table_ptr(&ctx->module->tables);
  *table_ptr = table;
  return Result::Ok;
}

static Result on_memory_count(uint32_t count, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  reserve_memory_ptrs(&ctx->module->memories,
                      ctx->module->num_memory_imports + count);
  return Result::Ok;
}

static Result on_memory(uint32_t index,
                        const Limits* page_limits,
                        void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  assert(index < ctx->module->memories.capacity);

  ModuleField* field = append_module_field(ctx->module);
  field->type = ModuleFieldType::Memory;

  Memory* memory = &field->memory;
  WABT_ZERO_MEMORY(*memory);
  memory->page_limits = *page_limits;

  MemoryPtr* memory_ptr = append_memory_ptr(&ctx->module->memories);
  *memory_ptr = memory;
  return Result::Ok;
}

static Result on_global_count(uint32_t count, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  reserve_global_ptrs(&ctx->module->globals,
                      ctx->module->num_global_imports + count);
  return Result::Ok;
}

static Result begin_global(uint32_t index,
                           Type type,
                           bool mutable_,
                           void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  assert(index < ctx->module->globals.capacity);

  ModuleField* field = append_module_field(ctx->module);
  field->type = ModuleFieldType::Global;

  Global* global = &field->global;
  WABT_ZERO_MEMORY(*global);
  global->type = type;
  global->mutable_ = mutable_;

  GlobalPtr* global_ptr = append_global_ptr(&ctx->module->globals);
  *global_ptr = global;
  return Result::Ok;
}

static Result begin_global_init_expr(uint32_t index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  assert(index == ctx->module->globals.size - 1);
  Global* global = ctx->module->globals.data[index];
  ctx->current_init_expr = &global->init_expr;
  return Result::Ok;
}

static Result end_global_init_expr(uint32_t index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  ctx->current_init_expr = nullptr;
  return Result::Ok;
}

static Result on_export_count(uint32_t count, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  reserve_export_ptrs(&ctx->module->exports, count);
  return Result::Ok;
}

static Result on_export(uint32_t index,
                        ExternalKind kind,
                        uint32_t item_index,
                        StringSlice name,
                        void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  ModuleField* field = append_module_field(ctx->module);
  field->type = ModuleFieldType::Export;

  Export* export_ = &field->export_;
  WABT_ZERO_MEMORY(*export_);
  export_->name = dup_string_slice(name);
  switch (kind) {
    case ExternalKind::Func:
      assert(item_index < ctx->module->funcs.size);
      break;
    case ExternalKind::Table:
      assert(item_index < ctx->module->tables.size);
      break;
    case ExternalKind::Memory:
      assert(item_index < ctx->module->memories.size);
      break;
    case ExternalKind::Global:
      assert(item_index < ctx->module->globals.size);
      break;
  }
  export_->var.type = VarType::Index;
  export_->var.index = item_index;
  export_->kind = kind;

  assert(index < ctx->module->exports.capacity);
  ExportPtr* export_ptr = append_export_ptr(&ctx->module->exports);
  *export_ptr = export_;
  return Result::Ok;
}

static Result on_start_function(uint32_t func_index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  ModuleField* field = append_module_field(ctx->module);
  field->type = ModuleFieldType::Start;

  field->start.type = VarType::Index;
  assert(func_index < ctx->module->funcs.size);
  field->start.index = func_index;

  ctx->module->start = &field->start;
  return Result::Ok;
}

static Result on_function_bodies_count(uint32_t count, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  assert(ctx->module->num_func_imports + count == ctx->module->funcs.size);
  WABT_USE(ctx);
  return Result::Ok;
}

static Result begin_function_body(BinaryReaderContext* context,
                                  uint32_t index) {
  Context* ctx = static_cast<Context*>(context->user_data);
  assert(index < ctx->module->funcs.size);
  ctx->current_func = ctx->module->funcs.data[index];
  push_label(ctx, LabelType::Func, &ctx->current_func->first_expr);
  return Result::Ok;
}

static Result on_local_decl(uint32_t decl_index,
                            uint32_t count,
                            Type type,
                            void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  size_t old_local_count = ctx->current_func->local_types.size;
  size_t new_local_count = old_local_count + count;
  reserve_types(&ctx->current_func->local_types, new_local_count);
  TypeVector* types = &ctx->current_func->local_types;
  size_t i;
  for (i = 0; i < count; ++i)
    types->data[old_local_count + i] = type;
  types->size = new_local_count;
  return Result::Ok;
}

static Result on_binary_expr(Opcode opcode, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_binary_expr();
  expr->binary.opcode = opcode;
  return append_expr(ctx, expr);
}

static Result on_block_expr(uint32_t num_types,
                            Type* sig_types,
                            void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_block_expr();
  TypeVector src;
  WABT_ZERO_MEMORY(src);
  src.size = num_types;
  src.data = sig_types;
  extend_types(&expr->block.sig, &src);
  append_expr(ctx, expr);
  push_label(ctx, LabelType::Block, &expr->block.first);
  return Result::Ok;
}

static Result on_br_expr(uint32_t depth, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_br_expr();
  expr->br.var.type = VarType::Index;
  expr->br.var.index = depth;
  return append_expr(ctx, expr);
}

static Result on_br_if_expr(uint32_t depth, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_br_if_expr();
  expr->br_if.var.type = VarType::Index;
  expr->br_if.var.index = depth;
  return append_expr(ctx, expr);
}

static Result on_br_table_expr(BinaryReaderContext* context,
                               uint32_t num_targets,
                               uint32_t* target_depths,
                               uint32_t default_target_depth) {
  Context* ctx = static_cast<Context*>(context->user_data);
  Expr* expr = new_br_table_expr();
  reserve_vars(&expr->br_table.targets, num_targets);
  expr->br_table.targets.size = num_targets;
  uint32_t i;
  for (i = 0; i < num_targets; ++i) {
    Var* var = &expr->br_table.targets.data[i];
    var->type = VarType::Index;
    var->index = target_depths[i];
  }
  expr->br_table.default_target.type = VarType::Index;
  expr->br_table.default_target.index = default_target_depth;
  return append_expr(ctx, expr);
}

static Result on_call_expr(uint32_t func_index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  assert(func_index < ctx->module->funcs.size);
  Expr* expr = new_call_expr();
  expr->call.var.type = VarType::Index;
  expr->call.var.index = func_index;
  return append_expr(ctx, expr);
}

static Result on_call_indirect_expr(uint32_t sig_index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  assert(sig_index < ctx->module->func_types.size);
  Expr* expr = new_call_indirect_expr();
  expr->call_indirect.var.type = VarType::Index;
  expr->call_indirect.var.index = sig_index;
  return append_expr(ctx, expr);
}

static Result on_compare_expr(Opcode opcode, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_compare_expr();
  expr->compare.opcode = opcode;
  return append_expr(ctx, expr);
}

static Result on_convert_expr(Opcode opcode, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_convert_expr();
  expr->convert.opcode = opcode;
  return append_expr(ctx, expr);
}

static Result on_current_memory_expr(void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_current_memory_expr();
  return append_expr(ctx, expr);
}

static Result on_drop_expr(void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_drop_expr();
  return append_expr(ctx, expr);
}

static Result on_else_expr(void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  LabelNode* label;
  CHECK_RESULT(top_label(ctx, &label));
  if (label->label_type != LabelType::If) {
    print_error(ctx, "else expression without matching if");
    return Result::Error;
  }

  LabelNode* parent_label;
  CHECK_RESULT(get_label_at(ctx, &parent_label, 1));
  assert(parent_label->last->type == ExprType::If);

  label->label_type = LabelType::Else;
  label->first = &parent_label->last->if_.false_;
  label->last = nullptr;
  return Result::Ok;
}

static Result on_end_expr(void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  return pop_label(ctx);
}

static Result on_f32_const_expr(uint32_t value_bits, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_const_expr();
  expr->const_.type = Type::F32;
  expr->const_.f32_bits = value_bits;
  return append_expr(ctx, expr);
}

static Result on_f64_const_expr(uint64_t value_bits, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_const_expr();
  expr->const_.type = Type::F64;
  expr->const_.f64_bits = value_bits;
  return append_expr(ctx, expr);
}

static Result on_get_global_expr(uint32_t global_index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_get_global_expr();
  expr->get_global.var.type = VarType::Index;
  expr->get_global.var.index = global_index;
  return append_expr(ctx, expr);
}

static Result on_get_local_expr(uint32_t local_index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_get_local_expr();
  expr->get_local.var.type = VarType::Index;
  expr->get_local.var.index = local_index;
  return append_expr(ctx, expr);
}

static Result on_grow_memory_expr(void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_grow_memory_expr();
  return append_expr(ctx, expr);
}

static Result on_i32_const_expr(uint32_t value, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_const_expr();
  expr->const_.type = Type::I32;
  expr->const_.u32 = value;
  return append_expr(ctx, expr);
}

static Result on_i64_const_expr(uint64_t value, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_const_expr();
  expr->const_.type = Type::I64;
  expr->const_.u64 = value;
  return append_expr(ctx, expr);
}

static Result on_if_expr(uint32_t num_types, Type* sig_types, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_if_expr();
  TypeVector src;
  WABT_ZERO_MEMORY(src);
  src.size = num_types;
  src.data = sig_types;
  extend_types(&expr->if_.true_.sig, &src);
  append_expr(ctx, expr);
  push_label(ctx, LabelType::If, &expr->if_.true_.first);
  return Result::Ok;
}

static Result on_load_expr(Opcode opcode,
                           uint32_t alignment_log2,
                           uint32_t offset,
                           void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_load_expr();
  expr->load.opcode = opcode;
  expr->load.align = 1 << alignment_log2;
  expr->load.offset = offset;
  return append_expr(ctx, expr);
}

static Result on_loop_expr(uint32_t num_types,
                           Type* sig_types,
                           void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_loop_expr();
  TypeVector src;
  WABT_ZERO_MEMORY(src);
  src.size = num_types;
  src.data = sig_types;
  extend_types(&expr->loop.sig, &src);
  append_expr(ctx, expr);
  push_label(ctx, LabelType::Loop, &expr->loop.first);
  return Result::Ok;
}

static Result on_nop_expr(void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_nop_expr();
  return append_expr(ctx, expr);
}

static Result on_return_expr(void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_return_expr();
  return append_expr(ctx, expr);
}

static Result on_select_expr(void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_select_expr();
  return append_expr(ctx, expr);
}

static Result on_set_global_expr(uint32_t global_index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_set_global_expr();
  expr->set_global.var.type = VarType::Index;
  expr->set_global.var.index = global_index;
  return append_expr(ctx, expr);
}

static Result on_set_local_expr(uint32_t local_index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_set_local_expr();
  expr->set_local.var.type = VarType::Index;
  expr->set_local.var.index = local_index;
  return append_expr(ctx, expr);
}

static Result on_store_expr(Opcode opcode,
                            uint32_t alignment_log2,
                            uint32_t offset,
                            void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_store_expr();
  expr->store.opcode = opcode;
  expr->store.align = 1 << alignment_log2;
  expr->store.offset = offset;
  return append_expr(ctx, expr);
}

static Result on_tee_local_expr(uint32_t local_index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_tee_local_expr();
  expr->tee_local.var.type = VarType::Index;
  expr->tee_local.var.index = local_index;
  return append_expr(ctx, expr);
}

static Result on_unary_expr(Opcode opcode, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_unary_expr();
  expr->unary.opcode = opcode;
  return append_expr(ctx, expr);
}

static Result on_unreachable_expr(void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_unreachable_expr();
  return append_expr(ctx, expr);
}

static Result end_function_body(uint32_t index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_RESULT(pop_label(ctx));
  ctx->current_func = nullptr;
  return Result::Ok;
}

static Result on_elem_segment_count(uint32_t count, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  reserve_elem_segment_ptrs(&ctx->module->elem_segments, count);
  return Result::Ok;
}

static Result begin_elem_segment(uint32_t index,
                                 uint32_t table_index,
                                 void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  ModuleField* field = append_module_field(ctx->module);
  field->type = ModuleFieldType::ElemSegment;

  ElemSegment* segment = &field->elem_segment;
  WABT_ZERO_MEMORY(*segment);
  segment->table_var.type = VarType::Index;
  segment->table_var.index = table_index;

  assert(index == ctx->module->elem_segments.size);
  assert(index < ctx->module->elem_segments.capacity);
  ElemSegmentPtr* segment_ptr =
      append_elem_segment_ptr(&ctx->module->elem_segments);
  *segment_ptr = segment;
  return Result::Ok;
}

static Result begin_elem_segment_init_expr(uint32_t index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  assert(index == ctx->module->elem_segments.size - 1);
  ElemSegment* segment = ctx->module->elem_segments.data[index];
  ctx->current_init_expr = &segment->offset;
  return Result::Ok;
}

static Result end_elem_segment_init_expr(uint32_t index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  ctx->current_init_expr = nullptr;
  return Result::Ok;
}

static Result on_elem_segment_function_index_count(BinaryReaderContext* context,
                                                   uint32_t index,
                                                   uint32_t count) {
  Context* ctx = static_cast<Context*>(context->user_data);
  assert(index == ctx->module->elem_segments.size - 1);
  ElemSegment* segment = ctx->module->elem_segments.data[index];
  reserve_vars(&segment->vars, count);
  return Result::Ok;
}

static Result on_elem_segment_function_index(uint32_t index,
                                             uint32_t func_index,
                                             void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  assert(index == ctx->module->elem_segments.size - 1);
  ElemSegment* segment = ctx->module->elem_segments.data[index];
  Var* var = append_var(&segment->vars);
  var->type = VarType::Index;
  var->index = func_index;
  return Result::Ok;
}

static Result on_data_segment_count(uint32_t count, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  reserve_data_segment_ptrs(&ctx->module->data_segments, count);
  return Result::Ok;
}

static Result begin_data_segment(uint32_t index,
                                 uint32_t memory_index,
                                 void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  ModuleField* field = append_module_field(ctx->module);
  field->type = ModuleFieldType::DataSegment;

  DataSegment* segment = &field->data_segment;
  WABT_ZERO_MEMORY(*segment);
  segment->memory_var.type = VarType::Index;
  segment->memory_var.index = memory_index;

  assert(index == ctx->module->data_segments.size);
  assert(index < ctx->module->data_segments.capacity);
  DataSegmentPtr* segment_ptr =
      append_data_segment_ptr(&ctx->module->data_segments);
  *segment_ptr = segment;
  return Result::Ok;
}

static Result begin_data_segment_init_expr(uint32_t index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  assert(index == ctx->module->data_segments.size - 1);
  DataSegment* segment = ctx->module->data_segments.data[index];
  ctx->current_init_expr = &segment->offset;
  return Result::Ok;
}

static Result end_data_segment_init_expr(uint32_t index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  ctx->current_init_expr = nullptr;
  return Result::Ok;
}

static Result on_data_segment_data(uint32_t index,
                                   const void* data,
                                   uint32_t size,
                                   void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  assert(index == ctx->module->data_segments.size - 1);
  DataSegment* segment = ctx->module->data_segments.data[index];
  segment->data = new char[size];
  segment->size = size;
  memcpy(segment->data, data, size);
  return Result::Ok;
}

static Result on_function_names_count(uint32_t count, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  if (count > ctx->module->funcs.size) {
    print_error(
        ctx, "expected function name count (%u) <= function count (%" PRIzd ")",
        count, ctx->module->funcs.size);
    return Result::Error;
  }
  return Result::Ok;
}

static Result on_function_name(uint32_t index,
                               StringSlice name,
                               void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);

  StringSlice new_name;
  dup_name(ctx, &name, &new_name);

  Binding* binding = insert_binding(&ctx->module->func_bindings, &new_name);
  binding->index = index;

  Func* func = ctx->module->funcs.data[index];
  func->name = new_name;
  return Result::Ok;
}

static Result on_local_name_local_count(uint32_t index,
                                        uint32_t count,
                                        void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Module* module = ctx->module;
  assert(index < module->funcs.size);
  Func* func = module->funcs.data[index];
  uint32_t num_params_and_locals = get_num_params_and_locals(func);
  if (count > num_params_and_locals) {
    print_error(ctx, "expected local name count (%d) <= local count (%d)",
                count, num_params_and_locals);
    return Result::Error;
  }
  return Result::Ok;
}

static Result on_init_expr_f32_const_expr(uint32_t index,
                                          uint32_t value,
                                          void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_const_expr();
  expr->const_.type = Type::F32;
  expr->const_.f32_bits = value;
  *ctx->current_init_expr = expr;
  return Result::Ok;
}

static Result on_init_expr_f64_const_expr(uint32_t index,
                                          uint64_t value,
                                          void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_const_expr();
  expr->const_.type = Type::F64;
  expr->const_.f64_bits = value;
  *ctx->current_init_expr = expr;
  return Result::Ok;
}

static Result on_init_expr_get_global_expr(uint32_t index,
                                           uint32_t global_index,
                                           void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_get_global_expr();
  expr->get_global.var.type = VarType::Index;
  expr->get_global.var.index = global_index;
  *ctx->current_init_expr = expr;
  return Result::Ok;
}

static Result on_init_expr_i32_const_expr(uint32_t index,
                                          uint32_t value,
                                          void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_const_expr();
  expr->const_.type = Type::I32;
  expr->const_.u32 = value;
  *ctx->current_init_expr = expr;
  return Result::Ok;
}

static Result on_init_expr_i64_const_expr(uint32_t index,
                                          uint64_t value,
                                          void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Expr* expr = new_const_expr();
  expr->const_.type = Type::I64;
  expr->const_.u64 = value;
  *ctx->current_init_expr = expr;
  return Result::Ok;
}

static Result on_local_name(uint32_t func_index,
                            uint32_t local_index,
                            StringSlice name,
                            void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Module* module = ctx->module;
  Func* func = module->funcs.data[func_index];
  uint32_t num_params = get_num_params(func);
  StringSlice new_name;
  dup_name(ctx, &name, &new_name);
  BindingHash* bindings;
  Binding* binding;
  uint32_t index;
  if (local_index < num_params) {
    /* param name */
    bindings = &func->param_bindings;
    index = local_index;
  } else {
    /* local name */
    bindings = &func->local_bindings;
    index = local_index - num_params;
  }
  binding = insert_binding(bindings, &new_name);
  binding->index = index;
  return Result::Ok;
}

static void destroy_label_node(LabelNode* node) {
  if (*node->first)
    destroy_expr_list(*node->first);
}

Result read_binary_ast(const void* data,
                       size_t size,
                       const ReadBinaryOptions* options,
                       BinaryErrorHandler* error_handler,
                       struct Module* out_module) {
  Context ctx;
  WABT_ZERO_MEMORY(ctx);
  ctx.error_handler = error_handler;
  ctx.module = out_module;

  BinaryReader reader;
  WABT_ZERO_MEMORY(reader);
  reader.user_data = &ctx;
  reader.on_error = on_error;

  reader.on_signature_count = on_signature_count;
  reader.on_signature = on_signature;

  reader.on_import_count = on_import_count;
  reader.on_import = on_import;
  reader.on_import_func = on_import_func;
  reader.on_import_table = on_import_table;
  reader.on_import_memory = on_import_memory;
  reader.on_import_global = on_import_global;

  reader.on_function_signatures_count = on_function_signatures_count;
  reader.on_function_signature = on_function_signature;

  reader.on_table_count = on_table_count;
  reader.on_table = on_table;

  reader.on_memory_count = on_memory_count;
  reader.on_memory = on_memory;

  reader.on_global_count = on_global_count;
  reader.begin_global = begin_global;
  reader.begin_global_init_expr = begin_global_init_expr;
  reader.end_global_init_expr = end_global_init_expr;

  reader.on_export_count = on_export_count;
  reader.on_export = on_export;

  reader.on_start_function = on_start_function;

  reader.on_function_bodies_count = on_function_bodies_count;
  reader.begin_function_body = begin_function_body;
  reader.on_local_decl = on_local_decl;
  reader.on_binary_expr = on_binary_expr;
  reader.on_block_expr = on_block_expr;
  reader.on_br_expr = on_br_expr;
  reader.on_br_if_expr = on_br_if_expr;
  reader.on_br_table_expr = on_br_table_expr;
  reader.on_call_expr = on_call_expr;
  reader.on_call_indirect_expr = on_call_indirect_expr;
  reader.on_compare_expr = on_compare_expr;
  reader.on_convert_expr = on_convert_expr;
  reader.on_current_memory_expr = on_current_memory_expr;
  reader.on_drop_expr = on_drop_expr;
  reader.on_else_expr = on_else_expr;
  reader.on_end_expr = on_end_expr;
  reader.on_f32_const_expr = on_f32_const_expr;
  reader.on_f64_const_expr = on_f64_const_expr;
  reader.on_get_global_expr = on_get_global_expr;
  reader.on_get_local_expr = on_get_local_expr;
  reader.on_grow_memory_expr = on_grow_memory_expr;
  reader.on_i32_const_expr = on_i32_const_expr;
  reader.on_i64_const_expr = on_i64_const_expr;
  reader.on_if_expr = on_if_expr;
  reader.on_load_expr = on_load_expr;
  reader.on_loop_expr = on_loop_expr;
  reader.on_nop_expr = on_nop_expr;
  reader.on_return_expr = on_return_expr;
  reader.on_select_expr = on_select_expr;
  reader.on_set_global_expr = on_set_global_expr;
  reader.on_set_local_expr = on_set_local_expr;
  reader.on_store_expr = on_store_expr;
  reader.on_tee_local_expr = on_tee_local_expr;
  reader.on_unary_expr = on_unary_expr;
  reader.on_unreachable_expr = on_unreachable_expr;
  reader.end_function_body = end_function_body;

  reader.on_elem_segment_count = on_elem_segment_count;
  reader.begin_elem_segment = begin_elem_segment;
  reader.begin_elem_segment_init_expr = begin_elem_segment_init_expr;
  reader.end_elem_segment_init_expr = end_elem_segment_init_expr;
  reader.on_elem_segment_function_index_count =
      on_elem_segment_function_index_count;
  reader.on_elem_segment_function_index = on_elem_segment_function_index;

  reader.on_data_segment_count = on_data_segment_count;
  reader.begin_data_segment = begin_data_segment;
  reader.begin_data_segment_init_expr = begin_data_segment_init_expr;
  reader.end_data_segment_init_expr = end_data_segment_init_expr;
  reader.on_data_segment_data = on_data_segment_data;

  reader.on_function_names_count = on_function_names_count;
  reader.on_function_name = on_function_name;
  reader.on_local_name_local_count = on_local_name_local_count;
  reader.on_local_name = on_local_name;

  reader.on_init_expr_f32_const_expr = on_init_expr_f32_const_expr;
  reader.on_init_expr_f64_const_expr = on_init_expr_f64_const_expr;
  reader.on_init_expr_get_global_expr = on_init_expr_get_global_expr;
  reader.on_init_expr_i32_const_expr = on_init_expr_i32_const_expr;
  reader.on_init_expr_i64_const_expr = on_init_expr_i64_const_expr;

  Result result = read_binary(data, size, &reader, 1, options);
  WABT_DESTROY_VECTOR_AND_ELEMENTS(ctx.label_stack, label_node);
  if (WABT_FAILED(result))
    destroy_module(out_module);
  return result;
}

}  // namespace wabt
