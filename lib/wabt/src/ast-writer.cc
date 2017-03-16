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

#include "ast-writer.h"

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

#include "ast.h"
#include "common.h"
#include "literal.h"
#include "stream.h"
#include "writer.h"

#define INDENT_SIZE 2
#define NO_FORCE_NEWLINE 0
#define FORCE_NEWLINE 1

namespace wabt {

static const uint8_t s_is_char_escaped[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

enum class NextChar {
  None,
  Space,
  Newline,
  ForceNewline,
};

struct Context {
  Stream stream;
  Result result;
  int indent;
  NextChar next_char;
  int depth;
  StringSliceVector index_to_name;

  int func_index;
  int global_index;
  int export_index;
  int table_index;
  int memory_index;
  int func_type_index;
};

static void indent(Context* ctx) {
  ctx->indent += INDENT_SIZE;
}

static void dedent(Context* ctx) {
  ctx->indent -= INDENT_SIZE;
  assert(ctx->indent >= 0);
}

static void write_indent(Context* ctx) {
  static char s_indent[] =
      "                                                                       "
      "                                                                       ";
  static size_t s_indent_len = sizeof(s_indent) - 1;
  size_t indent = ctx->indent;
  while (indent > s_indent_len) {
    write_data(&ctx->stream, s_indent, s_indent_len, nullptr);
    indent -= s_indent_len;
  }
  if (indent > 0) {
    write_data(&ctx->stream, s_indent, indent, nullptr);
  }
}

static void write_next_char(Context* ctx) {
  switch (ctx->next_char) {
    case NextChar::Space:
      write_data(&ctx->stream, " ", 1, nullptr);
      break;
    case NextChar::Newline:
    case NextChar::ForceNewline:
      write_data(&ctx->stream, "\n", 1, nullptr);
      write_indent(ctx);
      break;

    default:
    case NextChar::None:
      break;
  }
  ctx->next_char = NextChar::None;
}

static void write_data_with_next_char(Context* ctx,
                                      const void* src,
                                      size_t size) {
  write_next_char(ctx);
  write_data(&ctx->stream, src, size, nullptr);
}

static void WABT_PRINTF_FORMAT(2, 3)
    writef(Context* ctx, const char* format, ...) {
  WABT_SNPRINTF_ALLOCA(buffer, length, format);
  /* default to following space */
  write_data_with_next_char(ctx, buffer, length);
  ctx->next_char = NextChar::Space;
}

static void write_putc(Context* ctx, char c) {
  write_data(&ctx->stream, &c, 1, nullptr);
}

static void write_puts(Context* ctx, const char* s, NextChar next_char) {
  size_t len = strlen(s);
  write_data_with_next_char(ctx, s, len);
  ctx->next_char = next_char;
}

static void write_puts_space(Context* ctx, const char* s) {
  write_puts(ctx, s, NextChar::Space);
}

static void write_puts_newline(Context* ctx, const char* s) {
  write_puts(ctx, s, NextChar::Newline);
}

static void write_newline(Context* ctx, bool force) {
  if (ctx->next_char == NextChar::ForceNewline)
    write_next_char(ctx);
  ctx->next_char = force ? NextChar::ForceNewline : NextChar::Newline;
}

static void write_open(Context* ctx, const char* name, NextChar next_char) {
  write_puts(ctx, "(", NextChar::None);
  write_puts(ctx, name, next_char);
  indent(ctx);
}

static void write_open_newline(Context* ctx, const char* name) {
  write_open(ctx, name, NextChar::Newline);
}

static void write_open_space(Context* ctx, const char* name) {
  write_open(ctx, name, NextChar::Space);
}

static void write_close(Context* ctx, NextChar next_char) {
  if (ctx->next_char != NextChar::ForceNewline)
    ctx->next_char = NextChar::None;
  dedent(ctx);
  write_puts(ctx, ")", next_char);
}

static void write_close_newline(Context* ctx) {
  write_close(ctx, NextChar::Newline);
}

static void write_close_space(Context* ctx) {
  write_close(ctx, NextChar::Space);
}

static void write_string_slice(Context* ctx,
                               const StringSlice* str,
                               NextChar next_char) {
  writef(ctx, PRIstringslice, WABT_PRINTF_STRING_SLICE_ARG(*str));
  ctx->next_char = next_char;
}

static bool write_string_slice_opt(Context* ctx,
                                   const StringSlice* str,
                                   NextChar next_char) {
  if (str->start)
    write_string_slice(ctx, str, next_char);
  return !!str->start;
}

static void write_string_slice_or_index(Context* ctx,
                                        const StringSlice* str,
                                        uint32_t index,
                                        NextChar next_char) {
  if (str->start)
    write_string_slice(ctx, str, next_char);
  else
    writef(ctx, "(;%u;)", index);
}

static void write_quoted_data(Context* ctx, const void* data, size_t length) {
  const uint8_t* u8_data = static_cast<const uint8_t*>(data);
  static const char s_hexdigits[] = "0123456789abcdef";
  write_next_char(ctx);
  write_putc(ctx, '\"');
  size_t i;
  for (i = 0; i < length; ++i) {
    uint8_t c = u8_data[i];
    if (s_is_char_escaped[c]) {
      write_putc(ctx, '\\');
      write_putc(ctx, s_hexdigits[c >> 4]);
      write_putc(ctx, s_hexdigits[c & 0xf]);
    } else {
      write_putc(ctx, c);
    }
  }
  write_putc(ctx, '\"');
  ctx->next_char = NextChar::Space;
}

static void write_quoted_string_slice(Context* ctx,
                                      const StringSlice* str,
                                      NextChar next_char) {
  write_quoted_data(ctx, str->start, str->length);
  ctx->next_char = next_char;
}

static void write_var(Context* ctx, const Var* var, NextChar next_char) {
  if (var->type == VarType::Index) {
    writef(ctx, "%" PRId64, var->index);
    ctx->next_char = next_char;
  } else {
    write_string_slice(ctx, &var->name, next_char);
  }
}

static void write_br_var(Context* ctx, const Var* var, NextChar next_char) {
  if (var->type == VarType::Index) {
    writef(ctx, "%" PRId64 " (;@%" PRId64 ";)", var->index,
           ctx->depth - var->index - 1);
    ctx->next_char = next_char;
  } else {
    write_string_slice(ctx, &var->name, next_char);
  }
}

static void write_type(Context* ctx, Type type, NextChar next_char) {
  const char* type_name = get_type_name(type);
  assert(type_name);
  write_puts(ctx, type_name, next_char);
}

static void write_types(Context* ctx,
                        const TypeVector* types,
                        const char* name) {
  if (types->size) {
    size_t i;
    if (name)
      write_open_space(ctx, name);
    for (i = 0; i < types->size; ++i)
      write_type(ctx, types->data[i], NextChar::Space);
    if (name)
      write_close_space(ctx);
  }
}

static void write_func_sig_space(Context* ctx, const FuncSignature* func_sig) {
  write_types(ctx, &func_sig->param_types, "param");
  write_types(ctx, &func_sig->result_types, "result");
}

static void write_expr_list(Context* ctx, const Expr* first);

static void write_expr(Context* ctx, const Expr* expr);

static void write_begin_block(Context* ctx,
                              const Block* block,
                              const char* text) {
  write_puts_space(ctx, text);
  bool has_label = write_string_slice_opt(ctx, &block->label, NextChar::Space);
  write_types(ctx, &block->sig, nullptr);
  if (!has_label)
    writef(ctx, " ;; label = @%d", ctx->depth);
  write_newline(ctx, FORCE_NEWLINE);
  ctx->depth++;
  indent(ctx);
}

static void write_end_block(Context* ctx) {
  dedent(ctx);
  ctx->depth--;
  write_puts_newline(ctx, get_opcode_name(Opcode::End));
}

static void write_block(Context* ctx,
                        const Block* block,
                        const char* start_text) {
  write_begin_block(ctx, block, start_text);
  write_expr_list(ctx, block->first);
  write_end_block(ctx);
}

static void write_const(Context* ctx, const Const* const_) {
  switch (const_->type) {
    case Type::I32:
      write_puts_space(ctx, get_opcode_name(Opcode::I32Const));
      writef(ctx, "%d", static_cast<int32_t>(const_->u32));
      write_newline(ctx, NO_FORCE_NEWLINE);
      break;

    case Type::I64:
      write_puts_space(ctx, get_opcode_name(Opcode::I64Const));
      writef(ctx, "%" PRId64, static_cast<int64_t>(const_->u64));
      write_newline(ctx, NO_FORCE_NEWLINE);
      break;

    case Type::F32: {
      write_puts_space(ctx, get_opcode_name(Opcode::F32Const));
      char buffer[128];
      write_float_hex(buffer, 128, const_->f32_bits);
      write_puts_space(ctx, buffer);
      float f32;
      memcpy(&f32, &const_->f32_bits, sizeof(f32));
      writef(ctx, "(;=%g;)", f32);
      write_newline(ctx, NO_FORCE_NEWLINE);
      break;
    }

    case Type::F64: {
      write_puts_space(ctx, get_opcode_name(Opcode::F64Const));
      char buffer[128];
      write_double_hex(buffer, 128, const_->f64_bits);
      write_puts_space(ctx, buffer);
      double f64;
      memcpy(&f64, &const_->f64_bits, sizeof(f64));
      writef(ctx, "(;=%g;)", f64);
      write_newline(ctx, NO_FORCE_NEWLINE);
      break;
    }

    default:
      assert(0);
      break;
  }
}

static void write_expr(Context* ctx, const Expr* expr) {
  switch (expr->type) {
    case ExprType::Binary:
      write_puts_newline(ctx, get_opcode_name(expr->binary.opcode));
      break;

    case ExprType::Block:
      write_block(ctx, &expr->block, get_opcode_name(Opcode::Block));
      break;

    case ExprType::Br:
      write_puts_space(ctx, get_opcode_name(Opcode::Br));
      write_br_var(ctx, &expr->br.var, NextChar::Newline);
      break;

    case ExprType::BrIf:
      write_puts_space(ctx, get_opcode_name(Opcode::BrIf));
      write_br_var(ctx, &expr->br_if.var, NextChar::Newline);
      break;

    case ExprType::BrTable: {
      write_puts_space(ctx, get_opcode_name(Opcode::BrTable));
      size_t i;
      for (i = 0; i < expr->br_table.targets.size; ++i)
        write_br_var(ctx, &expr->br_table.targets.data[i], NextChar::Space);
      write_br_var(ctx, &expr->br_table.default_target, NextChar::Newline);
      break;
    }

    case ExprType::Call:
      write_puts_space(ctx, get_opcode_name(Opcode::Call));
      write_var(ctx, &expr->call.var, NextChar::Newline);
      break;

    case ExprType::CallIndirect:
      write_puts_space(ctx, get_opcode_name(Opcode::CallIndirect));
      write_var(ctx, &expr->call_indirect.var, NextChar::Newline);
      break;

    case ExprType::Compare:
      write_puts_newline(ctx, get_opcode_name(expr->compare.opcode));
      break;

    case ExprType::Const:
      write_const(ctx, &expr->const_);
      break;

    case ExprType::Convert:
      write_puts_newline(ctx, get_opcode_name(expr->convert.opcode));
      break;

    case ExprType::Drop:
      write_puts_newline(ctx, get_opcode_name(Opcode::Drop));
      break;

    case ExprType::GetGlobal:
      write_puts_space(ctx, get_opcode_name(Opcode::GetGlobal));
      write_var(ctx, &expr->get_global.var, NextChar::Newline);
      break;

    case ExprType::GetLocal:
      write_puts_space(ctx, get_opcode_name(Opcode::GetLocal));
      write_var(ctx, &expr->get_local.var, NextChar::Newline);
      break;

    case ExprType::GrowMemory:
      write_puts_newline(ctx, get_opcode_name(Opcode::GrowMemory));
      break;

    case ExprType::If:
      write_begin_block(ctx, &expr->if_.true_, get_opcode_name(Opcode::If));
      write_expr_list(ctx, expr->if_.true_.first);
      if (expr->if_.false_) {
        dedent(ctx);
        write_puts_space(ctx, get_opcode_name(Opcode::Else));
        indent(ctx);
        write_newline(ctx, FORCE_NEWLINE);
        write_expr_list(ctx, expr->if_.false_);
      }
      write_end_block(ctx);
      break;

    case ExprType::Load:
      write_puts_space(ctx, get_opcode_name(expr->load.opcode));
      if (expr->load.offset)
        writef(ctx, "offset=%" PRIu64, expr->load.offset);
      if (!is_naturally_aligned(expr->load.opcode, expr->load.align))
        writef(ctx, "align=%u", expr->load.align);
      write_newline(ctx, NO_FORCE_NEWLINE);
      break;

    case ExprType::Loop:
      write_block(ctx, &expr->loop, get_opcode_name(Opcode::Loop));
      break;

    case ExprType::CurrentMemory:
      write_puts_newline(ctx, get_opcode_name(Opcode::CurrentMemory));
      break;

    case ExprType::Nop:
      write_puts_newline(ctx, get_opcode_name(Opcode::Nop));
      break;

    case ExprType::Return:
      write_puts_newline(ctx, get_opcode_name(Opcode::Return));
      break;

    case ExprType::Select:
      write_puts_newline(ctx, get_opcode_name(Opcode::Select));
      break;

    case ExprType::SetGlobal:
      write_puts_space(ctx, get_opcode_name(Opcode::SetGlobal));
      write_var(ctx, &expr->set_global.var, NextChar::Newline);
      break;

    case ExprType::SetLocal:
      write_puts_space(ctx, get_opcode_name(Opcode::SetLocal));
      write_var(ctx, &expr->set_local.var, NextChar::Newline);
      break;

    case ExprType::Store:
      write_puts_space(ctx, get_opcode_name(expr->store.opcode));
      if (expr->store.offset)
        writef(ctx, "offset=%" PRIu64, expr->store.offset);
      if (!is_naturally_aligned(expr->store.opcode, expr->store.align))
        writef(ctx, "align=%u", expr->store.align);
      write_newline(ctx, NO_FORCE_NEWLINE);
      break;

    case ExprType::TeeLocal:
      write_puts_space(ctx, get_opcode_name(Opcode::TeeLocal));
      write_var(ctx, &expr->tee_local.var, NextChar::Newline);
      break;

    case ExprType::Unary:
      write_puts_newline(ctx, get_opcode_name(expr->unary.opcode));
      break;

    case ExprType::Unreachable:
      write_puts_newline(ctx, get_opcode_name(Opcode::Unreachable));
      break;

    default:
      fprintf(stderr, "bad expr type: %d\n", static_cast<int>(expr->type));
      assert(0);
      break;
  }
}

static void write_expr_list(Context* ctx, const Expr* first) {
  const Expr* expr;
  for (expr = first; expr; expr = expr->next)
    write_expr(ctx, expr);
}

static void write_init_expr(Context* ctx, const Expr* expr) {
  if (expr) {
    write_puts(ctx, "(", NextChar::None);
    write_expr(ctx, expr);
    /* clear the next char, so we don't write a newline after the expr */
    ctx->next_char = NextChar::None;
    write_puts(ctx, ")", NextChar::Space);
  }
}

static void write_type_bindings(Context* ctx,
                                const char* prefix,
                                const Func* func,
                                const TypeVector* types,
                                const BindingHash* bindings) {
  make_type_binding_reverse_mapping(types, bindings, &ctx->index_to_name);

  /* named params/locals must be specified by themselves, but nameless
   * params/locals can be compressed, e.g.:
   *   (param $foo i32)
   *   (param i32 i64 f32)
   */
  bool is_open = false;
  size_t i;
  for (i = 0; i < types->size; ++i) {
    if (!is_open) {
      write_open_space(ctx, prefix);
      is_open = true;
    }

    const StringSlice* name = &ctx->index_to_name.data[i];
    bool has_name = !!name->start;
    if (has_name)
      write_string_slice(ctx, name, NextChar::Space);
    write_type(ctx, types->data[i], NextChar::Space);
    if (has_name) {
      write_close_space(ctx);
      is_open = false;
    }
  }
  if (is_open)
    write_close_space(ctx);
}

static void write_func(Context* ctx, const Module* module, const Func* func) {
  write_open_space(ctx, "func");
  write_string_slice_or_index(ctx, &func->name, ctx->func_index++,
                              NextChar::Space);
  if (decl_has_func_type(&func->decl)) {
    write_open_space(ctx, "type");
    write_var(ctx, &func->decl.type_var, NextChar::None);
    write_close_space(ctx);
  }
  write_type_bindings(ctx, "param", func, &func->decl.sig.param_types,
                      &func->param_bindings);
  write_types(ctx, &func->decl.sig.result_types, "result");
  write_newline(ctx, NO_FORCE_NEWLINE);
  if (func->local_types.size) {
    write_type_bindings(ctx, "local", func, &func->local_types,
                        &func->local_bindings);
  }
  write_newline(ctx, NO_FORCE_NEWLINE);
  ctx->depth = 1; /* for the implicit "return" label */
  write_expr_list(ctx, func->first_expr);
  write_close_newline(ctx);
}

static void write_begin_global(Context* ctx, const Global* global) {
  write_open_space(ctx, "global");
  write_string_slice_or_index(ctx, &global->name, ctx->global_index++,
                              NextChar::Space);
  if (global->mutable_) {
    write_open_space(ctx, "mut");
    write_type(ctx, global->type, NextChar::Space);
    write_close_space(ctx);
  } else {
    write_type(ctx, global->type, NextChar::Space);
  }
}

static void write_global(Context* ctx, const Global* global) {
  write_begin_global(ctx, global);
  write_init_expr(ctx, global->init_expr);
  write_close_newline(ctx);
}

static void write_limits(Context* ctx, const Limits* limits) {
  writef(ctx, "%" PRIu64, limits->initial);
  if (limits->has_max)
    writef(ctx, "%" PRIu64, limits->max);
}

static void write_table(Context* ctx, const Table* table) {
  write_open_space(ctx, "table");
  write_string_slice_or_index(ctx, &table->name, ctx->table_index++,
                              NextChar::Space);
  write_limits(ctx, &table->elem_limits);
  write_puts_space(ctx, "anyfunc");
  write_close_newline(ctx);
}

static void write_elem_segment(Context* ctx, const ElemSegment* segment) {
  write_open_space(ctx, "elem");
  write_init_expr(ctx, segment->offset);
  size_t i;
  for (i = 0; i < segment->vars.size; ++i)
    write_var(ctx, &segment->vars.data[i], NextChar::Space);
  write_close_newline(ctx);
}

static void write_memory(Context* ctx, const Memory* memory) {
  write_open_space(ctx, "memory");
  write_string_slice_or_index(ctx, &memory->name, ctx->memory_index++,
                              NextChar::Space);
  write_limits(ctx, &memory->page_limits);
  write_close_newline(ctx);
}

static void write_data_segment(Context* ctx, const DataSegment* segment) {
  write_open_space(ctx, "data");
  write_init_expr(ctx, segment->offset);
  write_quoted_data(ctx, segment->data, segment->size);
  write_close_newline(ctx);
}

static void write_import(Context* ctx, const Import* import) {
  write_open_space(ctx, "import");
  write_quoted_string_slice(ctx, &import->module_name, NextChar::Space);
  write_quoted_string_slice(ctx, &import->field_name, NextChar::Space);
  switch (import->kind) {
    case ExternalKind::Func:
      write_open_space(ctx, "func");
      write_string_slice_or_index(ctx, &import->func.name, ctx->func_index++,
                                  NextChar::Space);
      if (decl_has_func_type(&import->func.decl)) {
        write_open_space(ctx, "type");
        write_var(ctx, &import->func.decl.type_var, NextChar::None);
        write_close_space(ctx);
      } else {
        write_func_sig_space(ctx, &import->func.decl.sig);
      }
      write_close_space(ctx);
      break;

    case ExternalKind::Table:
      write_table(ctx, &import->table);
      break;

    case ExternalKind::Memory:
      write_memory(ctx, &import->memory);
      break;

    case ExternalKind::Global:
      write_begin_global(ctx, &import->global);
      write_close_space(ctx);
      break;
  }
  write_close_newline(ctx);
}

static void write_export(Context* ctx, const Export* export_) {
  static const char* s_kind_names[] = {"func", "table", "memory", "global"};
  WABT_STATIC_ASSERT(WABT_ARRAY_SIZE(s_kind_names) == kExternalKindCount);
  write_open_space(ctx, "export");
  write_quoted_string_slice(ctx, &export_->name, NextChar::Space);
  assert(static_cast<size_t>(export_->kind) < WABT_ARRAY_SIZE(s_kind_names));
  write_open_space(ctx, s_kind_names[static_cast<size_t>(export_->kind)]);
  write_var(ctx, &export_->var, NextChar::Space);
  write_close_space(ctx);
  write_close_newline(ctx);
}

static void write_func_type(Context* ctx, const FuncType* func_type) {
  write_open_space(ctx, "type");
  write_string_slice_or_index(ctx, &func_type->name, ctx->func_type_index++,
                              NextChar::Space);
  write_open_space(ctx, "func");
  write_func_sig_space(ctx, &func_type->sig);
  write_close_space(ctx);
  write_close_newline(ctx);
}

static void write_start_function(Context* ctx, const Var* start) {
  write_open_space(ctx, "start");
  write_var(ctx, start, NextChar::None);
  write_close_newline(ctx);
}

static void write_module(Context* ctx, const Module* module) {
  write_open_newline(ctx, "module");
  const ModuleField* field;
  for (field = module->first_field; field; field = field->next) {
    switch (field->type) {
      case ModuleFieldType::Func:
        write_func(ctx, module, &field->func);
        break;
      case ModuleFieldType::Global:
        write_global(ctx, &field->global);
        break;
      case ModuleFieldType::Import:
        write_import(ctx, &field->import);
        break;
      case ModuleFieldType::Export:
        write_export(ctx, &field->export_);
        break;
      case ModuleFieldType::Table:
        write_table(ctx, &field->table);
        break;
      case ModuleFieldType::ElemSegment:
        write_elem_segment(ctx, &field->elem_segment);
        break;
      case ModuleFieldType::Memory:
        write_memory(ctx, &field->memory);
        break;
      case ModuleFieldType::DataSegment:
        write_data_segment(ctx, &field->data_segment);
        break;
      case ModuleFieldType::FuncType:
        write_func_type(ctx, &field->func_type);
        break;
      case ModuleFieldType::Start:
        write_start_function(ctx, &field->start);
        break;
    }
  }
  write_close_newline(ctx);
  /* force the newline to be written */
  write_next_char(ctx);
}

Result write_ast(Writer* writer, const Module* module) {
  Context ctx;
  WABT_ZERO_MEMORY(ctx);
  ctx.result = Result::Ok;
  init_stream(&ctx.stream, writer, nullptr);
  write_module(&ctx, module);
  /* the memory for the actual string slice is shared with the module, so we
   * only need to free the vector */
  destroy_string_slice_vector(&ctx.index_to_name);
  return ctx.result;
}

}  // namespace wabt
