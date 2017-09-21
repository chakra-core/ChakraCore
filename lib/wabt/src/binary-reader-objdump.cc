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

#include "src/binary-reader-objdump.h"

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <vector>

#include "src/binary-reader-nop.h"
#include "src/literal.h"

namespace wabt {

namespace {

class BinaryReaderObjdumpBase : public BinaryReaderNop {
 public:
  BinaryReaderObjdumpBase(const uint8_t* data,
                          size_t size,
                          ObjdumpOptions* options,
                          ObjdumpState* state);

  Result BeginModule(uint32_t version) override;
  Result BeginSection(BinarySection section_type, Offset size) override;

  Result OnRelocCount(Index count,
                      BinarySection section_code,
                      string_view section_name) override;

 protected:
  const char* GetFunctionName(Index index) const;
  void PrintRelocation(const Reloc& reloc, Offset offset) const;
  Offset GetSectionStart(BinarySection section_code) const {
    return section_starts_[static_cast<size_t>(section_code)];
  }

  ObjdumpOptions* options_;
  ObjdumpState* objdump_state_;
  const uint8_t* data_;
  size_t size_;
  bool print_details_ = false;
  BinarySection reloc_section_ = BinarySection::Invalid;
  Offset section_starts_[kBinarySectionCount];
  bool section_found_ = false;
};

BinaryReaderObjdumpBase::BinaryReaderObjdumpBase(const uint8_t* data,
                                                 size_t size,
                                                 ObjdumpOptions* options,
                                                 ObjdumpState* objdump_state)
    : options_(options),
      objdump_state_(objdump_state),
      data_(data),
      size_(size) {
  ZeroMemory(section_starts_);
}

Result BinaryReaderObjdumpBase::BeginSection(BinarySection section_code,
                                             Offset size) {
  section_starts_[static_cast<size_t>(section_code)] = state->offset;
  return Result::Ok;
}

Result BinaryReaderObjdumpBase::BeginModule(uint32_t version) {
  switch (options_->mode) {
    case ObjdumpMode::Headers:
      printf("\n");
      printf("Sections:\n\n");
      break;
    case ObjdumpMode::Details:
      printf("\n");
      printf("Section Details:\n\n");
      break;
    case ObjdumpMode::Disassemble:
      printf("\n");
      printf("Code Disassembly:\n\n");
      break;
    case ObjdumpMode::Prepass: {
      const char* last_slash = strrchr(options_->filename, '/');
      const char* last_backslash = strrchr(options_->filename, '\\');
      const char* basename;
      if (last_slash && last_backslash) {
        basename = std::max(last_slash, last_backslash) + 1;
      } else if (last_slash) {
        basename = last_slash + 1;
      } else if (last_backslash) {
        basename = last_backslash + 1;
      } else {
        basename = options_->filename;
      }

      printf("%s:\tfile format wasm %#x\n", basename, version);
      break;
    }
    case ObjdumpMode::RawData:
      break;
  }

  return Result::Ok;
}

const char* BinaryReaderObjdumpBase::GetFunctionName(Index index) const {
  if (index >= objdump_state_->function_names.size() ||
      objdump_state_->function_names[index].empty())
    return nullptr;

  return objdump_state_->function_names[index].c_str();
}

void BinaryReaderObjdumpBase::PrintRelocation(const Reloc& reloc,
                                              Offset offset) const {
  printf("           %06" PRIzx ": %-18s %" PRIindex "", offset,
         GetRelocTypeName(reloc.type), reloc.index);
  switch (reloc.type) {
    case RelocType::MemoryAddressLEB:
    case RelocType::MemoryAddressSLEB:
    case RelocType::MemoryAddressI32:
      printf(" + %d", reloc.addend);
      break;
    case RelocType::FuncIndexLEB:
      if (const char* name = GetFunctionName(reloc.index)) {
        printf(" <%s>", name);
      }
    default:
      break;
  }
  printf("\n");
}

Result BinaryReaderObjdumpBase::OnRelocCount(Index count,
                                             BinarySection section_code,
                                             string_view section_name) {
  reloc_section_ = section_code;
  return Result::Ok;
}

class BinaryReaderObjdumpPrepass : public BinaryReaderObjdumpBase {
 public:
  using BinaryReaderObjdumpBase::BinaryReaderObjdumpBase;

  Result OnFunctionName(Index function_index,
                        string_view function_name) override;
  Result OnReloc(RelocType type,
                 Offset offset,
                 Index index,
                 uint32_t addend) override;
};

Result BinaryReaderObjdumpPrepass::OnFunctionName(Index index,
                                                  string_view name) {
  objdump_state_->function_names.resize(index + 1);
  objdump_state_->function_names[index] = name.to_string();
  return Result::Ok;
}

Result BinaryReaderObjdumpPrepass::OnReloc(RelocType type,
                                           Offset offset,
                                           Index index,
                                           uint32_t addend) {
  BinaryReaderObjdumpBase::OnReloc(type, offset, index, addend);
  if (reloc_section_ == BinarySection::Code) {
    objdump_state_->code_relocations.emplace_back(type, offset, index, addend);
  } else if (reloc_section_ == BinarySection::Data) {
    objdump_state_->data_relocations.emplace_back(type, offset, index, addend);
  }
  return Result::Ok;
}

class BinaryReaderObjdumpDisassemble : public BinaryReaderObjdumpBase {
 public:
  using BinaryReaderObjdumpBase::BinaryReaderObjdumpBase;

  Result BeginFunctionBody(Index index) override;

  Result OnOpcode(Opcode Opcode) override;
  Result OnOpcodeBare() override;
  Result OnOpcodeIndex(Index value) override;
  Result OnOpcodeUint32(uint32_t value) override;
  Result OnOpcodeUint32Uint32(uint32_t value, uint32_t value2) override;
  Result OnOpcodeUint64(uint64_t value) override;
  Result OnOpcodeF32(uint32_t value) override;
  Result OnOpcodeF64(uint64_t value) override;
  Result OnOpcodeBlockSig(Index num_types, Type* sig_types) override;

  Result OnBrTableExpr(Index num_targets,
                       Index* target_depths,
                       Index default_target_depth) override;
  Result OnEndExpr() override;
  Result OnEndFunc() override;

 private:
  void LogOpcode(const uint8_t* data, size_t data_size, const char* fmt, ...);

  Opcode current_opcode = Opcode::Unreachable;
  Offset current_opcode_offset = 0;
  Offset last_opcode_end = 0;
  int indent_level = 0;
  Index next_reloc = 0;
};

Result BinaryReaderObjdumpDisassemble::OnOpcode(Opcode opcode) {
  if (options_->debug) {
    const char* opcode_name = opcode.GetName();
    printf("on_opcode: %#" PRIzx ": %s\n", state->offset, opcode_name);
  }

  if (last_opcode_end) {
    if (state->offset != last_opcode_end + opcode.GetLength()) {
      Opcode missing_opcode = Opcode::FromCode(data_[last_opcode_end]);
      const char* opcode_name = missing_opcode.GetName();
      fprintf(stderr,
              "warning: %#" PRIzx " missing opcode callback at %#" PRIzx
              " (%#02x=%s)\n",
              state->offset, last_opcode_end + 1, data_[last_opcode_end],
              opcode_name);
      return Result::Error;
    }
  }

  current_opcode_offset = state->offset;
  current_opcode = opcode;
  return Result::Ok;
}

#define IMMEDIATE_OCTET_COUNT 9

void BinaryReaderObjdumpDisassemble::LogOpcode(const uint8_t* data,
                                               size_t data_size,
                                               const char* fmt,
                                               ...) {
  Offset offset = current_opcode_offset;

  // Print binary data
  printf(" %06" PRIzx ":", offset - 1);
  if (current_opcode.HasPrefix())
    printf(" %02x", current_opcode.GetPrefix());
  printf(" %02x", current_opcode.GetCode());
  for (size_t i = 0; i < data_size && i < IMMEDIATE_OCTET_COUNT;
       i++, offset++) {
    printf(" %02x", data[offset]);
  }
  for (size_t i = data_size + current_opcode.GetLength();
       i < IMMEDIATE_OCTET_COUNT; i++) {
    printf("   ");
  }
  printf(" | ");

  // Print disassemble
  int indent_level = this->indent_level;
  switch (current_opcode) {
    case Opcode::Else:
    case Opcode::Catch:
    case Opcode::CatchAll:
      indent_level--;
    default:
      break;
  }
  for (int j = 0; j < indent_level; j++) {
    printf("  ");
  }

  const char* opcode_name = current_opcode.GetName();
  printf("%s", opcode_name);
  if (fmt) {
    printf(" ");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
  }

  printf("\n");

  last_opcode_end = current_opcode_offset + data_size;

  if (options_->relocs &&
      next_reloc < objdump_state_->code_relocations.size()) {
    const Reloc& reloc = objdump_state_->code_relocations[next_reloc];
    Offset code_start = GetSectionStart(BinarySection::Code);
    Offset abs_offset = code_start + reloc.offset;
    if (last_opcode_end > abs_offset) {
      PrintRelocation(reloc, abs_offset);
      next_reloc++;
    }
  }
}

Result BinaryReaderObjdumpDisassemble::OnOpcodeBare() {
  LogOpcode(data_, 0, nullptr);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnOpcodeIndex(Index value) {
  Offset immediate_len = state->offset - current_opcode_offset;
  const char* name;
  if (current_opcode == Opcode::Call && (name = GetFunctionName(value)))
    LogOpcode(data_, immediate_len, "%d <%s>", value, name);
  else
    LogOpcode(data_, immediate_len, "%d", value);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnOpcodeUint32(uint32_t value) {
  Offset immediate_len = state->offset - current_opcode_offset;
  LogOpcode(data_, immediate_len, "%u", value);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnOpcodeUint32Uint32(uint32_t value,
                                                            uint32_t value2) {
  Offset immediate_len = state->offset - current_opcode_offset;
  LogOpcode(data_, immediate_len, "%lu %lu", value, value2);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnOpcodeUint64(uint64_t value) {
  Offset immediate_len = state->offset - current_opcode_offset;
  LogOpcode(data_, immediate_len, "%" PRId64, value);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnOpcodeF32(uint32_t value) {
  Offset immediate_len = state->offset - current_opcode_offset;
  char buffer[WABT_MAX_FLOAT_HEX];
  WriteFloatHex(buffer, sizeof(buffer), value);
  LogOpcode(data_, immediate_len, buffer);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnOpcodeF64(uint64_t value) {
  Offset immediate_len = state->offset - current_opcode_offset;
  char buffer[WABT_MAX_DOUBLE_HEX];
  WriteDoubleHex(buffer, sizeof(buffer), value);
  LogOpcode(data_, immediate_len, buffer);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnBrTableExpr(
    Index num_targets,
    Index* target_depths,
    Index default_target_depth) {
  Offset immediate_len = state->offset - current_opcode_offset;
  /* TODO(sbc): Print targets */
  LogOpcode(data_, immediate_len, nullptr);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnEndFunc() {
  LogOpcode(nullptr, 0, nullptr);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnEndExpr() {
  indent_level--;
  assert(indent_level >= 0);
  LogOpcode(nullptr, 0, nullptr);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::BeginFunctionBody(Index index) {
  const char* name = GetFunctionName(index);
  if (name)
    printf("%06" PRIzx " <%s>:\n", state->offset, name);
  else
    printf("%06" PRIzx " func[%" PRIindex "]:\n", state->offset, index);

  last_opcode_end = 0;
  return Result::Ok;
}

const char* type_name(Type type) {
  switch (type) {
    case Type::I32:
      return "i32";

    case Type::I64:
      return "i64";

    case Type::F32:
      return "f32";

    case Type::F64:
      return "f64";

    default:
      assert(0);
      return "INVALID TYPE";
  }
}

Result BinaryReaderObjdumpDisassemble::OnOpcodeBlockSig(Index num_types,
                                                        Type* sig_types) {
  if (num_types)
    LogOpcode(data_, 1, "%s", type_name(*sig_types));
  else
    LogOpcode(data_, 1, nullptr);
  indent_level++;
  return Result::Ok;
}

enum class InitExprType {
  I32,
  F32,
  I64,
  F64,
  Global,
};

struct InitExpr {
  InitExprType type;
  union {
    Index global;
    uint32_t i32;
    uint32_t f32;
    uint64_t i64;
    uint64_t f64;
  } value;
};

class BinaryReaderObjdump : public BinaryReaderObjdumpBase {
 public:
  BinaryReaderObjdump(const uint8_t* data,
                      size_t size,
                      ObjdumpOptions* options,
                      ObjdumpState* state);

  Result EndModule() override;
  Result BeginSection(BinarySection section_type, Offset size) override;
  Result BeginCustomSection(Offset size, string_view section_name) override;

  Result OnTypeCount(Index count) override;
  Result OnType(Index index,
                Index param_count,
                Type* param_types,
                Index result_count,
                Type* result_types) override;

  Result OnImportCount(Index count) override;
  Result OnImportFunc(Index import_index,
                      string_view module_name,
                      string_view field_name,
                      Index func_index,
                      Index sig_index) override;
  Result OnImportTable(Index import_index,
                       string_view module_name,
                       string_view field_name,
                       Index table_index,
                       Type elem_type,
                       const Limits* elem_limits) override;
  Result OnImportMemory(Index import_index,
                        string_view module_name,
                        string_view field_name,
                        Index memory_index,
                        const Limits* page_limits) override;
  Result OnImportGlobal(Index import_index,
                        string_view module_name,
                        string_view field_name,
                        Index global_index,
                        Type type,
                        bool mutable_) override;
  Result OnImportException(Index import_index,
                           string_view module_name,
                           string_view field_name,
                           Index except_index,
                           TypeVector& sig) override;

  Result OnFunctionCount(Index count) override;
  Result OnFunction(Index index, Index sig_index) override;

  Result OnTableCount(Index count) override;
  Result OnTable(Index index,
                 Type elem_type,
                 const Limits* elem_limits) override;

  Result OnMemoryCount(Index count) override;
  Result OnMemory(Index index, const Limits* limits) override;

  Result OnGlobalCount(Index count) override;
  Result BeginGlobal(Index index, Type type, bool mutable_) override;

  Result OnExportCount(Index count) override;
  Result OnExport(Index index,
                  ExternalKind kind,
                  Index item_index,
                  string_view name) override;

  Result OnStartFunction(Index func_index) override;

  Result OnFunctionBodyCount(Index count) override;

  Result OnElemSegmentCount(Index count) override;
  Result BeginElemSegment(Index index, Index table_index) override;
  Result OnElemSegmentFunctionIndex(Index segment_index,
                                    Index func_index) override;

  Result BeginDataSection(Offset size) override {
    in_data_section_ = true;
    return Result::Ok;
  }
  Result EndDataSection() override {
    in_data_section_ = false;
    return Result::Ok;
  }
  Result OnDataSegmentCount(Index count) override;
  Result BeginDataSegment(Index index, Index memory_index) override;
  Result OnDataSegmentData(Index index,
                           const void* data,
                           Address size) override;

  Result OnFunctionName(Index function_index,
                        string_view function_name) override;
  Result OnLocalName(Index function_index,
                     Index local_index,
                     string_view local_name) override;

  Result OnInitExprF32ConstExpr(Index index, uint32_t value) override;
  Result OnInitExprF64ConstExpr(Index index, uint64_t value) override;
  Result OnInitExprGetGlobalExpr(Index index, Index global_index) override;
  Result OnInitExprI32ConstExpr(Index index, uint32_t value) override;
  Result OnInitExprI64ConstExpr(Index index, uint64_t value) override;

  Result OnRelocCount(Index count,
                      BinarySection section_code,
                      string_view section_name) override;
  Result OnReloc(RelocType type,
                 Offset offset,
                 Index index,
                 uint32_t addend) override;

  Result OnStackGlobal(Index stack_global) override;
  Result OnSymbolInfoCount(Index count) override;
  Result OnSymbolInfo(string_view name, uint32_t flags) override;
  Result OnDataSize(uint32_t data_size) override;
  Result OnDataAlignment(uint32_t data_alignment) override;

  Result OnExceptionCount(Index count) override;
  Result OnExceptionType(Index index, TypeVector& sig) override;

 private:
  bool ShouldPrintDetails();
  void PrintDetails(const char* fmt, ...);
  void PrintInitExpr(const InitExpr& expr);
  Result OnCount(Index count);

  std::unique_ptr<FileStream> out_stream_;
  Index elem_index_ = 0;
  Index next_data_reloc_ = 0;
  bool in_data_section_ = false;
  InitExpr data_init_expr_;
};

BinaryReaderObjdump::BinaryReaderObjdump(const uint8_t* data,
                                         size_t size,
                                         ObjdumpOptions* options,
                                         ObjdumpState* objdump_state)
    : BinaryReaderObjdumpBase(data, size, options, objdump_state),
      out_stream_(FileStream::CreateStdout()) {}

Result BinaryReaderObjdump::BeginCustomSection(Offset size,
                                               string_view section_name) {
  PrintDetails(" - name: \"" PRIstringview "\"\n",
               WABT_PRINTF_STRING_VIEW_ARG(section_name));
  if (options_->mode == ObjdumpMode::Headers) {
    printf("\"" PRIstringview "\"\n",
           WABT_PRINTF_STRING_VIEW_ARG(section_name));
  }
  return Result::Ok;
}

Result BinaryReaderObjdump::BeginSection(BinarySection section_code,
                                         Offset size) {
  BinaryReaderObjdumpBase::BeginSection(section_code, size);

  const char* name = GetSectionName(section_code);

  bool section_match =
      !options_->section_name || !strcasecmp(options_->section_name, name);
  if (section_match)
    section_found_ = true;

  switch (options_->mode) {
    case ObjdumpMode::Headers:
      printf("%9s start=%#010" PRIzx " end=%#010" PRIzx " (size=%#010" PRIoffset
             ") ",
             name, state->offset, state->offset + size, size);
      break;
    case ObjdumpMode::Details:
      if (section_match) {
        if (section_code != BinarySection::Code)
          printf("%s:\n", name);
        print_details_ = true;
      } else {
        print_details_ = false;
      }
      break;
    case ObjdumpMode::RawData:
      if (section_match) {
        printf("\nContents of section %s:\n", name);
        out_stream_->WriteMemoryDump(data_ + state->offset, size, state->offset,
                                     PrintChars::Yes);
      }
      break;
    case ObjdumpMode::Prepass:
    case ObjdumpMode::Disassemble:
      break;
  }
  return Result::Ok;
}

bool BinaryReaderObjdump::ShouldPrintDetails() {
  if (options_->mode != ObjdumpMode::Details)
    return false;
  return print_details_;
}

void WABT_PRINTF_FORMAT(2, 3) BinaryReaderObjdump::PrintDetails(const char* fmt,
                                                                ...) {
  if (!ShouldPrintDetails())
    return;
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
}

Result BinaryReaderObjdump::OnCount(Index count) {
  if (options_->mode == ObjdumpMode::Headers) {
    printf("count: %" PRIindex "\n", count);
  }
  return Result::Ok;
}

Result BinaryReaderObjdump::EndModule() {
  if (options_->section_name && !section_found_) {
    fprintf(stderr, "Section not found: %s\n", options_->section_name);
    return Result::Error;
  }

  if (options_->relocs) {
    if (next_data_reloc_ != objdump_state_->data_relocations.size()) {
      fprintf(stderr, "Data reloctions outside of segments\n");
      return Result::Error;
    }
  }

  return Result::Ok;
}

Result BinaryReaderObjdump::OnTypeCount(Index count) {
  return OnCount(count);
}

Result BinaryReaderObjdump::OnType(Index index,
                                   Index param_count,
                                   Type* param_types,
                                   Index result_count,
                                   Type* result_types) {
  if (!ShouldPrintDetails())
    return Result::Ok;
  printf(" - type[%" PRIindex "] (", index);
  for (Index i = 0; i < param_count; i++) {
    if (i != 0) {
      printf(", ");
    }
    printf("%s", type_name(param_types[i]));
  }
  printf(") -> ");
  if (result_count)
    printf("%s", type_name(result_types[0]));
  else
    printf("nil");
  printf("\n");
  return Result::Ok;
}

Result BinaryReaderObjdump::OnFunctionCount(Index count) {
  return OnCount(count);
}

Result BinaryReaderObjdump::OnFunction(Index index, Index sig_index) {
  PrintDetails(" - func[%" PRIindex "] sig=%" PRIindex, index, sig_index);
  if (const char* name = GetFunctionName(index))
    PrintDetails(" <%s>", name);
  PrintDetails("\n");
  return Result::Ok;
}

Result BinaryReaderObjdump::OnFunctionBodyCount(Index count) {
  return OnCount(count);
}

Result BinaryReaderObjdump::OnStartFunction(Index func_index) {
  if (options_->mode == ObjdumpMode::Headers) {
    printf("start: %" PRIindex "\n", func_index);
  } else {
    PrintDetails(" - start function: %" PRIindex "\n", func_index);
  }
  return Result::Ok;
}

Result BinaryReaderObjdump::OnImportCount(Index count) {
  return OnCount(count);
}

Result BinaryReaderObjdump::OnImportFunc(Index import_index,
                                         string_view module_name,
                                         string_view field_name,
                                         Index func_index,
                                         Index sig_index) {
  PrintDetails(" - func[%" PRIindex "] sig=%" PRIindex, func_index, sig_index);
  if (const char* name = GetFunctionName(func_index))
    PrintDetails(" <%s>", name);
  PrintDetails(" <- " PRIstringview "." PRIstringview "\n",
               WABT_PRINTF_STRING_VIEW_ARG(module_name),
               WABT_PRINTF_STRING_VIEW_ARG(field_name));
  return Result::Ok;
}

Result BinaryReaderObjdump::OnImportTable(Index import_index,
                                          string_view module_name,
                                          string_view field_name,
                                          Index table_index,
                                          Type elem_type,
                                          const Limits* elem_limits) {
  PrintDetails(" - " PRIstringview "." PRIstringview
               " -> table elem_type=%s init=%" PRId64 " max=%" PRId64 "\n",
               WABT_PRINTF_STRING_VIEW_ARG(module_name),
               WABT_PRINTF_STRING_VIEW_ARG(field_name), GetTypeName(elem_type),
               elem_limits->initial, elem_limits->max);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnImportMemory(Index import_index,
                                           string_view module_name,
                                           string_view field_name,
                                           Index memory_index,
                                           const Limits* page_limits) {
  PrintDetails(" - " PRIstringview "." PRIstringview " -> memory\n",
               WABT_PRINTF_STRING_VIEW_ARG(module_name),
               WABT_PRINTF_STRING_VIEW_ARG(field_name));
  return Result::Ok;
}

Result BinaryReaderObjdump::OnImportGlobal(Index import_index,
                                           string_view module_name,
                                           string_view field_name,
                                           Index global_index,
                                           Type type,
                                           bool mutable_) {
  PrintDetails(" - global[%" PRIindex "] %s mutable=%d <- " PRIstringview
               "." PRIstringview "\n",
               global_index, GetTypeName(type), mutable_,
               WABT_PRINTF_STRING_VIEW_ARG(module_name),
               WABT_PRINTF_STRING_VIEW_ARG(field_name));
  return Result::Ok;
}

Result BinaryReaderObjdump::OnImportException(Index import_index,
                                              string_view module_name,
                                              string_view field_name,
                                              Index except_index,
                                              TypeVector& sig) {
  PrintDetails(" - except[%" PRIindex "] (", except_index);
  for (Index i = 0; i < sig.size(); ++i) {
    if (i != 0) {
      PrintDetails(", ");
    }
    PrintDetails("%s", type_name(sig[i]));
  }
  PrintDetails(") <- " PRIstringview "." PRIstringview "\n",
               WABT_PRINTF_STRING_VIEW_ARG(module_name),
               WABT_PRINTF_STRING_VIEW_ARG(field_name));
  return Result::Ok;
}

Result BinaryReaderObjdump::OnMemoryCount(Index count) {
  return OnCount(count);
}

Result BinaryReaderObjdump::OnMemory(Index index, const Limits* page_limits) {
  PrintDetails(" - memory[%" PRIindex "] pages: initial=%" PRId64, index,
               page_limits->initial);
  if (page_limits->has_max)
    PrintDetails(" max=%" PRId64, page_limits->max);
  PrintDetails("\n");
  return Result::Ok;
}

Result BinaryReaderObjdump::OnTableCount(Index count) {
  return OnCount(count);
}

Result BinaryReaderObjdump::OnTable(Index index,
                                    Type elem_type,
                                    const Limits* elem_limits) {
  PrintDetails(" - table[%" PRIindex "] type=%s initial=%" PRId64, index,
               GetTypeName(elem_type), elem_limits->initial);
  if (elem_limits->has_max)
    PrintDetails(" max=%" PRId64, elem_limits->max);
  PrintDetails("\n");
  return Result::Ok;
}

Result BinaryReaderObjdump::OnExportCount(Index count) {
  return OnCount(count);
}

Result BinaryReaderObjdump::OnExport(Index index,
                                     ExternalKind kind,
                                     Index item_index,
                                     string_view name) {
  PrintDetails(" - %s[%" PRIindex "]", GetKindName(kind), item_index);
  if (kind == ExternalKind::Func) {
    if (const char* name = GetFunctionName(item_index))
      PrintDetails(" <%s>", name);
  }

  PrintDetails(" -> \"" PRIstringview "\"\n",
               WABT_PRINTF_STRING_VIEW_ARG(name));
  return Result::Ok;
}

Result BinaryReaderObjdump::OnElemSegmentFunctionIndex(Index segment_index,
                                                       Index func_index) {
  PrintDetails("  - elem[%" PRIindex "] = func[%" PRIindex "]", elem_index_,
               func_index);
  if (const char* name = GetFunctionName(func_index))
    PrintDetails(" <%s>", name);
  PrintDetails("\n");
  elem_index_++;
  return Result::Ok;
}

Result BinaryReaderObjdump::OnElemSegmentCount(Index count) {
  return OnCount(count);
}

Result BinaryReaderObjdump::BeginElemSegment(Index index, Index table_index) {
  PrintDetails(" - segment[%" PRIindex "] table=%" PRIindex "\n", index,
               table_index);
  elem_index_ = 0;
  return Result::Ok;
}

Result BinaryReaderObjdump::OnGlobalCount(Index count) {
  return OnCount(count);
}

Result BinaryReaderObjdump::BeginGlobal(Index index, Type type, bool mutable_) {
  PrintDetails(" - global[%" PRIindex "] %s mutable=%d", index,
               GetTypeName(type), mutable_);
  return Result::Ok;
}

void BinaryReaderObjdump::PrintInitExpr(const InitExpr& expr) {
  switch (expr.type) {
    case InitExprType::I32:
      PrintDetails(" - init i32=%d\n", expr.value.i32);
      break;
    case InitExprType::I64:
      PrintDetails(" - init i64=%" PRId64 "\n", expr.value.i64);
      break;
    case InitExprType::F64: {
      char buffer[WABT_MAX_DOUBLE_HEX];
      WriteFloatHex(buffer, sizeof(buffer), expr.value.f64);
      PrintDetails(" - init f64=%s\n", buffer);
      break;
    }
    case InitExprType::F32: {
      char buffer[WABT_MAX_FLOAT_HEX];
      WriteFloatHex(buffer, sizeof(buffer), expr.value.f32);
      PrintDetails(" - init f32=%s\n", buffer);
      break;
    }
    case InitExprType::Global:
      PrintDetails(" - init global=%" PRIindex "\n", expr.value.global);
      break;
  }
}

Result BinaryReaderObjdump::OnInitExprF32ConstExpr(Index index,
                                                   uint32_t value) {
  InitExpr expr;
  expr.type = InitExprType::F32;
  expr.value.f32 = value;
  if (in_data_section_) {
    data_init_expr_ = expr;
  } else {
    PrintInitExpr(expr);
  }
  return Result::Ok;
}

Result BinaryReaderObjdump::OnInitExprF64ConstExpr(Index index,
                                                   uint64_t value) {
  InitExpr expr;
  expr.type = InitExprType::F64;
  expr.value.f64 = value;
  if (in_data_section_) {
    data_init_expr_ = expr;
  } else {
    PrintInitExpr(expr);
  }
  return Result::Ok;
}

Result BinaryReaderObjdump::OnInitExprGetGlobalExpr(Index index,
                                                    Index global_index) {
  InitExpr expr;
  expr.type = InitExprType::Global;
  expr.value.global = global_index;
  if (in_data_section_) {
    data_init_expr_ = expr;
  } else {
    PrintInitExpr(expr);
  }
  return Result::Ok;
}

Result BinaryReaderObjdump::OnInitExprI32ConstExpr(Index index,
                                                   uint32_t value) {
  InitExpr expr;
  expr.type = InitExprType::I32;
  expr.value.i32 = value;
  if (in_data_section_) {
    data_init_expr_ = expr;
  } else {
    PrintInitExpr(expr);
  }
  return Result::Ok;
}

Result BinaryReaderObjdump::OnInitExprI64ConstExpr(Index index,
                                                   uint64_t value) {
  InitExpr expr;
  expr.type = InitExprType::I64;
  expr.value.i64 = value;
  if (in_data_section_) {
    data_init_expr_ = expr;
  } else {
    PrintInitExpr(expr);
  }
  return Result::Ok;
}

Result BinaryReaderObjdump::OnFunctionName(Index index, string_view name) {
  PrintDetails(" - func[%" PRIindex "] " PRIstringview "\n", index,
               WABT_PRINTF_STRING_VIEW_ARG(name));
  return Result::Ok;
}

Result BinaryReaderObjdump::OnLocalName(Index func_index,
                                        Index local_index,
                                        string_view name) {
  if (!name.empty()) {
    PrintDetails(" - func[%" PRIindex "] local[%" PRIindex "] " PRIstringview
                 "\n",
                 func_index, local_index, WABT_PRINTF_STRING_VIEW_ARG(name));
  }
  return Result::Ok;
}

Result BinaryReaderObjdump::OnDataSegmentCount(Index count) {
  return OnCount(count);
}

Result BinaryReaderObjdump::BeginDataSegment(Index index, Index memory_index) {
  // TODO(sbc): Display memory_index once multiple memories become a thing
  // PrintDetails(" - memory[%" PRIindex "]", memory_index);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnDataSegmentData(Index index,
                                              const void* src_data,
                                              Address size) {
  if (!ShouldPrintDetails())
    return Result::Ok;

  PrintDetails(" - segment[%" PRIindex "] size=%" PRIaddress, index, size);
  PrintInitExpr(data_init_expr_);

  size_t voffset = 0;
  switch (data_init_expr_.type) {
    case InitExprType::I32:
      voffset = data_init_expr_.value.i32;
      break;
    case InitExprType::Global:
      break;
    case InitExprType::I64:
    case InitExprType::F32:
    case InitExprType::F64:
      fprintf(stderr, "Segment offset must be an i32 init expr");
      return Result::Error;
      break;
  }

  out_stream_->WriteMemoryDump(src_data, size, voffset, PrintChars::Yes,
                               "  - ");

  // Print relocations from this segment.
  if (!options_->relocs)
    return Result::Ok;

  Offset data_start = GetSectionStart(BinarySection::Data);
  Offset segment_start = state->offset - size;
  Offset segment_offset = segment_start - data_start;
  while (next_data_reloc_ < objdump_state_->data_relocations.size()) {
    const Reloc& reloc = objdump_state_->data_relocations[next_data_reloc_];
    Offset abs_offset = data_start + reloc.offset;
    if (abs_offset > state->offset)
      break;
    PrintRelocation(reloc, reloc.offset - segment_offset + voffset);
    next_data_reloc_++;
  }

  return Result::Ok;
}

Result BinaryReaderObjdump::OnRelocCount(Index count,
                                         BinarySection section_code,
                                         string_view section_name) {
  BinaryReaderObjdumpBase::OnRelocCount(count, section_code, section_name);
  PrintDetails("  - section: %s\n", GetSectionName(section_code));
  return Result::Ok;
}

Result BinaryReaderObjdump::OnReloc(RelocType type,
                                    Offset offset,
                                    Index index,
                                    uint32_t addend) {
  Offset total_offset = GetSectionStart(reloc_section_) + offset;
  PrintDetails("   - %-18s offset=%#08" PRIoffset "(file=%#08" PRIoffset
               ") index=%" PRIindex,
               GetRelocTypeName(type), offset, total_offset, index);
  if (addend) {
    int32_t signed_addend = static_cast<int32_t>(addend);
    if (signed_addend < 0) {
      PrintDetails("-");
      signed_addend = -signed_addend;
    } else {
      PrintDetails("+");
    }
    PrintDetails("%#x", signed_addend);
  }
  PrintDetails("\n");
  return Result::Ok;
}

Result BinaryReaderObjdump::OnStackGlobal(Index stack_global) {
  PrintDetails("  - stack pointer global: %d\n", stack_global);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnSymbolInfoCount(Index count) {
  PrintDetails("  - symbol info [count=%d]\n", count);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnSymbolInfo(string_view name, uint32_t flags) {
  const char* binding_name = nullptr;
  SymbolBinding binding = static_cast<SymbolBinding>(flags & 0x3);
  switch (binding) {
    case SymbolBinding::Global:
      binding_name = "GLOBAL";
      break;
    case SymbolBinding::Local:
      binding_name = "LOCAL ";
      break;
    case SymbolBinding::Weak:
      binding_name = "WEAK  ";
      break;
  }

  PrintDetails("   - %s <" PRIstringview ">\n", binding_name,
               WABT_PRINTF_STRING_VIEW_ARG(name));
  return Result::Ok;
}

Result BinaryReaderObjdump::OnExceptionCount(Index count) {
  return OnCount(count);
}

Result BinaryReaderObjdump::OnExceptionType(Index index, TypeVector& sig) {
  if (!ShouldPrintDetails())
    return Result::Ok;
  printf(" - except[%" PRIindex "] (", index);
  for (Index i = 0; i < sig.size(); ++i) {
    if (i != 0) {
      printf(", ");
    }
    printf("%s", type_name(sig[i]));
  }
  printf(")\n");
  return Result::Ok;
}

Result BinaryReaderObjdump::OnDataSize(uint32_t data_size) {
  PrintDetails("  - data size : %d\n", data_size);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnDataAlignment(uint32_t data_alignment) {
  PrintDetails("  - data align: %d\n", data_alignment);
  return Result::Ok;
}

}  // end anonymous namespace

Result ReadBinaryObjdump(const uint8_t* data,
                         size_t size,
                         ObjdumpOptions* options,
                         ObjdumpState* state) {
  Features features;
  features.EnableAll();
  ReadBinaryOptions read_options(features, options->log_stream, true);

  switch (options->mode) {
    case ObjdumpMode::Prepass: {
      BinaryReaderObjdumpPrepass reader(data, size, options, state);
      return ReadBinary(data, size, &reader, &read_options);
    }
    case ObjdumpMode::Disassemble: {
      BinaryReaderObjdumpDisassemble reader(data, size, options, state);
      return ReadBinary(data, size, &reader, &read_options);
    }
    default: {
      BinaryReaderObjdump reader(data, size, options, state);
      return ReadBinary(data, size, &reader, &read_options);
    }
  }
}

}  // namespace wabt
