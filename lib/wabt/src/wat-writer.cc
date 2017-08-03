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

#include "wat-writer.h"

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cstdarg>
#include <cstdio>
#include <map>
#include <iterator>
#include <string>
#include <vector>

#include "common.h"
#include "ir.h"
#include "literal.h"
#include "stream.h"
#include "writer.h"

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

enum class NextChar {
  None,
  Space,
  Newline,
  ForceNewline,
};

struct ExprTree {
  explicit ExprTree(const Expr* expr) : expr(expr) {}

  const Expr* expr;
  std::vector<ExprTree> children;
};

struct Label {
  Label(LabelType label_type, StringSlice name, const BlockSignature& sig)
      : name(name), label_type(label_type), sig(sig) {}

  StringSlice name;
  LabelType label_type;
  const BlockSignature& sig;  // Share with Expr.
};

class WatWriter {
 public:
  WatWriter(Writer* writer, const WriteWatOptions* options)
      : options_(options), stream_(writer) {}

  Result WriteModule(const Module* module);

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
  void WriteStringSlice(const StringSlice* str, NextChar next_char);
  bool WriteStringSliceOpt(const StringSlice* str, NextChar next_char);
  void WriteName(const StringSlice* str, NextChar next_char);
  void WriteNameOrIndex(const StringSlice* str,
                        Index index,
                        NextChar next_char);
  void WriteQuotedData(const void* data, size_t length);
  void WriteQuotedStringSlice(const StringSlice* str, NextChar next_char);
  void WriteVar(const Var* var, NextChar next_char);
  void WriteBrVar(const Var* var, NextChar next_char);
  void WriteType(Type type, NextChar next_char);
  void WriteTypes(const TypeVector& types, const char* name);
  void WriteFuncSigSpace(const FuncSignature* func_sig);
  void WriteBeginBlock(LabelType label_type,
                       const Block* block,
                       const char* text);
  void WriteEndBlock();
  void WriteBlock(LabelType label_type,
                  const Block* block,
                  const char* start_text);
  void WriteConst(const Const* const_);
  void WriteExpr(const Expr* expr);
  void WriteExprList(const Expr* first);
  void WriteInitExpr(const Expr* expr);
  void WriteTypeBindings(const char* prefix,
                         const Func* func,
                         const TypeVector& types,
                         const BindingHash& bindings);
  void WriteFunc(const Module* module, const Func* func);
  void WriteBeginGlobal(const Global* global);
  void WriteGlobal(const Global* global);
  void WriteLimits(const Limits* limits);
  void WriteTable(const Table* table);
  void WriteElemSegment(const ElemSegment* segment);
  void WriteMemory(const Memory* memory);
  void WriteDataSegment(const DataSegment* segment);
  void WriteImport(const Import* import);
  void WriteExport(const Export* export_);
  void WriteFuncType(const FuncType* func_type);
  void WriteStartFunction(const Var* start);

  Index GetLabelStackSize() { return label_stack_.size(); }
  Label* GetLabel(const Var* var);
  Index GetLabelArity(const Var* var);
  Index GetFuncParamCount(const Var* var);
  Index GetFuncResultCount(const Var* var);
  Index GetFuncSigParamCount(const Var* var);
  Index GetFuncSigResultCount(const Var* var);
  void PushExpr(const Expr* expr, Index operand_count, Index result_count);
  void FlushExprTree(const ExprTree& expr_tree);
  void FlushExprTreeVector(const std::vector<ExprTree>&);
  void FlushExprTreeStack();
  void WriteFoldedExpr(const Expr* first);
  void WriteFoldedExprList(const Expr* first);

  void BuildExportMaps();
  void WriteInlineExport(const Export* export_);

  const WriteWatOptions* options_ = nullptr;
  const Module* module_ = nullptr;
  const Func* current_func_ = nullptr;
  Stream stream_;
  Result result_ = Result::Ok;
  int indent_ = 0;
  NextChar next_char_ = NextChar::None;
  std::vector<std::string> index_to_name_;
  std::vector<Label> label_stack_;
  std::vector<ExprTree> expr_tree_stack_;
  std::vector<const Export*> func_to_export_map_;
  std::vector<const Export*> global_to_export_map_;
  std::vector<const Export*> table_to_export_map_;
  std::vector<const Export*> memory_to_export_map_;

  Index func_index_ = 0;
  Index global_index_ = 0;
  Index table_index_ = 0;
  Index memory_index_ = 0;
  Index func_type_index_ = 0;
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
  size_t indent = indent_;
  while (static_cast<size_t>(indent_) > s_indent_len) {
    stream_.WriteData(s_indent, s_indent_len);
    indent -= s_indent_len;
  }
  if (indent > 0) {
    stream_.WriteData(s_indent, indent);
  }
}

void WatWriter::WriteNextChar() {
  switch (next_char_) {
    case NextChar::Space:
      stream_.WriteChar(' ');
      break;
    case NextChar::Newline:
    case NextChar::ForceNewline:
      stream_.WriteChar('\n');
      WriteIndent();
      break;

    default:
    case NextChar::None:
      break;
  }
  next_char_ = NextChar::None;
}

void WatWriter::WriteDataWithNextChar(const void* src, size_t size) {
  WriteNextChar();
  stream_.WriteData(src, size);
}

void WABT_PRINTF_FORMAT(2, 3) WatWriter::Writef(const char* format, ...) {
  WABT_SNPRINTF_ALLOCA(buffer, length, format);
  /* default to following space */
  WriteDataWithNextChar(buffer, length);
  next_char_ = NextChar::Space;
}

void WatWriter::WritePutc(char c) {
  stream_.WriteChar(c);
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
  if (next_char_ == NextChar::ForceNewline)
    WriteNextChar();
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
  if (next_char_ != NextChar::ForceNewline)
    next_char_ = NextChar::None;
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

void WatWriter::WriteStringSlice(const StringSlice* str, NextChar next_char) {
  Writef(PRIstringslice, WABT_PRINTF_STRING_SLICE_ARG(*str));
  next_char_ = next_char;
}

bool WatWriter::WriteStringSliceOpt(const StringSlice* str,
                                    NextChar next_char) {
  if (str->start)
    WriteStringSlice(str, next_char);
  return !!str->start;
}

void WatWriter::WriteName(const StringSlice* str, NextChar next_char) {
  // Debug names must begin with a $ for for wast file to be valid
  assert(str->length > 0 && str->start[0] == '$');
  WriteStringSlice(str, next_char);
}

void WatWriter::WriteNameOrIndex(const StringSlice* str,
                                 Index index,
                                 NextChar next_char) {
  if (str->start)
    WriteName(str, next_char);
  else
    Writef("(;%u;)", index);
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

void WatWriter::WriteQuotedStringSlice(const StringSlice* str,
                                       NextChar next_char) {
  WriteQuotedData(str->start, str->length);
  next_char_ = next_char;
}

void WatWriter::WriteVar(const Var* var, NextChar next_char) {
  if (var->type == VarType::Index) {
    Writef("%" PRIindex, var->index);
    next_char_ = next_char;
  } else {
    WriteName(&var->name, next_char);
  }
}

void WatWriter::WriteBrVar(const Var* var, NextChar next_char) {
  if (var->type == VarType::Index) {
    if (var->index < GetLabelStackSize()) {
      Writef("%" PRIindex " (;@%" PRIindex ";)", var->index,
             GetLabelStackSize() - var->index - 1);
    } else {
      Writef("%" PRIindex " (; INVALID ;)", var->index);
    }
    next_char_ = next_char;
  } else {
    WriteStringSlice(&var->name, next_char);
  }
}

void WatWriter::WriteType(Type type, NextChar next_char) {
  const char* type_name = get_type_name(type);
  assert(type_name);
  WritePuts(type_name, next_char);
}

void WatWriter::WriteTypes(const TypeVector& types, const char* name) {
  if (types.size()) {
    if (name)
      WriteOpenSpace(name);
    for (Type type : types)
      WriteType(type, NextChar::Space);
    if (name)
      WriteCloseSpace();
  }
}

void WatWriter::WriteFuncSigSpace(const FuncSignature* func_sig) {
  WriteTypes(func_sig->param_types, "param");
  WriteTypes(func_sig->result_types, "result");
}

void WatWriter::WriteBeginBlock(LabelType label_type,
                                const Block* block,
                                const char* text) {
  WritePutsSpace(text);
  bool has_label = WriteStringSliceOpt(&block->label, NextChar::Space);
  WriteTypes(block->sig, "result");
  if (!has_label)
    Writef(" ;; label = @%" PRIindex, GetLabelStackSize());
  WriteNewline(FORCE_NEWLINE);
  label_stack_.emplace_back(label_type, block->label, block->sig);
  Indent();
}

void WatWriter::WriteEndBlock() {
  Dedent();
  label_stack_.pop_back();
  WritePutsNewline(get_opcode_name(Opcode::End));
}

void WatWriter::WriteBlock(LabelType label_type,
                           const Block* block,
                           const char* start_text) {
  WriteBeginBlock(label_type, block, start_text);
  WriteExprList(block->first);
  WriteEndBlock();
}

void WatWriter::WriteConst(const Const* const_) {
  switch (const_->type) {
    case Type::I32:
      WritePutsSpace(get_opcode_name(Opcode::I32Const));
      Writef("%d", static_cast<int32_t>(const_->u32));
      WriteNewline(NO_FORCE_NEWLINE);
      break;

    case Type::I64:
      WritePutsSpace(get_opcode_name(Opcode::I64Const));
      Writef("%" PRId64, static_cast<int64_t>(const_->u64));
      WriteNewline(NO_FORCE_NEWLINE);
      break;

    case Type::F32: {
      WritePutsSpace(get_opcode_name(Opcode::F32Const));
      char buffer[128];
      write_float_hex(buffer, 128, const_->f32_bits);
      WritePutsSpace(buffer);
      float f32;
      memcpy(&f32, &const_->f32_bits, sizeof(f32));
      Writef("(;=%g;)", f32);
      WriteNewline(NO_FORCE_NEWLINE);
      break;
    }

    case Type::F64: {
      WritePutsSpace(get_opcode_name(Opcode::F64Const));
      char buffer[128];
      write_double_hex(buffer, 128, const_->f64_bits);
      WritePutsSpace(buffer);
      double f64;
      memcpy(&f64, &const_->f64_bits, sizeof(f64));
      Writef("(;=%g;)", f64);
      WriteNewline(NO_FORCE_NEWLINE);
      break;
    }

    default:
      assert(0);
      break;
  }
}

void WatWriter::WriteExpr(const Expr* expr) {
  switch (expr->type) {
    case ExprType::Binary:
      WritePutsNewline(get_opcode_name(expr->binary.opcode));
      break;

    case ExprType::Block:
      WriteBlock(LabelType::Block, expr->block, get_opcode_name(Opcode::Block));
      break;

    case ExprType::Br:
      WritePutsSpace(get_opcode_name(Opcode::Br));
      WriteBrVar(&expr->br.var, NextChar::Newline);
      break;

    case ExprType::BrIf:
      WritePutsSpace(get_opcode_name(Opcode::BrIf));
      WriteBrVar(&expr->br_if.var, NextChar::Newline);
      break;

    case ExprType::BrTable: {
      WritePutsSpace(get_opcode_name(Opcode::BrTable));
      for (const Var& var : *expr->br_table.targets)
        WriteBrVar(&var, NextChar::Space);
      WriteBrVar(&expr->br_table.default_target, NextChar::Newline);
      break;
    }

    case ExprType::Call:
      WritePutsSpace(get_opcode_name(Opcode::Call));
      WriteVar(&expr->call.var, NextChar::Newline);
      break;

    case ExprType::CallIndirect:
      WritePutsSpace(get_opcode_name(Opcode::CallIndirect));
      WriteVar(&expr->call_indirect.var, NextChar::Newline);
      break;

    case ExprType::Compare:
      WritePutsNewline(get_opcode_name(expr->compare.opcode));
      break;

    case ExprType::Const:
      WriteConst(&expr->const_);
      break;

    case ExprType::Convert:
      WritePutsNewline(get_opcode_name(expr->convert.opcode));
      break;

    case ExprType::Drop:
      WritePutsNewline(get_opcode_name(Opcode::Drop));
      break;

    case ExprType::GetGlobal:
      WritePutsSpace(get_opcode_name(Opcode::GetGlobal));
      WriteVar(&expr->get_global.var, NextChar::Newline);
      break;

    case ExprType::GetLocal:
      WritePutsSpace(get_opcode_name(Opcode::GetLocal));
      WriteVar(&expr->get_local.var, NextChar::Newline);
      break;

    case ExprType::GrowMemory:
      WritePutsNewline(get_opcode_name(Opcode::GrowMemory));
      break;

    case ExprType::If:
      WriteBeginBlock(LabelType::If, expr->if_.true_,
                      get_opcode_name(Opcode::If));
      WriteExprList(expr->if_.true_->first);
      if (expr->if_.false_) {
        Dedent();
        WritePutsSpace(get_opcode_name(Opcode::Else));
        Indent();
        WriteNewline(FORCE_NEWLINE);
        WriteExprList(expr->if_.false_);
      }
      WriteEndBlock();
      break;

    case ExprType::Load:
      WritePutsSpace(get_opcode_name(expr->load.opcode));
      if (expr->load.offset)
        Writef("offset=%u", expr->load.offset);
      if (!is_naturally_aligned(expr->load.opcode, expr->load.align))
        Writef("align=%u", expr->load.align);
      WriteNewline(NO_FORCE_NEWLINE);
      break;

    case ExprType::Loop:
      WriteBlock(LabelType::Loop, expr->loop, get_opcode_name(Opcode::Loop));
      break;

    case ExprType::CurrentMemory:
      WritePutsNewline(get_opcode_name(Opcode::CurrentMemory));
      break;

    case ExprType::Nop:
      WritePutsNewline(get_opcode_name(Opcode::Nop));
      break;

    case ExprType::Return:
      WritePutsNewline(get_opcode_name(Opcode::Return));
      break;

    case ExprType::Select:
      WritePutsNewline(get_opcode_name(Opcode::Select));
      break;

    case ExprType::SetGlobal:
      WritePutsSpace(get_opcode_name(Opcode::SetGlobal));
      WriteVar(&expr->set_global.var, NextChar::Newline);
      break;

    case ExprType::SetLocal:
      WritePutsSpace(get_opcode_name(Opcode::SetLocal));
      WriteVar(&expr->set_local.var, NextChar::Newline);
      break;

    case ExprType::Store:
      WritePutsSpace(get_opcode_name(expr->store.opcode));
      if (expr->store.offset)
        Writef("offset=%u", expr->store.offset);
      if (!is_naturally_aligned(expr->store.opcode, expr->store.align))
        Writef("align=%u", expr->store.align);
      WriteNewline(NO_FORCE_NEWLINE);
      break;

    case ExprType::TeeLocal:
      WritePutsSpace(get_opcode_name(Opcode::TeeLocal));
      WriteVar(&expr->tee_local.var, NextChar::Newline);
      break;

    case ExprType::Unary:
      WritePutsNewline(get_opcode_name(expr->unary.opcode));
      break;

    case ExprType::Unreachable:
      WritePutsNewline(get_opcode_name(Opcode::Unreachable));
      break;

    default:
      fprintf(stderr, "bad expr type: %d\n", static_cast<int>(expr->type));
      assert(0);
      break;
  }
}

void WatWriter::WriteExprList(const Expr* first) {
  for (const Expr* expr = first; expr; expr = expr->next)
    WriteExpr(expr);
}

Label* WatWriter::GetLabel(const Var* var) {
  if (var->type == VarType::Name) {
    for (Index i = GetLabelStackSize(); i > 0; --i) {
      Label* label = &label_stack_[i - 1];
      if (string_slices_are_equal(&label->name, &var->name))
        return label;
    }
  } else if (var->index < GetLabelStackSize()) {
    Label* label = &label_stack_[GetLabelStackSize() - var->index - 1];
    return label;
  }
  return nullptr;
}


Index WatWriter::GetLabelArity(const Var* var) {
  Label* label = GetLabel(var);
  return label && label->label_type != LabelType::Loop ? label->sig.size() : 0;
}

Index WatWriter::GetFuncParamCount(const Var* var) {
  const Func* func = module_->GetFunc(*var);
  return func ? func->GetNumParams() : 0;
}

Index WatWriter::GetFuncResultCount(const Var* var) {
  const Func* func = module_->GetFunc(*var);
  return func ? func->GetNumResults() : 0;
}

Index WatWriter::GetFuncSigParamCount(const Var* var) {
  const FuncType* func_type = module_->GetFuncType(*var);
  return func_type ? func_type->GetNumParams() : 0;
}

Index WatWriter::GetFuncSigResultCount(const Var* var) {
  const FuncType* func_type = module_->GetFuncType(*var);
  return func_type ? func_type->GetNumResults() : 0;
}

void WatWriter::WriteFoldedExpr(const Expr* expr) {
  switch (expr->type) {
    case ExprType::Binary:
    case ExprType::Compare:
    case ExprType::Store:
      PushExpr(expr, 2, 1);
      break;

    case ExprType::Block:
      PushExpr(expr, 0, expr->block->sig.size());
      break;

    case ExprType::Br:
      PushExpr(expr, GetLabelArity(&expr->br.var), 1);
      break;

    case ExprType::BrIf: {
      Index arity = GetLabelArity(&expr->br_if.var);
      PushExpr(expr, arity + 1, arity);
      break;
    }

    case ExprType::BrTable:
      PushExpr(expr, GetLabelArity(&expr->br_table.default_target) + 1, 1);
      break;

    case ExprType::Call:
      PushExpr(expr, GetFuncParamCount(&expr->call.var),
               GetFuncResultCount(&expr->call.var));
      break;

    case ExprType::CallIndirect:
      PushExpr(expr, GetFuncSigParamCount(&expr->call_indirect.var) + 1,
               GetFuncSigResultCount(&expr->call_indirect.var));
      break;

    case ExprType::Const:
    case ExprType::CurrentMemory:
    case ExprType::GetGlobal:
    case ExprType::GetLocal:
    case ExprType::Unreachable:
      PushExpr(expr, 0, 1);
      break;

    case ExprType::Convert:
    case ExprType::GrowMemory:
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
      PushExpr(expr, 1, expr->if_.true_->sig.size());
      break;

    case ExprType::Loop:
      PushExpr(expr, 0, expr->loop->sig.size());
      break;

    case ExprType::Nop:
      PushExpr(expr, 0, 0);
      break;

    case ExprType::Return:
      PushExpr(expr, current_func_->decl.sig.result_types.size(), 1);
      break;

    case ExprType::Select:
      PushExpr(expr, 3, 1);
      break;

    default:
      fprintf(stderr, "bad expr type: %d\n", static_cast<int>(expr->type));
      assert(0);
      break;
  }
}

void WatWriter::WriteFoldedExprList(const Expr* first) {
  for (const Expr* expr = first; expr; expr = expr->next)
    WriteFoldedExpr(expr);
}

void WatWriter::PushExpr(const Expr* expr,
                         Index operand_count,
                         Index result_count) {
  if (operand_count <= expr_tree_stack_.size()) {
    auto last_operand = expr_tree_stack_.end();
    auto first_operand = last_operand - operand_count;
    ExprTree tree(expr);
    std::move(first_operand, last_operand, std::back_inserter(tree.children));
    expr_tree_stack_.erase(first_operand, last_operand);
    expr_tree_stack_.push_back(std::move(tree));
    if (result_count == 0)
      FlushExprTreeStack();
  } else {
    expr_tree_stack_.emplace_back(expr);
    FlushExprTreeStack();
  }
}

void WatWriter::FlushExprTree(const ExprTree& expr_tree) {
  switch (expr_tree.expr->type) {
    case ExprType::Block:
      WritePuts("(", NextChar::None);
      WriteBeginBlock(LabelType::Block, expr_tree.expr->block,
                      get_opcode_name(Opcode::Block));
      WriteFoldedExprList(expr_tree.expr->block->first);
      FlushExprTreeStack();
      WriteCloseNewline();
      break;

    case ExprType::Loop:
      WritePuts("(", NextChar::None);
      WriteBeginBlock(LabelType::Loop, expr_tree.expr->loop,
                      get_opcode_name(Opcode::Loop));
      WriteFoldedExprList(expr_tree.expr->loop->first);
      FlushExprTreeStack();
      WriteCloseNewline();
      break;

    case ExprType::If:
      WritePuts("(", NextChar::None);
      WriteBeginBlock(LabelType::If, expr_tree.expr->if_.true_,
                      get_opcode_name(Opcode::If));
      FlushExprTreeVector(expr_tree.children);
      WriteOpenNewline("then");
      WriteFoldedExprList(expr_tree.expr->if_.true_->first);
      FlushExprTreeStack();
      WriteCloseNewline();
      if (expr_tree.expr->if_.false_) {
        WriteOpenNewline("else");
        WriteFoldedExprList(expr_tree.expr->if_.false_);
        FlushExprTreeStack();
        WriteCloseNewline();
      }
      WriteCloseNewline();
      break;

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
  for (auto expr_tree : expr_trees)
    FlushExprTree(expr_tree);
}

void WatWriter::FlushExprTreeStack() {
  std::vector<ExprTree> stack_copy(std::move(expr_tree_stack_));
  expr_tree_stack_.clear();
  FlushExprTreeVector(stack_copy);
}

void WatWriter::WriteInitExpr(const Expr* expr) {
  if (expr) {
    WritePuts("(", NextChar::None);
    WriteExpr(expr);
    /* clear the next char, so we don't write a newline after the expr */
    next_char_ = NextChar::None;
    WritePuts(")", NextChar::Space);
  }
}

void WatWriter::WriteTypeBindings(const char* prefix,
                                  const Func* func,
                                  const TypeVector& types,
                                  const BindingHash& bindings) {
  MakeTypeBindingReverseMapping(types, bindings, &index_to_name_);

  /* named params/locals must be specified by themselves, but nameless
   * params/locals can be compressed, e.g.:
   *   (param $foo i32)
   *   (param i32 i64 f32)
   */
  bool is_open = false;
  for (size_t i = 0; i < types.size(); ++i) {
    if (!is_open) {
      WriteOpenSpace(prefix);
      is_open = true;
    }

    const std::string& name = index_to_name_[i];
    if (!name.empty())
      WriteString(name, NextChar::Space);
    WriteType(types[i], NextChar::Space);
    if (!name.empty()) {
      WriteCloseSpace();
      is_open = false;
    }
  }
  if (is_open)
    WriteCloseSpace();
}

void WatWriter::WriteFunc(const Module* module, const Func* func) {
  WriteOpenSpace("func");
  WriteNameOrIndex(&func->name, func_index_, NextChar::Space);
  WriteInlineExport(func_to_export_map_[func_index_]);
  if (func->decl.has_func_type) {
    WriteOpenSpace("type");
    WriteVar(&func->decl.type_var, NextChar::None);
    WriteCloseSpace();
  }
  WriteTypeBindings("param", func, func->decl.sig.param_types,
                    func->param_bindings);
  WriteTypes(func->decl.sig.result_types, "result");
  WriteNewline(NO_FORCE_NEWLINE);
  if (func->local_types.size()) {
    WriteTypeBindings("local", func, func->local_types, func->local_bindings);
  }
  WriteNewline(NO_FORCE_NEWLINE);
  label_stack_.clear();
  label_stack_.emplace_back(LabelType::Func, empty_string_slice(),
                            func->decl.sig.result_types);
  current_func_ = func;
  if (options_->fold_exprs) {
    WriteFoldedExprList(func->first_expr);
    FlushExprTreeStack();
  } else {
    WriteExprList(func->first_expr);
  }
  current_func_ = nullptr;
  WriteCloseNewline();
  func_index_++;
}

void WatWriter::WriteBeginGlobal(const Global* global) {
  WriteOpenSpace("global");
  WriteNameOrIndex(&global->name, global_index_, NextChar::Space);
  WriteInlineExport(global_to_export_map_[global_index_]);
  if (global->mutable_) {
    WriteOpenSpace("mut");
    WriteType(global->type, NextChar::Space);
    WriteCloseSpace();
  } else {
    WriteType(global->type, NextChar::Space);
  }
  global_index_++;
}

void WatWriter::WriteGlobal(const Global* global) {
  WriteBeginGlobal(global);
  WriteInitExpr(global->init_expr);
  WriteCloseNewline();
}

void WatWriter::WriteLimits(const Limits* limits) {
  Writef("%" PRIu64, limits->initial);
  if (limits->has_max)
    Writef("%" PRIu64, limits->max);
}

void WatWriter::WriteTable(const Table* table) {
  WriteOpenSpace("table");
  WriteNameOrIndex(&table->name, table_index_, NextChar::Space);
  WriteInlineExport(table_to_export_map_[table_index_]);
  WriteLimits(&table->elem_limits);
  WritePutsSpace("anyfunc");
  WriteCloseNewline();
  table_index_++;
}

void WatWriter::WriteElemSegment(const ElemSegment* segment) {
  WriteOpenSpace("elem");
  WriteInitExpr(segment->offset);
  for (const Var& var : segment->vars)
    WriteVar(&var, NextChar::Space);
  WriteCloseNewline();
}

void WatWriter::WriteMemory(const Memory* memory) {
  WriteOpenSpace("memory");
  WriteNameOrIndex(&memory->name, memory_index_, NextChar::Space);
  WriteInlineExport(memory_to_export_map_[memory_index_]);
  WriteLimits(&memory->page_limits);
  WriteCloseNewline();
  memory_index_++;
}

void WatWriter::WriteDataSegment(const DataSegment* segment) {
  WriteOpenSpace("data");
  WriteInitExpr(segment->offset);
  WriteQuotedData(segment->data, segment->size);
  WriteCloseNewline();
}

void WatWriter::WriteImport(const Import* import) {
  WriteOpenSpace("import");
  WriteQuotedStringSlice(&import->module_name, NextChar::Space);
  WriteQuotedStringSlice(&import->field_name, NextChar::Space);
  switch (import->kind) {
    case ExternalKind::Func:
      WriteOpenSpace("func");
      WriteNameOrIndex(&import->func->name, func_index_++, NextChar::Space);
      if (import->func->decl.has_func_type) {
        WriteOpenSpace("type");
        WriteVar(&import->func->decl.type_var, NextChar::None);
        WriteCloseSpace();
      } else {
        WriteFuncSigSpace(&import->func->decl.sig);
      }
      WriteCloseSpace();
      break;

    case ExternalKind::Table:
      WriteTable(import->table);
      break;

    case ExternalKind::Memory:
      WriteMemory(import->memory);
      break;

    case ExternalKind::Global:
      WriteBeginGlobal(import->global);
      WriteCloseSpace();
      break;

    case ExternalKind::Except:
      // TODO(karlschimpf) Define
      WABT_FATAL("WriteImport(except) not implemented");
      break;
  }
  WriteCloseNewline();
}

void WatWriter::WriteExport(const Export* export_) {
  if (options_->inline_export)
    return;
  WriteOpenSpace("export");
  WriteQuotedStringSlice(&export_->name, NextChar::Space);
  WriteOpenSpace(get_kind_name(export_->kind));
  WriteVar(&export_->var, NextChar::Space);
  WriteCloseSpace();
  WriteCloseNewline();
}

void WatWriter::WriteFuncType(const FuncType* func_type) {
  WriteOpenSpace("type");
  WriteNameOrIndex(&func_type->name, func_type_index_++, NextChar::Space);
  WriteOpenSpace("func");
  WriteFuncSigSpace(&func_type->sig);
  WriteCloseSpace();
  WriteCloseNewline();
}

void WatWriter::WriteStartFunction(const Var* start) {
  WriteOpenSpace("start");
  WriteVar(start, NextChar::None);
  WriteCloseNewline();
}

Result WatWriter::WriteModule(const Module* module) {
  module_ = module;
  BuildExportMaps();
  WriteOpenNewline("module");
  for (const ModuleField* field = module->first_field; field;
       field = field->next) {
    switch (field->type) {
      case ModuleFieldType::Func:
        WriteFunc(module, field->func);
        break;
      case ModuleFieldType::Global:
        WriteGlobal(field->global);
        break;
      case ModuleFieldType::Import:
        WriteImport(field->import);
        break;
      case ModuleFieldType::Except:
        // TODO(karlschimpf) Define
        WABT_FATAL("WriteModule(except) not implemented");
        break;
      case ModuleFieldType::Export:
        WriteExport(field->export_);
        break;
      case ModuleFieldType::Table:
        WriteTable(field->table);
        break;
      case ModuleFieldType::ElemSegment:
        WriteElemSegment(field->elem_segment);
        break;
      case ModuleFieldType::Memory:
        WriteMemory(field->memory);
        break;
      case ModuleFieldType::DataSegment:
        WriteDataSegment(field->data_segment);
        break;
      case ModuleFieldType::FuncType:
        WriteFuncType(field->func_type);
        break;
      case ModuleFieldType::Start:
        WriteStartFunction(&field->start);
        break;
    }
  }
  WriteCloseNewline();
  /* force the newline to be written */
  WriteNextChar();
  return result_;
}

void WatWriter::BuildExportMaps() {
  assert(module_);
  func_to_export_map_.resize(module_->funcs.size());
  global_to_export_map_.resize(module_->globals.size());
  table_to_export_map_.resize(module_->tables.size());
  memory_to_export_map_.resize(module_->memories.size());
  for (Export* export_ : module_->exports) {
    switch (export_->kind) {
      case ExternalKind::Func: {
        Index func_index = module_->GetFuncIndex(export_->var);
        if (func_index != kInvalidIndex)
          func_to_export_map_[func_index] = export_;
        break;
      }

      case ExternalKind::Table: {
        Index table_index = module_->GetTableIndex(export_->var);
        if (table_index != kInvalidIndex)
          table_to_export_map_[table_index] = export_;
        break;
      }

      case ExternalKind::Memory: {
        Index memory_index = module_->GetMemoryIndex(export_->var);
        if (memory_index != kInvalidIndex)
          memory_to_export_map_[memory_index] = export_;
        break;
      }

      case ExternalKind::Global: {
        Index global_index = module_->GetGlobalIndex(export_->var);
        if (global_index != kInvalidIndex)
          global_to_export_map_[global_index] = export_;
        break;
      }

      case ExternalKind::Except:
        WABT_FATAL("BuildExportMaps(except) not implemented");
        break;
    }
  }
}

void WatWriter::WriteInlineExport(const Export* export_) {
  if (export_ && options_->inline_export) {
    WriteOpenSpace("export");
    WriteQuotedStringSlice(&export_->name, NextChar::None);
    WriteCloseSpace();
  }
}

}  // namespace

Result write_wat(Writer* writer,
                 const Module* module,
                 const WriteWatOptions* options) {
  WatWriter wat_writer(writer, options);
  return wat_writer.WriteModule(module);
}

}  // namespace wabt
