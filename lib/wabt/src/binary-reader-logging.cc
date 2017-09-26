/*
 * Copyright 2017 WebAssembly Community Group participants
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

#include "src/binary-reader-logging.h"

#include <cinttypes>

#include "src/stream.h"

namespace wabt {

#define INDENT_SIZE 2

#define LOGF_NOINDENT(...) stream_->Writef(__VA_ARGS__)

#define LOGF(...)               \
  do {                          \
    WriteIndent();              \
    LOGF_NOINDENT(__VA_ARGS__); \
  } while (0)

namespace {

void SPrintLimits(char* dst, size_t size, const Limits* limits) {
  int result;
  if (limits->has_max) {
    result = wabt_snprintf(dst, size, "initial: %" PRIu64 ", max: %" PRIu64,
                           limits->initial, limits->max);
  } else {
    result = wabt_snprintf(dst, size, "initial: %" PRIu64, limits->initial);
  }
  WABT_USE(result);
  assert(static_cast<size_t>(result) < size);
}

}  // end anonymous namespace

BinaryReaderLogging::BinaryReaderLogging(Stream* stream,
                                         BinaryReaderDelegate* forward)
    : stream_(stream), reader_(forward), indent_(0) {}

void BinaryReaderLogging::Indent() {
  indent_ += INDENT_SIZE;
}

void BinaryReaderLogging::Dedent() {
  indent_ -= INDENT_SIZE;
  assert(indent_ >= 0);
}

void BinaryReaderLogging::WriteIndent() {
  static char s_indent[] =
      "                                                                       "
      "                                                                       ";
  static const size_t s_indent_len = sizeof(s_indent) - 1;
  size_t i = indent_;
  while (i > s_indent_len) {
    stream_->WriteData(s_indent, s_indent_len);
    i -= s_indent_len;
  }
  if (i > 0) {
    stream_->WriteData(s_indent, indent_);
  }
}

void BinaryReaderLogging::LogTypes(Index type_count, Type* types) {
  LOGF_NOINDENT("[");
  for (Index i = 0; i < type_count; ++i) {
    LOGF_NOINDENT("%s", GetTypeName(types[i]));
    if (i != type_count - 1)
      LOGF_NOINDENT(", ");
  }
  LOGF_NOINDENT("]");
}

void BinaryReaderLogging::LogTypes(TypeVector& types) {
  LogTypes(types.size(), types.data());
}

bool BinaryReaderLogging::OnError(const char* message) {
  return reader_->OnError(message);
}

void BinaryReaderLogging::OnSetState(const State* s) {
  BinaryReaderDelegate::OnSetState(s);
  reader_->OnSetState(s);
}

Result BinaryReaderLogging::BeginModule(uint32_t version) {
  LOGF("BeginModule(version: %u)\n", version);
  Indent();
  return reader_->BeginModule(version);
}

Result BinaryReaderLogging::BeginSection(BinarySection section_type,
                                         Offset size) {
  return reader_->BeginSection(section_type, size);
}

Result BinaryReaderLogging::BeginCustomSection(Offset size,
                                               string_view section_name) {
  LOGF("BeginCustomSection('" PRIstringview "', size: %" PRIzd ")\n",
       WABT_PRINTF_STRING_VIEW_ARG(section_name), size);
  Indent();
  return reader_->BeginCustomSection(size, section_name);
}

Result BinaryReaderLogging::OnType(Index index,
                                   Index param_count,
                                   Type* param_types,
                                   Index result_count,
                                   Type* result_types) {
  LOGF("OnType(index: %" PRIindex ", params: ", index);
  LogTypes(param_count, param_types);
  LOGF_NOINDENT(", results: ");
  LogTypes(result_count, result_types);
  LOGF_NOINDENT(")\n");
  return reader_->OnType(index, param_count, param_types, result_count,
                         result_types);
}

Result BinaryReaderLogging::OnImport(Index index,
                                     string_view module_name,
                                     string_view field_name) {
  LOGF("OnImport(index: %" PRIindex ", module: \"" PRIstringview
       "\", field: \"" PRIstringview "\")\n",
       index, WABT_PRINTF_STRING_VIEW_ARG(module_name),
       WABT_PRINTF_STRING_VIEW_ARG(field_name));
  return reader_->OnImport(index, module_name, field_name);
}

Result BinaryReaderLogging::OnImportFunc(Index import_index,
                                         string_view module_name,
                                         string_view field_name,
                                         Index func_index,
                                         Index sig_index) {
  LOGF("OnImportFunc(import_index: %" PRIindex ", func_index: %" PRIindex
       ", sig_index: %" PRIindex ")\n",
       import_index, func_index, sig_index);
  return reader_->OnImportFunc(import_index, module_name, field_name,
                               func_index, sig_index);
}

Result BinaryReaderLogging::OnImportTable(Index import_index,
                                          string_view module_name,
                                          string_view field_name,
                                          Index table_index,
                                          Type elem_type,
                                          const Limits* elem_limits) {
  char buf[100];
  SPrintLimits(buf, sizeof(buf), elem_limits);
  LOGF("OnImportTable(import_index: %" PRIindex ", table_index: %" PRIindex
       ", elem_type: %s, %s)\n",
       import_index, table_index, GetTypeName(elem_type), buf);
  return reader_->OnImportTable(import_index, module_name, field_name,
                                table_index, elem_type, elem_limits);
}

Result BinaryReaderLogging::OnImportMemory(Index import_index,
                                           string_view module_name,
                                           string_view field_name,
                                           Index memory_index,
                                           const Limits* page_limits) {
  char buf[100];
  SPrintLimits(buf, sizeof(buf), page_limits);
  LOGF("OnImportMemory(import_index: %" PRIindex ", memory_index: %" PRIindex
       ", %s)\n",
       import_index, memory_index, buf);
  return reader_->OnImportMemory(import_index, module_name, field_name,
                                 memory_index, page_limits);
}

Result BinaryReaderLogging::OnImportGlobal(Index import_index,
                                           string_view module_name,
                                           string_view field_name,
                                           Index global_index,
                                           Type type,
                                           bool mutable_) {
  LOGF("OnImportGlobal(import_index: %" PRIindex ", global_index: %" PRIindex
       ", type: %s, mutable: "
       "%s)\n",
       import_index, global_index, GetTypeName(type),
       mutable_ ? "true" : "false");
  return reader_->OnImportGlobal(import_index, module_name, field_name,
                                 global_index, type, mutable_);
}

Result BinaryReaderLogging::OnImportException(Index import_index,
                                              string_view module_name,
                                              string_view field_name,
                                              Index except_index,
                                              TypeVector& sig) {
  LOGF("OnImportException(import_index: %" PRIindex ", except_index: %" PRIindex
       ", sig: ",
       import_index, except_index);
  LogTypes(sig);
  LOGF_NOINDENT(")\n");
  return reader_->OnImportException(import_index, module_name, field_name,
                                    except_index, sig);
}

Result BinaryReaderLogging::OnTable(Index index,
                                    Type elem_type,
                                    const Limits* elem_limits) {
  char buf[100];
  SPrintLimits(buf, sizeof(buf), elem_limits);
  LOGF("OnTable(index: %" PRIindex ", elem_type: %s, %s)\n", index,
       GetTypeName(elem_type), buf);
  return reader_->OnTable(index, elem_type, elem_limits);
}

Result BinaryReaderLogging::OnMemory(Index index, const Limits* page_limits) {
  char buf[100];
  SPrintLimits(buf, sizeof(buf), page_limits);
  LOGF("OnMemory(index: %" PRIindex ", %s)\n", index, buf);
  return reader_->OnMemory(index, page_limits);
}

Result BinaryReaderLogging::BeginGlobal(Index index, Type type, bool mutable_) {
  LOGF("BeginGlobal(index: %" PRIindex ", type: %s, mutable: %s)\n", index,
       GetTypeName(type), mutable_ ? "true" : "false");
  return reader_->BeginGlobal(index, type, mutable_);
}

Result BinaryReaderLogging::OnExport(Index index,
                                     ExternalKind kind,
                                     Index item_index,
                                     string_view name) {
  LOGF("OnExport(index: %" PRIindex ", kind: %s, item_index: %" PRIindex
       ", name: \"" PRIstringview "\")\n",
       index, GetKindName(kind), item_index, WABT_PRINTF_STRING_VIEW_ARG(name));
  return reader_->OnExport(index, kind, item_index, name);
}

Result BinaryReaderLogging::OnLocalDecl(Index decl_index,
                                        Index count,
                                        Type type) {
  LOGF("OnLocalDecl(index: %" PRIindex ", count: %" PRIindex ", type: %s)\n",
       decl_index, count, GetTypeName(type));
  return reader_->OnLocalDecl(decl_index, count, type);
}

Result BinaryReaderLogging::OnBlockExpr(Index num_types, Type* sig_types) {
  LOGF("OnBlockExpr(sig: ");
  LogTypes(num_types, sig_types);
  LOGF_NOINDENT(")\n");
  return reader_->OnBlockExpr(num_types, sig_types);
}

Result BinaryReaderLogging::OnBrExpr(Index depth) {
  LOGF("OnBrExpr(depth: %" PRIindex ")\n", depth);
  return reader_->OnBrExpr(depth);
}

Result BinaryReaderLogging::OnBrIfExpr(Index depth) {
  LOGF("OnBrIfExpr(depth: %" PRIindex ")\n", depth);
  return reader_->OnBrIfExpr(depth);
}

Result BinaryReaderLogging::OnBrTableExpr(Index num_targets,
                                          Index* target_depths,
                                          Index default_target_depth) {
  LOGF("OnBrTableExpr(num_targets: %" PRIindex ", depths: [", num_targets);
  for (Index i = 0; i < num_targets; ++i) {
    LOGF_NOINDENT("%" PRIindex, target_depths[i]);
    if (i != num_targets - 1)
      LOGF_NOINDENT(", ");
  }
  LOGF_NOINDENT("], default: %" PRIindex ")\n", default_target_depth);
  return reader_->OnBrTableExpr(num_targets, target_depths,
                                default_target_depth);
}

Result BinaryReaderLogging::OnExceptionType(Index index, TypeVector& sig) {
  LOGF("OnType(index: %" PRIindex ", values: ", index);
  LogTypes(sig);
  LOGF_NOINDENT(")\n");
  return reader_->OnExceptionType(index, sig);
}

Result BinaryReaderLogging::OnF32ConstExpr(uint32_t value_bits) {
  float value;
  memcpy(&value, &value_bits, sizeof(value));
  LOGF("OnF32ConstExpr(%g (0x04%x))\n", value, value_bits);
  return reader_->OnF32ConstExpr(value_bits);
}

Result BinaryReaderLogging::OnF64ConstExpr(uint64_t value_bits) {
  double value;
  memcpy(&value, &value_bits, sizeof(value));
  LOGF("OnF64ConstExpr(%g (0x08%" PRIx64 "))\n", value, value_bits);
  return reader_->OnF64ConstExpr(value_bits);
}

Result BinaryReaderLogging::OnI32ConstExpr(uint32_t value) {
  LOGF("OnI32ConstExpr(%u (0x%x))\n", value, value);
  return reader_->OnI32ConstExpr(value);
}

Result BinaryReaderLogging::OnI64ConstExpr(uint64_t value) {
  LOGF("OnI64ConstExpr(%" PRIu64 " (0x%" PRIx64 "))\n", value, value);
  return reader_->OnI64ConstExpr(value);
}

Result BinaryReaderLogging::OnIfExpr(Index num_types, Type* sig_types) {
  LOGF("OnIfExpr(sig: ");
  LogTypes(num_types, sig_types);
  LOGF_NOINDENT(")\n");
  return reader_->OnIfExpr(num_types, sig_types);
}

Result BinaryReaderLogging::OnLoadExpr(Opcode opcode,
                                       uint32_t alignment_log2,
                                       Address offset) {
  LOGF("OnLoadExpr(opcode: \"%s\" (%u), align log2: %u, offset: %" PRIaddress
       ")\n",
       opcode.GetName(), opcode.GetCode(), alignment_log2, offset);
  return reader_->OnLoadExpr(opcode, alignment_log2, offset);
}

Result BinaryReaderLogging::OnLoopExpr(Index num_types, Type* sig_types) {
  LOGF("OnLoopExpr(sig: ");
  LogTypes(num_types, sig_types);
  LOGF_NOINDENT(")\n");
  return reader_->OnLoopExpr(num_types, sig_types);
}

Result BinaryReaderLogging::OnStoreExpr(Opcode opcode,
                                        uint32_t alignment_log2,
                                        Address offset) {
  LOGF("OnStoreExpr(opcode: \"%s\" (%u), align log2: %u, offset: %" PRIaddress
       ")\n",
       opcode.GetName(), opcode.GetCode(), alignment_log2, offset);
  return reader_->OnStoreExpr(opcode, alignment_log2, offset);
}

Result BinaryReaderLogging::OnTryExpr(Index num_types, Type* sig_types) {
  LOGF("OnTryExpr(sig: ");
  LogTypes(num_types, sig_types);
  LOGF_NOINDENT(")\n");
  return reader_->OnTryExpr(num_types, sig_types);
}

Result BinaryReaderLogging::OnDataSegmentData(Index index,
                                              const void* data,
                                              Address size) {
  LOGF("OnDataSegmentData(index:%" PRIindex ", size:%" PRIaddress ")\n", index,
       size);
  return reader_->OnDataSegmentData(index, data, size);
}

Result BinaryReaderLogging::OnFunctionNameSubsection(Index index,
                                                     uint32_t name_type,
                                                     Offset subsection_size) {
  LOGF("OnFunctionNameSubsection(index:%" PRIindex ", nametype:%u, size:%" PRIzd
       ")\n",
       index, name_type, subsection_size);
  return reader_->OnFunctionNameSubsection(index, name_type, subsection_size);
}

Result BinaryReaderLogging::OnFunctionName(Index index, string_view name) {
  LOGF("OnFunctionName(index: %" PRIindex ", name: \"" PRIstringview "\")\n",
       index, WABT_PRINTF_STRING_VIEW_ARG(name));
  return reader_->OnFunctionName(index, name);
}

Result BinaryReaderLogging::OnLocalNameSubsection(Index index,
                                                  uint32_t name_type,
                                                  Offset subsection_size) {
  LOGF("OnLocalNameSubsection(index:%" PRIindex ", nametype:%u, size:%" PRIzd
       ")\n",
       index, name_type, subsection_size);
  return reader_->OnLocalNameSubsection(index, name_type, subsection_size);
}

Result BinaryReaderLogging::OnLocalName(Index func_index,
                                        Index local_index,
                                        string_view name) {
  LOGF("OnLocalName(func_index: %" PRIindex ", local_index: %" PRIindex
       ", name: \"" PRIstringview "\")\n",
       func_index, local_index, WABT_PRINTF_STRING_VIEW_ARG(name));
  return reader_->OnLocalName(func_index, local_index, name);
}

Result BinaryReaderLogging::OnInitExprF32ConstExpr(Index index,
                                                   uint32_t value_bits) {
  float value;
  memcpy(&value, &value_bits, sizeof(value));
  LOGF("OnInitExprF32ConstExpr(index: %" PRIindex ", value: %g (0x04%x))\n",
       index, value, value_bits);
  return reader_->OnInitExprF32ConstExpr(index, value_bits);
}

Result BinaryReaderLogging::OnInitExprF64ConstExpr(Index index,
                                                   uint64_t value_bits) {
  double value;
  memcpy(&value, &value_bits, sizeof(value));
  LOGF("OnInitExprF64ConstExpr(index: %" PRIindex " value: %g (0x08%" PRIx64
       "))\n",
       index, value, value_bits);
  return reader_->OnInitExprF64ConstExpr(index, value_bits);
}

Result BinaryReaderLogging::OnInitExprI32ConstExpr(Index index,
                                                   uint32_t value) {
  LOGF("OnInitExprI32ConstExpr(index: %" PRIindex ", value: %u)\n", index,
       value);
  return reader_->OnInitExprI32ConstExpr(index, value);
}

Result BinaryReaderLogging::OnInitExprI64ConstExpr(Index index,
                                                   uint64_t value) {
  LOGF("OnInitExprI64ConstExpr(index: %" PRIindex ", value: %" PRIu64 ")\n",
       index, value);
  return reader_->OnInitExprI64ConstExpr(index, value);
}

Result BinaryReaderLogging::OnRelocCount(Index count,
                                         BinarySection section_code,
                                         string_view section_name) {
  LOGF("OnRelocCount(count: %" PRIindex
       ", section: %s, section_name: " PRIstringview ")\n",
       count, GetSectionName(section_code),
       WABT_PRINTF_STRING_VIEW_ARG(section_name));
  return reader_->OnRelocCount(count, section_code, section_name);
}

Result BinaryReaderLogging::OnReloc(RelocType type,
                                    Offset offset,
                                    Index index,
                                    uint32_t addend) {
  int32_t signed_addend = static_cast<int32_t>(addend);
  LOGF("OnReloc(type: %s, offset: %" PRIzd ", index: %" PRIindex
       ", addend: %d)\n",
       GetRelocTypeName(type), offset, index, signed_addend);
  return reader_->OnReloc(type, offset, index, addend);
}

Result BinaryReaderLogging::OnSymbolInfo(string_view name, uint32_t flags) {
  LOGF("(OnSymbolInfo name: " PRIstringview ", flags: 0x%x)\n",
       WABT_PRINTF_STRING_VIEW_ARG(name), flags);
  return reader_->OnSymbolInfo(name, flags);
}

#define DEFINE_BEGIN(name)                        \
  Result BinaryReaderLogging::name(Offset size) { \
    LOGF(#name "(%" PRIzd ")\n", size);           \
    Indent();                                     \
    return reader_->name(size);                   \
  }

#define DEFINE_END(name)               \
  Result BinaryReaderLogging::name() { \
    Dedent();                          \
    LOGF(#name "\n");                  \
    return reader_->name();            \
  }

#define DEFINE_INDEX(name)                        \
  Result BinaryReaderLogging::name(Index value) { \
    LOGF(#name "(%" PRIindex ")\n", value);       \
    return reader_->name(value);                  \
  }

#define DEFINE_INDEX_DESC(name, desc)                 \
  Result BinaryReaderLogging::name(Index value) {     \
    LOGF(#name "(" desc ": %" PRIindex ")\n", value); \
    return reader_->name(value);                      \
  }

#define DEFINE_INDEX_INDEX(name, desc0, desc1)                           \
  Result BinaryReaderLogging::name(Index value0, Index value1) {         \
    LOGF(#name "(" desc0 ": %" PRIindex ", " desc1 ": %" PRIindex ")\n", \
         value0, value1);                                                \
    return reader_->name(value0, value1);                                \
  }

#define DEFINE_OPCODE(name)                                            \
  Result BinaryReaderLogging::name(Opcode opcode) {                    \
    LOGF(#name "(\"%s\" (%u))\n", opcode.GetName(), opcode.GetCode()); \
    return reader_->name(opcode);                                      \
  }

#define DEFINE0(name)                  \
  Result BinaryReaderLogging::name() { \
    LOGF(#name "\n");                  \
    return reader_->name();            \
  }

DEFINE_END(EndModule)

DEFINE_END(EndCustomSection)

DEFINE_BEGIN(BeginTypeSection)
DEFINE_INDEX(OnTypeCount)
DEFINE_END(EndTypeSection)

DEFINE_BEGIN(BeginImportSection)
DEFINE_INDEX(OnImportCount)
DEFINE_END(EndImportSection)

DEFINE_BEGIN(BeginFunctionSection)
DEFINE_INDEX(OnFunctionCount)
DEFINE_INDEX_INDEX(OnFunction, "index", "sig_index")
DEFINE_END(EndFunctionSection)

DEFINE_BEGIN(BeginTableSection)
DEFINE_INDEX(OnTableCount)
DEFINE_END(EndTableSection)

DEFINE_BEGIN(BeginMemorySection)
DEFINE_INDEX(OnMemoryCount)
DEFINE_END(EndMemorySection)

DEFINE_BEGIN(BeginGlobalSection)
DEFINE_INDEX(OnGlobalCount)
DEFINE_INDEX(BeginGlobalInitExpr)
DEFINE_INDEX(EndGlobalInitExpr)
DEFINE_INDEX(EndGlobal)
DEFINE_END(EndGlobalSection)

DEFINE_BEGIN(BeginExportSection)
DEFINE_INDEX(OnExportCount)
DEFINE_END(EndExportSection)

DEFINE_BEGIN(BeginStartSection)
DEFINE_INDEX(OnStartFunction)
DEFINE_END(EndStartSection)

DEFINE_BEGIN(BeginCodeSection)
DEFINE_INDEX(OnFunctionBodyCount)
DEFINE_INDEX(BeginFunctionBody)
DEFINE_INDEX(EndFunctionBody)
DEFINE_INDEX(OnLocalDeclCount)
DEFINE_OPCODE(OnBinaryExpr)
DEFINE_INDEX_DESC(OnCallExpr, "func_index")
DEFINE_INDEX_DESC(OnCallIndirectExpr, "sig_index")
DEFINE_INDEX_DESC(OnCatchExpr, "except_index");
DEFINE0(OnCatchAllExpr)
DEFINE_OPCODE(OnCompareExpr)
DEFINE_OPCODE(OnConvertExpr)
DEFINE0(OnCurrentMemoryExpr)
DEFINE0(OnDropExpr)
DEFINE0(OnElseExpr)
DEFINE0(OnEndExpr)
DEFINE_INDEX_DESC(OnGetGlobalExpr, "index")
DEFINE_INDEX_DESC(OnGetLocalExpr, "index")
DEFINE0(OnGrowMemoryExpr)
DEFINE0(OnNopExpr)
DEFINE_INDEX_DESC(OnRethrowExpr, "depth");
DEFINE0(OnReturnExpr)
DEFINE0(OnSelectExpr)
DEFINE_INDEX_DESC(OnSetGlobalExpr, "index")
DEFINE_INDEX_DESC(OnSetLocalExpr, "index")
DEFINE_INDEX_DESC(OnTeeLocalExpr, "index")
DEFINE_INDEX_DESC(OnThrowExpr, "except_index")
DEFINE0(OnUnreachableExpr)
DEFINE_OPCODE(OnUnaryExpr)
DEFINE_END(EndCodeSection)

DEFINE_BEGIN(BeginElemSection)
DEFINE_INDEX(OnElemSegmentCount)
DEFINE_INDEX_INDEX(BeginElemSegment, "index", "table_index")
DEFINE_INDEX(BeginElemSegmentInitExpr)
DEFINE_INDEX(EndElemSegmentInitExpr)
DEFINE_INDEX_INDEX(OnElemSegmentFunctionIndexCount, "index", "count")
DEFINE_INDEX_INDEX(OnElemSegmentFunctionIndex, "index", "func_index")
DEFINE_INDEX(EndElemSegment)
DEFINE_END(EndElemSection)

DEFINE_BEGIN(BeginDataSection)
DEFINE_INDEX(OnDataSegmentCount)
DEFINE_INDEX_INDEX(BeginDataSegment, "index", "memory_index")
DEFINE_INDEX(BeginDataSegmentInitExpr)
DEFINE_INDEX(EndDataSegmentInitExpr)
DEFINE_INDEX(EndDataSegment)
DEFINE_END(EndDataSection)

DEFINE_BEGIN(BeginNamesSection)
DEFINE_INDEX(OnFunctionNamesCount)
DEFINE_INDEX(OnLocalNameFunctionCount)
DEFINE_INDEX_INDEX(OnLocalNameLocalCount, "index", "count")
DEFINE_END(EndNamesSection)

DEFINE_BEGIN(BeginRelocSection)
DEFINE_END(EndRelocSection)
DEFINE_INDEX_INDEX(OnInitExprGetGlobalExpr, "index", "global_index")

DEFINE_BEGIN(BeginLinkingSection)
DEFINE_INDEX(OnSymbolInfoCount)
DEFINE_INDEX(OnStackGlobal)
DEFINE_INDEX(OnDataSize)
DEFINE_INDEX(OnDataAlignment)
DEFINE_END(EndLinkingSection)

DEFINE_BEGIN(BeginExceptionSection);
DEFINE_INDEX(OnExceptionCount);

DEFINE_END(EndExceptionSection);

// We don't need to log these (the individual opcodes are logged instead), but
// we still need to forward the calls.
Result BinaryReaderLogging::OnOpcode(Opcode opcode) {
  return reader_->OnOpcode(opcode);
}

Result BinaryReaderLogging::OnOpcodeBare() {
  return reader_->OnOpcodeBare();
}

Result BinaryReaderLogging::OnOpcodeIndex(Index value) {
  return reader_->OnOpcodeIndex(value);
}

Result BinaryReaderLogging::OnOpcodeUint32(uint32_t value) {
  return reader_->OnOpcodeUint32(value);
}

Result BinaryReaderLogging::OnOpcodeUint32Uint32(uint32_t value,
                                                 uint32_t value2) {
  return reader_->OnOpcodeUint32Uint32(value, value2);
}

Result BinaryReaderLogging::OnOpcodeUint64(uint64_t value) {
  return reader_->OnOpcodeUint64(value);
}

Result BinaryReaderLogging::OnOpcodeF32(uint32_t value) {
  return reader_->OnOpcodeF32(value);
}

Result BinaryReaderLogging::OnOpcodeF64(uint64_t value) {
  return reader_->OnOpcodeF64(value);
}

Result BinaryReaderLogging::OnOpcodeBlockSig(Index num_types, Type* sig_types) {
  return reader_->OnOpcodeBlockSig(num_types, sig_types);
}

Result BinaryReaderLogging::OnEndFunc() {
  return reader_->OnEndFunc();
}

}  // namespace wabt
