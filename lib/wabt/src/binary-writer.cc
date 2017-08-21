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

#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "binary.h"
#include "cast.h"
#include "ir.h"
#include "stream.h"
#include "string-view.h"
#include "writer.h"

#define PRINT_HEADER_NO_INDEX -1
#define MAX_U32_LEB128_BYTES 5
#define MAX_U64_LEB128_BYTES 10

namespace wabt {

// TODO(binji): move the LEB128 functions somewhere else.

Offset U32Leb128Length(uint32_t value) {
  uint32_t size = 0;
  do {
    value >>= 7;
    size++;
  } while (value != 0);
  return size;
}

#define LEB128_LOOP_UNTIL(end_cond) \
  do {                              \
    uint8_t byte = value & 0x7f;    \
    value >>= 7;                    \
    if (end_cond) {                 \
      data[length++] = byte;        \
      break;                        \
    } else {                        \
      data[length++] = byte | 0x80; \
    }                               \
  } while (1)

Offset WriteFixedU32Leb128At(Stream* stream,
                             Offset offset,
                             uint32_t value,
                             const char* desc) {
  uint8_t data[MAX_U32_LEB128_BYTES];
  Offset length =
      WriteFixedU32Leb128Raw(data, data + MAX_U32_LEB128_BYTES, value);
  stream->WriteDataAt(offset, data, length, desc);
  return length;
}

void WriteU32Leb128(Stream* stream, uint32_t value, const char* desc) {
  uint8_t data[MAX_U32_LEB128_BYTES];
  Offset length = 0;
  LEB128_LOOP_UNTIL(value == 0);
  stream->WriteData(data, length, desc);
}

void WriteFixedU32Leb128(Stream* stream, uint32_t value, const char* desc) {
  uint8_t data[MAX_U32_LEB128_BYTES];
  Offset length =
      WriteFixedU32Leb128Raw(data, data + MAX_U32_LEB128_BYTES, value);
  stream->WriteData(data, length, desc);
}

/* returns the length of the leb128 */
Offset WriteU32Leb128At(Stream* stream,
                        Offset offset,
                        uint32_t value,
                        const char* desc) {
  uint8_t data[MAX_U32_LEB128_BYTES];
  Offset length = 0;
  LEB128_LOOP_UNTIL(value == 0);
  stream->WriteDataAt(offset, data, length, desc);
  return length;
}

Offset WriteFixedU32Leb128Raw(uint8_t* data, uint8_t* end, uint32_t value) {
  if (end - data < MAX_U32_LEB128_BYTES)
    return 0;
  data[0] = (value & 0x7f) | 0x80;
  data[1] = ((value >> 7) & 0x7f) | 0x80;
  data[2] = ((value >> 14) & 0x7f) | 0x80;
  data[3] = ((value >> 21) & 0x7f) | 0x80;
  data[4] = ((value >> 28) & 0x0f);
  return MAX_U32_LEB128_BYTES;
}

void WriteI32Leb128(Stream* stream, int32_t value, const char* desc) {
  uint8_t data[MAX_U32_LEB128_BYTES];
  Offset length = 0;
  if (value < 0)
    LEB128_LOOP_UNTIL(value == -1 && (byte & 0x40));
  else
    LEB128_LOOP_UNTIL(value == 0 && !(byte & 0x40));

  stream->WriteData(data, length, desc);
}

static void WriteI64Leb128(Stream* stream, int64_t value, const char* desc) {
  uint8_t data[MAX_U64_LEB128_BYTES];
  Offset length = 0;
  if (value < 0)
    LEB128_LOOP_UNTIL(value == -1 && (byte & 0x40));
  else
    LEB128_LOOP_UNTIL(value == 0 && !(byte & 0x40));

  stream->WriteData(data, length, desc);
}


#undef LEB128_LOOP_UNTIL

void WriteStr(Stream* stream,
              string_view s,
              const char* desc,
              PrintChars print_chars) {
  WriteU32Leb128(stream, s.length(), "string length");
  stream->WriteData(s.data(), s.length(), desc, print_chars);
}

void WriteOpcode(Stream* stream, Opcode opcode) {
  if (opcode.HasPrefix()) {
    stream->WriteU8(opcode.GetPrefix(), "prefix");
    WriteU32Leb128(stream, opcode.GetCode(), opcode.GetName());
  } else {
    stream->WriteU8(opcode.GetCode(), opcode.GetName());
  }
}

void WriteType(Stream* stream, Type type) {
  WriteI32Leb128Enum(stream, type, GetTypeName(type));
}

void WriteLimits(Stream* stream, const Limits* limits) {
  uint32_t flags = limits->has_max ? WABT_BINARY_LIMITS_HAS_MAX_FLAG : 0;
  WriteU32Leb128(stream, flags, "limits: flags");
  WriteU32Leb128(stream, limits->initial, "limits: initial");
  if (limits->has_max)
    WriteU32Leb128(stream, limits->max, "limits: max");
}

void WriteDebugName(Stream* stream, string_view name, const char* desc) {
  string_view stripped_name = name;
  if (!stripped_name.empty()) {
    // Strip leading $ from name
    assert(stripped_name.front() == '$');
    stripped_name.remove_prefix(1);
  }
  WriteStr(stream, stripped_name, desc, PrintChars::Yes);
}

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

class BinaryWriter {
  WABT_DISALLOW_COPY_AND_ASSIGN(BinaryWriter);

 public:
  BinaryWriter(Writer*, const WriteBinaryOptions* options);

  Result WriteModule(const Module* module);

 private:
  void WriteHeader(const char* name, int index);
  Offset WriteU32Leb128Space(Offset leb_size_guess, const char* desc);
  Offset WriteFixupU32Leb128Size(Offset offset,
                                 Offset leb_size_guess,
                                 const char* desc);
  void BeginKnownSection(BinarySection section_code, size_t leb_size_guess);
  void BeginCustomSection(const char* name, size_t leb_size_guess);
  void EndSection();
  void BeginSubsection(const char* name, size_t leb_size_guess);
  void EndSubsection();
  Index GetLabelVarDepth(const Var* var);
  Index GetExceptVarDepth(const Var* var);
  Index GetLocalIndex(const Func* func, const Var& var);
  void AddReloc(RelocType reloc_type, Index index);
  void WriteU32Leb128WithReloc(Index index,
                               const char* desc,
                               RelocType reloc_type);
  void WriteExpr(const Module* module, const Func* func, const Expr* expr);
  void WriteExprList(const Module* module,
                     const Func* func,
                     const ExprList& exprs);
  void WriteInitExpr(const Module* module, const ExprList& expr);
  void WriteFuncLocals(const Module* module,
                       const Func* func,
                       const TypeVector& local_types);
  void WriteFunc(const Module* module, const Func* func);
  void WriteTable(const Table* table);
  void WriteMemory(const Memory* memory);
  void WriteGlobalHeader(const Global* global);
  void WriteExceptType(const TypeVector* except_types);
  void WriteRelocSection(const RelocSection* reloc_section);

  Stream stream_;
  const WriteBinaryOptions* options_ = nullptr;

  std::vector<RelocSection> reloc_sections_;
  RelocSection* current_reloc_section_ = nullptr;

  size_t last_section_offset_ = 0;
  size_t last_section_leb_size_guess_ = 0;
  BinarySection last_section_type_ = BinarySection::Invalid;
  size_t last_section_payload_offset_ = 0;

  size_t last_subsection_offset_ = 0;
  size_t last_subsection_leb_size_guess_ = 0;
  size_t last_subsection_payload_offset_ = 0;
};

static uint8_t log2_u32(uint32_t x) {
  uint8_t result = 0;
  while (x > 1) {
    x >>= 1;
    result++;
  }
  return result;
}

BinaryWriter::BinaryWriter(Writer* writer, const WriteBinaryOptions* options)
    : stream_(writer, options->log_stream), options_(options) {}

void BinaryWriter::WriteHeader(const char* name, int index) {
  if (stream_.has_log_stream()) {
    if (index == PRINT_HEADER_NO_INDEX) {
      stream_.log_stream().Writef("; %s\n", name);
    } else {
      stream_.log_stream().Writef("; %s %d\n", name, index);
    }
  }
}

/* returns offset of leb128 */
Offset BinaryWriter::WriteU32Leb128Space(Offset leb_size_guess,
                                         const char* desc) {
  assert(leb_size_guess <= MAX_U32_LEB128_BYTES);
  uint8_t data[MAX_U32_LEB128_BYTES] = {0};
  Offset result = stream_.offset();
  Offset bytes_to_write =
      options_->canonicalize_lebs ? leb_size_guess : MAX_U32_LEB128_BYTES;
  stream_.WriteData(data, bytes_to_write, desc);
  return result;
}

Offset BinaryWriter::WriteFixupU32Leb128Size(Offset offset,
                                             Offset leb_size_guess,
                                             const char* desc) {
  if (options_->canonicalize_lebs) {
    Offset size = stream_.offset() - offset - leb_size_guess;
    Offset leb_size = U32Leb128Length(size);
    Offset delta = leb_size - leb_size_guess;
    if (delta != 0) {
      Offset src_offset = offset + leb_size_guess;
      Offset dst_offset = offset + leb_size;
      stream_.MoveData(dst_offset, src_offset, size);
    }
    WriteU32Leb128At(&stream_, offset, size, desc);
    stream_.AddOffset(delta);
    return delta;
  } else {
    Offset size = stream_.offset() - offset - MAX_U32_LEB128_BYTES;
    WriteFixedU32Leb128At(&stream_, offset, size, desc);
    return 0;
  }
}

static void write_inline_signature_type(Stream* stream,
                                        const BlockSignature& sig) {
  if (sig.size() == 0) {
    WriteType(stream, Type::Void);
  } else if (sig.size() == 1) {
    WriteType(stream, sig[0]);
  } else {
    /* this is currently unrepresentable */
    stream->WriteU8(0xff, "INVALID INLINE SIGNATURE");
  }
}

void BinaryWriter::BeginKnownSection(BinarySection section_code,
                                     size_t leb_size_guess) {
  assert(last_section_leb_size_guess_ == 0);
  char desc[100];
  wabt_snprintf(desc, sizeof(desc), "section \"%s\" (%u)",
                GetSectionName(section_code),
                static_cast<unsigned>(section_code));
  WriteHeader(desc, PRINT_HEADER_NO_INDEX);
  stream_.WriteU8Enum(section_code, "section code");
  last_section_type_ = section_code;
  last_section_leb_size_guess_ = leb_size_guess;
  last_section_offset_ =
      WriteU32Leb128Space(leb_size_guess, "section size (guess)");
  last_section_payload_offset_ = stream_.offset();
}

void BinaryWriter::BeginCustomSection(const char* name, size_t leb_size_guess) {
  assert(last_section_leb_size_guess_ == 0);
  char desc[100];
  wabt_snprintf(desc, sizeof(desc), "section \"%s\"", name);
  WriteHeader(desc, PRINT_HEADER_NO_INDEX);
  stream_.WriteU8Enum(BinarySection::Custom, "custom section code");
  last_section_type_ = BinarySection::Custom;
  last_section_leb_size_guess_ = leb_size_guess;
  last_section_offset_ =
      WriteU32Leb128Space(leb_size_guess, "section size (guess)");
  last_section_payload_offset_ = stream_.offset();
  WriteStr(&stream_, name, "custom section name", PrintChars::Yes);
}

void BinaryWriter::EndSection() {
  assert(last_section_leb_size_guess_ != 0);
  Offset delta = WriteFixupU32Leb128Size(
      last_section_offset_, last_section_leb_size_guess_, "FIXUP section size");
  if (current_reloc_section_ && delta != 0) {
    for (Reloc& reloc: current_reloc_section_->relocations)
      reloc.offset += delta;
  }
  last_section_leb_size_guess_ = 0;
}

void BinaryWriter::BeginSubsection(const char* name, size_t leb_size_guess) {
  assert(last_subsection_leb_size_guess_ == 0);
  last_subsection_leb_size_guess_ = leb_size_guess;
  last_subsection_offset_ =
      WriteU32Leb128Space(leb_size_guess, "subsection size (guess)");
  last_subsection_payload_offset_ = stream_.offset();
}

void BinaryWriter::EndSubsection() {
  assert(last_subsection_leb_size_guess_ != 0);
  WriteFixupU32Leb128Size(last_subsection_offset_,
                          last_subsection_leb_size_guess_,
                          "FIXUP subsection size");
  last_subsection_leb_size_guess_ = 0;
}

Index BinaryWriter::GetLabelVarDepth(const Var* var) {
  return var->index();
}

Index BinaryWriter::GetExceptVarDepth(const Var* var) {
  return var->index();
}

void BinaryWriter::AddReloc(RelocType reloc_type, Index index) {
  // Add a new reloc section if needed
  if (!current_reloc_section_ ||
      current_reloc_section_->section_code != last_section_type_) {
    reloc_sections_.emplace_back(GetSectionName(last_section_type_),
                                 last_section_type_);
    current_reloc_section_ = &reloc_sections_.back();
  }

  // Add a new relocation to the curent reloc section
  size_t offset = stream_.offset() - last_section_payload_offset_;
  current_reloc_section_->relocations.emplace_back(reloc_type, offset, index);
}

void BinaryWriter::WriteU32Leb128WithReloc(Index index,
                                           const char* desc,
                                           RelocType reloc_type) {
  if (options_->relocatable) {
    AddReloc(reloc_type, index);
    WriteFixedU32Leb128(&stream_, index, desc);
  } else {
    WriteU32Leb128(&stream_, index, desc);
  }
}

Index BinaryWriter::GetLocalIndex(const Func* func, const Var& var) {
  // func can be nullptr when using get_local/set_local/tee_local in an
  // init_expr.
  if (func) {
    return func->GetLocalIndex(var);
  } else if (var.is_index()) {
    return var.index();
  } else {
    return kInvalidIndex;
  }
}

void BinaryWriter::WriteExpr(const Module* module,
                             const Func* func,
                             const Expr* expr) {
  switch (expr->type) {
    case ExprType::Binary:
      WriteOpcode(&stream_, cast<BinaryExpr>(expr)->opcode);
      break;
    case ExprType::Block:
      WriteOpcode(&stream_, Opcode::Block);
      write_inline_signature_type(&stream_, cast<BlockExpr>(expr)->block->sig);
      WriteExprList(module, func, cast<BlockExpr>(expr)->block->exprs);
      WriteOpcode(&stream_, Opcode::End);
      break;
    case ExprType::Br:
      WriteOpcode(&stream_, Opcode::Br);
      WriteU32Leb128(&stream_, GetLabelVarDepth(&cast<BrExpr>(expr)->var),
                     "break depth");
      break;
    case ExprType::BrIf:
      WriteOpcode(&stream_, Opcode::BrIf);
      WriteU32Leb128(&stream_, GetLabelVarDepth(&cast<BrIfExpr>(expr)->var),
                     "break depth");
      break;
    case ExprType::BrTable: {
      auto br_table_expr = cast<BrTableExpr>(expr);
      WriteOpcode(&stream_, Opcode::BrTable);
      WriteU32Leb128(&stream_, br_table_expr->targets->size(), "num targets");
      Index depth;
      for (const Var& var : *br_table_expr->targets) {
        depth = GetLabelVarDepth(&var);
        WriteU32Leb128(&stream_, depth, "break depth");
      }
      depth = GetLabelVarDepth(&br_table_expr->default_target);
      WriteU32Leb128(&stream_, depth, "break depth for default");
      break;
    }
    case ExprType::Call: {
      Index index = module->GetFuncIndex(cast<CallExpr>(expr)->var);
      WriteOpcode(&stream_, Opcode::Call);
      WriteU32Leb128WithReloc(index, "function index", RelocType::FuncIndexLEB);
      break;
    }
    case ExprType::CallIndirect: {
      Index index = module->GetFuncTypeIndex(cast<CallIndirectExpr>(expr)->var);
      WriteOpcode(&stream_, Opcode::CallIndirect);
      WriteU32Leb128WithReloc(index, "signature index",
                              RelocType::TypeIndexLEB);
      WriteU32Leb128(&stream_, 0, "call_indirect reserved");
      break;
    }
    case ExprType::Compare:
      WriteOpcode(&stream_, cast<CompareExpr>(expr)->opcode);
      break;
    case ExprType::Const: {
      const Const& const_ = cast<ConstExpr>(expr)->const_;
      switch (const_.type) {
        case Type::I32: {
          WriteOpcode(&stream_, Opcode::I32Const);
          WriteI32Leb128(&stream_, const_.u32, "i32 literal");
          break;
        }
        case Type::I64:
          WriteOpcode(&stream_, Opcode::I64Const);
          WriteI64Leb128(&stream_, const_.u64, "i64 literal");
          break;
        case Type::F32:
          WriteOpcode(&stream_, Opcode::F32Const);
          stream_.WriteU32(const_.f32_bits, "f32 literal");
          break;
        case Type::F64:
          WriteOpcode(&stream_, Opcode::F64Const);
          stream_.WriteU64(const_.f64_bits, "f64 literal");
          break;
        default:
          assert(0);
      }
      break;
    }
    case ExprType::Convert:
      WriteOpcode(&stream_, cast<ConvertExpr>(expr)->opcode);
      break;
    case ExprType::CurrentMemory:
      WriteOpcode(&stream_, Opcode::CurrentMemory);
      WriteU32Leb128(&stream_, 0, "current_memory reserved");
      break;
    case ExprType::Drop:
      WriteOpcode(&stream_, Opcode::Drop);
      break;
    case ExprType::GetGlobal: {
      Index index = module->GetGlobalIndex(cast<GetGlobalExpr>(expr)->var);
      WriteOpcode(&stream_, Opcode::GetGlobal);
      WriteU32Leb128WithReloc(index, "global index", RelocType::GlobalIndexLEB);
      break;
    }
    case ExprType::GetLocal: {
      Index index = GetLocalIndex(func, cast<GetLocalExpr>(expr)->var);
      WriteOpcode(&stream_, Opcode::GetLocal);
      WriteU32Leb128(&stream_, index, "local index");
      break;
    }
    case ExprType::GrowMemory:
      WriteOpcode(&stream_, Opcode::GrowMemory);
      WriteU32Leb128(&stream_, 0, "grow_memory reserved");
      break;
    case ExprType::If: {
      auto if_expr = cast<IfExpr>(expr);
      WriteOpcode(&stream_, Opcode::If);
      write_inline_signature_type(&stream_, if_expr->true_->sig);
      WriteExprList(module, func, if_expr->true_->exprs);
      if (!if_expr->false_.empty()) {
        WriteOpcode(&stream_, Opcode::Else);
        WriteExprList(module, func, if_expr->false_);
      }
      WriteOpcode(&stream_, Opcode::End);
      break;
    }
    case ExprType::Load: {
      auto load_expr = cast<LoadExpr>(expr);
      WriteOpcode(&stream_, load_expr->opcode);
      Address align = load_expr->opcode.GetAlignment(load_expr->align);
      stream_.WriteU8(log2_u32(align), "alignment");
      WriteU32Leb128(&stream_, load_expr->offset, "load offset");
      break;
    }
    case ExprType::Loop:
      WriteOpcode(&stream_, Opcode::Loop);
      write_inline_signature_type(&stream_, cast<LoopExpr>(expr)->block->sig);
      WriteExprList(module, func, cast<LoopExpr>(expr)->block->exprs);
      WriteOpcode(&stream_, Opcode::End);
      break;
    case ExprType::Nop:
      WriteOpcode(&stream_, Opcode::Nop);
      break;
    case ExprType::Rethrow:
      WriteOpcode(&stream_, Opcode::Rethrow);
      WriteU32Leb128(&stream_, GetLabelVarDepth(&cast<RethrowExpr>(expr)->var),
                     "rethrow depth");
      break;
    case ExprType::Return:
      WriteOpcode(&stream_, Opcode::Return);
      break;
    case ExprType::Select:
      WriteOpcode(&stream_, Opcode::Select);
      break;
    case ExprType::SetGlobal: {
      Index index = module->GetGlobalIndex(cast<SetGlobalExpr>(expr)->var);
      WriteOpcode(&stream_, Opcode::SetGlobal);
      WriteU32Leb128WithReloc(index, "global index", RelocType::GlobalIndexLEB);
      break;
    }
    case ExprType::SetLocal: {
      Index index = GetLocalIndex(func, cast<SetLocalExpr>(expr)->var);
      WriteOpcode(&stream_, Opcode::SetLocal);
      WriteU32Leb128(&stream_, index, "local index");
      break;
    }
    case ExprType::Store: {
      auto store_expr = cast<StoreExpr>(expr);
      WriteOpcode(&stream_, store_expr->opcode);
      Address align = store_expr->opcode.GetAlignment(store_expr->align);
      stream_.WriteU8(log2_u32(align), "alignment");
      WriteU32Leb128(&stream_, store_expr->offset, "store offset");
      break;
    }
    case ExprType::TeeLocal: {
      Index index = GetLocalIndex(func, cast<TeeLocalExpr>(expr)->var);
      WriteOpcode(&stream_, Opcode::TeeLocal);
      WriteU32Leb128(&stream_, index, "local index");
      break;
    }
    case ExprType::Throw:
      WriteOpcode(&stream_, Opcode::Throw);
      WriteU32Leb128(&stream_, GetExceptVarDepth(&cast<ThrowExpr>(expr)->var),
                     "throw exception");
      break;
    case ExprType::TryBlock: {
      auto try_expr = cast<TryExpr>(expr);
      WriteOpcode(&stream_, Opcode::Try);
      write_inline_signature_type(&stream_, try_expr->block->sig);
      WriteExprList(module, func, try_expr->block->exprs);
      for (Catch* catch_ : try_expr->catches) {
        if (catch_->IsCatchAll()) {
          WriteOpcode(&stream_, Opcode::CatchAll);
        } else {
          WriteOpcode(&stream_, Opcode::Catch);
          WriteU32Leb128(&stream_, GetExceptVarDepth(&catch_->var),
                         "catch exception");
        }
        WriteExprList(module, func, catch_->exprs);
      }
      WriteOpcode(&stream_, Opcode::End);
      break;
    }
    case ExprType::Unary:
      WriteOpcode(&stream_, cast<UnaryExpr>(expr)->opcode);
      break;
    case ExprType::Unreachable:
      WriteOpcode(&stream_, Opcode::Unreachable);
      break;
  }
}

void BinaryWriter::WriteExprList(const Module* module,
                                 const Func* func,
                                 const ExprList& exprs) {
  for (const Expr& expr : exprs)
    WriteExpr(module, func, &expr);
}

void BinaryWriter::WriteInitExpr(const Module* module, const ExprList& expr) {
  WriteExprList(module, nullptr, expr);
  WriteOpcode(&stream_, Opcode::End);
}

void BinaryWriter::WriteFuncLocals(const Module* module,
                                   const Func* func,
                                   const TypeVector& local_types) {
  if (local_types.size() == 0) {
    WriteU32Leb128(&stream_, 0, "local decl count");
    return;
  }

  Index num_params = func->GetNumParams();

#define FIRST_LOCAL_INDEX (num_params)
#define LAST_LOCAL_INDEX (num_params + local_types.size())
#define GET_LOCAL_TYPE(x) (local_types[x - num_params])

  /* loop through once to count the number of local declaration runs */
  Type current_type = GET_LOCAL_TYPE(FIRST_LOCAL_INDEX);
  Index local_decl_count = 1;
  for (Index i = FIRST_LOCAL_INDEX + 1; i < LAST_LOCAL_INDEX; ++i) {
    Type type = GET_LOCAL_TYPE(i);
    if (current_type != type) {
      local_decl_count++;
      current_type = type;
    }
  }

  /* loop through again to write everything out */
  WriteU32Leb128(&stream_, local_decl_count, "local decl count");
  current_type = GET_LOCAL_TYPE(FIRST_LOCAL_INDEX);
  Index local_type_count = 1;
  for (Index i = FIRST_LOCAL_INDEX + 1; i <= LAST_LOCAL_INDEX; ++i) {
    /* loop through an extra time to catch the final type transition */
    Type type = i == LAST_LOCAL_INDEX ? Type::Void : GET_LOCAL_TYPE(i);
    if (current_type == type) {
      local_type_count++;
    } else {
      WriteU32Leb128(&stream_, local_type_count, "local type count");
      WriteType(&stream_, current_type);
      local_type_count = 1;
      current_type = type;
    }
  }
}

void BinaryWriter::WriteFunc(const Module* module, const Func* func) {
  WriteFuncLocals(module, func, func->local_types);
  WriteExprList(module, func, func->exprs);
  WriteOpcode(&stream_, Opcode::End);
}

void BinaryWriter::WriteTable(const Table* table) {
  WriteType(&stream_, Type::Anyfunc);
  WriteLimits(&stream_, &table->elem_limits);
}

void BinaryWriter::WriteMemory(const Memory* memory) {
  WriteLimits(&stream_, &memory->page_limits);
}

void BinaryWriter::WriteGlobalHeader(const Global* global) {
  WriteType(&stream_, global->type);
  stream_.WriteU8(global->mutable_, "global mutability");
}

void BinaryWriter::WriteExceptType(const TypeVector* except_types) {
  WriteU32Leb128(&stream_, except_types->size(), "exception type count");
  for (Type ty : *except_types)
    WriteType(&stream_, ty);
}

void BinaryWriter::WriteRelocSection(const RelocSection* reloc_section) {
  char section_name[128];
  wabt_snprintf(section_name, sizeof(section_name), "%s.%s",
                WABT_BINARY_SECTION_RELOC, reloc_section->name);
  BeginCustomSection(section_name, LEB_SECTION_SIZE_GUESS);
  WriteU32Leb128Enum(&stream_, reloc_section->section_code,
                     "reloc section type");
  const std::vector<Reloc>& relocs = reloc_section->relocations;
  WriteU32Leb128(&stream_, relocs.size(), "num relocs");

  for (const Reloc& reloc : relocs) {
    WriteU32Leb128Enum(&stream_, reloc.type, "reloc type");
    WriteU32Leb128(&stream_, reloc.offset, "reloc offset");
    WriteU32Leb128(&stream_, reloc.index, "reloc index");
    switch (reloc.type) {
      case RelocType::GlobalAddressLEB:
      case RelocType::GlobalAddressSLEB:
      case RelocType::GlobalAddressI32:
        WriteU32Leb128(&stream_, reloc.addend, "reloc addend");
        break;
      default:
        break;
    }
  }

  EndSection();
}

Result BinaryWriter::WriteModule(const Module* module) {
  stream_.WriteU32(WABT_BINARY_MAGIC, "WASM_BINARY_MAGIC");
  stream_.WriteU32(WABT_BINARY_VERSION, "WASM_BINARY_VERSION");

  if (module->func_types.size()) {
    BeginKnownSection(BinarySection::Type, LEB_SECTION_SIZE_GUESS);
    WriteU32Leb128(&stream_, module->func_types.size(), "num types");
    for (size_t i = 0; i < module->func_types.size(); ++i) {
      const FuncType* func_type = module->func_types[i];
      const FuncSignature* sig = &func_type->sig;
      WriteHeader("type", i);
      WriteType(&stream_, Type::Func);

      Index num_params = sig->param_types.size();
      Index num_results = sig->result_types.size();
      WriteU32Leb128(&stream_, num_params, "num params");
      for (size_t j = 0; j < num_params; ++j)
        WriteType(&stream_, sig->param_types[j]);

      WriteU32Leb128(&stream_, num_results, "num results");
      for (size_t j = 0; j < num_results; ++j)
        WriteType(&stream_, sig->result_types[j]);
    }
    EndSection();
  }

  if (module->imports.size()) {
    BeginKnownSection(BinarySection::Import, LEB_SECTION_SIZE_GUESS);
    WriteU32Leb128(&stream_, module->imports.size(), "num imports");

    for (size_t i = 0; i < module->imports.size(); ++i) {
      const Import* import = module->imports[i];
      WriteHeader("import header", i);
      WriteStr(&stream_, import->module_name, "import module name",
               PrintChars::Yes);
      WriteStr(&stream_, import->field_name, "import field name",
               PrintChars::Yes);
      stream_.WriteU8Enum(import->kind, "import kind");
      switch (import->kind) {
        case ExternalKind::Func:
          WriteU32Leb128(&stream_, module->GetFuncTypeIndex(import->func->decl),
                         "import signature index");
          break;
        case ExternalKind::Table:
          WriteTable(import->table);
          break;
        case ExternalKind::Memory:
          WriteMemory(import->memory);
          break;
        case ExternalKind::Global:
          WriteGlobalHeader(import->global);
          break;
        case ExternalKind::Except:
          WriteExceptType(&import->except->sig);
          break;
      }
    }
    EndSection();
  }

  assert(module->funcs.size() >= module->num_func_imports);
  Index num_funcs = module->funcs.size() - module->num_func_imports;
  if (num_funcs) {
    BeginKnownSection(BinarySection::Function, LEB_SECTION_SIZE_GUESS);
    WriteU32Leb128(&stream_, num_funcs, "num functions");

    for (size_t i = 0; i < num_funcs; ++i) {
      const Func* func = module->funcs[i + module->num_func_imports];
      char desc[100];
      wabt_snprintf(desc, sizeof(desc), "function %" PRIzd " signature index",
                    i);
      WriteU32Leb128(&stream_, module->GetFuncTypeIndex(func->decl), desc);
    }
    EndSection();
  }

  assert(module->tables.size() >= module->num_table_imports);
  Index num_tables = module->tables.size() - module->num_table_imports;
  if (num_tables) {
    BeginKnownSection(BinarySection::Table, LEB_SECTION_SIZE_GUESS);
    WriteU32Leb128(&stream_, num_tables, "num tables");
    for (size_t i = 0; i < num_tables; ++i) {
      const Table* table = module->tables[i + module->num_table_imports];
      WriteHeader("table", i);
      WriteTable(table);
    }
    EndSection();
  }

  assert(module->memories.size() >= module->num_memory_imports);
  Index num_memories = module->memories.size() - module->num_memory_imports;
  if (num_memories) {
    BeginKnownSection(BinarySection::Memory, LEB_SECTION_SIZE_GUESS);
    WriteU32Leb128(&stream_, num_memories, "num memories");
    for (size_t i = 0; i < num_memories; ++i) {
      const Memory* memory = module->memories[i + module->num_memory_imports];
      WriteHeader("memory", i);
      WriteMemory(memory);
    }
    EndSection();
  }

  assert(module->globals.size() >= module->num_global_imports);
  Index num_globals = module->globals.size() - module->num_global_imports;
  if (num_globals) {
    BeginKnownSection(BinarySection::Global, LEB_SECTION_SIZE_GUESS);
    WriteU32Leb128(&stream_, num_globals, "num globals");

    for (size_t i = 0; i < num_globals; ++i) {
      const Global* global = module->globals[i + module->num_global_imports];
      WriteGlobalHeader(global);
      WriteInitExpr(module, global->init_expr);
    }
    EndSection();
  }

  if (module->exports.size()) {
    BeginKnownSection(BinarySection::Export, LEB_SECTION_SIZE_GUESS);
    WriteU32Leb128(&stream_, module->exports.size(), "num exports");

    for (const Export* export_ : module->exports) {
      WriteStr(&stream_, export_->name, "export name", PrintChars::Yes);
      stream_.WriteU8Enum(export_->kind, "export kind");
      switch (export_->kind) {
        case ExternalKind::Func: {
          Index index = module->GetFuncIndex(export_->var);
          WriteU32Leb128(&stream_, index, "export func index");
          break;
        }
        case ExternalKind::Table: {
          Index index = module->GetTableIndex(export_->var);
          WriteU32Leb128(&stream_, index, "export table index");
          break;
        }
        case ExternalKind::Memory: {
          Index index = module->GetMemoryIndex(export_->var);
          WriteU32Leb128(&stream_, index, "export memory index");
          break;
        }
        case ExternalKind::Global: {
          Index index = module->GetGlobalIndex(export_->var);
          WriteU32Leb128(&stream_, index, "export global index");
          break;
        }
        case ExternalKind::Except: {
          Index index = module->GetExceptIndex(export_->var);
          WriteU32Leb128(&stream_, index, "export exception index");
          break;
        }
      }
    }
    EndSection();
  }

  if (module->start) {
    Index start_func_index = module->GetFuncIndex(*module->start);
    if (start_func_index != kInvalidIndex) {
      BeginKnownSection(BinarySection::Start, LEB_SECTION_SIZE_GUESS);
      WriteU32Leb128(&stream_, start_func_index, "start func index");
      EndSection();
    }
  }

  if (module->elem_segments.size()) {
    BeginKnownSection(BinarySection::Elem, LEB_SECTION_SIZE_GUESS);
    WriteU32Leb128(&stream_, module->elem_segments.size(), "num elem segments");
    for (size_t i = 0; i < module->elem_segments.size(); ++i) {
      ElemSegment* segment = module->elem_segments[i];
      Index table_index = module->GetTableIndex(segment->table_var);
      WriteHeader("elem segment header", i);
      WriteU32Leb128(&stream_, table_index, "table index");
      WriteInitExpr(module, segment->offset);
      WriteU32Leb128(&stream_, segment->vars.size(), "num function indices");
      for (const Var& var : segment->vars) {
        Index index = module->GetFuncIndex(var);
        WriteU32Leb128WithReloc(index, "function index",
                                RelocType::FuncIndexLEB);
      }
    }
    EndSection();
  }

  assert(module->excepts.size() >= module->num_except_imports);
  Index num_exceptions = module->excepts.size() - module->num_except_imports;
  if (num_exceptions) {
    BeginCustomSection("exception", LEB_SECTION_SIZE_GUESS);
    WriteU32Leb128(&stream_, num_exceptions, "exception count");
    for (Index i = module->num_except_imports; i < num_exceptions; ++i) {
      WriteExceptType(&module->excepts[i]->sig);
    }
    EndSection();
  }

  if (num_funcs) {
    BeginKnownSection(BinarySection::Code, LEB_SECTION_SIZE_GUESS);
    WriteU32Leb128(&stream_, num_funcs, "num functions");

    for (size_t i = 0; i < num_funcs; ++i) {
      WriteHeader("function body", i);
      const Func* func = module->funcs[i + module->num_func_imports];

      /* TODO(binji): better guess of the size of the function body section */
      const Offset leb_size_guess = 1;
      Offset body_size_offset =
          WriteU32Leb128Space(leb_size_guess, "func body size (guess)");
      WriteFunc(module, func);
      WriteFixupU32Leb128Size(body_size_offset, leb_size_guess,
                              "FIXUP func body size");
    }
    EndSection();
  }

  if (module->data_segments.size()) {
    BeginKnownSection(BinarySection::Data, LEB_SECTION_SIZE_GUESS);
    WriteU32Leb128(&stream_, module->data_segments.size(), "num data segments");
    for (size_t i = 0; i < module->data_segments.size(); ++i) {
      const DataSegment* segment = module->data_segments[i];
      WriteHeader("data segment header", i);
      Index memory_index = module->GetMemoryIndex(segment->memory_var);
      WriteU32Leb128(&stream_, memory_index, "memory index");
      WriteInitExpr(module, segment->offset);
      WriteU32Leb128(&stream_, segment->data.size(), "data segment size");
      WriteHeader("data segment data", i);
      stream_.WriteData(segment->data, "data segment data");
    }
    EndSection();
  }

  if (options_->write_debug_names) {
    std::vector<std::string> index_to_name;

    char desc[100];
    BeginCustomSection(WABT_BINARY_SECTION_NAME, LEB_SECTION_SIZE_GUESS);

    size_t named_functions = 0;
    for (const Func* func : module->funcs) {
      if (!func->name.empty())
        named_functions++;
    }

    if (named_functions > 0) {
      WriteU32Leb128(&stream_, 1, "function name type");
      BeginSubsection("function name subsection", LEB_SECTION_SIZE_GUESS);

      WriteU32Leb128(&stream_, named_functions, "num functions");
      for (size_t i = 0; i < module->funcs.size(); ++i) {
        const Func* func = module->funcs[i];
        if (func->name.empty())
          continue;
        WriteU32Leb128(&stream_, i, "function index");
        wabt_snprintf(desc, sizeof(desc), "func name %" PRIzd, i);
        WriteDebugName(&stream_, func->name, desc);
      }
      EndSubsection();
    }

    WriteU32Leb128(&stream_, 2, "local name type");

    BeginSubsection("local name subsection", LEB_SECTION_SIZE_GUESS);
    WriteU32Leb128(&stream_, module->funcs.size(), "num functions");
    for (size_t i = 0; i < module->funcs.size(); ++i) {
      const Func* func = module->funcs[i];
      Index num_params = func->GetNumParams();
      Index num_locals = func->local_types.size();
      Index num_params_and_locals = func->GetNumParamsAndLocals();

      WriteU32Leb128(&stream_, i, "function index");
      WriteU32Leb128(&stream_, num_params_and_locals, "num locals");

      MakeTypeBindingReverseMapping(func->decl.sig.param_types,
                                    func->param_bindings, &index_to_name);
      for (size_t j = 0; j < num_params; ++j) {
        const std::string& name = index_to_name[j];
        wabt_snprintf(desc, sizeof(desc), "local name %" PRIzd, j);
        WriteU32Leb128(&stream_, j, "local index");
        WriteDebugName(&stream_, name, desc);
      }

      MakeTypeBindingReverseMapping(func->local_types, func->local_bindings,
                                    &index_to_name);
      for (size_t j = 0; j < num_locals; ++j) {
        const std::string& name = index_to_name[j];
        wabt_snprintf(desc, sizeof(desc), "local name %" PRIzd, num_params + j);
        WriteU32Leb128(&stream_, num_params + j, "local index");
        WriteDebugName(&stream_, name, desc);
      }
    }
    EndSubsection();
    EndSection();
  }

  if (options_->relocatable) {
    for (RelocSection& section : reloc_sections_) {
      WriteRelocSection(&section);
    }
  }


  return stream_.result();
}

}  // end anonymous namespace

Result WriteBinaryModule(Writer* writer,
                         const Module* module,
                         const WriteBinaryOptions* options) {
  BinaryWriter binary_writer(writer, options);
  return binary_writer.WriteModule(module);
}

}  // namespace wabt
