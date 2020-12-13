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
#include "src/filenames.h"
#include "src/literal.h"

namespace wabt {

namespace {

class BinaryReaderObjdumpBase : public BinaryReaderNop {
 public:
  BinaryReaderObjdumpBase(const uint8_t* data,
                          size_t size,
                          ObjdumpOptions* options,
                          ObjdumpState* state);

  bool OnError(const Error&) override;

  Result BeginModule(uint32_t version) override;
  Result BeginSection(BinarySection section_type, Offset size) override;

  Result OnRelocCount(Index count, Index section_index) override;

 protected:
  const char* GetFunctionName(Index index) const;
  const char* GetGlobalName(Index index) const;
  const char* GetSectionName(Index index) const;
  string_view GetSymbolName(Index symbol_index) const;
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
  // Map of section index to section type
  std::vector<BinarySection> section_types_;
  bool section_found_ = false;
  std::string module_name_;
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
  section_types_.push_back(section_code);
  return Result::Ok;
}

bool BinaryReaderObjdumpBase::OnError(const Error&) {
  // Tell the BinaryReader that this error is "handled" for all passes other
  // than the prepass. When the error is handled the default message will be
  // suppressed.
  return options_->mode != ObjdumpMode::Prepass;
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
      string_view basename = GetBasename(options_->filename);
      printf("%s:\tfile format wasm %#x\n", basename.to_string().c_str(),
             version);
      break;
    }
    case ObjdumpMode::RawData:
      break;
  }

  return Result::Ok;
}

const char* BinaryReaderObjdumpBase::GetFunctionName(Index index) const {
  if (index >= objdump_state_->function_names.size() ||
      objdump_state_->function_names[index].empty()) {
    return nullptr;
  }

  return objdump_state_->function_names[index].c_str();
}

const char* BinaryReaderObjdumpBase::GetGlobalName(Index index) const {
  if (index >= objdump_state_->global_names.size() ||
      objdump_state_->global_names[index].empty()) {
    return nullptr;
  }

  return objdump_state_->global_names[index].c_str();
}

const char* BinaryReaderObjdumpBase::GetSectionName(Index index) const {
  assert(index < objdump_state_->section_names.size() &&
         !objdump_state_->section_names[index].empty());

  return objdump_state_->section_names[index].c_str();
}

string_view BinaryReaderObjdumpBase::GetSymbolName(Index symbol_index) const {
  assert(symbol_index < objdump_state_->symtab.size());
  ObjdumpSymbol& sym = objdump_state_->symtab[symbol_index];
  switch (sym.kind) {
    case SymbolType::Function:
      return GetFunctionName(sym.index);
    case SymbolType::Data:
      return sym.name;
    case SymbolType::Global:
      return GetGlobalName(sym.index);
    case SymbolType::Section:
      return GetSectionName(sym.index);
  }
  WABT_UNREACHABLE;
}

void BinaryReaderObjdumpBase::PrintRelocation(const Reloc& reloc,
                                              Offset offset) const {
  printf("           %06" PRIzx ": %-18s %" PRIindex, offset,
         GetRelocTypeName(reloc.type), reloc.index);
  if (reloc.addend) {
    printf(" + %d", reloc.addend);
  }
  if (reloc.type != RelocType::TypeIndexLEB) {
    printf(" <" PRIstringview ">",
           WABT_PRINTF_STRING_VIEW_ARG(GetSymbolName(reloc.index)));
  }
  printf("\n");
}

Result BinaryReaderObjdumpBase::OnRelocCount(Index count, Index section_index) {
  reloc_section_ = section_types_[section_index];
  return Result::Ok;
}

class BinaryReaderObjdumpPrepass : public BinaryReaderObjdumpBase {
 public:
  using BinaryReaderObjdumpBase::BinaryReaderObjdumpBase;

  Result BeginSection(BinarySection section_code, Offset size) override {
    BinaryReaderObjdumpBase::BeginSection(section_code, size);
    if (section_code != BinarySection::Custom) {
      objdump_state_->section_names.push_back(
          wabt::GetSectionName(section_code));
    }
    return Result::Ok;
  }

  Result BeginCustomSection(Offset size, string_view section_name) override {
    objdump_state_->section_names.push_back(section_name.to_string());
    return Result::Ok;
  }

  Result OnFunctionName(Index index, string_view name) override {
    SetFunctionName(index, name);
    return Result::Ok;
  }

  Result OnSymbolCount(Index count) override {
    objdump_state_->symtab.resize(count);
    return Result::Ok;
  }

  Result OnDataSymbol(Index index,
                      uint32_t flags,
                      string_view name,
                      Index segment,
                      uint32_t offset,
                      uint32_t size) override {
    objdump_state_->symtab[index] = {SymbolType::Data, name.to_string(), 0};
    return Result::Ok;
  }

  Result OnFunctionSymbol(Index index,
                          uint32_t flags,
                          string_view name,
                          Index func_index) override {
    if (!name.empty()) {
      SetFunctionName(func_index, name);
    }
    objdump_state_->symtab[index] = {SymbolType::Function, name.to_string(),
                                     func_index};
    return Result::Ok;
  }

  Result OnGlobalSymbol(Index index,
                        uint32_t flags,
                        string_view name,
                        Index global_index) override {
    if (!name.empty()) {
      SetGlobalName(global_index, name);
    }
    objdump_state_->symtab[index] = {SymbolType::Global, name.to_string(),
                                     global_index};
    return Result::Ok;
  }

  Result OnSectionSymbol(Index index,
                         uint32_t flags,
                         Index section_index) override {
    objdump_state_->symtab[index] = {
        SymbolType::Section, GetSectionName(section_index), section_index};
    return Result::Ok;
  }

  Result OnImportFunc(Index import_index,
                      string_view module_name,
                      string_view field_name,
                      Index func_index,
                      Index sig_index) override {
    SetFunctionName(func_index, field_name);
    return Result::Ok;
  }

  Result OnImportGlobal(Index import_index,
                        string_view module_name,
                        string_view field_name,
                        Index global_index,
                        Type type,
                        bool mutable_) override {
    SetGlobalName(global_index, field_name);
    return Result::Ok;
  }

  Result OnExport(Index index,
                  ExternalKind kind,
                  Index item_index,
                  string_view name) override {
    if (kind == ExternalKind::Func) {
      SetFunctionName(item_index, name);
    } else if (kind == ExternalKind::Global) {
      SetGlobalName(item_index, name);
    }
    return Result::Ok;
  }

  Result OnReloc(RelocType type,
                 Offset offset,
                 Index index,
                 uint32_t addend) override;

  Result OnModuleName(string_view name) override {
    if (options_->mode == ObjdumpMode::Prepass) {
      printf("module name: <" PRIstringview ">\n",
             WABT_PRINTF_STRING_VIEW_ARG(name));
    }
    return Result::Ok;
  }

 protected:
  void SetFunctionName(Index index, string_view name);
  void SetGlobalName(Index index, string_view name);
};

void BinaryReaderObjdumpPrepass::SetFunctionName(Index index,
                                                 string_view name) {
  if (objdump_state_->function_names.size() <= index)
    objdump_state_->function_names.resize(index + 1);
  objdump_state_->function_names[index] = name.to_string();
}

void BinaryReaderObjdumpPrepass::SetGlobalName(Index index, string_view name) {
  if (objdump_state_->global_names.size() <= index)
    objdump_state_->global_names.resize(index + 1);
  objdump_state_->global_names[index] = name.to_string();
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

  std::string BlockSigToString(Type type) const;

  Result BeginFunctionBody(Index index, Offset size) override;

  Result OnLocalDeclCount(Index count) override;
  Result OnLocalDecl(Index decl_index, Index count, Type type) override;

  Result OnOpcode(Opcode Opcode) override;
  Result OnOpcodeBare() override;
  Result OnOpcodeIndex(Index value) override;
  Result OnOpcodeUint32(uint32_t value) override;
  Result OnOpcodeUint32Uint32(uint32_t value, uint32_t value2) override;
  Result OnOpcodeUint64(uint64_t value) override;
  Result OnOpcodeF32(uint32_t value) override;
  Result OnOpcodeF64(uint64_t value) override;
  Result OnOpcodeV128(v128 value) override;
  Result OnOpcodeBlockSig(Type sig_type) override;

  Result OnBrTableExpr(Index num_targets,
                       Index* target_depths,
                       Index default_target_depth) override;
  Result OnIfExceptExpr(Type sig_type, Index except_index) override;
  Result OnEndExpr() override;
  Result OnEndFunc() override;

 private:
  void LogOpcode(size_t data_size, const char* fmt, ...);

  Opcode current_opcode = Opcode::Unreachable;
  Offset current_opcode_offset = 0;
  Offset last_opcode_end = 0;
  int indent_level = 0;
  Index next_reloc = 0;
  Index local_index_ = 0;
};

std::string BinaryReaderObjdumpDisassemble::BlockSigToString(Type type) const {
  if (IsTypeIndex(type)) {
    return StringPrintf("type[%d]", GetTypeIndex(type));
  } else if (type == Type::Void) {
    return "";
  } else {
    return GetTypeName(type);
  }
}

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

Result BinaryReaderObjdumpDisassemble::OnLocalDeclCount(Index count) {
  local_index_ = 0;
  current_opcode_offset = state->offset;
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnLocalDecl(Index decl_index,
                                                   Index count,
                                                   Type type) {
  Offset offset = current_opcode_offset;
  size_t data_size = state->offset - offset;

  printf(" %06" PRIzx ":", offset);
  for (size_t i = 0; i < data_size && i < IMMEDIATE_OCTET_COUNT;
       i++, offset++) {
    printf(" %02x", data_[offset]);
  }
  for (size_t i = data_size; i < IMMEDIATE_OCTET_COUNT; i++) {
    printf("   ");
  }
  printf(" | local[%" PRIindex, local_index_);

  if (count != 1) {
    printf("..%" PRIindex "", local_index_ + count - 1);
  }
  local_index_ += count;

  printf("] type=%s\n", GetTypeName(type));

  last_opcode_end = current_opcode_offset + data_size;
  current_opcode_offset = last_opcode_end;

  return Result::Ok;
}

void BinaryReaderObjdumpDisassemble::LogOpcode(size_t data_size,
                                               const char* fmt,
                                               ...) {
  const Offset opcode_size = current_opcode.GetLength();
  const Offset total_size = opcode_size + data_size;
  // current_opcode_offset has already read past this opcode; rewind it by the
  // size of this opcode, which may be more than one byte.
  Offset offset = current_opcode_offset - opcode_size;
  const Offset offset_end = offset + total_size;

  bool first_line = true;
  while (offset < offset_end) {
    // Print bytes, but only display a maximum of IMMEDIATE_OCTET_COUNT on each
    // line.
    printf(" %06" PRIzx ":", offset);
    size_t i;
    for (i = 0; offset < offset_end && i < IMMEDIATE_OCTET_COUNT;
         ++i, ++offset) {
      printf(" %02x", data_[offset]);
    }
    // Fill the rest of the remaining space with spaces.
    for (; i < IMMEDIATE_OCTET_COUNT; ++i) {
      printf("   ");
    }
    printf(" | ");

    if (first_line) {
      first_line = false;

      // Print disassembly.
      int indent_level = this->indent_level;
      switch (current_opcode) {
        case Opcode::Else:
        case Opcode::Catch:
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
    }

    printf("\n");
  }

  last_opcode_end = offset_end;

  // Print relocation after then full (potentially multi-line) instruction.
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
  LogOpcode(0, nullptr);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnOpcodeIndex(Index value) {
  Offset immediate_len = state->offset - current_opcode_offset;
  const char* name;
  if (current_opcode == Opcode::Call && (name = GetFunctionName(value))) {
    LogOpcode(immediate_len, "%d <%s>", value, name);
  } else if ((current_opcode == Opcode::GetGlobal ||
              current_opcode == Opcode::SetGlobal) &&
             (name = GetGlobalName(value))) {
    LogOpcode(immediate_len, "%d <%s>", value, name);
  } else {
    LogOpcode(immediate_len, "%d", value);
  }
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnOpcodeUint32(uint32_t value) {
  Offset immediate_len = state->offset - current_opcode_offset;
  LogOpcode(immediate_len, "%u", value);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnOpcodeUint32Uint32(uint32_t value,
                                                            uint32_t value2) {
  Offset immediate_len = state->offset - current_opcode_offset;
  LogOpcode(immediate_len, "%lu %lu", value, value2);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnOpcodeUint64(uint64_t value) {
  Offset immediate_len = state->offset - current_opcode_offset;
  LogOpcode(immediate_len, "%" PRId64, value);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnOpcodeF32(uint32_t value) {
  Offset immediate_len = state->offset - current_opcode_offset;
  char buffer[WABT_MAX_FLOAT_HEX];
  WriteFloatHex(buffer, sizeof(buffer), value);
  LogOpcode(immediate_len, buffer);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnOpcodeF64(uint64_t value) {
  Offset immediate_len = state->offset - current_opcode_offset;
  char buffer[WABT_MAX_DOUBLE_HEX];
  WriteDoubleHex(buffer, sizeof(buffer), value);
  LogOpcode(immediate_len, buffer);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnOpcodeV128(v128 value) {
  Offset immediate_len = state->offset - current_opcode_offset;
  LogOpcode(immediate_len, "0x%08x 0x%08x 0x%08x 0x%08x", value.v[0],
            value.v[1], value.v[2], value.v[3]);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnBrTableExpr(
    Index num_targets,
    Index* target_depths,
    Index default_target_depth) {
  Offset immediate_len = state->offset - current_opcode_offset;
  /* TODO(sbc): Print targets */
  LogOpcode(immediate_len, nullptr);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnIfExceptExpr(Type sig_type,
                                                      Index except_index) {
  Offset immediate_len = state->offset - current_opcode_offset;
  if (sig_type != Type::Void) {
    LogOpcode(immediate_len, "%s %u", BlockSigToString(sig_type).c_str(),
              except_index);
  } else {
    LogOpcode(immediate_len, "%u", except_index);
  }
  indent_level++;
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnEndFunc() {
  LogOpcode(0, nullptr);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnEndExpr() {
  indent_level--;
  assert(indent_level >= 0);
  LogOpcode(0, nullptr);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::BeginFunctionBody(Index index,
                                                         Offset size) {
  const char* name = GetFunctionName(index);
  if (name) {
    printf("%06" PRIzx " <%s>:\n", state->offset, name);
  } else {
    printf("%06" PRIzx " func[%" PRIindex "]:\n", state->offset, index);
  }

  last_opcode_end = 0;
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnOpcodeBlockSig(Type sig_type) {
  Offset immediate_len = state->offset - current_opcode_offset;
  if (sig_type != Type::Void) {
    LogOpcode(immediate_len, "%s", BlockSigToString(sig_type).c_str());
  } else {
    LogOpcode(immediate_len, nullptr);
  }
  indent_level++;
  return Result::Ok;
}

enum class InitExprType {
  I32,
  F32,
  I64,
  F64,
  V128,
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
    v128 v128_v;
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
  Result BeginFunctionBody(Index index, Offset size) override;

  Result BeginElemSection(Offset size) override {
    in_elem_section_ = true;
    return Result::Ok;
  }
  Result EndElemSection() override {
    in_elem_section_ = false;
    return Result::Ok;
  }

  Result OnElemSegmentCount(Index count) override;
  Result BeginElemSegment(Index index, Index table_index, bool passive) override;
  Result OnElemSegmentFunctionIndexCount(Index index, Index count) override;
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
  Result BeginDataSegment(Index index, Index memory_index, bool passive) override;
  Result OnDataSegmentData(Index index,
                           const void* data,
                           Address size) override;

  Result OnModuleName(string_view name) override;
  Result OnFunctionName(Index function_index,
                        string_view function_name) override;
  Result OnLocalName(Index function_index,
                     Index local_index,
                     string_view local_name) override;

  Result OnInitExprF32ConstExpr(Index index, uint32_t value) override;
  Result OnInitExprF64ConstExpr(Index index, uint64_t value) override;
  Result OnInitExprV128ConstExpr(Index index, v128 value) override;
  Result OnInitExprGetGlobalExpr(Index index, Index global_index) override;
  Result OnInitExprI32ConstExpr(Index index, uint32_t value) override;
  Result OnInitExprI64ConstExpr(Index index, uint64_t value) override;

  Result OnDylinkInfo(uint32_t mem_size,
                      uint32_t mem_align,
                      uint32_t table_size,
                      uint32_t table_align) override;
  Result OnDylinkNeededCount(Index count) override;
  Result OnDylinkNeeded(string_view so_name) override;

  Result OnRelocCount(Index count, Index section_index) override;
  Result OnReloc(RelocType type,
                 Offset offset,
                 Index index,
                 uint32_t addend) override;

  Result OnSymbolCount(Index count) override;
  Result OnDataSymbol(Index index,
                      uint32_t flags,
                      string_view name,
                      Index segment,
                      uint32_t offset,
                      uint32_t size) override;
  Result OnFunctionSymbol(Index index,
                          uint32_t flags,
                          string_view name,
                          Index func_index) override;
  Result OnGlobalSymbol(Index index,
                        uint32_t flags,
                        string_view name,
                        Index global_index) override;
  Result OnSectionSymbol(Index index,
                         uint32_t flags,
                         Index section_index) override;
  Result OnSegmentInfoCount(Index count) override;
  Result OnSegmentInfo(Index index,
                       string_view name,
                       uint32_t alignment,
                       uint32_t flags) override;
  Result OnInitFunctionCount(Index count) override;
  Result OnInitFunction(uint32_t priority, Index function_index) override;

  Result OnExceptionCount(Index count) override;
  Result OnExceptionType(Index index, TypeVector& sig) override;

 private:
  Result HandleInitExpr(const InitExpr& expr);
  bool ShouldPrintDetails();
  void PrintDetails(const char* fmt, ...);
  void PrintSymbolFlags(uint32_t flags);
  void PrintInitExpr(const InitExpr& expr);
  Result OnCount(Index count);

  std::unique_ptr<FileStream> out_stream_;
  Index elem_index_ = 0;
  Index table_index_ = 0;
  Index next_data_reloc_ = 0;
  bool in_data_section_ = false;
  bool in_elem_section_ = false;
  InitExpr data_init_expr_;
  InitExpr elem_init_expr_;
  uint32_t data_offset_ = 0;
  uint32_t elem_offset_ = 0;
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

  const char* name = wabt::GetSectionName(section_code);

  bool section_match =
      !options_->section_name || !strcasecmp(options_->section_name, name);
  if (section_match) {
    section_found_ = true;
  }

  switch (options_->mode) {
    case ObjdumpMode::Headers:
      printf("%9s start=%#010" PRIzx " end=%#010" PRIzx " (size=%#010" PRIoffset
             ") ",
             name, state->offset, state->offset + size, size);
      break;
    case ObjdumpMode::Details:
      if (section_match) {
        printf("%s", name);
        // All known section types except the start section have a count
        // in which case this line gets completed in OnCount().
        if (section_code == BinarySection::Start ||
            section_code == BinarySection::Custom) {
          printf(":\n");
        }
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
  if (options_->mode != ObjdumpMode::Details) {
    return false;
  }
  return print_details_;
}

void WABT_PRINTF_FORMAT(2, 3) BinaryReaderObjdump::PrintDetails(const char* fmt,
                                                                ...) {
  if (!ShouldPrintDetails()) {
    return;
  }
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
}

Result BinaryReaderObjdump::OnCount(Index count) {
  if (options_->mode == ObjdumpMode::Headers) {
    printf("count: %" PRIindex "\n", count);
  } else if (options_->mode == ObjdumpMode::Details && print_details_) {
    printf("[%" PRIindex "]:\n", count);
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
  if (!ShouldPrintDetails()) {
    return Result::Ok;
  }
  printf(" - type[%" PRIindex "] (", index);
  for (Index i = 0; i < param_count; i++) {
    if (i != 0) {
      printf(", ");
    }
    printf("%s", GetTypeName(param_types[i]));
  }
  printf(") -> ");
  if (result_count) {
    printf("%s", GetTypeName(result_types[0]));
  } else {
    printf("nil");
  }
  printf("\n");
  return Result::Ok;
}

Result BinaryReaderObjdump::OnFunctionCount(Index count) {
  return OnCount(count);
}

Result BinaryReaderObjdump::OnFunction(Index index, Index sig_index) {
  PrintDetails(" - func[%" PRIindex "] sig=%" PRIindex, index, sig_index);
  if (const char* name = GetFunctionName(index)) {
    PrintDetails(" <%s>", name);
  }
  PrintDetails("\n");
  return Result::Ok;
}

Result BinaryReaderObjdump::OnFunctionBodyCount(Index count) {
  return OnCount(count);
}

Result BinaryReaderObjdump::BeginFunctionBody(Index index, Offset size) {
  PrintDetails(" - func[%" PRIindex "] size=%" PRIzd "\n", index, size);
  return Result::Ok;
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
  if (const char* name = GetFunctionName(func_index)) {
    PrintDetails(" <%s>", name);
  }
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
  PrintDetails(" - table[%" PRIindex "] elem_type=%s init=%" PRId64
               " max=%" PRId64,
               table_index, GetTypeName(elem_type), elem_limits->initial,
               elem_limits->max);
  PrintDetails(" <- " PRIstringview "." PRIstringview "\n",
               WABT_PRINTF_STRING_VIEW_ARG(module_name),
               WABT_PRINTF_STRING_VIEW_ARG(field_name));
  return Result::Ok;
}

Result BinaryReaderObjdump::OnImportMemory(Index import_index,
                                           string_view module_name,
                                           string_view field_name,
                                           Index memory_index,
                                           const Limits* page_limits) {
  PrintDetails(" - memory[%" PRIindex "] pages: initial=%" PRId64, memory_index,
               page_limits->initial);
  if (page_limits->has_max) {
    PrintDetails(" max=%" PRId64, page_limits->max);
  }
  PrintDetails(" <- " PRIstringview "." PRIstringview "\n",
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
  PrintDetails(" - global[%" PRIindex "] %s mutable=%d", global_index,
               GetTypeName(type), mutable_);
  PrintDetails(" <- " PRIstringview "." PRIstringview "\n",
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
    PrintDetails("%s", GetTypeName(sig[i]));
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
  if (page_limits->has_max) {
    PrintDetails(" max=%" PRId64, page_limits->max);
  }
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
  if (elem_limits->has_max) {
    PrintDetails(" max=%" PRId64, elem_limits->max);
  }
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
    if (const char* name = GetFunctionName(item_index)) {
      PrintDetails(" <%s>", name);
    }
  }

  PrintDetails(" -> \"" PRIstringview "\"\n",
               WABT_PRINTF_STRING_VIEW_ARG(name));
  return Result::Ok;
}

Result BinaryReaderObjdump::OnElemSegmentFunctionIndex(Index segment_index,
                                                       Index func_index) {
  PrintDetails("  - elem[%" PRIindex "] = func[%" PRIindex "]",
               elem_offset_ + elem_index_, func_index);
  if (const char* name = GetFunctionName(func_index)) {
    PrintDetails(" <%s>", name);
  }
  PrintDetails("\n");
  elem_index_++;
  return Result::Ok;
}

Result BinaryReaderObjdump::OnElemSegmentCount(Index count) {
  return OnCount(count);
}

Result BinaryReaderObjdump::BeginElemSegment(Index index, Index table_index, bool passive) {
  table_index_ = table_index;
  elem_index_ = 0;
  return Result::Ok;
}

Result BinaryReaderObjdump::OnElemSegmentFunctionIndexCount(Index index,
                                                            Index count) {
  PrintDetails(" - segment[%" PRIindex "] table=%" PRIindex " count=%" PRIindex,
               index, table_index_, count);
  PrintInitExpr(elem_init_expr_);
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
    case InitExprType::V128: {
      PrintDetails(" - init v128=0x%08x 0x%08x 0x%08x 0x%08x \n",
                   expr.value.v128_v.v[0], expr.value.v128_v.v[1],
                   expr.value.v128_v.v[2], expr.value.v128_v.v[3]);
      break;
    }
    case InitExprType::Global:
      PrintDetails(" - init global=%" PRIindex "\n", expr.value.global);
      break;
  }
}

static Result InitExprToConstOffset(const InitExpr& expr,
                                    uint32_t* out_offset) {
  switch (expr.type) {
    case InitExprType::I32:
      *out_offset = expr.value.i32;
      break;
    case InitExprType::Global:
      *out_offset = 0;
      break;
    case InitExprType::I64:
    case InitExprType::F32:
    case InitExprType::F64:
    case InitExprType::V128:
      fprintf(stderr, "Segment/Elem offset must be an i32 init expr");
      return Result::Error;
      break;
  }
  return Result::Ok;
}

Result BinaryReaderObjdump::HandleInitExpr(const InitExpr& expr) {
  if (in_data_section_) {
    data_init_expr_ = expr;
    return InitExprToConstOffset(expr, &data_offset_);
  } else if (in_elem_section_) {
    elem_init_expr_ = expr;
    return InitExprToConstOffset(expr, &elem_offset_);
  } else {
    PrintInitExpr(expr);
  }
  return Result::Ok;
}

Result BinaryReaderObjdump::OnInitExprF32ConstExpr(Index index,
                                                   uint32_t value) {
  InitExpr expr;
  expr.type = InitExprType::F32;
  expr.value.f32 = value;
  HandleInitExpr(expr);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnInitExprF64ConstExpr(Index index,
                                                   uint64_t value) {
  InitExpr expr;
  expr.type = InitExprType::F64;
  expr.value.f64 = value;
  HandleInitExpr(expr);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnInitExprV128ConstExpr(Index index, v128 value) {
  InitExpr expr;
  expr.type = InitExprType::V128;
  expr.value.v128_v = value;
  HandleInitExpr(expr);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnInitExprGetGlobalExpr(Index index,
                                                    Index global_index) {
  InitExpr expr;
  expr.type = InitExprType::Global;
  expr.value.global = global_index;
  HandleInitExpr(expr);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnInitExprI32ConstExpr(Index index,
                                                   uint32_t value) {
  InitExpr expr;
  expr.type = InitExprType::I32;
  expr.value.i32 = value;
  HandleInitExpr(expr);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnInitExprI64ConstExpr(Index index,
                                                   uint64_t value) {
  InitExpr expr;
  expr.type = InitExprType::I64;
  expr.value.i64 = value;
  HandleInitExpr(expr);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnModuleName(string_view name) {
  PrintDetails(" - module <" PRIstringview ">\n",
               WABT_PRINTF_STRING_VIEW_ARG(name));
  return Result::Ok;
}

Result BinaryReaderObjdump::OnFunctionName(Index index, string_view name) {
  PrintDetails(" - func[%" PRIindex "] <" PRIstringview ">\n", index,
               WABT_PRINTF_STRING_VIEW_ARG(name));
  return Result::Ok;
}

Result BinaryReaderObjdump::OnLocalName(Index func_index,
                                        Index local_index,
                                        string_view name) {
  if (!name.empty()) {
    PrintDetails(" - func[%" PRIindex "] local[%" PRIindex "] <" PRIstringview
                 ">\n",
                 func_index, local_index, WABT_PRINTF_STRING_VIEW_ARG(name));
  }
  return Result::Ok;
}

Result BinaryReaderObjdump::OnDataSegmentCount(Index count) {
  return OnCount(count);
}

Result BinaryReaderObjdump::BeginDataSegment(Index index, Index memory_index, bool passive) {
  // TODO(sbc): Display memory_index once multiple memories become a thing
  // PrintDetails(" - memory[%" PRIindex "]", memory_index);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnDataSegmentData(Index index,
                                              const void* src_data,
                                              Address size) {
  if (!ShouldPrintDetails()) {
    return Result::Ok;
  }

  PrintDetails(" - segment[%" PRIindex "] size=%" PRIaddress, index, size);
  PrintInitExpr(data_init_expr_);

  out_stream_->WriteMemoryDump(src_data, size, data_offset_, PrintChars::Yes,
                               "  - ");

  // Print relocations from this segment.
  if (!options_->relocs) {
    return Result::Ok;
  }

  Offset data_start = GetSectionStart(BinarySection::Data);
  Offset segment_start = state->offset - size;
  Offset segment_offset = segment_start - data_start;
  while (next_data_reloc_ < objdump_state_->data_relocations.size()) {
    const Reloc& reloc = objdump_state_->data_relocations[next_data_reloc_];
    Offset abs_offset = data_start + reloc.offset;
    if (abs_offset > state->offset) {
      break;
    }
    PrintRelocation(reloc, reloc.offset - segment_offset + data_offset_);
    next_data_reloc_++;
  }

  return Result::Ok;
}

Result BinaryReaderObjdump::OnDylinkInfo(uint32_t mem_size,
                                         uint32_t mem_align,
                                         uint32_t table_size,
                                         uint32_t table_align) {
  PrintDetails(" - mem_size   : %u\n", mem_size);
  PrintDetails(" - mem_align  : %u\n", mem_align);
  PrintDetails(" - table_size : %u\n", table_size);
  PrintDetails(" - table_align: %u\n", table_align);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnDylinkNeededCount(Index count) {
  if (count) {
    PrintDetails(" - needed_dynlibs[%u]:\n", count);
  }
  return Result::Ok;
}

Result BinaryReaderObjdump::OnDylinkNeeded(string_view so_name) {
  PrintDetails("  - " PRIstringview "\n", WABT_PRINTF_STRING_VIEW_ARG(so_name));
  return Result::Ok;
}

Result BinaryReaderObjdump::OnRelocCount(Index count, Index section_index) {
  BinaryReaderObjdumpBase::OnRelocCount(count, section_index);
  PrintDetails("  - relocations for section: %d (%s) [%d]\n", section_index,
               GetSectionName(section_index), count);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnReloc(RelocType type,
                                    Offset offset,
                                    Index index,
                                    uint32_t addend) {
  Offset total_offset = GetSectionStart(reloc_section_) + offset;
  PrintDetails("   - %-18s offset=%#08" PRIoffset "(file=%#08" PRIoffset ") ",
               GetRelocTypeName(type), offset, total_offset);
  if (type == RelocType::TypeIndexLEB) {
    PrintDetails("type=%" PRIindex, index);
  } else {
    PrintDetails("symbol=%" PRIindex " <" PRIstringview ">", index,
                 WABT_PRINTF_STRING_VIEW_ARG(GetSymbolName(index)));
  }

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

Result BinaryReaderObjdump::OnSymbolCount(Index count) {
  PrintDetails("  - symbol table [count=%d]\n", count);
  return Result::Ok;
}

void BinaryReaderObjdump::PrintSymbolFlags(uint32_t flags) {
  const char* binding_name = nullptr;
  SymbolBinding binding =
      static_cast<SymbolBinding>(flags & WABT_SYMBOL_MASK_BINDING);
  switch (binding) {
    case SymbolBinding::Global:
      binding_name = "global";
      break;
    case SymbolBinding::Local:
      binding_name = "local";
      break;
    case SymbolBinding::Weak:
      binding_name = "weak";
      break;
  }

  const char* vis_name = nullptr;
  SymbolVisibility vis =
      static_cast<SymbolVisibility>(flags & WABT_SYMBOL_MASK_VISIBILITY);
  switch (vis) {
    case SymbolVisibility::Hidden:
      vis_name = "hidden";
      break;
    case SymbolVisibility::Default:
      vis_name = "default";
      break;
  }
  if (flags & WABT_SYMBOL_FLAG_UNDEFINED)
    PrintDetails(" undefined");
  PrintDetails(" binding=%s vis=%s\n", binding_name, vis_name);
}

Result BinaryReaderObjdump::OnDataSymbol(Index index,
                                         uint32_t flags,
                                         string_view name,
                                         Index segment,
                                         uint32_t offset,
                                         uint32_t size) {
  PrintDetails("   - [%d] D <" PRIstringview ">", index,
               WABT_PRINTF_STRING_VIEW_ARG(name));
  if (!(flags & WABT_SYMBOL_FLAG_UNDEFINED))
    PrintDetails(" segment=%" PRIindex " offset=%d size=%d", segment, offset,
                 size);
  PrintSymbolFlags(flags);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnFunctionSymbol(Index index,
                                             uint32_t flags,
                                             string_view name,
                                             Index func_index) {
  std::string sym_name = name.to_string();
  if (sym_name.empty()) {
    sym_name = GetFunctionName(func_index);
  }
  assert(!sym_name.empty());
  PrintDetails("   - [%d] F <" PRIstringview "> func=%" PRIindex, index,
               WABT_PRINTF_STRING_VIEW_ARG(sym_name), func_index);
  PrintSymbolFlags(flags);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnGlobalSymbol(Index index,
                                           uint32_t flags,
                                           string_view name,
                                           Index global_index) {
  std::string sym_name = name.to_string();
  if (sym_name.empty()) {
    if (const char* Name = GetGlobalName(global_index)) {
      sym_name = Name;
    }
  }
  assert(!sym_name.empty());
  PrintDetails("   - [%d] G <" PRIstringview "> global=%" PRIindex, index,
               WABT_PRINTF_STRING_VIEW_ARG(sym_name), global_index);
  PrintSymbolFlags(flags);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnSectionSymbol(Index index,
                                            uint32_t flags,
                                            Index section_index) {
  const char* sym_name = GetSectionName(section_index);
  assert(sym_name);
  PrintDetails("   - [%d] S <%s> section=%" PRIindex, index, sym_name,
               section_index);
  PrintSymbolFlags(flags);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnSegmentInfoCount(Index count) {
  PrintDetails("  - segment info [count=%d]\n", count);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnSegmentInfo(Index index,
                                          string_view name,
                                          uint32_t alignment,
                                          uint32_t flags) {
  PrintDetails("   - %d: " PRIstringview " align=%d flags=%#x\n", index,
               WABT_PRINTF_STRING_VIEW_ARG(name), alignment, flags);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnInitFunctionCount(Index count) {
  PrintDetails("  - init functions [count=%d]\n", count);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnInitFunction(uint32_t priority,
                                           Index function_index) {
  PrintDetails("   - %d: priority=%d\n", function_index, priority);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnExceptionCount(Index count) {
  return OnCount(count);
}

Result BinaryReaderObjdump::OnExceptionType(Index index, TypeVector& sig) {
  if (!ShouldPrintDetails()) {
    return Result::Ok;
  }
  printf(" - except[%" PRIindex "] (", index);
  for (Index i = 0; i < sig.size(); ++i) {
    if (i != 0) {
      printf(", ");
    }
    printf("%s", GetTypeName(sig[i]));
  }
  printf(")\n");
  return Result::Ok;
}

}  // end anonymous namespace

Result ReadBinaryObjdump(const uint8_t* data,
                         size_t size,
                         ObjdumpOptions* options,
                         ObjdumpState* state) {
  Features features;
  features.EnableAll();
  const bool kReadDebugNames = true;
  const bool kStopOnFirstError = false;
  const bool kFailOnCustomSectionError = false;
  ReadBinaryOptions read_options(features, options->log_stream, kReadDebugNames,
                                 kStopOnFirstError, kFailOnCustomSectionError);

  switch (options->mode) {
    case ObjdumpMode::Prepass: {
      BinaryReaderObjdumpPrepass reader(data, size, options, state);
      return ReadBinary(data, size, &reader, read_options);
    }
    case ObjdumpMode::Disassemble: {
      BinaryReaderObjdumpDisassemble reader(data, size, options, state);
      return ReadBinary(data, size, &reader, read_options);
    }
    default: {
      BinaryReaderObjdump reader(data, size, options, state);
      return ReadBinary(data, size, &reader, read_options);
    }
  }
}

}  // namespace wabt
