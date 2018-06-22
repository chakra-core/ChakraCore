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

#include "src/wat-writer.h"

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cstdarg>
#include <cstdio>
#include <iterator>
#include <map>
#include <string>
#include <vector>

#include "src/cast.h"
#include "src/common.h"
#include "src/expr-visitor.h"
#include "src/ir.h"
#include "src/literal.h"
#include "src/stream.h"

#define WABT_TRACING 0
#include "src/tracing.h"

#define INDENT_SIZE 2
#define NO_FORCE_NEWLINE 0
#define FORCE_NEWLINE 1

namespace wabt {

namespace {

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

// This table matches the characters allowed by wast-lexer.cc for `symbol`.
// The disallowed printable characters are: "(),;[]{} and <space>.
static const uint8_t s_valid_name_chars[256] = {
    //         0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
    /* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0x10 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 0x20 */ 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1,
    /* 0x30 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1,
    /* 0x40 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /* 0x50 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1,
    /* 0x60 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /* 0x70 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0,
};

enum class NextChar {
  None,
  Space,
  Newline,
  ForceNewline,
};

struct ExprTree {
  explicit ExprTree(const Expr* expr = nullptr) : expr(expr) {}
  // For debugging.
  std::string describe() const {
    std::string result("ExprTree(");
    if (expr) {
      result.append(GetExprTypeName(*expr));
    }
    return result + ")";
  }

  const Expr* expr;
  std::vector<ExprTree> children;
};

struct Label {
  Label(LabelType label_type,
        const std::string& name,
        const TypeVector& param_types,
        const TypeVector& result_types)
      : name(name),
        label_type(label_type),
        param_types(param_types),
        result_types(result_types) {}

  std::string name;
  LabelType label_type;
  TypeVector param_types;
  TypeVector result_types;
};

class WatWriter {
 public:
  WatWriter(Stream* stream, const WriteWatOptions* options)
      : options_(options), stream_(stream) {}

  Result WriteModule(const Module& module);

 private:
  void Indent();
  void Dedent();
  void WriteIndent();
  void WriteNextChar();
  void WriteDataWithNextChar(const void* src, size_t size);
  void Writef(const char* format, ...);
  void WritePutc(char c);
  void WritePuts(const char* s, NextChar next_char);
  void WritePutsSpace(const char* s);
  void WritePutsNewline(const char* s);
  void WriteNewline(bool force);
  void WriteOpen(const char* name, NextChar next_char);
  void WriteOpenNewline(const char* name);
  void WriteOpenSpace(const char* name);
  void WriteClose(NextChar next_char);
  void WriteCloseNewline();
  void WriteCloseSpace();
  void WriteString(const std::string& str, NextChar next_char);
  void WriteName(string_view str, NextChar next_char);
  void WriteNameOrIndex(string_view str, Index index, NextChar next_char);
  void WriteQuotedData(const void* data, size_t length);
  void WriteQuotedString(string_view str, NextChar next_char);
  void WriteVar(const Var& var, NextChar next_char);
  void WriteBrVar(const Var& var, NextChar next_char);
  void WriteType(Type type, NextChar next_char);
  void WriteTypes(const TypeVector& types, const char* name);
  void WriteFuncSigSpace(const FuncSignature& func_sig);
  void WriteBeginBlock(LabelType label_type,
                       const Block& block,
                       const char* text);
  void WriteBeginIfExceptBlock(const IfExceptExpr* expr);
  void WriteEndBlock();
  void WriteConst(const Const& const_);
  void WriteExpr(const Expr* expr);
  template <typename T>
  void WriteLoadStoreExpr(const Expr* expr);
  void WriteExprList(const ExprList& exprs);
  void WriteInitExpr(const ExprList& expr);
  template <typename T>
  void WriteTypeBindings(const char* prefix,
                         const Func& func,
                         const T& types,
                         const BindingHash& bindings);
  void WriteBeginFunc(const Func& func);
  void WriteFunc(const Func& func);
  void WriteBeginGlobal(const Global& global);
  void WriteGlobal(const Global& global);
  void WriteBeginException(const Exception& except);
  void WriteException(const Exception& except);
  void WriteLimits(const Limits& limits);
  void WriteTable(const Table& table);
  void WriteElemSegment(const ElemSegment& segment);
  void WriteMemory(const Memory& memory);
  void WriteDataSegment(const DataSegment& segment);
  void WriteImport(const Import& import);
  void WriteExport(const Export& export_);
  void WriteFuncType(const FuncType& func_type);
  void WriteStartFunction(const Var& start);

  class ExprVisitorDelegate;

  Index GetLabelStackSize() { return label_stack_.size(); }
  Label* GetLabel(const Var& var);
  Index GetLabelArity(const Var& var);
  Index GetFuncParamCount(const Var& var);
  Index GetFuncResultCount(const Var& var);
  void PushExpr(const Expr* expr, Index operand_count, Index result_count);
  void FlushExprTree(const ExprTree& expr_tree);
  void FlushExprTreeVector(const std::vector<ExprTree>&);
  void FlushExprTreeStack();
  void WriteFoldedExpr(const Expr*);
  void WriteFoldedExprList(const ExprList&);

  void BuildInlineExportMap();
  void WriteInlineExports(ExternalKind, Index);
  bool IsInlineExport(const Export& export_);
  void BuildInlineImportMap();
  void WriteInlineImport(ExternalKind, Index);

  const WriteWatOptions* options_ = nullptr;
  const Module* module_ = nullptr;
  const Func* current_func_ = nullptr;
  Stream* stream_ = nullptr;
  Result result_ = Result::Ok;
  int indent_ = 0;
  NextChar next_char_ = NextChar::None;
  std::vector<std::string> index_to_name_;
  std::vector<Label> label_stack_;
  std::vector<ExprTree> expr_tree_stack_;
  std::multimap<std::pair<ExternalKind, Index>, const Export*>
      inline_export_map_;
  std::vector<const Import*> inline_import_map_[kExternalKindCount];

  Index func_index_ = 0;
  Index global_index_ = 0;
  Index table_index_ = 0;
  Index memory_index_ = 0;
  Index func_type_index_ = 0;
  Index except_index_ = 0;
};

void WatWriter::Indent() {
  indent_ += INDENT_SIZE;
}

void WatWriter::Dedent() {
  indent_ -= INDENT_SIZE;
  assert(indent_ >= 0);
}

void WatWriter::WriteIndent() {
  static char s_indent[] =
      "                                                                       "
      "                                                                       ";
  static size_t s_indent_len = sizeof(s_indent) - 1;
  size_t to_write = indent_;
  while (to_write >= s_indent_len) {
    stream_->WriteData(s_indent, s_indent_len);
    to_write -= s_indent_len;
  }
  if (to_write > 0) {
    stream_->WriteData(s_indent, to_write);
  }
}

void WatWriter::WriteNextChar() {
  switch (next_char_) {
    case NextChar::Space:
      stream_->WriteChar(' ');
      break;
    case NextChar::Newline:
    case NextChar::ForceNewline:
      stream_->WriteChar('\n');
      WriteIndent();
      break;
    case NextChar::None:
      break;
  }
  next_char_ = NextChar::None;
}

void WatWriter::WriteDataWithNextChar(const void* src, size_t size) {
  WriteNextChar();
  stream_->WriteData(src, size);
}

void WABT_PRINTF_FORMAT(2, 3) WatWriter::Writef(const char* format, ...) {
  WABT_SNPRINTF_ALLOCA(buffer, length, format);
  /* default to following space */
  WriteDataWithNextChar(buffer, length);
  next_char_ = NextChar::Space;
}

void WatWriter::WritePutc(char c) {
  stream_->WriteChar(c);
}

void WatWriter::WritePuts(const char* s, NextChar next_char) {
  size_t len = strlen(s);
  WriteDataWithNextChar(s, len);
  next_char_ = next_char;
}

void WatWriter::WritePutsSpace(const char* s) {
  WritePuts(s, NextChar::Space);
}

void WatWriter::WritePutsNewline(const char* s) {
  WritePuts(s, NextChar::Newline);
}

void WatWriter::WriteNewline(bool force) {
  if (next_char_ == NextChar::ForceNewline) {
    WriteNextChar();
  }
  next_char_ = force ? NextChar::ForceNewline : NextChar::Newline;
}

void WatWriter::WriteOpen(const char* name, NextChar next_char) {
  WritePuts("(", NextChar::None);
  WritePuts(name, next_char);
  Indent();
}

void WatWriter::WriteOpenNewline(const char* name) {
  WriteOpen(name, NextChar::Newline);
}

void WatWriter::WriteOpenSpace(const char* name) {
  WriteOpen(name, NextChar::Space);
}

void WatWriter::WriteClose(NextChar next_char) {
  if (next_char_ != NextChar::ForceNewline) {
    next_char_ = NextChar::None;
  }
  Dedent();
  WritePuts(")", next_char);
}

void WatWriter::WriteCloseNewline() {
  WriteClose(NextChar::Newline);
}

void WatWriter::WriteCloseSpace() {
  WriteClose(NextChar::Space);
}

void WatWriter::WriteString(const std::string& str, NextChar next_char) {
  WritePuts(str.c_str(), next_char);
}

void WatWriter::WriteName(string_view str, NextChar next_char) {
  // Debug names must begin with a $ for for wast file to be valid
  assert(!str.empty() && str.front() == '$');
  bool has_invalid_chars = std::any_of(
      str.begin(), str.end(), [](uint8_t c) { return !s_valid_name_chars[c]; });

  if (has_invalid_chars) {
    std::string valid_str;
    std::transform(str.begin(), str.end(), std::back_inserter(valid_str),
                   [](uint8_t c) { return s_valid_name_chars[c] ? c : '_'; });
    WriteDataWithNextChar(valid_str.data(), valid_str.length());
  } else {
    WriteDataWithNextChar(str.data(), str.length());
  }

  next_char_ = next_char;
}

void WatWriter::WriteNameOrIndex(string_view str,
                                 Index index,
                                 NextChar next_char) {
  if (!str.empty()) {
    WriteName(str, next_char);
  } else {
    Writef("(;%u;)", index);
  }
}

void WatWriter::WriteQuotedData(const void* data, size_t length) {
  const uint8_t* u8_data = static_cast<const uint8_t*>(data);
  static const char s_hexdigits[] = "0123456789abcdef";
  WriteNextChar();
  WritePutc('\"');
  for (size_t i = 0; i < length; ++i) {
    uint8_t c = u8_data[i];
    if (s_is_char_escaped[c]) {
      WritePutc('\\');
      WritePutc(s_hexdigits[c >> 4]);
      WritePutc(s_hexdigits[c & 0xf]);
    } else {
      WritePutc(c);
    }
  }
  WritePutc('\"');
  next_char_ = NextChar::Space;
}

void WatWriter::WriteQuotedString(string_view str, NextChar next_char) {
  WriteQuotedData(str.data(), str.length());
  next_char_ = next_char;
}

void WatWriter::WriteVar(const Var& var, NextChar next_char) {
  if (var.is_index()) {
    Writef("%" PRIindex, var.index());
    next_char_ = next_char;
  } else {
    WriteName(var.name(), next_char);
  }
}

void WatWriter::WriteBrVar(const Var& var, NextChar next_char) {
  if (var.is_index()) {
    if (var.index() < GetLabelStackSize()) {
      Writef("%" PRIindex " (;@%" PRIindex ";)", var.index(),
             GetLabelStackSize() - var.index() - 1);
    } else {
      Writef("%" PRIindex " (; INVALID ;)", var.index());
    }
    next_char_ = next_char;
  } else {
    WriteString(var.name(), next_char);
  }
}

void WatWriter::WriteType(Type type, NextChar next_char) {
  const char* type_name = GetTypeName(type);
  assert(type_name);
  WritePuts(type_name, next_char);
}

void WatWriter::WriteTypes(const TypeVector& types, const char* name) {
  if (types.size()) {
    if (name) {
      WriteOpenSpace(name);
    }
    for (Type type : types) {
      WriteType(type, NextChar::Space);
    }
    if (name) {
      WriteCloseSpace();
    }
  }
}

void WatWriter::WriteFuncSigSpace(const FuncSignature& func_sig) {
  WriteTypes(func_sig.param_types, "param");
  WriteTypes(func_sig.result_types, "result");
}

void WatWriter::WriteBeginBlock(LabelType label_type,
                                const Block& block,
                                const char* text) {
  WritePutsSpace(text);
  bool has_label = !block.label.empty();
  if (has_label) {
    WriteString(block.label, NextChar::Space);
  }
  WriteTypes(block.decl.sig.param_types, "param");
  WriteTypes(block.decl.sig.result_types, "result");
  if (!has_label) {
    Writef(" ;; label = @%" PRIindex, GetLabelStackSize());
  }
  WriteNewline(FORCE_NEWLINE);
  label_stack_.emplace_back(label_type, block.label, block.decl.sig.param_types,
                            block.decl.sig.result_types);
  Indent();
}

void WatWriter::WriteBeginIfExceptBlock(const IfExceptExpr* expr) {
  const Block& block = expr->true_;
  WritePutsSpace(Opcode::IfExcept_Opcode.GetName());
  bool has_label = !block.label.empty();
  if (has_label) {
    WriteString(block.label, NextChar::Space);
  }
  WriteTypes(block.decl.sig.param_types, "param");
  WriteTypes(block.decl.sig.result_types, "result");
  WriteVar(expr->except_var, NextChar::Space);
  if (!has_label) {
    Writef(" ;; label = @%" PRIindex, GetLabelStackSize());
  }
  WriteNewline(FORCE_NEWLINE);
  label_stack_.emplace_back(LabelType::IfExcept, block.label,
                            block.decl.sig.param_types,
                            block.decl.sig.result_types);
  Indent();
}

void WatWriter::WriteEndBlock() {
  Dedent();
  label_stack_.pop_back();
  WritePutsNewline(Opcode::End_Opcode.GetName());
}

void WatWriter::WriteConst(const Const& const_) {
  switch (const_.type) {
    case Type::I32:
      WritePutsSpace(Opcode::I32Const_Opcode.GetName());
      Writef("%d", static_cast<int32_t>(const_.u32));
      WriteNewline(NO_FORCE_NEWLINE);
      break;

    case Type::I64:
      WritePutsSpace(Opcode::I64Const_Opcode.GetName());
      Writef("%" PRId64, static_cast<int64_t>(const_.u64));
      WriteNewline(NO_FORCE_NEWLINE);
      break;

    case Type::F32: {
      WritePutsSpace(Opcode::F32Const_Opcode.GetName());
      char buffer[128];
      WriteFloatHex(buffer, 128, const_.f32_bits);
      WritePutsSpace(buffer);
      float f32;
      memcpy(&f32, &const_.f32_bits, sizeof(f32));
      Writef("(;=%g;)", f32);
      WriteNewline(NO_FORCE_NEWLINE);
      break;
    }

    case Type::F64: {
      WritePutsSpace(Opcode::F64Const_Opcode.GetName());
      char buffer[128];
      WriteDoubleHex(buffer, 128, const_.f64_bits);
      WritePutsSpace(buffer);
      double f64;
      memcpy(&f64, &const_.f64_bits, sizeof(f64));
      Writef("(;=%g;)", f64);
      WriteNewline(NO_FORCE_NEWLINE);
      break;
    }

    case Type::V128: {
      WritePutsSpace(Opcode::V128Const_Opcode.GetName());
      Writef("i32 0x%08x 0x%08x 0x%08x 0x%08x", const_.v128_bits.v[0],
             const_.v128_bits.v[1], const_.v128_bits.v[2],
             const_.v128_bits.v[3]);
      WriteNewline(NO_FORCE_NEWLINE);
      break;
    }

    default:
      assert(0);
      break;
  }
}

template <typename T>
void WatWriter::WriteLoadStoreExpr(const Expr* expr) {
  auto typed_expr = cast<T>(expr);
  WritePutsSpace(typed_expr->opcode.GetName());
  if (typed_expr->offset) {
    Writef("offset=%u", typed_expr->offset);
  }
  if (!typed_expr->opcode.IsNaturallyAligned(typed_expr->align)) {
    Writef("align=%u", typed_expr->align);
  }
  WriteNewline(NO_FORCE_NEWLINE);
}

class WatWriter::ExprVisitorDelegate : public ExprVisitor::Delegate {
 public:
  explicit ExprVisitorDelegate(WatWriter* writer) : writer_(writer) {}

  Result OnBinaryExpr(BinaryExpr*) override;
  Result BeginBlockExpr(BlockExpr*) override;
  Result EndBlockExpr(BlockExpr*) override;
  Result OnBrExpr(BrExpr*) override;
  Result OnBrIfExpr(BrIfExpr*) override;
  Result OnBrTableExpr(BrTableExpr*) override;
  Result OnCallExpr(CallExpr*) override;
  Result OnCallIndirectExpr(CallIndirectExpr*) override;
  Result OnCompareExpr(CompareExpr*) override;
  Result OnConstExpr(ConstExpr*) override;
  Result OnConvertExpr(ConvertExpr*) override;
  Result OnDropExpr(DropExpr*) override;
  Result OnGetGlobalExpr(GetGlobalExpr*) override;
  Result OnGetLocalExpr(GetLocalExpr*) override;
  Result BeginIfExpr(IfExpr*) override;
  Result AfterIfTrueExpr(IfExpr*) override;
  Result EndIfExpr(IfExpr*) override;
  Result BeginIfExceptExpr(IfExceptExpr*) override;
  Result AfterIfExceptTrueExpr(IfExceptExpr*) override;
  Result EndIfExceptExpr(IfExceptExpr*) override;
  Result OnLoadExpr(LoadExpr*) override;
  Result BeginLoopExpr(LoopExpr*) override;
  Result EndLoopExpr(LoopExpr*) override;
  Result OnMemoryGrowExpr(MemoryGrowExpr*) override;
  Result OnMemorySizeExpr(MemorySizeExpr*) override;
  Result OnNopExpr(NopExpr*) override;
  Result OnReturnExpr(ReturnExpr*) override;
  Result OnSelectExpr(SelectExpr*) override;
  Result OnSetGlobalExpr(SetGlobalExpr*) override;
  Result OnSetLocalExpr(SetLocalExpr*) override;
  Result OnStoreExpr(StoreExpr*) override;
  Result OnTeeLocalExpr(TeeLocalExpr*) override;
  Result OnUnaryExpr(UnaryExpr*) override;
  Result OnUnreachableExpr(UnreachableExpr*) override;
  Result BeginTryExpr(TryExpr*) override;
  Result OnCatchExpr(TryExpr*) override;
  Result EndTryExpr(TryExpr*) override;
  Result OnThrowExpr(ThrowExpr*) override;
  Result OnRethrowExpr(RethrowExpr*) override;
  Result OnAtomicWaitExpr(AtomicWaitExpr*) override;
  Result OnAtomicWakeExpr(AtomicWakeExpr*) override;
  Result OnAtomicLoadExpr(AtomicLoadExpr*) override;
  Result OnAtomicStoreExpr(AtomicStoreExpr*) override;
  Result OnAtomicRmwExpr(AtomicRmwExpr*) override;
  Result OnAtomicRmwCmpxchgExpr(AtomicRmwCmpxchgExpr*) override;
  Result OnTernaryExpr(TernaryExpr*) override;
  Result OnSimdLaneOpExpr(SimdLaneOpExpr*) override;
  Result OnSimdShuffleOpExpr(SimdShuffleOpExpr*) override;

 private:
  WatWriter* writer_;
};

Result WatWriter::ExprVisitorDelegate::OnBinaryExpr(BinaryExpr* expr) {
  writer_->WritePutsNewline(expr->opcode.GetName());
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::BeginBlockExpr(BlockExpr* expr) {
  writer_->WriteBeginBlock(LabelType::Block, expr->block,
                           Opcode::Block_Opcode.GetName());
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::EndBlockExpr(BlockExpr* expr) {
  writer_->WriteEndBlock();
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnBrExpr(BrExpr* expr) {
  writer_->WritePutsSpace(Opcode::Br_Opcode.GetName());
  writer_->WriteBrVar(expr->var, NextChar::Newline);
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnBrIfExpr(BrIfExpr* expr) {
  writer_->WritePutsSpace(Opcode::BrIf_Opcode.GetName());
  writer_->WriteBrVar(expr->var, NextChar::Newline);
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnBrTableExpr(BrTableExpr* expr) {
  writer_->WritePutsSpace(Opcode::BrTable_Opcode.GetName());
  for (const Var& var : expr->targets) {
    writer_->WriteBrVar(var, NextChar::Space);
  }
  writer_->WriteBrVar(expr->default_target, NextChar::Newline);
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnCallExpr(CallExpr* expr) {
  writer_->WritePutsSpace(Opcode::Call_Opcode.GetName());
  writer_->WriteVar(expr->var, NextChar::Newline);
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnCallIndirectExpr(
    CallIndirectExpr* expr) {
  writer_->WritePutsSpace(Opcode::CallIndirect_Opcode.GetName());
  writer_->WriteOpenSpace("type");
  writer_->WriteVar(expr->decl.type_var, NextChar::Space);
  writer_->WriteCloseNewline();
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnCompareExpr(CompareExpr* expr) {
  writer_->WritePutsNewline(expr->opcode.GetName());
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnConstExpr(ConstExpr* expr) {
  writer_->WriteConst(expr->const_);
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnConvertExpr(ConvertExpr* expr) {
  writer_->WritePutsNewline(expr->opcode.GetName());
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnDropExpr(DropExpr* expr) {
  writer_->WritePutsNewline(Opcode::Drop_Opcode.GetName());
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnGetGlobalExpr(GetGlobalExpr* expr) {
  writer_->WritePutsSpace(Opcode::GetGlobal_Opcode.GetName());
  writer_->WriteVar(expr->var, NextChar::Newline);
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnGetLocalExpr(GetLocalExpr* expr) {
  writer_->WritePutsSpace(Opcode::GetLocal_Opcode.GetName());
  writer_->WriteVar(expr->var, NextChar::Newline);
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::BeginIfExpr(IfExpr* expr) {
  writer_->WriteBeginBlock(LabelType::If, expr->true_,
                           Opcode::If_Opcode.GetName());
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::AfterIfTrueExpr(IfExpr* expr) {
  if (!expr->false_.empty()) {
    writer_->Dedent();
    writer_->WritePutsSpace(Opcode::Else_Opcode.GetName());
    writer_->Indent();
    writer_->WriteNewline(FORCE_NEWLINE);
  }
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::EndIfExpr(IfExpr* expr) {
  writer_->WriteEndBlock();
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::BeginIfExceptExpr(IfExceptExpr* expr) {
  // Can't use WriteBeginBlock because if_except has an additional exception
  // index argument.
  writer_->WriteBeginIfExceptBlock(expr);
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::AfterIfExceptTrueExpr(
    IfExceptExpr* expr) {
  if (!expr->false_.empty()) {
    writer_->Dedent();
    writer_->WritePutsSpace(Opcode::Else_Opcode.GetName());
    writer_->Indent();
    writer_->WriteNewline(FORCE_NEWLINE);
  }
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::EndIfExceptExpr(IfExceptExpr* expr) {
  writer_->WriteEndBlock();
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnLoadExpr(LoadExpr* expr) {
  writer_->WriteLoadStoreExpr<LoadExpr>(expr);
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::BeginLoopExpr(LoopExpr* expr) {
  writer_->WriteBeginBlock(LabelType::Loop, expr->block,
                           Opcode::Loop_Opcode.GetName());
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::EndLoopExpr(LoopExpr* expr) {
  writer_->WriteEndBlock();
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnMemoryGrowExpr(MemoryGrowExpr* expr) {
  writer_->WritePutsNewline(Opcode::MemoryGrow_Opcode.GetName());
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnMemorySizeExpr(MemorySizeExpr* expr) {
  writer_->WritePutsNewline(Opcode::MemorySize_Opcode.GetName());
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnNopExpr(NopExpr* expr) {
  writer_->WritePutsNewline(Opcode::Nop_Opcode.GetName());
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnReturnExpr(ReturnExpr* expr) {
  writer_->WritePutsNewline(Opcode::Return_Opcode.GetName());
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnSelectExpr(SelectExpr* expr) {
  writer_->WritePutsNewline(Opcode::Select_Opcode.GetName());
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnSetGlobalExpr(SetGlobalExpr* expr) {
  writer_->WritePutsSpace(Opcode::SetGlobal_Opcode.GetName());
  writer_->WriteVar(expr->var, NextChar::Newline);
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnSetLocalExpr(SetLocalExpr* expr) {
  writer_->WritePutsSpace(Opcode::SetLocal_Opcode.GetName());
  writer_->WriteVar(expr->var, NextChar::Newline);
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnStoreExpr(StoreExpr* expr) {
  writer_->WriteLoadStoreExpr<StoreExpr>(expr);
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnTeeLocalExpr(TeeLocalExpr* expr) {
  writer_->WritePutsSpace(Opcode::TeeLocal_Opcode.GetName());
  writer_->WriteVar(expr->var, NextChar::Newline);
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnUnaryExpr(UnaryExpr* expr) {
  writer_->WritePutsNewline(expr->opcode.GetName());
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnUnreachableExpr(
    UnreachableExpr* expr) {
  writer_->WritePutsNewline(Opcode::Unreachable_Opcode.GetName());
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::BeginTryExpr(TryExpr* expr) {
  writer_->WriteBeginBlock(LabelType::Try, expr->block,
                           Opcode::Try_Opcode.GetName());
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnCatchExpr(TryExpr* expr) {
  writer_->Dedent();
  writer_->WritePutsSpace(Opcode::Catch_Opcode.GetName());
  writer_->Indent();
  writer_->label_stack_.back().label_type = LabelType::Catch;
  writer_->WriteNewline(FORCE_NEWLINE);
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::EndTryExpr(TryExpr* expr) {
  writer_->WriteEndBlock();
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnThrowExpr(ThrowExpr* expr) {
  writer_->WritePutsSpace(Opcode::Throw_Opcode.GetName());
  writer_->WriteVar(expr->var, NextChar::Newline);
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnRethrowExpr(RethrowExpr* expr) {
  writer_->WritePutsSpace(Opcode::Rethrow_Opcode.GetName());
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnAtomicWaitExpr(AtomicWaitExpr* expr) {
  writer_->WriteLoadStoreExpr<AtomicWaitExpr>(expr);
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnAtomicWakeExpr(AtomicWakeExpr* expr) {
  writer_->WriteLoadStoreExpr<AtomicWakeExpr>(expr);
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnAtomicLoadExpr(AtomicLoadExpr* expr) {
  writer_->WriteLoadStoreExpr<AtomicLoadExpr>(expr);
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnAtomicStoreExpr(
    AtomicStoreExpr* expr) {
  writer_->WriteLoadStoreExpr<AtomicStoreExpr>(expr);
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnAtomicRmwExpr(AtomicRmwExpr* expr) {
  writer_->WriteLoadStoreExpr<AtomicRmwExpr>(expr);
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnAtomicRmwCmpxchgExpr(
    AtomicRmwCmpxchgExpr* expr) {
  writer_->WriteLoadStoreExpr<AtomicRmwCmpxchgExpr>(expr);
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnTernaryExpr(TernaryExpr* expr) {
  writer_->WritePutsNewline(expr->opcode.GetName());
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnSimdLaneOpExpr(SimdLaneOpExpr* expr) {
  writer_->WritePutsSpace(expr->opcode.GetName());
  writer_->Writef("%" PRIu64, (expr->val));
  writer_->WritePutsNewline("");
  return Result::Ok;
}

Result WatWriter::ExprVisitorDelegate::OnSimdShuffleOpExpr(
    SimdShuffleOpExpr* expr) {
  writer_->WritePutsSpace(expr->opcode.GetName());
  writer_->Writef(" 0x%08x 0x%08x 0x%08x 0x%08x", (expr->val.v[0]), (expr->val.v[1]),
                  (expr->val.v[2]), (expr->val.v[3]));
  writer_->WritePutsNewline("");
  return Result::Ok;
}

void WatWriter::WriteExpr(const Expr* expr) {
  WABT_TRACE(WriteExprList);
  ExprVisitorDelegate delegate(this);
  ExprVisitor visitor(&delegate);
  visitor.VisitExpr(const_cast<Expr*>(expr));
}

void WatWriter::WriteExprList(const ExprList& exprs) {
  WABT_TRACE(WriteExprList);
  ExprVisitorDelegate delegate(this);
  ExprVisitor visitor(&delegate);
  visitor.VisitExprList(const_cast<ExprList&>(exprs));
}

Label* WatWriter::GetLabel(const Var& var) {
  if (var.is_name()) {
    for (Index i = GetLabelStackSize(); i > 0; --i) {
      Label* label = &label_stack_[i - 1];
      if (label->name == var.name()) {
        return label;
      }
    }
  } else if (var.index() < GetLabelStackSize()) {
    Label* label = &label_stack_[GetLabelStackSize() - var.index() - 1];
    return label;
  }
  return nullptr;
}

Index WatWriter::GetLabelArity(const Var& var) {
  Label* label = GetLabel(var);
  if (!label) {
    return 0;
  }

  return label->label_type == LabelType::Loop ? label->param_types.size()
                                              : label->result_types.size();
}

Index WatWriter::GetFuncParamCount(const Var& var) {
  const Func* func = module_->GetFunc(var);
  return func ? func->GetNumParams() : 0;
}

Index WatWriter::GetFuncResultCount(const Var& var) {
  const Func* func = module_->GetFunc(var);
  return func ? func->GetNumResults() : 0;
}

void WatWriter::WriteFoldedExpr(const Expr* expr) {
  WABT_TRACE_ARGS(WriteFoldedExpr, "%s", GetExprTypeName(*expr));
  switch (expr->type()) {
    case ExprType::AtomicRmw:
    case ExprType::AtomicWake:
    case ExprType::Binary:
    case ExprType::Compare:
      PushExpr(expr, 2, 1);
      break;

    case ExprType::AtomicStore:
    case ExprType::Store:
      PushExpr(expr, 2, 0);
      break;

    case ExprType::Block:
      PushExpr(expr, 0, cast<BlockExpr>(expr)->block.decl.sig.GetNumResults());
      break;

    case ExprType::Br:
      PushExpr(expr, GetLabelArity(cast<BrExpr>(expr)->var), 1);
      break;

    case ExprType::BrIf: {
      Index arity = GetLabelArity(cast<BrIfExpr>(expr)->var);
      PushExpr(expr, arity + 1, arity);
      break;
    }

    case ExprType::BrTable:
      PushExpr(expr, GetLabelArity(cast<BrTableExpr>(expr)->default_target) + 1,
               1);
      break;

    case ExprType::Call: {
      const Var& var = cast<CallExpr>(expr)->var;
      PushExpr(expr, GetFuncParamCount(var), GetFuncResultCount(var));
      break;
    }

    case ExprType::CallIndirect: {
      const auto* ci_expr = cast<CallIndirectExpr>(expr);
      PushExpr(expr, ci_expr->decl.GetNumParams() + 1,
               ci_expr->decl.GetNumResults());
      break;
    }

    case ExprType::Const:
    case ExprType::MemorySize:
    case ExprType::GetGlobal:
    case ExprType::GetLocal:
    case ExprType::Unreachable:
      PushExpr(expr, 0, 1);
      break;

    case ExprType::AtomicLoad:
    case ExprType::Convert:
    case ExprType::MemoryGrow:
    case ExprType::Load:
    case ExprType::TeeLocal:
    case ExprType::Unary:
      PushExpr(expr, 1, 1);
      break;

    case ExprType::Drop:
    case ExprType::SetGlobal:
    case ExprType::SetLocal:
      PushExpr(expr, 1, 0);
      break;

    case ExprType::If:
      PushExpr(expr, 1, cast<IfExpr>(expr)->true_.decl.sig.GetNumResults());
      break;

    case ExprType::IfExcept:
      PushExpr(expr, 1,
               cast<IfExceptExpr>(expr)->true_.decl.sig.GetNumResults());
      break;

    case ExprType::Loop:
      PushExpr(expr, 0, cast<LoopExpr>(expr)->block.decl.sig.GetNumResults());
      break;

    case ExprType::Nop:
      PushExpr(expr, 0, 0);
      break;

    case ExprType::Return:
      PushExpr(expr, current_func_->decl.sig.result_types.size(), 1);
      break;

    case ExprType::Rethrow:
      PushExpr(expr, 0, 0);
      break;

    case ExprType::AtomicRmwCmpxchg:
    case ExprType::AtomicWait:
    case ExprType::Select:
      PushExpr(expr, 3, 1);
      break;

    case ExprType::Throw: {
      auto throw_ = cast<ThrowExpr>(expr);
      Index operand_count = 0;
      if (Exception* except = module_->GetExcept(throw_->var)) {
        operand_count = except->sig.size();
      }
      PushExpr(expr, operand_count, 0);
      break;
    }

    case ExprType::Try:
      PushExpr(expr, 0, cast<TryExpr>(expr)->block.decl.sig.GetNumResults());
      break;

    case ExprType::Ternary:
      PushExpr(expr, 3, 1);
      break;

    case ExprType::SimdLaneOp: {
      const Opcode opcode = cast<SimdLaneOpExpr>(expr)->opcode;
      switch (opcode) {
        case Opcode::I8X16ExtractLaneS:
        case Opcode::I8X16ExtractLaneU:
        case Opcode::I16X8ExtractLaneS:
        case Opcode::I16X8ExtractLaneU:
        case Opcode::I32X4ExtractLane:
        case Opcode::I64X2ExtractLane:
        case Opcode::F32X4ExtractLane:
        case Opcode::F64X2ExtractLane:
          PushExpr(expr, 1, 1);
          break;

        case Opcode::I8X16ReplaceLane:
        case Opcode::I16X8ReplaceLane:
        case Opcode::I32X4ReplaceLane:
        case Opcode::I64X2ReplaceLane:
        case Opcode::F32X4ReplaceLane:
        case Opcode::F64X2ReplaceLane:
          PushExpr(expr, 2, 1);
          break;

        default:
          fprintf(stderr, "Invalid Opcode for expr type: %s\n",
                  GetExprTypeName(*expr));
          assert(0);
      }
      break;
    }

    case ExprType::SimdShuffleOp:
      PushExpr(expr, 2, 1);
      break;

    default:
      fprintf(stderr, "bad expr type: %s\n", GetExprTypeName(*expr));
      assert(0);
      break;
  }
}

void WatWriter::WriteFoldedExprList(const ExprList& exprs) {
  WABT_TRACE(WriteFoldedExprList);
  for (const Expr& expr : exprs) {
    WriteFoldedExpr(&expr);
  }
}

void WatWriter::PushExpr(const Expr* expr,
                         Index operand_count,
                         Index result_count) {
  WABT_TRACE_ARGS(PushExpr, "%s, %" PRIindex ", %" PRIindex "",
                  GetExprTypeName(*expr), operand_count, result_count);
  if (operand_count <= expr_tree_stack_.size()) {
    auto last_operand = expr_tree_stack_.end();
    auto first_operand = last_operand - operand_count;
    ExprTree tree(expr);
    std::move(first_operand, last_operand, std::back_inserter(tree.children));
    expr_tree_stack_.erase(first_operand, last_operand);
    expr_tree_stack_.emplace_back(tree);
    if (result_count == 0) {
      FlushExprTreeStack();
    }
  } else {
    expr_tree_stack_.emplace_back(expr);
    FlushExprTreeStack();
  }
}

void WatWriter::FlushExprTree(const ExprTree& expr_tree) {
  WABT_TRACE_ARGS(FlushExprTree, "%s", GetExprTypeName(*expr_tree.expr));
  switch (expr_tree.expr->type()) {
    case ExprType::Block:
      WritePuts("(", NextChar::None);
      WriteBeginBlock(LabelType::Block, cast<BlockExpr>(expr_tree.expr)->block,
                      Opcode::Block_Opcode.GetName());
      WriteFoldedExprList(cast<BlockExpr>(expr_tree.expr)->block.exprs);
      FlushExprTreeStack();
      WriteCloseNewline();
      break;

    case ExprType::Loop:
      WritePuts("(", NextChar::None);
      WriteBeginBlock(LabelType::Loop, cast<LoopExpr>(expr_tree.expr)->block,
                      Opcode::Loop_Opcode.GetName());
      WriteFoldedExprList(cast<LoopExpr>(expr_tree.expr)->block.exprs);
      FlushExprTreeStack();
      WriteCloseNewline();
      break;

    case ExprType::If: {
      auto if_expr = cast<IfExpr>(expr_tree.expr);
      WritePuts("(", NextChar::None);
      WriteBeginBlock(LabelType::If, if_expr->true_,
                      Opcode::If_Opcode.GetName());
      FlushExprTreeVector(expr_tree.children);
      WriteOpenNewline("then");
      WriteFoldedExprList(if_expr->true_.exprs);
      FlushExprTreeStack();
      WriteCloseNewline();
      if (!if_expr->false_.empty()) {
        WriteOpenNewline("else");
        WriteFoldedExprList(if_expr->false_);
        FlushExprTreeStack();
        WriteCloseNewline();
      }
      WriteCloseNewline();
      break;
    }

    case ExprType::IfExcept: {
      auto if_except_expr = cast<IfExceptExpr>(expr_tree.expr);
      WritePuts("(", NextChar::None);
      WriteBeginIfExceptBlock(if_except_expr);
      FlushExprTreeVector(expr_tree.children);
      WriteOpenNewline("then");
      WriteFoldedExprList(if_except_expr->true_.exprs);
      FlushExprTreeStack();
      WriteCloseNewline();
      if (!if_except_expr->false_.empty()) {
        WriteOpenNewline("else");
        WriteFoldedExprList(if_except_expr->false_);
        FlushExprTreeStack();
        WriteCloseNewline();
      }
      WriteCloseNewline();
      break;
    }

    case ExprType::Try: {
      auto try_expr = cast<TryExpr>(expr_tree.expr);
      WritePuts("(", NextChar::None);
      WriteBeginBlock(LabelType::Try, try_expr->block,
                      Opcode::Try_Opcode.GetName());
      FlushExprTreeVector(expr_tree.children);
      WriteFoldedExprList(try_expr->block.exprs);
      FlushExprTreeStack();
      WriteOpenNewline("catch");
      WriteFoldedExprList(try_expr->catch_);
      FlushExprTreeStack();
      WriteCloseNewline();
      WriteCloseNewline();
      break;
    }

    default: {
      WritePuts("(", NextChar::None);
      WriteExpr(expr_tree.expr);
      Indent();
      FlushExprTreeVector(expr_tree.children);
      WriteCloseNewline();
      break;
    }
  }
}

void WatWriter::FlushExprTreeVector(const std::vector<ExprTree>& expr_trees) {
  WABT_TRACE_ARGS(FlushExprTreeVector, "%zu", expr_trees.size());
  for (auto expr_tree : expr_trees) {
    FlushExprTree(expr_tree);
  }
}

void WatWriter::FlushExprTreeStack() {
  std::vector<ExprTree> stack_copy(std::move(expr_tree_stack_));
  expr_tree_stack_.clear();
  FlushExprTreeVector(stack_copy);
}

void WatWriter::WriteInitExpr(const ExprList& expr) {
  if (!expr.empty()) {
    WritePuts("(", NextChar::None);
    WriteExprList(expr);
    /* clear the next char, so we don't write a newline after the expr */
    next_char_ = NextChar::None;
    WritePuts(")", NextChar::Space);
  }
}

template <typename T>
void WatWriter::WriteTypeBindings(const char* prefix,
                                  const Func& func,
                                  const T& types,
                                  const BindingHash& bindings) {
  MakeTypeBindingReverseMapping(types.size(), bindings, &index_to_name_);

  /* named params/locals must be specified by themselves, but nameless
   * params/locals can be compressed, e.g.:
   *   (param $foo i32)
   *   (param i32 i64 f32)
   */
  bool is_open = false;
  size_t index = 0;
  for (Type type : types) {
    if (!is_open) {
      WriteOpenSpace(prefix);
      is_open = true;
    }

    const std::string& name = index_to_name_[index];
    if (!name.empty()) {
      WriteString(name, NextChar::Space);
    }
    WriteType(type, NextChar::Space);
    if (!name.empty()) {
      WriteCloseSpace();
      is_open = false;
    }
    ++index;
  }
  if (is_open) {
    WriteCloseSpace();
  }
}

void WatWriter::WriteBeginFunc(const Func& func) {
  WriteOpenSpace("func");
  WriteNameOrIndex(func.name, func_index_, NextChar::Space);
  WriteInlineExports(ExternalKind::Func, func_index_);
  WriteInlineImport(ExternalKind::Func, func_index_);
  if (func.decl.has_func_type) {
    WriteOpenSpace("type");
    WriteVar(func.decl.type_var, NextChar::None);
    WriteCloseSpace();
  }

  if (module_->IsImport(ExternalKind::Func, Var(func_index_))) {
    // Imported functions can be written a few ways:
    //
    //   1. (import "module" "field" (func (type 0)))
    //   2. (import "module" "field" (func (param i32) (result i32)))
    //   3. (func (import "module" "field") (type 0))
    //   4. (func (import "module" "field") (param i32) (result i32))
    //   5. (func (import "module" "field") (type 0) (param i32) (result i32))
    //
    // Note that the text format does not allow including the param/result
    // explicitly when using the "(import..." syntax (#1 and #2).
    if (options_->inline_import || !func.decl.has_func_type) {
      WriteFuncSigSpace(func.decl.sig);
    }
  }
  func_index_++;
}

void WatWriter::WriteFunc(const Func& func) {
  WriteBeginFunc(func);
  WriteTypeBindings("param", func, func.decl.sig.param_types,
                    func.param_bindings);
  WriteTypes(func.decl.sig.result_types, "result");
  WriteNewline(NO_FORCE_NEWLINE);
  if (func.local_types.size()) {
    WriteTypeBindings("local", func, func.local_types, func.local_bindings);
  }
  WriteNewline(NO_FORCE_NEWLINE);
  label_stack_.clear();
  label_stack_.emplace_back(LabelType::Func, std::string(), TypeVector(),
                            func.decl.sig.result_types);
  current_func_ = &func;
  if (options_->fold_exprs) {
    WriteFoldedExprList(func.exprs);
    FlushExprTreeStack();
  } else {
    WriteExprList(func.exprs);
  }
  current_func_ = nullptr;
  WriteCloseNewline();
}

void WatWriter::WriteBeginGlobal(const Global& global) {
  WriteOpenSpace("global");
  WriteNameOrIndex(global.name, global_index_, NextChar::Space);
  WriteInlineExports(ExternalKind::Global, global_index_);
  WriteInlineImport(ExternalKind::Global, global_index_);
  if (global.mutable_) {
    WriteOpenSpace("mut");
    WriteType(global.type, NextChar::Space);
    WriteCloseSpace();
  } else {
    WriteType(global.type, NextChar::Space);
  }
  global_index_++;
}

void WatWriter::WriteGlobal(const Global& global) {
  WriteBeginGlobal(global);
  WriteInitExpr(global.init_expr);
  WriteCloseNewline();
}

void WatWriter::WriteBeginException(const Exception& except) {
  WriteOpenSpace("except");
  WriteNameOrIndex(except.name, except_index_, NextChar::Space);
  WriteInlineExports(ExternalKind::Except, except_index_);
  WriteInlineImport(ExternalKind::Except, except_index_);
  WriteTypes(except.sig, nullptr);
  ++except_index_;
}

void WatWriter::WriteException(const Exception& except) {
  WriteBeginException(except);
  WriteCloseNewline();
}

void WatWriter::WriteLimits(const Limits& limits) {
  Writef("%" PRIu64, limits.initial);
  if (limits.has_max) {
    Writef("%" PRIu64, limits.max);
  }
  if (limits.is_shared) {
    Writef("shared");
  }
}

void WatWriter::WriteTable(const Table& table) {
  WriteOpenSpace("table");
  WriteNameOrIndex(table.name, table_index_, NextChar::Space);
  WriteInlineExports(ExternalKind::Table, table_index_);
  WriteInlineImport(ExternalKind::Table, table_index_);
  WriteLimits(table.elem_limits);
  WritePutsSpace("anyfunc");
  WriteCloseNewline();
  table_index_++;
}

void WatWriter::WriteElemSegment(const ElemSegment& segment) {
  WriteOpenSpace("elem");
  WriteInitExpr(segment.offset);
  for (const Var& var : segment.vars) {
    WriteVar(var, NextChar::Space);
  }
  WriteCloseNewline();
}

void WatWriter::WriteMemory(const Memory& memory) {
  WriteOpenSpace("memory");
  WriteNameOrIndex(memory.name, memory_index_, NextChar::Space);
  WriteInlineExports(ExternalKind::Memory, memory_index_);
  WriteInlineImport(ExternalKind::Memory, memory_index_);
  WriteLimits(memory.page_limits);
  WriteCloseNewline();
  memory_index_++;
}

void WatWriter::WriteDataSegment(const DataSegment& segment) {
  WriteOpenSpace("data");
  WriteInitExpr(segment.offset);
  WriteQuotedData(segment.data.data(), segment.data.size());
  WriteCloseNewline();
}

void WatWriter::WriteImport(const Import& import) {
  if (!options_->inline_import) {
    WriteOpenSpace("import");
    WriteQuotedString(import.module_name, NextChar::Space);
    WriteQuotedString(import.field_name, NextChar::Space);
  }

  switch (import.kind()) {
    case ExternalKind::Func:
      WriteBeginFunc(cast<FuncImport>(&import)->func);
      WriteCloseSpace();
      break;

    case ExternalKind::Table:
      WriteTable(cast<TableImport>(&import)->table);
      break;

    case ExternalKind::Memory:
      WriteMemory(cast<MemoryImport>(&import)->memory);
      break;

    case ExternalKind::Global:
      WriteBeginGlobal(cast<GlobalImport>(&import)->global);
      WriteCloseSpace();
      break;

    case ExternalKind::Except:
      WriteBeginException(cast<ExceptionImport>(&import)->except);
      WriteCloseSpace();
      break;
  }

  if (options_->inline_import) {
    WriteNewline(NO_FORCE_NEWLINE);
  } else {
    WriteCloseNewline();
  }
}

void WatWriter::WriteExport(const Export& export_) {
  if (options_->inline_export && IsInlineExport(export_)) {
    return;
  }
  WriteOpenSpace("export");
  WriteQuotedString(export_.name, NextChar::Space);
  WriteOpenSpace(GetKindName(export_.kind));
  WriteVar(export_.var, NextChar::Space);
  WriteCloseSpace();
  WriteCloseNewline();
}

void WatWriter::WriteFuncType(const FuncType& func_type) {
  WriteOpenSpace("type");
  WriteNameOrIndex(func_type.name, func_type_index_++, NextChar::Space);
  WriteOpenSpace("func");
  WriteFuncSigSpace(func_type.sig);
  WriteCloseSpace();
  WriteCloseNewline();
}

void WatWriter::WriteStartFunction(const Var& start) {
  WriteOpenSpace("start");
  WriteVar(start, NextChar::None);
  WriteCloseNewline();
}

Result WatWriter::WriteModule(const Module& module) {
  module_ = &module;
  BuildInlineExportMap();
  BuildInlineImportMap();
  WriteOpenSpace("module");
  if (module.name.empty()) {
    WriteNewline(NO_FORCE_NEWLINE);
  } else {
    WriteName(module.name, NextChar::Newline);
  }
  for (const ModuleField& field : module.fields) {
    switch (field.type()) {
      case ModuleFieldType::Func:
        WriteFunc(cast<FuncModuleField>(&field)->func);
        break;
      case ModuleFieldType::Global:
        WriteGlobal(cast<GlobalModuleField>(&field)->global);
        break;
      case ModuleFieldType::Import:
        WriteImport(*cast<ImportModuleField>(&field)->import);
        break;
      case ModuleFieldType::Except:
        WriteException(cast<ExceptionModuleField>(&field)->except);
        break;
      case ModuleFieldType::Export:
        WriteExport(cast<ExportModuleField>(&field)->export_);
        break;
      case ModuleFieldType::Table:
        WriteTable(cast<TableModuleField>(&field)->table);
        break;
      case ModuleFieldType::ElemSegment:
        WriteElemSegment(cast<ElemSegmentModuleField>(&field)->elem_segment);
        break;
      case ModuleFieldType::Memory:
        WriteMemory(cast<MemoryModuleField>(&field)->memory);
        break;
      case ModuleFieldType::DataSegment:
        WriteDataSegment(cast<DataSegmentModuleField>(&field)->data_segment);
        break;
      case ModuleFieldType::FuncType:
        WriteFuncType(cast<FuncTypeModuleField>(&field)->func_type);
        break;
      case ModuleFieldType::Start:
        WriteStartFunction(cast<StartModuleField>(&field)->start);
        break;
    }
  }
  WriteCloseNewline();
  /* force the newline to be written */
  WriteNextChar();
  return result_;
}

void WatWriter::BuildInlineExportMap() {
  if (!options_->inline_export) {
    return;
  }

  assert(module_);
  for (Export* export_ : module_->exports) {
    Index index = kInvalidIndex;

    // Exported imports can't be written with inline exports, unless the
    // imports are also inline. For example, the following is invalid:
    //
    //   (import "module" "field" (func (export "e")))
    //
    // But this is valid:
    //
    //   (func (export "e") (import "module" "field"))
    //
    if (!options_->inline_import && module_->IsImport(*export_)) {
      continue;
    }

    switch (export_->kind) {
      case ExternalKind::Func:
        index = module_->GetFuncIndex(export_->var);
        break;

      case ExternalKind::Table:
        index = module_->GetTableIndex(export_->var);
        break;

      case ExternalKind::Memory:
        index = module_->GetMemoryIndex(export_->var);
        break;

      case ExternalKind::Global:
        index = module_->GetGlobalIndex(export_->var);
        break;

      case ExternalKind::Except:
        index = module_->GetExceptIndex(export_->var);
        break;
    }

    if (index != kInvalidIndex) {
      auto key = std::make_pair(export_->kind, index);
      inline_export_map_.insert(std::make_pair(key, export_));
    }
  }
}

void WatWriter::WriteInlineExports(ExternalKind kind, Index index) {
  if (!options_->inline_export) {
    return;
  }

  auto iter_pair = inline_export_map_.equal_range(std::make_pair(kind, index));
  for (auto iter = iter_pair.first; iter != iter_pair.second; ++iter) {
    const Export* export_ = iter->second;
    WriteOpenSpace("export");
    WriteQuotedString(export_->name, NextChar::None);
    WriteCloseSpace();
  }
}

bool WatWriter::IsInlineExport(const Export& export_) {
  Index index;
  switch (export_.kind) {
    case ExternalKind::Func:
      index = module_->GetFuncIndex(export_.var);
      break;

    case ExternalKind::Table:
      index = module_->GetTableIndex(export_.var);
      break;

    case ExternalKind::Memory:
      index = module_->GetMemoryIndex(export_.var);
      break;

    case ExternalKind::Global:
      index = module_->GetGlobalIndex(export_.var);
      break;

    case ExternalKind::Except:
      index = module_->GetExceptIndex(export_.var);
      break;
  }

  return inline_export_map_.find(std::make_pair(export_.kind, index)) !=
         inline_export_map_.end();
}

void WatWriter::BuildInlineImportMap() {
  if (!options_->inline_import) {
    return;
  }

  assert(module_);
  for (const Import* import : module_->imports) {
    inline_import_map_[static_cast<size_t>(import->kind())].push_back(import);
  }
}

void WatWriter::WriteInlineImport(ExternalKind kind, Index index) {
  if (!options_->inline_import) {
    return;
  }

  size_t kind_index = static_cast<size_t>(kind);

  if (index >= inline_import_map_[kind_index].size()) {
    return;
  }

  const Import* import = inline_import_map_[kind_index][index];
  WriteOpenSpace("import");
  WriteQuotedString(import->module_name, NextChar::Space);
  WriteQuotedString(import->field_name, NextChar::Space);
  WriteCloseSpace();
}

}  // end anonymous namespace

Result WriteWat(Stream* stream,
                const Module* module,
                const WriteWatOptions* options) {
  WatWriter wat_writer(stream, options);
  return wat_writer.WriteModule(*module);
}

}  // namespace wabt
