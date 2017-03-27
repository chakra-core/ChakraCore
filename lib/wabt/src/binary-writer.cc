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

#include "binary-writer.h"
#include "config.h"

#include <assert.h>
#include <math.h>
#include <memory.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include <vector>

#include "ast.h"
#include "binary.h"
#include "stream.h"
#include "writer.h"

#define PRINT_HEADER_NO_INDEX -1
#define MAX_U32_LEB128_BYTES 5
#define MAX_U64_LEB128_BYTES 10

namespace wabt {

namespace {

/* TODO(binji): better leb size guess. Some sections we know will only be 1
 byte, but others we can be fairly certain will be larger. */
static const size_t LEB_SECTION_SIZE_GUESS = 1;

#define ALLOC_FAILURE \
  fprintf(stderr, "%s:%d: allocation failed\n", __FILE__, __LINE__)

struct RelocSection {
  RelocSection(const char* name, BinarySection code);

  const char* name;
  BinarySection section_code;
  std::vector<Reloc> relocations;
};

RelocSection::RelocSection(const char* name, BinarySection code)
    : name(name), section_code(code) {}

struct Context {
  WABT_DISALLOW_COPY_AND_ASSIGN(Context);
  Context();

  Stream stream;
  Stream* log_stream = nullptr;
  const WriteBinaryOptions* options = nullptr;

  std::vector<RelocSection> reloc_sections;
  RelocSection* current_reloc_section = nullptr;

  size_t last_section_offset = 0;
  size_t last_section_leb_size_guess = 0;
  BinarySection last_section_type = BinarySection::Invalid;
  size_t last_section_payload_offset = 0;

  size_t last_subsection_offset = 0;
  size_t last_subsection_leb_size_guess = 0;
  size_t last_subsection_payload_offset = 0;
};

Context::Context() {
  WABT_ZERO_MEMORY(stream);
}

}  // namespace

static uint8_t log2_u32(uint32_t x) {
  uint8_t result = 0;
  while (x > 1) {
    x >>= 1;
    result++;
  }
  return result;
}

static void write_header(Context* ctx, const char* name, int index) {
  if (ctx->log_stream) {
    if (index == PRINT_HEADER_NO_INDEX) {
      writef(ctx->log_stream, "; %s\n", name);
    } else {
      writef(ctx->log_stream, "; %s %d\n", name, index);
    }
  }
}

#define LEB128_LOOP_UNTIL(end_cond) \
  do {                              \
    uint8_t byte = value & 0x7f;    \
    value >>= 7;                    \
    if (end_cond) {                 \
      data[i++] = byte;             \
      break;                        \
    } else {                        \
      data[i++] = byte | 0x80;      \
    }                               \
  } while (1)

uint32_t u32_leb128_length(uint32_t value) {
  uint8_t data[MAX_U32_LEB128_BYTES] WABT_UNUSED;
  uint32_t i = 0;
  LEB128_LOOP_UNTIL(value == 0);
  return i;
}

/* returns the length of the leb128 */
uint32_t write_u32_leb128_at(Stream* stream,
                             uint32_t offset,
                             uint32_t value,
                             const char* desc) {
  uint8_t data[MAX_U32_LEB128_BYTES];
  uint32_t i = 0;
  LEB128_LOOP_UNTIL(value == 0);
  uint32_t length = i;
  write_data_at(stream, offset, data, length, PrintChars::No, desc);
  return length;
}

uint32_t write_fixed_u32_leb128_raw(uint8_t* data,
                                    uint8_t* end,
                                    uint32_t value) {
  if (end - data < MAX_U32_LEB128_BYTES)
    return 0;
  data[0] = (value & 0x7f) | 0x80;
  data[1] = ((value >> 7) & 0x7f) | 0x80;
  data[2] = ((value >> 14) & 0x7f) | 0x80;
  data[3] = ((value >> 21) & 0x7f) | 0x80;
  data[4] = ((value >> 28) & 0x0f);
  return MAX_U32_LEB128_BYTES;
}

uint32_t write_fixed_u32_leb128_at(Stream* stream,
                                   uint32_t offset,
                                   uint32_t value,
                                   const char* desc) {
  uint8_t data[MAX_U32_LEB128_BYTES];
  uint32_t length =
      write_fixed_u32_leb128_raw(data, data + MAX_U32_LEB128_BYTES, value);
  write_data_at(stream, offset, data, length, PrintChars::No, desc);
  return length;
}

void write_u32_leb128(Stream* stream, uint32_t value, const char* desc) {
  uint32_t length = write_u32_leb128_at(stream, stream->offset, value, desc);
  stream->offset += length;
}

void write_fixed_u32_leb128(Stream* stream, uint32_t value, const char* desc) {
  uint32_t length =
      write_fixed_u32_leb128_at(stream, stream->offset, value, desc);
  stream->offset += length;
}

void write_i32_leb128(Stream* stream, int32_t value, const char* desc) {
  uint8_t data[MAX_U32_LEB128_BYTES];
  uint32_t i = 0;
  if (value < 0)
    LEB128_LOOP_UNTIL(value == -1 && (byte & 0x40));
  else
    LEB128_LOOP_UNTIL(value == 0 && !(byte & 0x40));

  uint32_t length = i;
  write_data_at(stream, stream->offset, data, length, PrintChars::No, desc);
  stream->offset += length;
}

static void write_i64_leb128(Stream* stream, int64_t value, const char* desc) {
  uint8_t data[MAX_U64_LEB128_BYTES];
  uint32_t i = 0;
  if (value < 0)
    LEB128_LOOP_UNTIL(value == -1 && (byte & 0x40));
  else
    LEB128_LOOP_UNTIL(value == 0 && !(byte & 0x40));

  int length = i;
  write_data_at(stream, stream->offset, data, length, PrintChars::No, desc);
  stream->offset += length;
}

#undef LEB128_LOOP_UNTIL

static uint32_t size_u32_leb128(uint32_t value) {
  uint32_t size = 0;
  do {
    value >>= 7;
    size++;
  } while (value != 0);
  return size;
}

/* returns offset of leb128 */
static uint32_t write_u32_leb128_space(Context* ctx,
                                       uint32_t leb_size_guess,
                                       const char* desc) {
  assert(leb_size_guess <= MAX_U32_LEB128_BYTES);
  uint8_t data[MAX_U32_LEB128_BYTES] = {0};
  uint32_t result = ctx->stream.offset;
  uint32_t bytes_to_write =
      ctx->options->canonicalize_lebs ? leb_size_guess : MAX_U32_LEB128_BYTES;
  write_data(&ctx->stream, data, bytes_to_write, desc);
  return result;
}

static void write_fixup_u32_leb128_size(Context* ctx,
                                        uint32_t offset,
                                        uint32_t leb_size_guess,
                                        const char* desc) {
  if (ctx->options->canonicalize_lebs) {
    uint32_t size = ctx->stream.offset - offset - leb_size_guess;
    uint32_t leb_size = size_u32_leb128(size);
    if (leb_size != leb_size_guess) {
      uint32_t src_offset = offset + leb_size_guess;
      uint32_t dst_offset = offset + leb_size;
      move_data(&ctx->stream, dst_offset, src_offset, size);
    }
    write_u32_leb128_at(&ctx->stream, offset, size, desc);
    ctx->stream.offset += leb_size - leb_size_guess;
  } else {
    uint32_t size = ctx->stream.offset - offset - MAX_U32_LEB128_BYTES;
    write_fixed_u32_leb128_at(&ctx->stream, offset, size, desc);
  }
}

void write_str(Stream* stream,
               const char* s,
               size_t length,
               PrintChars print_chars,
               const char* desc) {
  write_u32_leb128(stream, length, "string length");
  write_data_at(stream, stream->offset, s, length, print_chars, desc);
  stream->offset += length;
}

void write_opcode(Stream* stream, Opcode opcode) {
  write_u8_enum(stream, opcode, get_opcode_name(opcode));
}

void write_type(Stream* stream, Type type) {
  write_i32_leb128_enum(stream, type, get_type_name(type));
}

static void write_inline_signature_type(Stream* stream,
                                        const BlockSignature& sig) {
  if (sig.size() == 0) {
    write_type(stream, Type::Void);
  } else if (sig.size() == 1) {
    write_type(stream, sig[0]);
  } else {
    /* this is currently unrepresentable */
    write_u8(stream, 0xff, "INVALID INLINE SIGNATURE");
  }
}

static void begin_known_section(Context* ctx,
                                BinarySection section_code,
                                size_t leb_size_guess) {
  assert(ctx->last_section_leb_size_guess == 0);
  char desc[100];
  wabt_snprintf(desc, sizeof(desc), "section \"%s\" (%u)",
           get_section_name(section_code), static_cast<unsigned>(section_code));
  write_header(ctx, desc, PRINT_HEADER_NO_INDEX);
  write_u8_enum(&ctx->stream, section_code, "section code");
  ctx->last_section_type = section_code;
  ctx->last_section_leb_size_guess = leb_size_guess;
  ctx->last_section_offset =
      write_u32_leb128_space(ctx, leb_size_guess, "section size (guess)");
  ctx->last_section_payload_offset = ctx->stream.offset;
}

static void begin_custom_section(Context* ctx,
                                 const char* name,
                                 size_t leb_size_guess) {
  assert(ctx->last_section_leb_size_guess == 0);
  char desc[100];
  wabt_snprintf(desc, sizeof(desc), "section \"%s\"", name);
  write_header(ctx, desc, PRINT_HEADER_NO_INDEX);
  write_u8_enum(&ctx->stream, BinarySection::Custom, "custom section code");
  ctx->last_section_type = BinarySection::Custom;
  ctx->last_section_leb_size_guess = leb_size_guess;
  ctx->last_section_offset =
      write_u32_leb128_space(ctx, leb_size_guess, "section size (guess)");
  ctx->last_section_payload_offset = ctx->stream.offset;
  write_str(&ctx->stream, name, strlen(name), PrintChars::Yes,
            "custom section name");
}

static void end_section(Context* ctx) {
  assert(ctx->last_section_leb_size_guess != 0);
  write_fixup_u32_leb128_size(ctx, ctx->last_section_offset,
                              ctx->last_section_leb_size_guess,
                              "FIXUP section size");
  ctx->last_section_leb_size_guess = 0;
}

static void begin_subsection(Context* ctx,
                             const char* name,
                             size_t leb_size_guess) {
  assert(ctx->last_subsection_leb_size_guess == 0);
  char desc[100];
  wabt_snprintf(desc, sizeof(desc), "subsection \"%s\"", name);
  ctx->last_subsection_leb_size_guess = leb_size_guess;
  ctx->last_subsection_offset =
      write_u32_leb128_space(ctx, leb_size_guess, "subsection size (guess)");
  ctx->last_subsection_payload_offset = ctx->stream.offset;
}

static void end_subsection(Context* ctx) {
  assert(ctx->last_subsection_leb_size_guess != 0);
  write_fixup_u32_leb128_size(ctx, ctx->last_subsection_offset,
                              ctx->last_subsection_leb_size_guess,
                              "FIXUP subsection size");
  ctx->last_subsection_leb_size_guess = 0;
}

static uint32_t get_label_var_depth(Context* ctx, const Var* var) {
  assert(var->type == VarType::Index);
  return var->index;
}

static void write_expr_list(Context* ctx,
                            const Module* module,
                            const Func* func,
                            const Expr* first_expr);

static void add_reloc(Context* ctx, RelocType reloc_type, uint32_t index) {
  // Add a new reloc section if needed
  if (!ctx->current_reloc_section ||
      ctx->current_reloc_section->section_code != ctx->last_section_type) {
    ctx->reloc_sections.emplace_back(get_section_name(ctx->last_section_type),
                                     ctx->last_section_type);
    ctx->current_reloc_section = &ctx->reloc_sections.back();
  }

  // Add a new relocation to the curent reloc section
  size_t offset = ctx->stream.offset - ctx->last_section_payload_offset;
  ctx->current_reloc_section->relocations.emplace_back(reloc_type, offset,
                                                       index);
}

static void write_u32_leb128_with_reloc(Context* ctx,
                                        uint32_t index,
                                        const char* desc,
                                        RelocType reloc_type) {
  if (ctx->options->relocatable) {
    add_reloc(ctx, reloc_type, index);
    write_fixed_u32_leb128(&ctx->stream, index, desc);
  } else {
    write_u32_leb128(&ctx->stream, index, desc);
  }
}

static void write_expr(Context* ctx,
                       const Module* module,
                       const Func* func,
                       const Expr* expr) {
  switch (expr->type) {
    case ExprType::Binary:
      write_opcode(&ctx->stream, expr->binary.opcode);
      break;
    case ExprType::Block:
      write_opcode(&ctx->stream, Opcode::Block);
      write_inline_signature_type(&ctx->stream, expr->block->sig);
      write_expr_list(ctx, module, func, expr->block->first);
      write_opcode(&ctx->stream, Opcode::End);
      break;
    case ExprType::Br:
      write_opcode(&ctx->stream, Opcode::Br);
      write_u32_leb128(&ctx->stream, get_label_var_depth(ctx, &expr->br.var),
                       "break depth");
      break;
    case ExprType::BrIf:
      write_opcode(&ctx->stream, Opcode::BrIf);
      write_u32_leb128(&ctx->stream, get_label_var_depth(ctx, &expr->br_if.var),
                       "break depth");
      break;
    case ExprType::BrTable: {
      write_opcode(&ctx->stream, Opcode::BrTable);
      write_u32_leb128(&ctx->stream, expr->br_table.targets->size(),
                       "num targets");
      uint32_t depth;
      for (const Var& var: *expr->br_table.targets) {
        depth = get_label_var_depth(ctx, &var);
        write_u32_leb128(&ctx->stream, depth, "break depth");
      }
      depth = get_label_var_depth(ctx, &expr->br_table.default_target);
      write_u32_leb128(&ctx->stream, depth, "break depth for default");
      break;
    }
    case ExprType::Call: {
      int index = get_func_index_by_var(module, &expr->call.var);
      write_opcode(&ctx->stream, Opcode::Call);
      write_u32_leb128_with_reloc(ctx, index, "function index",
                                  RelocType::FuncIndexLEB);
      break;
    }
    case ExprType::CallIndirect: {
      int index = get_func_type_index_by_var(module, &expr->call_indirect.var);
      write_opcode(&ctx->stream, Opcode::CallIndirect);
      write_u32_leb128(&ctx->stream, index, "signature index");
      write_u32_leb128(&ctx->stream, 0, "call_indirect reserved");
      break;
    }
    case ExprType::Compare:
      write_opcode(&ctx->stream, expr->compare.opcode);
      break;
    case ExprType::Const:
      switch (expr->const_.type) {
        case Type::I32: {
          write_opcode(&ctx->stream, Opcode::I32Const);
          write_i32_leb128(&ctx->stream, expr->const_.u32, "i32 literal");
          break;
        }
        case Type::I64:
          write_opcode(&ctx->stream, Opcode::I64Const);
          write_i64_leb128(&ctx->stream, expr->const_.u64, "i64 literal");
          break;
        case Type::F32:
          write_opcode(&ctx->stream, Opcode::F32Const);
          write_u32(&ctx->stream, expr->const_.f32_bits, "f32 literal");
          break;
        case Type::F64:
          write_opcode(&ctx->stream, Opcode::F64Const);
          write_u64(&ctx->stream, expr->const_.f64_bits, "f64 literal");
          break;
        default:
          assert(0);
      }
      break;
    case ExprType::Convert:
      write_opcode(&ctx->stream, expr->convert.opcode);
      break;
    case ExprType::CurrentMemory:
      write_opcode(&ctx->stream, Opcode::CurrentMemory);
      write_u32_leb128(&ctx->stream, 0, "current_memory reserved");
      break;
    case ExprType::Drop:
      write_opcode(&ctx->stream, Opcode::Drop);
      break;
    case ExprType::GetGlobal: {
      int index = get_global_index_by_var(module, &expr->get_global.var);
      write_opcode(&ctx->stream, Opcode::GetGlobal);
      write_u32_leb128_with_reloc(ctx, index, "global index",
                                  RelocType::GlobalIndexLEB);
      break;
    }
    case ExprType::GetLocal: {
      int index = get_local_index_by_var(func, &expr->get_local.var);
      write_opcode(&ctx->stream, Opcode::GetLocal);
      write_u32_leb128(&ctx->stream, index, "local index");
      break;
    }
    case ExprType::GrowMemory:
      write_opcode(&ctx->stream, Opcode::GrowMemory);
      write_u32_leb128(&ctx->stream, 0, "grow_memory reserved");
      break;
    case ExprType::If:
      write_opcode(&ctx->stream, Opcode::If);
      write_inline_signature_type(&ctx->stream, expr->if_.true_->sig);
      write_expr_list(ctx, module, func, expr->if_.true_->first);
      if (expr->if_.false_) {
        write_opcode(&ctx->stream, Opcode::Else);
        write_expr_list(ctx, module, func, expr->if_.false_);
      }
      write_opcode(&ctx->stream, Opcode::End);
      break;
    case ExprType::Load: {
      write_opcode(&ctx->stream, expr->load.opcode);
      uint32_t align =
          get_opcode_alignment(expr->load.opcode, expr->load.align);
      write_u8(&ctx->stream, log2_u32(align), "alignment");
      write_u32_leb128(&ctx->stream, expr->load.offset, "load offset");
      break;
    }
    case ExprType::Loop:
      write_opcode(&ctx->stream, Opcode::Loop);
      write_inline_signature_type(&ctx->stream, expr->loop->sig);
      write_expr_list(ctx, module, func, expr->loop->first);
      write_opcode(&ctx->stream, Opcode::End);
      break;
    case ExprType::Nop:
      write_opcode(&ctx->stream, Opcode::Nop);
      break;
    case ExprType::Return:
      write_opcode(&ctx->stream, Opcode::Return);
      break;
    case ExprType::Select:
      write_opcode(&ctx->stream, Opcode::Select);
      break;
    case ExprType::SetGlobal: {
      int index = get_global_index_by_var(module, &expr->get_global.var);
      write_opcode(&ctx->stream, Opcode::SetGlobal);
      write_u32_leb128_with_reloc(ctx, index, "global index",
                                  RelocType::GlobalIndexLEB);
      break;
    }
    case ExprType::SetLocal: {
      int index = get_local_index_by_var(func, &expr->get_local.var);
      write_opcode(&ctx->stream, Opcode::SetLocal);
      write_u32_leb128(&ctx->stream, index, "local index");
      break;
    }
    case ExprType::Store: {
      write_opcode(&ctx->stream, expr->store.opcode);
      uint32_t align =
          get_opcode_alignment(expr->store.opcode, expr->store.align);
      write_u8(&ctx->stream, log2_u32(align), "alignment");
      write_u32_leb128(&ctx->stream, expr->store.offset, "store offset");
      break;
    }
    case ExprType::TeeLocal: {
      int index = get_local_index_by_var(func, &expr->get_local.var);
      write_opcode(&ctx->stream, Opcode::TeeLocal);
      write_u32_leb128(&ctx->stream, index, "local index");
      break;
    }
    case ExprType::Unary:
      write_opcode(&ctx->stream, expr->unary.opcode);
      break;
    case ExprType::Unreachable:
      write_opcode(&ctx->stream, Opcode::Unreachable);
      break;
  }
}

static void write_expr_list(Context* ctx,
                            const Module* module,
                            const Func* func,
                            const Expr* first) {
  for (const Expr* expr = first; expr; expr = expr->next)
    write_expr(ctx, module, func, expr);
}

static void write_init_expr(Context* ctx,
                            const Module* module,
                            const Expr* expr) {
  if (expr)
    write_expr_list(ctx, module, nullptr, expr);
  write_opcode(&ctx->stream, Opcode::End);
}

static void write_func_locals(Context* ctx,
                              const Module* module,
                              const Func* func,
                              const TypeVector& local_types) {
  if (local_types.size() == 0) {
    write_u32_leb128(&ctx->stream, 0, "local decl count");
    return;
  }

  uint32_t num_params = get_num_params(func);

#define FIRST_LOCAL_INDEX (num_params)
#define LAST_LOCAL_INDEX (num_params + local_types.size())
#define GET_LOCAL_TYPE(x) (local_types[x - num_params])

  /* loop through once to count the number of local declaration runs */
  Type current_type = GET_LOCAL_TYPE(FIRST_LOCAL_INDEX);
  uint32_t local_decl_count = 1;
  for (uint32_t i = FIRST_LOCAL_INDEX + 1; i < LAST_LOCAL_INDEX; ++i) {
    Type type = GET_LOCAL_TYPE(i);
    if (current_type != type) {
      local_decl_count++;
      current_type = type;
    }
  }

  /* loop through again to write everything out */
  write_u32_leb128(&ctx->stream, local_decl_count, "local decl count");
  current_type = GET_LOCAL_TYPE(FIRST_LOCAL_INDEX);
  uint32_t local_type_count = 1;
  for (uint32_t i = FIRST_LOCAL_INDEX + 1; i <= LAST_LOCAL_INDEX; ++i) {
    /* loop through an extra time to catch the final type transition */
    Type type = i == LAST_LOCAL_INDEX ? Type::Void : GET_LOCAL_TYPE(i);
    if (current_type == type) {
      local_type_count++;
    } else {
      write_u32_leb128(&ctx->stream, local_type_count, "local type count");
      write_type(&ctx->stream, current_type);
      local_type_count = 1;
      current_type = type;
    }
  }
}

static void write_func(Context* ctx, const Module* module, const Func* func) {
  write_func_locals(ctx, module, func, func->local_types);
  write_expr_list(ctx, module, func, func->first_expr);
  write_opcode(&ctx->stream, Opcode::End);
}

void write_limits(Stream* stream, const Limits* limits) {
  uint32_t flags = limits->has_max ? WABT_BINARY_LIMITS_HAS_MAX_FLAG : 0;
  write_u32_leb128(stream, flags, "limits: flags");
  write_u32_leb128(stream, limits->initial, "limits: initial");
  if (limits->has_max)
    write_u32_leb128(stream, limits->max, "limits: max");
}

static void write_table(Context* ctx, const Table* table) {
  write_type(&ctx->stream, Type::Anyfunc);
  write_limits(&ctx->stream, &table->elem_limits);
}

static void write_memory(Context* ctx, const Memory* memory) {
  write_limits(&ctx->stream, &memory->page_limits);
}

static void write_global_header(Context* ctx, const Global* global) {
  write_type(&ctx->stream, global->type);
  write_u8(&ctx->stream, global->mutable_, "global mutability");
}

static void write_reloc_section(Context* ctx, RelocSection* reloc_section) {
  char section_name[128];
  wabt_snprintf(section_name, sizeof(section_name), "%s.%s",
           WABT_BINARY_SECTION_RELOC, reloc_section->name);
  begin_custom_section(ctx, section_name, LEB_SECTION_SIZE_GUESS);
  write_u32_leb128_enum(&ctx->stream, reloc_section->section_code,
                        "reloc section type");
  std::vector<Reloc>& relocs = reloc_section->relocations;
  write_u32_leb128(&ctx->stream, relocs.size(), "num relocs");

  for (const Reloc& reloc: relocs) {
    write_u32_leb128_enum(&ctx->stream, reloc.type, "reloc type");
    write_u32_leb128(&ctx->stream, reloc.offset, "reloc offset");
    write_u32_leb128(&ctx->stream, reloc.index, "reloc index");
    switch (reloc.type) {
      case RelocType::MemoryAddressLEB:
      case RelocType::MemoryAddressSLEB:
      case RelocType::MemoryAddressI32:
        write_u32_leb128(&ctx->stream, reloc.addend, "reloc addend");
        break;
      default:
        break;
    }
  }

  end_section(ctx);
}

static Result write_module(Context* ctx, const Module* module) {
  write_u32(&ctx->stream, WABT_BINARY_MAGIC, "WASM_BINARY_MAGIC");
  write_u32(&ctx->stream, WABT_BINARY_VERSION, "WASM_BINARY_VERSION");

  if (module->func_types.size()) {
    begin_known_section(ctx, BinarySection::Type, LEB_SECTION_SIZE_GUESS);
    write_u32_leb128(&ctx->stream, module->func_types.size(), "num types");
    for (size_t i = 0; i < module->func_types.size(); ++i) {
      const FuncType* func_type = module->func_types[i];
      const FuncSignature* sig = &func_type->sig;
      write_header(ctx, "type", i);
      write_type(&ctx->stream, Type::Func);

      uint32_t num_params = sig->param_types.size();
      uint32_t num_results = sig->result_types.size();
      write_u32_leb128(&ctx->stream, num_params, "num params");
      for (size_t j = 0; j < num_params; ++j)
        write_type(&ctx->stream, sig->param_types[j]);

      write_u32_leb128(&ctx->stream, num_results, "num results");
      for (size_t j = 0; j < num_results; ++j)
        write_type(&ctx->stream, sig->result_types[j]);
    }
    end_section(ctx);
  }

  if (module->imports.size()) {
    begin_known_section(ctx, BinarySection::Import, LEB_SECTION_SIZE_GUESS);
    write_u32_leb128(&ctx->stream, module->imports.size(), "num imports");

    for (size_t i = 0; i < module->imports.size(); ++i) {
      const Import* import = module->imports[i];
      write_header(ctx, "import header", i);
      write_str(&ctx->stream, import->module_name.start,
                import->module_name.length, PrintChars::Yes,
                "import module name");
      write_str(&ctx->stream, import->field_name.start,
                import->field_name.length, PrintChars::Yes,
                "import field name");
      write_u8_enum(&ctx->stream, import->kind, "import kind");
      switch (import->kind) {
        case ExternalKind::Func:
          write_u32_leb128(&ctx->stream, get_func_type_index_by_decl(
                                             module, &import->func->decl),
                           "import signature index");
          break;
        case ExternalKind::Table:
          write_table(ctx, import->table);
          break;
        case ExternalKind::Memory:
          write_memory(ctx, import->memory);
          break;
        case ExternalKind::Global:
          write_global_header(ctx, import->global);
          break;
      }
    }
    end_section(ctx);
  }

  assert(module->funcs.size() >= module->num_func_imports);
  uint32_t num_funcs = module->funcs.size() - module->num_func_imports;
  if (num_funcs) {
    begin_known_section(ctx, BinarySection::Function, LEB_SECTION_SIZE_GUESS);
    write_u32_leb128(&ctx->stream, num_funcs, "num functions");

    for (size_t i = 0; i < num_funcs; ++i) {
      const Func* func = module->funcs[i + module->num_func_imports];
      char desc[100];
      wabt_snprintf(desc, sizeof(desc), "function %" PRIzd " signature index",
                    i);
      write_u32_leb128(&ctx->stream,
                       get_func_type_index_by_decl(module, &func->decl), desc);
    }
    end_section(ctx);
  }

  assert(module->tables.size() >= module->num_table_imports);
  uint32_t num_tables = module->tables.size() - module->num_table_imports;
  if (num_tables) {
    begin_known_section(ctx, BinarySection::Table, LEB_SECTION_SIZE_GUESS);
    write_u32_leb128(&ctx->stream, num_tables, "num tables");
    for (size_t i = 0; i < num_tables; ++i) {
      const Table* table = module->tables[i + module->num_table_imports];
      write_header(ctx, "table", i);
      write_table(ctx, table);
    }
    end_section(ctx);
  }

  assert(module->memories.size() >= module->num_memory_imports);
  uint32_t num_memories = module->memories.size() - module->num_memory_imports;
  if (num_memories) {
    begin_known_section(ctx, BinarySection::Memory, LEB_SECTION_SIZE_GUESS);
    write_u32_leb128(&ctx->stream, num_memories, "num memories");
    for (size_t i = 0; i < num_memories; ++i) {
      const Memory* memory = module->memories[i + module->num_memory_imports];
      write_header(ctx, "memory", i);
      write_memory(ctx, memory);
    }
    end_section(ctx);
  }

  assert(module->globals.size() >= module->num_global_imports);
  uint32_t num_globals = module->globals.size() - module->num_global_imports;
  if (num_globals) {
    begin_known_section(ctx, BinarySection::Global, LEB_SECTION_SIZE_GUESS);
    write_u32_leb128(&ctx->stream, num_globals, "num globals");

    for (size_t i = 0; i < num_globals; ++i) {
      const Global* global = module->globals[i + module->num_global_imports];
      write_global_header(ctx, global);
      write_init_expr(ctx, module, global->init_expr);
    }
    end_section(ctx);
  }

  if (module->exports.size()) {
    begin_known_section(ctx, BinarySection::Export, LEB_SECTION_SIZE_GUESS);
    write_u32_leb128(&ctx->stream, module->exports.size(), "num exports");

    for (const Export* export_ : module->exports) {
      write_str(&ctx->stream, export_->name.start, export_->name.length,
                PrintChars::Yes, "export name");
      write_u8_enum(&ctx->stream, export_->kind, "export kind");
      switch (export_->kind) {
        case ExternalKind::Func: {
          int index = get_func_index_by_var(module, &export_->var);
          write_u32_leb128(&ctx->stream, index, "export func index");
          break;
        }
        case ExternalKind::Table: {
          int index = get_table_index_by_var(module, &export_->var);
          write_u32_leb128(&ctx->stream, index, "export table index");
          break;
        }
        case ExternalKind::Memory: {
          int index = get_memory_index_by_var(module, &export_->var);
          write_u32_leb128(&ctx->stream, index, "export memory index");
          break;
        }
        case ExternalKind::Global: {
          int index = get_global_index_by_var(module, &export_->var);
          write_u32_leb128(&ctx->stream, index, "export global index");
          break;
        }
      }
    }
    end_section(ctx);
  }

  if (module->start) {
    int start_func_index = get_func_index_by_var(module, module->start);
    if (start_func_index != -1) {
      begin_known_section(ctx, BinarySection::Start, LEB_SECTION_SIZE_GUESS);
      write_u32_leb128(&ctx->stream, start_func_index, "start func index");
      end_section(ctx);
    }
  }

  if (module->elem_segments.size()) {
    begin_known_section(ctx, BinarySection::Elem, LEB_SECTION_SIZE_GUESS);
    write_u32_leb128(&ctx->stream, module->elem_segments.size(),
                     "num elem segments");
    for (size_t i = 0; i < module->elem_segments.size(); ++i) {
      ElemSegment* segment = module->elem_segments[i];
      int table_index = get_table_index_by_var(module, &segment->table_var);
      write_header(ctx, "elem segment header", i);
      write_u32_leb128(&ctx->stream, table_index, "table index");
      write_init_expr(ctx, module, segment->offset);
      write_u32_leb128(&ctx->stream, segment->vars.size(),
                       "num function indices");
      for (const Var& var: segment->vars) {
        int index = get_func_index_by_var(module, &var);
        write_u32_leb128_with_reloc(ctx, index, "function index",
                                    RelocType::FuncIndexLEB);
      }
    }
    end_section(ctx);
  }

  if (num_funcs) {
    begin_known_section(ctx, BinarySection::Code, LEB_SECTION_SIZE_GUESS);
    write_u32_leb128(&ctx->stream, num_funcs, "num functions");

    for (size_t i = 0; i < num_funcs; ++i) {
      write_header(ctx, "function body", i);
      const Func* func = module->funcs[i + module->num_func_imports];

      /* TODO(binji): better guess of the size of the function body section */
      const uint32_t leb_size_guess = 1;
      uint32_t body_size_offset =
          write_u32_leb128_space(ctx, leb_size_guess, "func body size (guess)");
      write_func(ctx, module, func);
      write_fixup_u32_leb128_size(ctx, body_size_offset, leb_size_guess,
                                  "FIXUP func body size");
    }
    end_section(ctx);
  }

  if (module->data_segments.size()) {
    begin_known_section(ctx, BinarySection::Data, LEB_SECTION_SIZE_GUESS);
    write_u32_leb128(&ctx->stream, module->data_segments.size(),
                     "num data segments");
    for (size_t i = 0; i < module->data_segments.size(); ++i) {
      const DataSegment* segment = module->data_segments[i];
      write_header(ctx, "data segment header", i);
      int memory_index = get_memory_index_by_var(module, &segment->memory_var);
      write_u32_leb128(&ctx->stream, memory_index, "memory index");
      write_init_expr(ctx, module, segment->offset);
      write_u32_leb128(&ctx->stream, segment->size, "data segment size");
      write_header(ctx, "data segment data", i);
      write_data(&ctx->stream, segment->data, segment->size,
                 "data segment data");
    }
    end_section(ctx);
  }

  if (ctx->options->write_debug_names) {
    std::vector<std::string> index_to_name;

    char desc[100];
    begin_custom_section(ctx, WABT_BINARY_SECTION_NAME, LEB_SECTION_SIZE_GUESS);
    write_u32_leb128(&ctx->stream, 1, "function name type");
    begin_subsection(ctx, "function name subsection", LEB_SECTION_SIZE_GUESS);
    write_u32_leb128(&ctx->stream, module->funcs.size(), "num functions");
    for (size_t i = 0; i < module->funcs.size(); ++i) {
      const Func* func = module->funcs[i];
      write_u32_leb128(&ctx->stream, i, "function index");
      wabt_snprintf(desc, sizeof(desc), "func name %" PRIzd, i);
      write_str(&ctx->stream, func->name.start, func->name.length,
                PrintChars::Yes, desc);
    }
    end_subsection(ctx);

    write_u32_leb128(&ctx->stream, 2, "local name type");

    begin_subsection(ctx, "local name subsection", LEB_SECTION_SIZE_GUESS);
    write_u32_leb128(&ctx->stream, module->funcs.size(), "num functions");
    for (size_t i = 0; i < module->funcs.size(); ++i) {
      const Func* func = module->funcs[i];
      uint32_t num_params = get_num_params(func);
      uint32_t num_locals = func->local_types.size();
      uint32_t num_params_and_locals = get_num_params_and_locals(func);

      write_u32_leb128(&ctx->stream, i, "function index");
      write_u32_leb128(&ctx->stream, num_params_and_locals, "num locals");

      make_type_binding_reverse_mapping(func->decl.sig.param_types,
                                        func->param_bindings, &index_to_name);
      for (size_t j = 0; j < num_params; ++j) {
        const std::string& name = index_to_name[j];
        wabt_snprintf(desc, sizeof(desc), "local name %" PRIzd, j);
        write_u32_leb128(&ctx->stream, j, "local index");
        write_str(&ctx->stream, name.data(), name.length(), PrintChars::Yes,
                  desc);
      }

      make_type_binding_reverse_mapping(func->local_types, func->local_bindings,
                                        &index_to_name);
      for (size_t j = 0; j < num_locals; ++j) {
        const std::string& name = index_to_name[j];
        wabt_snprintf(desc, sizeof(desc), "local name %" PRIzd, num_params + j);
        write_u32_leb128(&ctx->stream, num_params + j, "local index");
        write_str(&ctx->stream, name.data(), name.length(), PrintChars::Yes,
                  desc);
      }
    }
    end_subsection(ctx);
    end_section(ctx);
  }

  if (ctx->options->relocatable) {
    for (size_t i = 0; i < ctx->reloc_sections.size(); i++) {
      write_reloc_section(ctx, &ctx->reloc_sections[i]);
    }
  }

  return ctx->stream.result;
}

Result write_binary_module(Writer* writer,
                           const Module* module,
                           const WriteBinaryOptions* options) {
  Context ctx;
  ctx.options = options;
  ctx.log_stream = options->log_stream;
  init_stream(&ctx.stream, writer, ctx.log_stream);
  return write_module(&ctx, module);
}

}  // namespace wabt
