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

#include "binary-reader-objdump.h"

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <vector>

#include "binary-reader-nop.h"
#include "literal.h"

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
                      StringSlice section_name) override;

 protected:
  const char* GetFunctionName(Index index);

  ObjdumpOptions* options;
  ObjdumpState* objdump_state;
  const uint8_t* data;
  size_t size;
  bool print_details = false;
  BinarySection reloc_section = BinarySection::Invalid;
  Offset section_starts[kBinarySectionCount];
  bool section_found = false;
};

BinaryReaderObjdumpBase::BinaryReaderObjdumpBase(const uint8_t* data,
                                                 size_t size,
                                                 ObjdumpOptions* options,
                                                 ObjdumpState* objdump_state)
    : options(options),
      objdump_state(objdump_state),
      data(data),
      size(size) {
  WABT_ZERO_MEMORY(section_starts);
}

Result BinaryReaderObjdumpBase::BeginSection(BinarySection section_code,
                                             Offset size) {
  section_starts[static_cast<size_t>(section_code)] = state->offset;
  return Result::Ok;
}

Result BinaryReaderObjdumpBase::BeginModule(uint32_t version) {
  switch (options->mode) {
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
      const char* last_slash = strrchr(options->filename, '/');
      const char* last_backslash = strrchr(options->filename, '\\');
      const char* basename;
      if (last_slash && last_backslash) {
        basename = std::max(last_slash, last_backslash) + 1;
      } else if (last_slash) {
        basename = last_slash + 1;
      } else if (last_backslash) {
        basename = last_backslash + 1;
      } else {
        basename = options->filename;
      }

      printf("%s:\tfile format wasm %#x\n", basename, version);
      break;
    }
    case ObjdumpMode::RawData:
      break;
  }

  return Result::Ok;
}

const char* BinaryReaderObjdumpBase::GetFunctionName(Index index) {
  if (index >= objdump_state->function_names.size() ||
      objdump_state->function_names[index].empty())
    return nullptr;

  return objdump_state->function_names[index].c_str();
}

Result BinaryReaderObjdumpBase::OnRelocCount(Index count,
                                             BinarySection section_code,
                                             StringSlice section_name) {
  reloc_section = section_code;
  return Result::Ok;
}

class BinaryReaderObjdumpPrepass : public BinaryReaderObjdumpBase {
 public:
  using BinaryReaderObjdumpBase::BinaryReaderObjdumpBase;

  Result OnFunctionName(Index function_index,
                        StringSlice function_name) override;
  Result OnReloc(RelocType type,
                 Offset offset,
                 Index index,
                 uint32_t addend) override;
};

Result BinaryReaderObjdumpPrepass::OnFunctionName(Index index,
                                                  StringSlice name) {
  objdump_state->function_names.resize(index + 1);
  objdump_state->function_names[index] = string_slice_to_string(name);
  return Result::Ok;
}

Result BinaryReaderObjdumpPrepass::OnReloc(RelocType type,
                                           Offset offset,
                                           Index index,
                                           uint32_t addend) {
  BinaryReaderObjdumpBase::OnReloc(type, offset, index, addend);
  if (reloc_section == BinarySection::Code) {
    objdump_state->code_relocations.emplace_back(type, offset, index, addend);
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
  size_t last_opcode_end = 0;
  int indent_level = 0;
  Index next_reloc = 0;
};

Result BinaryReaderObjdumpDisassemble::OnOpcode(Opcode opcode) {
  if (options->debug) {
    const char* opcode_name = get_opcode_name(opcode);
    printf("on_opcode: %#" PRIzx ": %s\n", state->offset, opcode_name);
  }

  if (last_opcode_end) {
    if (state->offset != last_opcode_end + 1) {
      uint8_t missing_opcode = data[last_opcode_end];
      const char* opcode_name =
          get_opcode_name(static_cast<Opcode>(missing_opcode));
      fprintf(stderr, "warning: %#" PRIzx " missing opcode callback at %#" PRIzx
                      " (%#02x=%s)\n",
              state->offset, last_opcode_end + 1, data[last_opcode_end],
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
  printf(" %06" PRIzx ": %02x", offset - 1,
         static_cast<unsigned>(current_opcode));
  for (size_t i = 0; i < data_size && i < IMMEDIATE_OCTET_COUNT;
       i++, offset++) {
    printf(" %02x", data[offset]);
  }
  for (size_t i = data_size + 1; i < IMMEDIATE_OCTET_COUNT; i++) {
    printf("   ");
  }
  printf(" | ");

  // Print disassemble
  int indent_level = this->indent_level;
  if (current_opcode == Opcode::Else)
    indent_level--;
  for (int j = 0; j < indent_level; j++) {
    printf("  ");
  }

  const char* opcode_name = get_opcode_name(current_opcode);
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

  if (options->relocs && next_reloc < objdump_state->code_relocations.size()) {
    Reloc* reloc = &objdump_state->code_relocations[next_reloc];
    Offset code_start =
        section_starts[static_cast<size_t>(BinarySection::Code)];
    Offset abs_offset = code_start + reloc->offset;
    if (last_opcode_end > abs_offset) {
      printf("           %06" PRIzx ": %-18s %" PRIindex "", abs_offset,
             get_reloc_type_name(reloc->type), reloc->index);
      switch (reloc->type) {
        case RelocType::GlobalAddressLEB:
        case RelocType::GlobalAddressSLEB:
        case RelocType::GlobalAddressI32:
          printf(" + %d", reloc->addend);
          break;
        case RelocType::FuncIndexLEB:
          if (const char* name = GetFunctionName(reloc->index)) {
            printf(" <%s>", name);
          }
        default:
          break;
      }
      printf("\n");
      next_reloc++;
    }
  }
}

Result BinaryReaderObjdumpDisassemble::OnOpcodeBare() {
  LogOpcode(data, 0, nullptr);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnOpcodeIndex(Index value) {
  Offset immediate_len = state->offset - current_opcode_offset;
  const char *name;
  if (current_opcode == Opcode::Call && (name = GetFunctionName(value)))
    LogOpcode(data, immediate_len, "%d <%s>", value, name);
  else
    LogOpcode(data, immediate_len, "%d", value);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnOpcodeUint32(uint32_t value) {
  Offset immediate_len = state->offset - current_opcode_offset;
  LogOpcode(data, immediate_len, "%u", value);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnOpcodeUint32Uint32(uint32_t value,
                                                 uint32_t value2) {
  Offset immediate_len = state->offset - current_opcode_offset;
  LogOpcode(data, immediate_len, "%lu %lu", value, value2);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnOpcodeUint64(uint64_t value) {
  Offset immediate_len = state->offset - current_opcode_offset;
  LogOpcode(data, immediate_len, "%" PRId64, value);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnOpcodeF32(uint32_t value) {
  Offset immediate_len = state->offset - current_opcode_offset;
  char buffer[WABT_MAX_FLOAT_HEX];
  write_float_hex(buffer, sizeof(buffer), value);
  LogOpcode(data, immediate_len, buffer);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnOpcodeF64(uint64_t value) {
  Offset immediate_len = state->offset - current_opcode_offset;
  char buffer[WABT_MAX_DOUBLE_HEX];
  write_double_hex(buffer, sizeof(buffer), value);
  LogOpcode(data, immediate_len, buffer);
  return Result::Ok;
}

Result BinaryReaderObjdumpDisassemble::OnBrTableExpr(
    Index num_targets,
    Index* target_depths,
    Index default_target_depth) {
  Offset immediate_len = state->offset - current_opcode_offset;
  /* TODO(sbc): Print targets */
  LogOpcode(data, immediate_len, nullptr);
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
    LogOpcode(data, 1, "%s", type_name(*sig_types));
  else
    LogOpcode(data, 1, nullptr);
  indent_level++;
  return Result::Ok;
}

class BinaryReaderObjdump : public BinaryReaderObjdumpBase {
 public:
  BinaryReaderObjdump(const uint8_t* data,
                      size_t size,
                      ObjdumpOptions* options,
                      ObjdumpState* state);

  Result EndModule() override;
  Result BeginSection(BinarySection section_type, Offset size) override;
  Result BeginCustomSection(Offset size, StringSlice section_name) override;

  Result OnTypeCount(Index count) override;
  Result OnType(Index index,
                Index param_count,
                Type* param_types,
                Index result_count,
                Type* result_types) override;

  Result OnImportCount(Index count) override;
  Result OnImportFunc(Index import_index,
                      StringSlice module_name,
                      StringSlice field_name,
                      Index func_index,
                      Index sig_index) override;
  Result OnImportTable(Index import_index,
                       StringSlice module_name,
                       StringSlice field_name,
                       Index table_index,
                       Type elem_type,
                       const Limits* elem_limits) override;
  Result OnImportMemory(Index import_index,
                        StringSlice module_name,
                        StringSlice field_name,
                        Index memory_index,
                        const Limits* page_limits) override;
  Result OnImportGlobal(Index import_index,
                        StringSlice module_name,
                        StringSlice field_name,
                        Index global_index,
                        Type type,
                        bool mutable_) override;

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
                  StringSlice name) override;

  Result OnStartFunction(Index func_index) override;

  Result OnFunctionBodyCount(Index count) override;

  Result OnElemSegmentCount(Index count) override;
  Result BeginElemSegment(Index index, Index table_index) override;
  Result OnElemSegmentFunctionIndex(Index segment_index,
                                    Index func_index) override;

  Result OnDataSegmentCount(Index count) override;
  Result BeginDataSegment(Index index, Index memory_index) override;
  Result OnDataSegmentData(Index index,
                           const void* data,
                           Address size) override;

  Result OnFunctionName(Index function_index,
                        StringSlice function_name) override;
  Result OnLocalName(Index function_index,
                     Index local_index,
                     StringSlice local_name) override;

  Result OnInitExprF32ConstExpr(Index index, uint32_t value) override;
  Result OnInitExprF64ConstExpr(Index index, uint64_t value) override;
  Result OnInitExprGetGlobalExpr(Index index, Index global_index) override;
  Result OnInitExprI32ConstExpr(Index index, uint32_t value) override;
  Result OnInitExprI64ConstExpr(Index index, uint64_t value) override;

  Result OnRelocCount(Index count,
                      BinarySection section_code,
                      StringSlice section_name) override;
  Result OnReloc(RelocType type,
                 Offset offset,
                 Index index,
                 uint32_t addend) override;

 private:
  bool ShouldPrintDetails();
  void PrintDetails(const char* fmt, ...);
  Result OnCount(Index count);

  std::unique_ptr<FileStream> out_stream_;
  Index elem_index_ = 0;
};

BinaryReaderObjdump::BinaryReaderObjdump(const uint8_t* data,
                                         size_t size,
                                         ObjdumpOptions* options,
                                         ObjdumpState* objdump_state)
    : BinaryReaderObjdumpBase(data, size, options, objdump_state),
      out_stream_(FileStream::CreateStdout()) {}

Result BinaryReaderObjdump::BeginCustomSection(Offset size,
                                               StringSlice section_name) {
  PrintDetails(" - name: \"" PRIstringslice "\"\n",
               WABT_PRINTF_STRING_SLICE_ARG(section_name));
  if (options->mode == ObjdumpMode::Headers) {
    printf("\"" PRIstringslice "\"\n",
           WABT_PRINTF_STRING_SLICE_ARG(section_name));
  }
  return Result::Ok;
}

Result BinaryReaderObjdump::BeginSection(BinarySection section_code,
                                         Offset size) {
  BinaryReaderObjdumpBase::BeginSection(section_code, size);

  const char* name = get_section_name(section_code);

  bool section_match =
      !options->section_name || !strcasecmp(options->section_name, name);
  if (section_match)
    section_found = true;

  switch (options->mode) {
    case ObjdumpMode::Headers:
      printf("%9s start=%#010" PRIzx " end=%#010" PRIzx
             " (size=%#010" PRIoffset ") ",
             name, state->offset, state->offset + size, size);
      break;
    case ObjdumpMode::Details:
      if (section_match) {
        if (section_code != BinarySection::Code)
          printf("%s:\n", name);
        print_details = true;
      } else {
        print_details = false;
      }
      break;
    case ObjdumpMode::RawData:
      if (section_match) {
        printf("\nContents of section %s:\n", name);
        out_stream_->WriteMemoryDump(data + state->offset, size, state->offset,
                                     nullptr, nullptr, PrintChars::Yes);
      }
      break;
    case ObjdumpMode::Prepass:
    case ObjdumpMode::Disassemble:
      break;
  }
  return Result::Ok;
}

bool BinaryReaderObjdump::ShouldPrintDetails() {
  if (options->mode != ObjdumpMode::Details)
    return false;
  return print_details;
}

void WABT_PRINTF_FORMAT(2, 3)
    BinaryReaderObjdump::PrintDetails(const char* fmt, ...) {
  if (!ShouldPrintDetails())
    return;
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
}

Result BinaryReaderObjdump::OnCount(Index count) {
  if (options->mode == ObjdumpMode::Headers) {
    printf("count: %" PRIindex "\n", count);
  }
  return Result::Ok;
}

Result BinaryReaderObjdump::EndModule() {
  if (options->section_name && !section_found) {
    printf("Section not found: %s\n", options->section_name);
    return Result::Error;
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
  if (options->mode == ObjdumpMode::Headers) {
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
                                         StringSlice module_name,
                                         StringSlice field_name,
                                         Index func_index,
                                         Index sig_index) {
  PrintDetails(" - func[%" PRIindex "] sig=%" PRIindex, func_index, sig_index);
  if (const char* name = GetFunctionName(func_index))
    PrintDetails(" <%s>", name);
  PrintDetails(" <- " PRIstringslice "." PRIstringslice "\n",
               WABT_PRINTF_STRING_SLICE_ARG(module_name),
               WABT_PRINTF_STRING_SLICE_ARG(field_name));
  return Result::Ok;
}

Result BinaryReaderObjdump::OnImportTable(Index import_index,
                                          StringSlice module_name,
                                          StringSlice field_name,
                                          Index table_index,
                                          Type elem_type,
                                          const Limits* elem_limits) {
  PrintDetails(" - " PRIstringslice "." PRIstringslice
               " -> table elem_type=%s init=%" PRId64 " max=%" PRId64 "\n",
               WABT_PRINTF_STRING_SLICE_ARG(module_name),
               WABT_PRINTF_STRING_SLICE_ARG(field_name),
               get_type_name(elem_type), elem_limits->initial,
               elem_limits->max);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnImportMemory(Index import_index,
                                           StringSlice module_name,
                                           StringSlice field_name,
                                           Index memory_index,
                                           const Limits* page_limits) {
  PrintDetails(" - " PRIstringslice "." PRIstringslice " -> memory\n",
               WABT_PRINTF_STRING_SLICE_ARG(module_name),
               WABT_PRINTF_STRING_SLICE_ARG(field_name));
  return Result::Ok;
}

Result BinaryReaderObjdump::OnImportGlobal(Index import_index,
                                           StringSlice module_name,
                                           StringSlice field_name,
                                           Index global_index,
                                           Type type,
                                           bool mutable_) {
  PrintDetails(" - global[%" PRIindex "] %s mutable=%d <- " PRIstringslice
               "." PRIstringslice "\n",
               global_index, get_type_name(type), mutable_,
               WABT_PRINTF_STRING_SLICE_ARG(module_name),
               WABT_PRINTF_STRING_SLICE_ARG(field_name));
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
               get_type_name(elem_type), elem_limits->initial);
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
                                     StringSlice name) {
  PrintDetails(" - %s[%" PRIindex "]", get_kind_name(kind), item_index);
  if (kind == ExternalKind::Func) {
    if (const char* name = GetFunctionName(item_index))
      PrintDetails(" <%s>", name);
  }

  PrintDetails(" -> \"" PRIstringslice "\"\n",
               WABT_PRINTF_STRING_SLICE_ARG(name));
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
               get_type_name(type), mutable_);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnInitExprF32ConstExpr(Index index,
                                                   uint32_t value) {
  char buffer[WABT_MAX_FLOAT_HEX];
  write_float_hex(buffer, sizeof(buffer), value);
  PrintDetails(" - init f32=%s\n", buffer);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnInitExprF64ConstExpr(Index index,
                                                   uint64_t value) {
  char buffer[WABT_MAX_DOUBLE_HEX];
  write_float_hex(buffer, sizeof(buffer), value);
  PrintDetails(" - init f64=%s\n", buffer);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnInitExprGetGlobalExpr(Index index,
                                                    Index global_index) {
  PrintDetails(" - init global=%" PRIindex "\n", global_index);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnInitExprI32ConstExpr(Index index,
                                                   uint32_t value) {
  PrintDetails(" - init i32=%d\n", value);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnInitExprI64ConstExpr(Index index,
                                                   uint64_t value) {
  PrintDetails(" - init i64=%" PRId64 "\n", value);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnFunctionName(Index index, StringSlice name) {
  PrintDetails(" - func[%" PRIindex "] " PRIstringslice "\n", index,
               WABT_PRINTF_STRING_SLICE_ARG(name));
  return Result::Ok;
}

Result BinaryReaderObjdump::OnLocalName(Index func_index,
                                        Index local_index,
                                        StringSlice name) {
  if (name.length) {
    PrintDetails(" - func[%" PRIindex "] local[%" PRIindex "] " PRIstringslice
                 "\n",
                 func_index, local_index, WABT_PRINTF_STRING_SLICE_ARG(name));
  }
  return Result::Ok;
}

Result BinaryReaderObjdump::OnDataSegmentCount(Index count) {
  return OnCount(count);
}

Result BinaryReaderObjdump::BeginDataSegment(Index index, Index memory_index) {
  PrintDetails(" - memory[%" PRIindex "]", memory_index);
  return Result::Ok;
}

Result BinaryReaderObjdump::OnDataSegmentData(Index index,
                                              const void* src_data,
                                              Address size) {
  if (ShouldPrintDetails()) {
    out_stream_->WriteMemoryDump(src_data, size, state->offset - size, "  - ",
                                 nullptr, PrintChars::Yes);
  }
  return Result::Ok;
}

Result BinaryReaderObjdump::OnRelocCount(Index count,
                                         BinarySection section_code,
                                         StringSlice section_name) {
  BinaryReaderObjdumpBase::OnRelocCount(count, section_code, section_name);
  PrintDetails("  - section: %s\n", get_section_name(section_code));
  return Result::Ok;
}

Result BinaryReaderObjdump::OnReloc(RelocType type,
                                    Offset offset,
                                    Index index,
                                    uint32_t addend) {
  Offset total_offset =
      section_starts[static_cast<size_t>(reloc_section)] + offset;
  PrintDetails("   - %-18s offset=%#08" PRIoffset "(file=%#08" PRIoffset
               ") index=%" PRIindex,
               get_reloc_type_name(type), offset, total_offset, index);
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

}  // namespace

Result read_binary_objdump(const uint8_t* data,
                           size_t size,
                           ObjdumpOptions* options,
                           ObjdumpState* state) {
  ReadBinaryOptions read_options = WABT_READ_BINARY_OPTIONS_DEFAULT;
  read_options.read_debug_names = true;
  read_options.log_stream = options->log_stream;

  switch (options->mode) {
    case ObjdumpMode::Prepass: {
      BinaryReaderObjdumpPrepass reader(data, size, options, state);
      return read_binary(data, size, &reader, &read_options);
    }
    case ObjdumpMode::Disassemble: {
      BinaryReaderObjdumpDisassemble reader(data, size, options, state);
      return read_binary(data, size, &reader, &read_options);
    }
    default: {
      BinaryReaderObjdump reader(data, size, options, state);
      return read_binary(data, size, &reader, &read_options);
    }
  }
}

}  // namespace wabt
