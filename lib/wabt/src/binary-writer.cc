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

#include "src/binary-writer.h"

#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "config.h"

#include "src/binary.h"
#include "src/cast.h"
#include "src/ir.h"
#include "src/leb128.h"
#include "src/stream.h"
#include "src/string-view.h"

#define PRINT_HEADER_NO_INDEX -1
#define MAX_U32_LEB128_BYTES 5

namespace wabt {

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
  WriteS32Leb128(stream, type, GetTypeName(type));
}

void WriteLimits(Stream* stream, const Limits* limits) {
  uint32_t flags = limits->has_max ? WABT_BINARY_LIMITS_HAS_MAX_FLAG : 0;
  flags |= limits->is_shared ? WABT_BINARY_LIMITS_IS_SHARED_FLAG : 0;
  WriteU32Leb128(stream, flags, "limits: flags");
  WriteU32Leb128(stream, limits->initial, "limits: initial");
  if (limits->has_max) {
    WriteU32Leb128(stream, limits->max, "limits: max");
  }
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
  RelocSection(const char* name, Index index)
      : name(name), section_index(index) {}

  const char* name;
  Index section_index;
  std::vector<Reloc> relocations;
};

struct Symbol {
  Index symbol_index;
  SymbolType type;
  Index element_index;
};

class BinaryWriter {
  WABT_DISALLOW_COPY_AND_ASSIGN(BinaryWriter);

 public:
  BinaryWriter(Stream*,
               const WriteBinaryOptions& options,
               const Module* module);

  Result WriteModule();

 private:
  void WriteHeader(const char* name, int index);
  Offset WriteU32Leb128Space(Offset leb_size_guess, const char* desc);
  Offset WriteFixupU32Leb128Size(Offset offset,
                                 Offset leb_size_guess,
                                 const char* desc);
  void BeginKnownSection(BinarySection section_code);
  void BeginCustomSection(const char* name);
  void WriteSectionHeader(const char* desc, BinarySection section_code);
  void EndSection();
  void BeginSubsection(const char* name);
  void EndSubsection();
  Index GetLabelVarDepth(const Var* var);
  Index GetExceptVarDepth(const Var* var);
  Index GetLocalIndex(const Func* func, const Var& var);
  Index GetSymbolIndex(RelocType reloc_type, Index index);
  void AddReloc(RelocType reloc_type, Index index);
  void WriteBlockDecl(const BlockDeclaration& decl);
  void WriteU32Leb128WithReloc(Index index,
                               const char* desc,
                               RelocType reloc_type);
  template <typename T>
  void WriteLoadStoreExpr(const Func* func, const Expr* expr, const char* desc);
  void WriteExpr(const Func* func, const Expr* expr);
  void WriteExprList(const Func* func, const ExprList& exprs);
  void WriteInitExpr(const ExprList& expr);
  void WriteFuncLocals(const Func* func, const LocalTypes& local_types);
  void WriteFunc(const Func* func);
  void WriteTable(const Table* table);
  void WriteMemory(const Memory* memory);
  void WriteGlobalHeader(const Global* global);
  void WriteExceptType(const TypeVector* except_types);
  void WriteRelocSection(const RelocSection* reloc_section);
  void WriteLinkingSection();

  Stream* stream_;
  const WriteBinaryOptions& options_;
  const Module* module_;

  std::unordered_map<std::string, Index> symtab_;
  std::vector<Symbol> symbols_;
  std::vector<RelocSection> reloc_sections_;
  RelocSection* current_reloc_section_ = nullptr;

  Index section_count_ = 0;
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

BinaryWriter::BinaryWriter(Stream* stream,
                           const WriteBinaryOptions& options,
                           const Module* module)
    : stream_(stream), options_(options), module_(module) {}

void BinaryWriter::WriteHeader(const char* name, int index) {
  if (stream_->has_log_stream()) {
    if (index == PRINT_HEADER_NO_INDEX) {
      stream_->log_stream().Writef("; %s\n", name);
    } else {
      stream_->log_stream().Writef("; %s %d\n", name, index);
    }
  }
}

/* returns offset of leb128 */
Offset BinaryWriter::WriteU32Leb128Space(Offset leb_size_guess,
                                         const char* desc) {
  assert(leb_size_guess <= MAX_U32_LEB128_BYTES);
  uint8_t data[MAX_U32_LEB128_BYTES] = {0};
  Offset result = stream_->offset();
  Offset bytes_to_write =
      options_.canonicalize_lebs ? leb_size_guess : MAX_U32_LEB128_BYTES;
  stream_->WriteData(data, bytes_to_write, desc);
  return result;
}

Offset BinaryWriter::WriteFixupU32Leb128Size(Offset offset,
                                             Offset leb_size_guess,
                                             const char* desc) {
  if (options_.canonicalize_lebs) {
    Offset size = stream_->offset() - offset - leb_size_guess;
    Offset leb_size = U32Leb128Length(size);
    Offset delta = leb_size - leb_size_guess;
    if (delta != 0) {
      Offset src_offset = offset + leb_size_guess;
      Offset dst_offset = offset + leb_size;
      stream_->MoveData(dst_offset, src_offset, size);
    }
    WriteU32Leb128At(stream_, offset, size, desc);
    stream_->AddOffset(delta);
    return delta;
  } else {
    Offset size = stream_->offset() - offset - MAX_U32_LEB128_BYTES;
    WriteFixedU32Leb128At(stream_, offset, size, desc);
    return 0;
  }
}

void BinaryWriter::WriteBlockDecl(const BlockDeclaration& decl) {
  if (decl.sig.GetNumParams() == 0 && decl.sig.GetNumResults() <= 1) {
    if (decl.sig.GetNumResults() == 0) {
      WriteType(stream_, Type::Void);
    } else if (decl.sig.GetNumResults() == 1) {
      WriteType(stream_, decl.sig.GetResultType(0));
    }
    return;
  }

  Index index = decl.has_func_type ? module_->GetFuncTypeIndex(decl.type_var)
                                   : module_->GetFuncTypeIndex(decl.sig);
  assert(index != kInvalidIndex);
  WriteS32Leb128(stream_, index, "block type function index");
}

void BinaryWriter::WriteSectionHeader(const char* desc,
                                      BinarySection section_code) {
  assert(last_section_leb_size_guess_ == 0);
  WriteHeader(desc, PRINT_HEADER_NO_INDEX);
  stream_->WriteU8Enum(section_code, "section code");
  last_section_type_ = section_code;
  last_section_leb_size_guess_ = LEB_SECTION_SIZE_GUESS;
  last_section_offset_ =
      WriteU32Leb128Space(LEB_SECTION_SIZE_GUESS, "section size (guess)");
  last_section_payload_offset_ = stream_->offset();
}

void BinaryWriter::BeginKnownSection(BinarySection section_code) {
  char desc[100];
  wabt_snprintf(desc, sizeof(desc), "section \"%s\" (%u)",
                GetSectionName(section_code),
                static_cast<unsigned>(section_code));
  WriteSectionHeader(desc, section_code);
}

void BinaryWriter::BeginCustomSection(const char* name) {
  char desc[100];
  wabt_snprintf(desc, sizeof(desc), "section \"%s\"", name);
  WriteSectionHeader(desc, BinarySection::Custom);
  WriteStr(stream_, name, "custom section name", PrintChars::Yes);
}

void BinaryWriter::EndSection() {
  assert(last_section_leb_size_guess_ != 0);
  Offset delta = WriteFixupU32Leb128Size(
      last_section_offset_, last_section_leb_size_guess_, "FIXUP section size");
  if (current_reloc_section_ && delta != 0) {
    for (Reloc& reloc : current_reloc_section_->relocations) {
      reloc.offset += delta;
    }
  }
  last_section_leb_size_guess_ = 0;
  section_count_++;
}

void BinaryWriter::BeginSubsection(const char* name) {
  assert(last_subsection_leb_size_guess_ == 0);
  last_subsection_leb_size_guess_ = LEB_SECTION_SIZE_GUESS;
  last_subsection_offset_ =
      WriteU32Leb128Space(LEB_SECTION_SIZE_GUESS, "subsection size (guess)");
  last_subsection_payload_offset_ = stream_->offset();
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

Index BinaryWriter::GetSymbolIndex(RelocType reloc_type, Index index) {
  std::string name;
  SymbolType type = SymbolType::Function;
  switch (reloc_type) {
    case RelocType::FuncIndexLEB:
      name = module_->funcs[index]->name;
      break;
    case RelocType::GlobalIndexLEB:
      type = SymbolType::Global;
      name = module_->globals[index]->name;
      break;
    default:
      WABT_UNREACHABLE;
  }
  auto iter = symtab_.find(name);
  if (iter != symtab_.end()) {
    return iter->second;
  }

  Index sym_index = Index(symbols_.size());
  symtab_[name] = sym_index;
  symbols_.push_back(Symbol{sym_index, type, index});
  return sym_index;
}

void BinaryWriter::AddReloc(RelocType reloc_type, Index index) {
  // Add a new reloc section if needed
  if (!current_reloc_section_ ||
      current_reloc_section_->section_index != section_count_) {
    reloc_sections_.emplace_back(GetSectionName(last_section_type_), section_count_);
    current_reloc_section_ = &reloc_sections_.back();
  }

  // Add a new relocation to the curent reloc section
  size_t offset = stream_->offset() - last_section_payload_offset_;
  Index symbol_index = GetSymbolIndex(reloc_type, index);
  current_reloc_section_->relocations.emplace_back(reloc_type, offset,
                                                   symbol_index);
}

void BinaryWriter::WriteU32Leb128WithReloc(Index index,
                                           const char* desc,
                                           RelocType reloc_type) {
  if (options_.relocatable) {
    AddReloc(reloc_type, index);
    WriteFixedU32Leb128(stream_, index, desc);
  } else {
    WriteU32Leb128(stream_, index, desc);
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

// TODO(binji): Rename this, it is used for more than loads/stores now.
template <typename T>
void BinaryWriter::WriteLoadStoreExpr(const Func* func,
                                      const Expr* expr,
                                      const char* desc) {
  auto* typed_expr = cast<T>(expr);
  WriteOpcode(stream_, typed_expr->opcode);
  Address align = typed_expr->opcode.GetAlignment(typed_expr->align);
  stream_->WriteU8(log2_u32(align), "alignment");
  WriteU32Leb128(stream_, typed_expr->offset, desc);
}

void BinaryWriter::WriteExpr(const Func* func, const Expr* expr) {
  switch (expr->type()) {
    case ExprType::AtomicLoad:
      WriteLoadStoreExpr<AtomicLoadExpr>(func, expr, "memory offset");
      break;
    case ExprType::AtomicRmw:
      WriteLoadStoreExpr<AtomicRmwExpr>(func, expr, "memory offset");
      break;
    case ExprType::AtomicRmwCmpxchg:
      WriteLoadStoreExpr<AtomicRmwCmpxchgExpr>(func, expr, "memory offset");
      break;
    case ExprType::AtomicStore:
      WriteLoadStoreExpr<AtomicStoreExpr>(func, expr, "memory offset");
      break;
    case ExprType::AtomicWait:
      WriteLoadStoreExpr<AtomicWaitExpr>(func, expr, "memory offset");
      break;
    case ExprType::AtomicWake:
      WriteLoadStoreExpr<AtomicWakeExpr>(func, expr, "memory offset");
      break;
    case ExprType::Binary:
      WriteOpcode(stream_, cast<BinaryExpr>(expr)->opcode);
      break;
    case ExprType::Block:
      WriteOpcode(stream_, Opcode::Block);
      WriteBlockDecl(cast<BlockExpr>(expr)->block.decl);
      WriteExprList(func, cast<BlockExpr>(expr)->block.exprs);
      WriteOpcode(stream_, Opcode::End);
      break;
    case ExprType::Br:
      WriteOpcode(stream_, Opcode::Br);
      WriteU32Leb128(stream_, GetLabelVarDepth(&cast<BrExpr>(expr)->var),
                     "break depth");
      break;
    case ExprType::BrIf:
      WriteOpcode(stream_, Opcode::BrIf);
      WriteU32Leb128(stream_, GetLabelVarDepth(&cast<BrIfExpr>(expr)->var),
                     "break depth");
      break;
    case ExprType::BrTable: {
      auto* br_table_expr = cast<BrTableExpr>(expr);
      WriteOpcode(stream_, Opcode::BrTable);
      WriteU32Leb128(stream_, br_table_expr->targets.size(), "num targets");
      Index depth;
      for (const Var& var : br_table_expr->targets) {
        depth = GetLabelVarDepth(&var);
        WriteU32Leb128(stream_, depth, "break depth");
      }
      depth = GetLabelVarDepth(&br_table_expr->default_target);
      WriteU32Leb128(stream_, depth, "break depth for default");
      break;
    }
    case ExprType::Call:{
      Index index = module_->GetFuncIndex(cast<CallExpr>(expr)->var);
      WriteOpcode(stream_, Opcode::Call);
      WriteU32Leb128WithReloc(index, "function index", RelocType::FuncIndexLEB);
      break;
    }
    case ExprType::ReturnCall: {
      Index index = module_->GetFuncIndex(cast<ReturnCallExpr>(expr)->var);
      WriteOpcode(stream_, Opcode::ReturnCall);
      WriteU32Leb128WithReloc(index, "function index", RelocType::FuncIndexLEB);
      break;
    }
    case ExprType::CallIndirect:{
      Index index =
        module_->GetFuncTypeIndex(cast<CallIndirectExpr>(expr)->decl);
      WriteOpcode(stream_, Opcode::CallIndirect);
      WriteU32Leb128WithReloc(index, "signature index", RelocType::TypeIndexLEB);
      WriteU32Leb128(stream_, 0, "call_indirect reserved");
      break;
    }
    case ExprType::ReturnCallIndirect: {
      Index index =
          module_->GetFuncTypeIndex(cast<ReturnCallIndirectExpr>(expr)->decl);
      WriteOpcode(stream_, Opcode::ReturnCallIndirect);
      WriteU32Leb128WithReloc(index, "signature index", RelocType::TypeIndexLEB);
      WriteU32Leb128(stream_, 0, "return_call_indirect reserved");
      break;
    }
    case ExprType::Compare:
      WriteOpcode(stream_, cast<CompareExpr>(expr)->opcode);
      break;
    case ExprType::Const: {
      const Const& const_ = cast<ConstExpr>(expr)->const_;
      switch (const_.type) {
        case Type::I32: {
          WriteOpcode(stream_, Opcode::I32Const);
          WriteS32Leb128(stream_, const_.u32, "i32 literal");
          break;
        }
        case Type::I64:
          WriteOpcode(stream_, Opcode::I64Const);
          WriteS64Leb128(stream_, const_.u64, "i64 literal");
          break;
        case Type::F32:
          WriteOpcode(stream_, Opcode::F32Const);
          stream_->WriteU32(const_.f32_bits, "f32 literal");
          break;
        case Type::F64:
          WriteOpcode(stream_, Opcode::F64Const);
          stream_->WriteU64(const_.f64_bits, "f64 literal");
          break;
        case Type::V128:
          WriteOpcode(stream_, Opcode::V128Const);
          stream_->WriteU128(const_.v128_bits, "v128 literal");
          break;
        default:
          assert(0);
      }
      break;
    }
    case ExprType::Convert:
      WriteOpcode(stream_, cast<ConvertExpr>(expr)->opcode);
      break;
    case ExprType::Drop:
      WriteOpcode(stream_, Opcode::Drop);
      break;
    case ExprType::GetGlobal: {
      Index index = module_->GetGlobalIndex(cast<GetGlobalExpr>(expr)->var);
      WriteOpcode(stream_, Opcode::GetGlobal);
      WriteU32Leb128WithReloc(index, "global index", RelocType::GlobalIndexLEB);
      break;
    }
    case ExprType::GetLocal: {
      Index index = GetLocalIndex(func, cast<GetLocalExpr>(expr)->var);
      WriteOpcode(stream_, Opcode::GetLocal);
      WriteU32Leb128(stream_, index, "local index");
      break;
    }
    case ExprType::If: {
      auto* if_expr = cast<IfExpr>(expr);
      WriteOpcode(stream_, Opcode::If);
      WriteBlockDecl(if_expr->true_.decl);
      WriteExprList(func, if_expr->true_.exprs);
      if (!if_expr->false_.empty()) {
        WriteOpcode(stream_, Opcode::Else);
        WriteExprList(func, if_expr->false_);
      }
      WriteOpcode(stream_, Opcode::End);
      break;
    }
    case ExprType::IfExcept: {
      auto* if_except_expr = cast<IfExceptExpr>(expr);
      WriteOpcode(stream_, Opcode::IfExcept);
      WriteBlockDecl(if_except_expr->true_.decl);
      Index index = module_->GetExceptIndex(if_except_expr->except_var);
      WriteU32Leb128(stream_, index, "exception index");
      WriteExprList(func, if_except_expr->true_.exprs);
      if (!if_except_expr->false_.empty()) {
        WriteOpcode(stream_, Opcode::Else);
        WriteExprList(func, if_except_expr->false_);
      }
      WriteOpcode(stream_, Opcode::End);
      break;
    }
    case ExprType::Load:
      WriteLoadStoreExpr<LoadExpr>(func, expr, "load offset");
      break;
    case ExprType::Loop:
      WriteOpcode(stream_, Opcode::Loop);
      WriteBlockDecl(cast<LoopExpr>(expr)->block.decl);
      WriteExprList(func, cast<LoopExpr>(expr)->block.exprs);
      WriteOpcode(stream_, Opcode::End);
      break;
    case ExprType::MemoryCopy:
      WriteOpcode(stream_, Opcode::MemoryCopy);
      WriteU32Leb128(stream_, 0, "memory.copy reserved");
      break;
    case ExprType::MemoryDrop: {
      Index index =
          module_->GetDataSegmentIndex(cast<MemoryDropExpr>(expr)->var);
      WriteOpcode(stream_, Opcode::MemoryDrop);
      WriteU32Leb128(stream_, index, "memory.drop segment");
      break;
    }
    case ExprType::MemoryFill:
      WriteOpcode(stream_, Opcode::MemoryFill);
      WriteU32Leb128(stream_, 0, "memory.fill reserved");
      break;
    case ExprType::MemoryGrow:
      WriteOpcode(stream_, Opcode::MemoryGrow);
      WriteU32Leb128(stream_, 0, "memory.grow reserved");
      break;
    case ExprType::MemoryInit: {
      Index index =
          module_->GetDataSegmentIndex(cast<MemoryInitExpr>(expr)->var);
      WriteOpcode(stream_, Opcode::MemoryInit);
      WriteU32Leb128(stream_, 0, "memory.init reserved");
      WriteU32Leb128(stream_, index, "memory.init segment");
      break;
    }
    case ExprType::MemorySize:
      WriteOpcode(stream_, Opcode::MemorySize);
      WriteU32Leb128(stream_, 0, "memory.size reserved");
      break;
    case ExprType::TableCopy:
      WriteOpcode(stream_, Opcode::TableCopy);
      WriteU32Leb128(stream_, 0, "table.copy reserved");
      break;
    case ExprType::TableDrop: {
      Index index =
          module_->GetElemSegmentIndex(cast<TableDropExpr>(expr)->var);
      WriteOpcode(stream_, Opcode::TableDrop);
      WriteU32Leb128(stream_, index, "table.drop segment");
      break;
    }
    case ExprType::TableInit: {
      Index index =
          module_->GetElemSegmentIndex(cast<TableInitExpr>(expr)->var);
      WriteOpcode(stream_, Opcode::TableInit);
      WriteU32Leb128(stream_, 0, "table.init reserved");
      WriteU32Leb128(stream_, index, "table.init segment");
      break;
    }
    case ExprType::Nop:
      WriteOpcode(stream_, Opcode::Nop);
      break;
    case ExprType::Rethrow:
      WriteOpcode(stream_, Opcode::Rethrow);
      break;
    case ExprType::Return:
      WriteOpcode(stream_, Opcode::Return);
      break;
    case ExprType::Select:
      WriteOpcode(stream_, Opcode::Select);
      break;
    case ExprType::SetGlobal: {
      Index index = module_->GetGlobalIndex(cast<SetGlobalExpr>(expr)->var);
      WriteOpcode(stream_, Opcode::SetGlobal);
      WriteU32Leb128WithReloc(index, "global index", RelocType::GlobalIndexLEB);
      break;
    }
    case ExprType::SetLocal: {
      Index index = GetLocalIndex(func, cast<SetLocalExpr>(expr)->var);
      WriteOpcode(stream_, Opcode::SetLocal);
      WriteU32Leb128(stream_, index, "local index");
      break;
    }
    case ExprType::Store:
      WriteLoadStoreExpr<StoreExpr>(func, expr, "store offset");
      break;
    case ExprType::TeeLocal: {
      Index index = GetLocalIndex(func, cast<TeeLocalExpr>(expr)->var);
      WriteOpcode(stream_, Opcode::TeeLocal);
      WriteU32Leb128(stream_, index, "local index");
      break;
    }
    case ExprType::Throw:
      WriteOpcode(stream_, Opcode::Throw);
      WriteU32Leb128(stream_, GetExceptVarDepth(&cast<ThrowExpr>(expr)->var),
                     "throw exception");
      break;
    case ExprType::Try: {
      auto* try_expr = cast<TryExpr>(expr);
      WriteOpcode(stream_, Opcode::Try);
      WriteBlockDecl(try_expr->block.decl);
      WriteExprList(func, try_expr->block.exprs);
      WriteOpcode(stream_, Opcode::Catch);
      WriteExprList(func, try_expr->catch_);
      WriteOpcode(stream_, Opcode::End);
      break;
    }
    case ExprType::Unary:
      WriteOpcode(stream_, cast<UnaryExpr>(expr)->opcode);
      break;
    case ExprType::Ternary:
      WriteOpcode(stream_, cast<TernaryExpr>(expr)->opcode);
      break;
    case ExprType::SimdLaneOp: {
      const Opcode opcode = cast<SimdLaneOpExpr>(expr)->opcode;
      WriteOpcode(stream_, opcode);
      stream_->WriteU8(static_cast<uint8_t>(cast<SimdLaneOpExpr>(expr)->val),
                       "Simd Lane literal");
      break;
    }
    case ExprType::SimdShuffleOp: {
      const Opcode opcode = cast<SimdShuffleOpExpr>(expr)->opcode;
      WriteOpcode(stream_, opcode);
      stream_->WriteU128(cast<SimdShuffleOpExpr>(expr)->val,
                         "Simd Lane[16] literal");
      break;
    }
    case ExprType::Unreachable:
      WriteOpcode(stream_, Opcode::Unreachable);
      break;
  }
}

void BinaryWriter::WriteExprList(const Func* func, const ExprList& exprs) {
  for (const Expr& expr : exprs) {
    WriteExpr(func, &expr);
  }
}

void BinaryWriter::WriteInitExpr(const ExprList& expr) {
  WriteExprList(nullptr, expr);
  WriteOpcode(stream_, Opcode::End);
}

void BinaryWriter::WriteFuncLocals(const Func* func,
                                   const LocalTypes& local_types) {
  if (local_types.size() == 0) {
    WriteU32Leb128(stream_, 0, "local decl count");
    return;
  }

  Index local_decl_count = local_types.decls().size();
  WriteU32Leb128(stream_, local_decl_count, "local decl count");
  for (auto decl : local_types.decls()) {
    WriteU32Leb128(stream_, decl.second, "local type count");
    WriteType(stream_, decl.first);
  }
}

void BinaryWriter::WriteFunc(const Func* func) {
  WriteFuncLocals(func, func->local_types);
  WriteExprList(func, func->exprs);
  WriteOpcode(stream_, Opcode::End);
}

void BinaryWriter::WriteTable(const Table* table) {
  WriteType(stream_, Type::Anyfunc);
  WriteLimits(stream_, &table->elem_limits);
}

void BinaryWriter::WriteMemory(const Memory* memory) {
  WriteLimits(stream_, &memory->page_limits);
}

void BinaryWriter::WriteGlobalHeader(const Global* global) {
  WriteType(stream_, global->type);
  stream_->WriteU8(global->mutable_, "global mutability");
}

void BinaryWriter::WriteExceptType(const TypeVector* except_types) {
  WriteU32Leb128(stream_, except_types->size(), "exception type count");
  for (Type ty : *except_types) {
    WriteType(stream_, ty);
  }
}

void BinaryWriter::WriteRelocSection(const RelocSection* reloc_section) {
  char section_name[128];
  wabt_snprintf(section_name, sizeof(section_name), "%s.%s",
                WABT_BINARY_SECTION_RELOC, reloc_section->name);
  BeginCustomSection(section_name);
  WriteU32Leb128(stream_, reloc_section->section_index, "reloc section index");
  const std::vector<Reloc>& relocs = reloc_section->relocations;
  WriteU32Leb128(stream_, relocs.size(), "num relocs");

  for (const Reloc& reloc : relocs) {
    WriteU32Leb128(stream_, reloc.type, "reloc type");
    WriteU32Leb128(stream_, reloc.offset, "reloc offset");
    WriteU32Leb128(stream_, reloc.index, "reloc index");
    switch (reloc.type) {
      case RelocType::MemoryAddressLEB:
      case RelocType::MemoryAddressSLEB:
      case RelocType::MemoryAddressI32:
        WriteU32Leb128(stream_, reloc.addend, "reloc addend");
        break;
      default:
        break;
    }
  }

  EndSection();
}

void BinaryWriter::WriteLinkingSection() {
  BeginCustomSection(WABT_BINARY_SECTION_LINKING);
  WriteU32Leb128(stream_, 1, "metadata version");
  if (symbols_.size()) {
    stream_->WriteU8Enum(LinkingEntryType::SymbolTable, "symbol table");
    BeginSubsection("symbol table");
    WriteU32Leb128(stream_, symbols_.size(), "num symbols");

    for (const Symbol& sym : symbols_) {
      bool is_defined = true;
      if (sym.type == SymbolType::Function) {
        if (sym.element_index < module_->num_func_imports) {
          is_defined = false;
        }
      }
      if (sym.type == SymbolType::Global) {
        if (sym.element_index < module_->num_global_imports) {
          is_defined = false;
        }
      }
      stream_->WriteU8Enum(sym.type, "symbol type");
      WriteU32Leb128(stream_, is_defined ? 0 : WABT_SYMBOL_FLAG_UNDEFINED,
                     "symbol flags");
      WriteU32Leb128(stream_, sym.element_index, "element index");
      if (is_defined) {
        if (sym.type == SymbolType::Function) {
          WriteStr(stream_, module_->funcs[sym.element_index]->name,
                   "function name", PrintChars::Yes);
        } else if (sym.type == SymbolType::Global) {
          WriteStr(stream_, module_->globals[sym.element_index]->name,
                   "global name", PrintChars::Yes);
        }
      }
    }
    EndSubsection();
  }
  EndSection();
}

Result BinaryWriter::WriteModule() {
  stream_->WriteU32(WABT_BINARY_MAGIC, "WASM_BINARY_MAGIC");
  stream_->WriteU32(WABT_BINARY_VERSION, "WASM_BINARY_VERSION");

  if (module_->func_types.size()) {
    BeginKnownSection(BinarySection::Type);
    WriteU32Leb128(stream_, module_->func_types.size(), "num types");
    for (size_t i = 0; i < module_->func_types.size(); ++i) {
      const FuncType* func_type = module_->func_types[i];
      const FuncSignature* sig = &func_type->sig;
      WriteHeader("type", i);
      WriteType(stream_, Type::Func);

      Index num_params = sig->param_types.size();
      Index num_results = sig->result_types.size();
      WriteU32Leb128(stream_, num_params, "num params");
      for (size_t j = 0; j < num_params; ++j) {
        WriteType(stream_, sig->param_types[j]);
      }

      WriteU32Leb128(stream_, num_results, "num results");
      for (size_t j = 0; j < num_results; ++j) {
        WriteType(stream_, sig->result_types[j]);
      }
    }
    EndSection();
  }

  if (module_->imports.size()) {
    BeginKnownSection(BinarySection::Import);
    WriteU32Leb128(stream_, module_->imports.size(), "num imports");

    for (size_t i = 0; i < module_->imports.size(); ++i) {
      const Import* import = module_->imports[i];
      WriteHeader("import header", i);
      WriteStr(stream_, import->module_name, "import module name",
               PrintChars::Yes);
      WriteStr(stream_, import->field_name, "import field name",
               PrintChars::Yes);
      stream_->WriteU8Enum(import->kind(), "import kind");
      switch (import->kind()) {
        case ExternalKind::Func:
          WriteU32Leb128(
              stream_,
              module_->GetFuncTypeIndex(cast<FuncImport>(import)->func.decl),
              "import signature index");
          break;

        case ExternalKind::Table:
          WriteTable(&cast<TableImport>(import)->table);
          break;

        case ExternalKind::Memory:
          WriteMemory(&cast<MemoryImport>(import)->memory);
          break;

        case ExternalKind::Global:
          WriteGlobalHeader(&cast<GlobalImport>(import)->global);
          break;

        case ExternalKind::Except:
          WriteExceptType(&cast<ExceptionImport>(import)->except.sig);
          break;
      }
    }
    EndSection();
  }

  assert(module_->funcs.size() >= module_->num_func_imports);
  Index num_funcs = module_->funcs.size() - module_->num_func_imports;
  if (num_funcs) {
    BeginKnownSection(BinarySection::Function);
    WriteU32Leb128(stream_, num_funcs, "num functions");

    for (size_t i = 0; i < num_funcs; ++i) {
      const Func* func = module_->funcs[i + module_->num_func_imports];
      char desc[100];
      wabt_snprintf(desc, sizeof(desc), "function %" PRIzd " signature index",
                    i);
      WriteU32Leb128(stream_, module_->GetFuncTypeIndex(func->decl), desc);
    }
    EndSection();
  }

  assert(module_->tables.size() >= module_->num_table_imports);
  Index num_tables = module_->tables.size() - module_->num_table_imports;
  if (num_tables) {
    BeginKnownSection(BinarySection::Table);
    WriteU32Leb128(stream_, num_tables, "num tables");
    for (size_t i = 0; i < num_tables; ++i) {
      const Table* table = module_->tables[i + module_->num_table_imports];
      WriteHeader("table", i);
      WriteTable(table);
    }
    EndSection();
  }

  assert(module_->memories.size() >= module_->num_memory_imports);
  Index num_memories = module_->memories.size() - module_->num_memory_imports;
  if (num_memories) {
    BeginKnownSection(BinarySection::Memory);
    WriteU32Leb128(stream_, num_memories, "num memories");
    for (size_t i = 0; i < num_memories; ++i) {
      const Memory* memory = module_->memories[i + module_->num_memory_imports];
      WriteHeader("memory", i);
      WriteMemory(memory);
    }
    EndSection();
  }

  assert(module_->globals.size() >= module_->num_global_imports);
  Index num_globals = module_->globals.size() - module_->num_global_imports;
  if (num_globals) {
    BeginKnownSection(BinarySection::Global);
    WriteU32Leb128(stream_, num_globals, "num globals");

    for (size_t i = 0; i < num_globals; ++i) {
      const Global* global = module_->globals[i + module_->num_global_imports];
      WriteGlobalHeader(global);
      WriteInitExpr(global->init_expr);
    }
    EndSection();
  }

  if (module_->exports.size()) {
    BeginKnownSection(BinarySection::Export);
    WriteU32Leb128(stream_, module_->exports.size(), "num exports");

    for (const Export* export_ : module_->exports) {
      WriteStr(stream_, export_->name, "export name", PrintChars::Yes);
      stream_->WriteU8Enum(export_->kind, "export kind");
      switch (export_->kind) {
        case ExternalKind::Func: {
          Index index = module_->GetFuncIndex(export_->var);
          WriteU32Leb128(stream_, index, "export func index");
          break;
        }
        case ExternalKind::Table: {
          Index index = module_->GetTableIndex(export_->var);
          WriteU32Leb128(stream_, index, "export table index");
          break;
        }
        case ExternalKind::Memory: {
          Index index = module_->GetMemoryIndex(export_->var);
          WriteU32Leb128(stream_, index, "export memory index");
          break;
        }
        case ExternalKind::Global: {
          Index index = module_->GetGlobalIndex(export_->var);
          WriteU32Leb128(stream_, index, "export global index");
          break;
        }
        case ExternalKind::Except: {
          Index index = module_->GetExceptIndex(export_->var);
          WriteU32Leb128(stream_, index, "export exception index");
          break;
        }
      }
    }
    EndSection();
  }

  if (module_->starts.size()) {
    Index start_func_index = module_->GetFuncIndex(*module_->starts[0]);
    if (start_func_index != kInvalidIndex) {
      BeginKnownSection(BinarySection::Start);
      WriteU32Leb128(stream_, start_func_index, "start func index");
      EndSection();
    }
  }

  if (module_->elem_segments.size()) {
    BeginKnownSection(BinarySection::Elem);
    WriteU32Leb128(stream_, module_->elem_segments.size(), "num elem segments");
    for (size_t i = 0; i < module_->elem_segments.size(); ++i) {
      ElemSegment* segment = module_->elem_segments[i];
      WriteHeader("elem segment header", i);
      if (segment->passive) {
        stream_->WriteU8(static_cast<uint8_t>(SegmentFlags::Passive));
      } else {
        assert(module_->GetTableIndex(segment->table_var) == 0);
        stream_->WriteU8(static_cast<uint8_t>(SegmentFlags::IndexZero));
        WriteInitExpr(segment->offset);
      }
      WriteU32Leb128(stream_, segment->vars.size(), "num function indices");
      for (const Var& var : segment->vars) {
        Index index = module_->GetFuncIndex(var);
        WriteU32Leb128WithReloc(index, "function index",
                                RelocType::FuncIndexLEB);
      }
    }
    EndSection();
  }

  assert(module_->excepts.size() >= module_->num_except_imports);
  Index num_exceptions = module_->excepts.size() - module_->num_except_imports;
  if (num_exceptions) {
    BeginCustomSection("exception");
    WriteU32Leb128(stream_, num_exceptions, "exception count");
    for (Index i = module_->num_except_imports; i < num_exceptions; ++i) {
      WriteExceptType(&module_->excepts[i]->sig);
    }
    EndSection();
  }

  if (num_funcs) {
    BeginKnownSection(BinarySection::Code);
    WriteU32Leb128(stream_, num_funcs, "num functions");

    for (size_t i = 0; i < num_funcs; ++i) {
      WriteHeader("function body", i);
      const Func* func = module_->funcs[i + module_->num_func_imports];

      /* TODO(binji): better guess of the size of the function body section */
      const Offset leb_size_guess = 1;
      Offset body_size_offset =
          WriteU32Leb128Space(leb_size_guess, "func body size (guess)");
      WriteFunc(func);
      WriteFixupU32Leb128Size(body_size_offset, leb_size_guess,
                              "FIXUP func body size");
    }
    EndSection();
  }

  if (module_->data_segments.size()) {
    BeginKnownSection(BinarySection::Data);
    WriteU32Leb128(stream_, module_->data_segments.size(), "num data segments");
    for (size_t i = 0; i < module_->data_segments.size(); ++i) {
      const DataSegment* segment = module_->data_segments[i];
      WriteHeader("data segment header", i);
      if (segment->passive) {
        stream_->WriteU8(static_cast<uint8_t>(SegmentFlags::Passive));
      } else {
        assert(module_->GetMemoryIndex(segment->memory_var) == 0);
        stream_->WriteU8(static_cast<uint8_t>(SegmentFlags::IndexZero));
        WriteInitExpr(segment->offset);
      }
      WriteU32Leb128(stream_, segment->data.size(), "data segment size");
      WriteHeader("data segment data", i);
      stream_->WriteData(segment->data, "data segment data");
    }
    EndSection();
  }

  if (options_.write_debug_names) {
    std::vector<std::string> index_to_name;

    char desc[100];
    BeginCustomSection(WABT_BINARY_SECTION_NAME);

    size_t named_functions = 0;
    for (const Func* func : module_->funcs) {
      if (!func->name.empty()) {
        named_functions++;
      }
    }

    if (!module_->name.empty()) {
      WriteU32Leb128(stream_, 0, "module name type");
      BeginSubsection("module name subsection");
      WriteDebugName(stream_, module_->name, "module name");
      EndSubsection();
    }

    if (named_functions > 0) {
      WriteU32Leb128(stream_, 1, "function name type");
      BeginSubsection("function name subsection");

      WriteU32Leb128(stream_, named_functions, "num functions");
      for (size_t i = 0; i < module_->funcs.size(); ++i) {
        const Func* func = module_->funcs[i];
        if (func->name.empty()) {
          continue;
        }
        WriteU32Leb128(stream_, i, "function index");
        wabt_snprintf(desc, sizeof(desc), "func name %" PRIzd, i);
        WriteDebugName(stream_, func->name, desc);
      }
      EndSubsection();
    }

    WriteU32Leb128(stream_, 2, "local name type");

    BeginSubsection("local name subsection");
    WriteU32Leb128(stream_, module_->funcs.size(), "num functions");
    for (size_t i = 0; i < module_->funcs.size(); ++i) {
      const Func* func = module_->funcs[i];
      Index num_params_and_locals = func->GetNumParamsAndLocals();

      WriteU32Leb128(stream_, i, "function index");
      WriteU32Leb128(stream_, num_params_and_locals, "num locals");

      MakeTypeBindingReverseMapping(num_params_and_locals, func->bindings,
                                    &index_to_name);
      for (size_t j = 0; j < num_params_and_locals; ++j) {
        const std::string& name = index_to_name[j];
        wabt_snprintf(desc, sizeof(desc), "local name %" PRIzd, j);
        WriteU32Leb128(stream_, j, "local index");
        WriteDebugName(stream_, name, desc);
      }
    }
    EndSubsection();
    EndSection();
  }

  if (options_.relocatable) {
    WriteLinkingSection();
    for (RelocSection& section : reloc_sections_) {
      WriteRelocSection(&section);
    }
  }

  return stream_->result();
}

}  // end anonymous namespace

Result WriteBinaryModule(Stream* stream,
                         const Module* module,
                         const WriteBinaryOptions& options) {
  BinaryWriter binary_writer(stream, options, module);
  return binary_writer.WriteModule();
}

}  // namespace wabt
