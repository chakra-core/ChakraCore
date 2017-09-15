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

#include "src/wasm-link.h"

#include <memory>
#include <vector>

#include "src/binary-reader.h"
#include "src/binding-hash.h"
#include "src/binary-writer.h"
#include "src/leb128.h"
#include "src/option-parser.h"
#include "src/stream.h"
#include "src/binary-reader-linker.h"

#define FIRST_KNOWN_SECTION static_cast<size_t>(BinarySection::Type)
#define LOG_DEBUG(fmt, ...) if (s_debug) s_log_stream->Writef(fmt, __VA_ARGS__);

using namespace wabt;
using namespace wabt::link;

static const char s_description[] =
R"(  link one or more wasm binary modules into a single binary module.
  $ wasm-link m1.wasm m2.wasm -o out.wasm
)";

static bool s_debug;
static bool s_relocatable;
static const char* s_outfile = "a.wasm";
static std::vector<std::string> s_infiles;
static std::unique_ptr<FileStream> s_log_stream;

static void ParseOptions(int argc, char** argv) {
  OptionParser parser("wasm-link", s_description);

  parser.AddOption("debug",
                   "Log extra information when reading and writing wasm files",
                   []() {
                     s_debug = true;
                     s_log_stream = FileStream::CreateStdout();
                   });
  parser.AddOption('o', "output", "FILE", "Output wasm binary file",
                   [](const char* argument) { s_outfile = argument; });
  parser.AddOption('r', "relocatable", "Output a relocatable object file",
                   []() { s_relocatable = true; });
  parser.AddHelpOption();

  parser.AddArgument(
      "filename", OptionParser::ArgumentCount::OneOrMore,
      [](const std::string& argument) { s_infiles.emplace_back(argument); });

  parser.Parse(argc, argv);
}

Section::Section()
    : binary(nullptr),
      section_code(BinarySection::Invalid),
      size(0),
      offset(0),
      payload_size(0),
      payload_offset(0),
      count(0),
      output_payload_offset(0) {
  ZeroMemory(data);
}

Section::~Section() {
  if (section_code == BinarySection::Data) {
    delete data.data_segments;
  }
}

LinkerInputBinary::LinkerInputBinary(const char* filename,
                                     const std::vector<uint8_t>& data)
    : filename(filename),
      data(data),
      active_function_imports(0),
      active_global_imports(0),
      type_index_offset(0),
      function_index_offset(0),
      imported_function_index_offset(0),
      table_index_offset(0),
      memory_page_count(0),
      memory_page_offset(0),
      table_elem_count(0) {}

bool LinkerInputBinary::IsFunctionImport(Index index) {
  assert(IsValidFunctionIndex(index));
  return index < function_imports.size();
}

bool LinkerInputBinary::IsInactiveFunctionImport(Index index){
  return IsFunctionImport(index) && !function_imports[index].active;
}

bool LinkerInputBinary::IsValidFunctionIndex(Index index) {
  return index < function_imports.size() + function_count;
}

Index LinkerInputBinary::RelocateFuncIndex(Index function_index) {
  Index offset;
  if (!IsFunctionImport(function_index)) {
    // locally declared function call.
    offset = function_index_offset;
    LOG_DEBUG("func reloc %d + %d\n", function_index, offset);
  } else {
    // imported function call.
    FunctionImport* import = &function_imports[function_index];
    if (!import->active) {
      function_index = import->foreign_index;
      offset = import->foreign_binary->function_index_offset;
      LOG_DEBUG("reloc for disabled import. new index = %d + %d\n",
                function_index, offset);
    } else {
      Index new_index = import->relocated_function_index;
      LOG_DEBUG("reloc for active import. old index = %d, new index = %d\n",
                function_index, new_index);
      return new_index;
    }
  }
  return function_index + offset;
}

Index LinkerInputBinary::RelocateTypeIndex(Index type_index) {
  return type_index + type_index_offset;
}

Index LinkerInputBinary::RelocateGlobalIndex(Index global_index) {
  Index offset;
  if (global_index >= global_imports.size()) {
    offset = global_index_offset;
  } else {
    offset = imported_global_index_offset;
  }
  return global_index + offset;
}

static void ApplyRelocation(const Section* section, const Reloc* r) {
  LinkerInputBinary* binary = section->binary;
  uint8_t* section_data = &binary->data[section->offset];
  size_t section_size = section->size;

  Index cur_value = 0, new_value = 0;
  ReadU32Leb128(section_data + r->offset, section_data + section_size,
                &cur_value);

  switch (r->type) {
    case RelocType::FuncIndexLEB:
      new_value = binary->RelocateFuncIndex(cur_value);
      break;
    case RelocType::TypeIndexLEB:
      new_value = binary->RelocateTypeIndex(cur_value);
      break;
    case RelocType::TableIndexSLEB:
      new_value = cur_value + binary->table_index_offset;
      break;
    case RelocType::GlobalIndexLEB:
      new_value = binary->RelocateGlobalIndex(cur_value);
      break;
    case RelocType::TableIndexI32:
    case RelocType::MemoryAddressLEB:
    case RelocType::MemoryAddressSLEB:
    case RelocType::MemoryAddressI32:
      WABT_FATAL("unhandled relocation type: %s\n", GetRelocTypeName(r->type));
  }

  WriteFixedU32Leb128Raw(section_data + r->offset, section_data + section_size,
                         new_value);
}

static void ApplyRelocations(const Section* section) {
  if (!section->relocations.size())
    return;

  LOG_DEBUG("ApplyRelocations: %s\n", GetSectionName(section->section_code));

  // Perform relocations in-place.
  for (const Reloc& reloc: section->relocations) {
    ApplyRelocation(section, &reloc);
  }
}

class Linker {
 public:
  WABT_DISALLOW_COPY_AND_ASSIGN(Linker);
  Linker() = default;

  void AppendBinary(LinkerInputBinary* binary) { inputs_.emplace_back(binary); }
  Result PerformLink();

 private:
  typedef std::pair<Offset, Offset> Fixup;
  Fixup WriteUnknownSize();
  void FixupSize(Fixup);

  void WriteSectionPayload(Section* sec);
  void WriteTableSection(const SectionPtrVector& sections);
  void WriteExportSection();
  void WriteElemSection(const SectionPtrVector& sections);
  void WriteMemorySection(const SectionPtrVector& sections);
  void WriteFunctionImport(const FunctionImport& import, Index offset);
  void WriteGlobalImport(const GlobalImport& import);
  void WriteImportSection();
  void WriteFunctionSection(const SectionPtrVector& sections,
                            Index total_count);
  void WriteDataSegment(const DataSegment& segment, Address offset);
  void WriteDataSection(const SectionPtrVector& sections, Index total_count);
  void WriteNamesSection();
  void WriteLinkingSection(uint32_t data_size, uint32_t data_alignment);
  void WriteRelocSection(BinarySection section_code,
                         const SectionPtrVector& sections);
  bool WriteCombinedSection(BinarySection section_code,
                            const SectionPtrVector& sections);
  void ResolveSymbols();
  void CalculateRelocOffsets();
  void WriteBinary();
  void DumpRelocOffsets();

  MemoryStream stream_;
  std::vector<std::unique_ptr<LinkerInputBinary>> inputs_;
  ssize_t current_section_payload_offset_ = 0;
};

void Linker::WriteSectionPayload(Section* sec) {
  assert(current_section_payload_offset_ != -1);

  sec->output_payload_offset =
      stream_.offset() - current_section_payload_offset_;

  uint8_t* payload = &sec->binary->data[sec->payload_offset];
  stream_.WriteData(payload, sec->payload_size, "section content");
}

Linker::Fixup Linker::WriteUnknownSize() {
  Offset fixup_offset = stream_.offset();
  WriteFixedU32Leb128(&stream_, 0, "unknown size");
  current_section_payload_offset_ = stream_.offset();
  return std::make_pair(fixup_offset, current_section_payload_offset_);
}

void Linker::FixupSize(Fixup fixup) {
  WriteFixedU32Leb128At(&stream_, fixup.first, stream_.offset() - fixup.second,
                        "fixup size");
}

void Linker::WriteTableSection(const SectionPtrVector& sections) {
  // Total section size includes the element count leb128 which is always 1 in
  // the current spec.
  Index table_count = 1;
  uint32_t flags = WABT_BINARY_LIMITS_HAS_MAX_FLAG;
  Index elem_count = 0;

  for (Section* section: sections) {
    elem_count += section->binary->table_elem_count;
  }

  Fixup fixup = WriteUnknownSize();
  WriteU32Leb128(&stream_, table_count, "table count");
  WriteType(&stream_, Type::Anyfunc);
  WriteU32Leb128(&stream_, flags, "table elem flags");
  WriteU32Leb128(&stream_, elem_count, "table initial length");
  WriteU32Leb128(&stream_, elem_count, "table max length");
  FixupSize(fixup);
}

void Linker::WriteExportSection() {
  Index total_exports = 0;
  for (const std::unique_ptr<LinkerInputBinary>& binary : inputs_) {
    total_exports += binary->exports.size();
  }

  Fixup fixup = WriteUnknownSize();
  WriteU32Leb128(&stream_, total_exports, "export count");

  for (const std::unique_ptr<LinkerInputBinary>& binary : inputs_) {
    for (const Export& export_ : binary->exports) {
      WriteStr(&stream_, export_.name, "export name");
      stream_.WriteU8Enum(export_.kind, "export kind");
      Index index = export_.index;
      switch (export_.kind) {
        case ExternalKind::Func:
          index = binary->RelocateFuncIndex(index);
          break;
        default:
          WABT_FATAL("unsupport export type: %d\n",
                     static_cast<int>(export_.kind));
          break;
      }
      WriteU32Leb128(&stream_, index, "export index");
    }
  }

  FixupSize(fixup);
}

void Linker::WriteElemSection(const SectionPtrVector& sections) {
  Fixup fixup = WriteUnknownSize();

  Index total_elem_count = 0;
  for (Section* section : sections) {
    total_elem_count += section->binary->table_elem_count;
  }

  WriteU32Leb128(&stream_, 1, "segment count");
  WriteU32Leb128(&stream_, 0, "table index");
  WriteOpcode(&stream_, Opcode::I32Const);
  WriteS32Leb128(&stream_, 0U, "elem init literal");
  WriteOpcode(&stream_, Opcode::End);
  WriteU32Leb128(&stream_, total_elem_count, "num elements");

  current_section_payload_offset_ = stream_.offset();

  for (Section* section : sections) {
    ApplyRelocations(section);
    WriteSectionPayload(section);
  }

  FixupSize(fixup);
}

void Linker::WriteMemorySection(const SectionPtrVector& sections) {
  Fixup fixup = WriteUnknownSize();

  WriteU32Leb128(&stream_, 1, "memory count");

  Limits limits;
  limits.has_max = true;
  for (Section* section: sections) {
    limits.initial += section->data.initial;
  }
  limits.max = limits.initial;
  WriteLimits(&stream_, &limits);

  FixupSize(fixup);
}

void Linker::WriteFunctionImport(const FunctionImport& import, Index offset) {
  WriteStr(&stream_, import.module_name, "import module name");
  WriteStr(&stream_, import.name, "import field name");
  stream_.WriteU8Enum(ExternalKind::Func, "import kind");
  WriteU32Leb128(&stream_, import.sig_index + offset, "import signature index");
}

void Linker::WriteGlobalImport(const GlobalImport& import) {
  WriteStr(&stream_, import.module_name, "import module name");
  WriteStr(&stream_, import.name, "import field name");
  stream_.WriteU8Enum(ExternalKind::Global, "import kind");
  WriteType(&stream_, import.type);
  stream_.WriteU8(import.mutable_, "global mutability");
}

void Linker::WriteImportSection() {
  Index num_imports = 0;
  for (const std::unique_ptr<LinkerInputBinary>& binary: inputs_) {
    for (const FunctionImport& import : binary->function_imports) {
      if (import.active)
        num_imports++;
    }
    num_imports += binary->global_imports.size();
  }

  Fixup fixup = WriteUnknownSize();
  WriteU32Leb128(&stream_, num_imports, "num imports");

  for (const std::unique_ptr<LinkerInputBinary>& binary: inputs_) {
    for (const FunctionImport& function_import : binary->function_imports) {
      if (function_import.active)
        WriteFunctionImport(function_import, binary->type_index_offset);
    }

    for (const GlobalImport& global_import : binary->global_imports) {
      WriteGlobalImport(global_import);
    }
  }

  FixupSize(fixup);
}

void Linker::WriteFunctionSection(const SectionPtrVector& sections,
                                  Index total_count) {
  Fixup fixup = WriteUnknownSize();

  WriteU32Leb128(&stream_, total_count, "function count");

  for (Section* sec: sections) {
    Index count = sec->count;
    Offset input_offset = 0;
    Index sig_index = 0;
    const uint8_t* start = &sec->binary->data[sec->payload_offset];
    const uint8_t* end =
        &sec->binary->data[sec->payload_offset + sec->payload_size];
    while (count--) {
      input_offset += ReadU32Leb128(start + input_offset, end, &sig_index);
      WriteU32Leb128(&stream_, sec->binary->RelocateTypeIndex(sig_index),
                     "sig");
    }
  }

  FixupSize(fixup);
}

void Linker::WriteDataSegment(const DataSegment& segment, Address offset) {
  assert(segment.memory_index == 0);
  WriteU32Leb128(&stream_, segment.memory_index, "memory index");
  WriteOpcode(&stream_, Opcode::I32Const);
  WriteU32Leb128(&stream_, segment.offset + offset, "offset");
  WriteOpcode(&stream_, Opcode::End);
  WriteU32Leb128(&stream_, segment.size, "segment size");
  stream_.WriteData(segment.data, segment.size, "segment data");
}

void Linker::WriteDataSection(const SectionPtrVector& sections,
                              Index total_count) {
  Fixup fixup = WriteUnknownSize();

  WriteU32Leb128(&stream_, total_count, "data segment count");
  for (const Section* sec: sections) {
    for (const DataSegment& segment: *sec->data.data_segments) {
      WriteDataSegment(segment,
                       sec->binary->memory_page_offset * WABT_PAGE_SIZE);
    }
  }

  FixupSize(fixup);
}

void Linker::WriteNamesSection() {
  Index total_count = 0;
  for (const std::unique_ptr<LinkerInputBinary>& binary : inputs_) {
    for (size_t i = 0; i < binary->debug_names.size(); i++) {
      if (binary->debug_names[i].empty())
        continue;
      if (binary->IsInactiveFunctionImport(i))
        continue;
      total_count++;
    }
  }

  if (!total_count)
    return;

  stream_.WriteU8Enum(BinarySection::Custom, "section code");
  Fixup fixup_section = WriteUnknownSize();
  WriteStr(&stream_, "name", "custom section name");

  stream_.WriteU8Enum(NameSectionSubsection::Function, "subsection code");
  Fixup fixup_subsection = WriteUnknownSize();
  WriteU32Leb128(&stream_, total_count, "element count");

  // Write import names.
  for (const std::unique_ptr<LinkerInputBinary>& binary : inputs_) {
    for (size_t i = 0; i < binary->debug_names.size(); i++) {
      if (binary->debug_names[i].empty() || !binary->IsFunctionImport(i))
        continue;
      if (binary->IsInactiveFunctionImport(i))
        continue;
      WriteU32Leb128(&stream_, binary->RelocateFuncIndex(i), "function index");
      WriteStr(&stream_, binary->debug_names[i], "function name");
    }
  }

  // Write non-import names.
  for (const std::unique_ptr<LinkerInputBinary>& binary : inputs_) {
    for (size_t i = 0; i < binary->debug_names.size(); i++) {
      if (binary->debug_names[i].empty() || binary->IsFunctionImport(i))
        continue;
      WriteU32Leb128(&stream_, binary->RelocateFuncIndex(i), "function index");
      WriteStr(&stream_, binary->debug_names[i], "function name");
    }
  }

  FixupSize(fixup_subsection);
  FixupSize(fixup_section);
}

void Linker::WriteLinkingSection(uint32_t data_size, uint32_t data_alignment) {
  stream_.WriteU8Enum(BinarySection::Custom, "section code");
  Fixup fixup = WriteUnknownSize();

  WriteStr(&stream_, "linking", "linking section name");

  {
    WriteU32Leb128(&stream_, LinkingEntryType::DataSize, "subsection code");
    Fixup fixup_subsection = WriteUnknownSize();
    WriteU32Leb128(&stream_, data_size, "data size");
    FixupSize(fixup_subsection);
  }

  {
    WriteU32Leb128(&stream_, LinkingEntryType::DataAlignment, "subsection code");
    Fixup fixup_subsection = WriteUnknownSize();
    WriteU32Leb128(&stream_, data_alignment, "data alignment");
    FixupSize(fixup_subsection);
  }

  FixupSize(fixup);
}

void Linker::WriteRelocSection(BinarySection section_code,
                               const SectionPtrVector& sections) {
  Index total_relocs = 0;

  // First pass to know total reloc count.
  for (Section* sec: sections)
    total_relocs += sec->relocations.size();

  if (!total_relocs)
    return;

  std::string section_name = StringPrintf("%s.%s", WABT_BINARY_SECTION_RELOC,
                                          GetSectionName(section_code));

  stream_.WriteU8Enum(BinarySection::Custom, "section code");
  Fixup fixup = WriteUnknownSize();
  WriteStr(&stream_, section_name, "reloc section name");
  WriteU32Leb128(&stream_, section_code, "reloc section");
  WriteU32Leb128(&stream_, total_relocs, "num relocs");

  for (Section* sec: sections) {
    for (const Reloc& reloc: sec->relocations) {
      WriteU32Leb128(&stream_, reloc.type, "reloc type");
      Offset new_offset = reloc.offset + sec->output_payload_offset;
      WriteU32Leb128(&stream_, new_offset, "reloc offset");
      Index relocated_index = 0;
      switch (reloc.type) {
        case RelocType::FuncIndexLEB:
          relocated_index = sec->binary->RelocateFuncIndex(reloc.index);
          break;
        case RelocType::TypeIndexLEB:
          relocated_index = sec->binary->RelocateTypeIndex(reloc.index);
          break;
        case RelocType::GlobalIndexLEB:
          relocated_index = sec->binary->RelocateGlobalIndex(reloc.index);
          break;
        case RelocType::MemoryAddressLEB:
        case RelocType::MemoryAddressSLEB:
        case RelocType::MemoryAddressI32:
        case RelocType::TableIndexSLEB:
        case RelocType::TableIndexI32:
          WABT_FATAL("Unhandled reloc type: %s\n",
                     GetRelocTypeName(reloc.type));
      }
      WriteU32Leb128(&stream_, relocated_index, "reloc index");
    }
  }

  FixupSize(fixup);
}

bool Linker::WriteCombinedSection(BinarySection section_code,
                                  const SectionPtrVector& sections) {
  if (!sections.size())
    return false;

  if (section_code == BinarySection::Start && sections.size() > 1) {
    WABT_FATAL("Don't know how to combine sections of type: %s\n",
               GetSectionName(section_code));
  }

  Index total_count = 0;
  Index total_size = 0;

  // Sum section size and element count.
  for (Section* sec : sections) {
    total_size += sec->payload_size;
    total_count += sec->count;
  }

  stream_.WriteU8Enum(section_code, "section code");
  current_section_payload_offset_ = -1;

  switch (section_code) {
    case BinarySection::Import:
      WriteImportSection();
      break;
    case BinarySection::Function:
      WriteFunctionSection(sections, total_count);
      break;
    case BinarySection::Table:
      WriteTableSection(sections);
      break;
    case BinarySection::Export:
      WriteExportSection();
      break;
    case BinarySection::Elem:
      WriteElemSection(sections);
      break;
    case BinarySection::Memory:
      WriteMemorySection(sections);
      break;
    case BinarySection::Data:
      WriteDataSection(sections, total_count);
      break;
    default: {
      // Total section size includes the element count leb128.
      total_size += U32Leb128Length(total_count);

      // Write section to stream.
      WriteU32Leb128(&stream_, total_size, "section size");
      WriteU32Leb128(&stream_, total_count, "element count");
      current_section_payload_offset_ = stream_.offset();
      for (Section* sec : sections) {
        ApplyRelocations(sec);
        WriteSectionPayload(sec);
      }
    }
  }

  return true;
}

struct ExportInfo {
  ExportInfo(const Export* export_, LinkerInputBinary* binary)
      : export_(export_), binary(binary) {}

  const Export* export_;
  LinkerInputBinary* binary;
};

void Linker::ResolveSymbols() {
  // Create hashmap of all exported symbols from all inputs.
  BindingHash export_map;
  std::vector<ExportInfo> export_list;

  for (const std::unique_ptr<LinkerInputBinary>& binary: inputs_) {
    for (const Export& export_ : binary->exports) {
      export_list.emplace_back(&export_, binary.get());

      // TODO(sbc): Handle duplicate names.
      export_map.emplace(export_.name, Binding(export_list.size() - 1));
    }
  }

  // Iterate through all imported functions resolving them against exported
  // ones.
  for (std::unique_ptr<LinkerInputBinary>& binary: inputs_) {
    for (FunctionImport& import: binary->function_imports) {
      Index export_index = export_map.FindIndex(import.name);
      if (export_index == kInvalidIndex) {
        if (!s_relocatable)
          WABT_FATAL("undefined symbol: %s\n", import.name.c_str());
        continue;
      }

      // We found the symbol exported by another module.
      const ExportInfo& export_info = export_list[export_index];

      // TODO(sbc): verify the foriegn function has the correct signature.
      import.active = false;
      import.foreign_binary = export_info.binary;
      import.foreign_index = export_info.export_->index;
      binary->active_function_imports--;
    }
  }
}

void Linker::CalculateRelocOffsets() {
  Index memory_page_offset = 0;
  Index type_count = 0;
  Index global_count = 0;
  Index function_count = 0;
  Index table_elem_count = 0;
  Index total_function_imports = 0;
  Index total_global_imports = 0;

  for (std::unique_ptr<LinkerInputBinary>& binary : inputs_) {
    // The imported_function_index_offset is the sum of all the function
    // imports from objects that precede this one.  i.e. the current running
    // total.
    binary->imported_function_index_offset = total_function_imports;
    binary->imported_global_index_offset = total_global_imports;
    binary->memory_page_offset = memory_page_offset;

    size_t delta = 0;
    for (size_t i = 0; i < binary->function_imports.size(); i++) {
      if (!binary->function_imports[i].active) {
        delta++;
      } else {
        binary->function_imports[i].relocated_function_index =
            total_function_imports + i - delta;
      }
    }

    memory_page_offset += binary->memory_page_count;
    total_function_imports += binary->active_function_imports;
    total_global_imports += binary->global_imports.size();
  }

  for (std::unique_ptr<LinkerInputBinary>& binary : inputs_) {
    binary->table_index_offset = table_elem_count;
    table_elem_count += binary->table_elem_count;
    for (std::unique_ptr<Section>& sec : binary->sections) {
      switch (sec->section_code) {
        case BinarySection::Type:
          binary->type_index_offset = type_count;
          type_count += sec->count;
          break;
        case BinarySection::Global:
          binary->global_index_offset = total_global_imports -
                                        sec->binary->global_imports.size() +
                                        global_count;
          global_count += sec->count;
          break;
        case BinarySection::Function:
          binary->function_index_offset = total_function_imports -
                                          sec->binary->function_imports.size() +
                                          function_count;
          function_count += sec->count;
          break;
        default:
          break;
      }
    }
  }
}

void Linker::WriteBinary() {
  // Find all the sections of each type.
  SectionPtrVector sections[kBinarySectionCount];

  for (std::unique_ptr<LinkerInputBinary>& binary : inputs_) {
    for (std::unique_ptr<Section>& sec : binary->sections) {
      SectionPtrVector& sec_list =
          sections[static_cast<int>(sec->section_code)];
      sec_list.push_back(sec.get());
    }
  }

  // Write the final binary.
  stream_.WriteU32(WABT_BINARY_MAGIC, "WABT_BINARY_MAGIC");
  stream_.WriteU32(WABT_BINARY_VERSION, "WABT_BINARY_VERSION");

  // Write known sections first.
  for (size_t i = FIRST_KNOWN_SECTION; i < kBinarySectionCount; i++) {
    WriteCombinedSection(static_cast<BinarySection>(i), sections[i]);
  }

  WriteNamesSection();

  /* Generate a new set of reloction sections */
  if (s_relocatable) {
    WriteLinkingSection(0, 0);
    for (size_t i = FIRST_KNOWN_SECTION; i < kBinarySectionCount; i++) {
      WriteRelocSection(static_cast<BinarySection>(i), sections[i]);
    }
  }
}

void Linker::DumpRelocOffsets() {
  if (s_debug) {
    for (const std::unique_ptr<LinkerInputBinary>& binary : inputs_) {
      LOG_DEBUG("Relocation info for: %s\n", binary->filename);
      LOG_DEBUG(" - type index offset       : %d\n", binary->type_index_offset);
      LOG_DEBUG(" - mem page offset         : %d\n",
                binary->memory_page_offset);
      LOG_DEBUG(" - function index offset   : %d\n",
                binary->function_index_offset);
      LOG_DEBUG(" - global index offset     : %d\n",
                binary->global_index_offset);
      LOG_DEBUG(" - imported function offset: %d\n",
                binary->imported_function_index_offset);
      LOG_DEBUG(" - imported global offset  : %d\n",
                binary->imported_global_index_offset);
    }
  }
}

Result Linker::PerformLink() {
  if (s_debug)
    stream_.set_log_stream(s_log_stream.get());

  LOG_DEBUG("writing file: %s\n", s_outfile);
  CalculateRelocOffsets();
  ResolveSymbols();
  CalculateRelocOffsets();
  DumpRelocOffsets();
  WriteBinary();

  if (Failed(stream_.WriteToFile(s_outfile))) {
    WABT_FATAL("error writing linked output to file\n");
  }

  return Result::Ok;
}

int ProgramMain(int argc, char** argv) {
  InitStdio();

  Linker linker;

  ParseOptions(argc, argv);

  Result result = Result::Ok;
  for (const std::string& input_filename: s_infiles) {
    LOG_DEBUG("reading file: %s\n", input_filename.c_str());
    std::vector<uint8_t> file_data;
    result = ReadFile(input_filename.c_str(), &file_data);
    if (Failed(result))
      return result != Result::Ok;

    auto binary = new LinkerInputBinary(input_filename.c_str(), file_data);
    linker.AppendBinary(binary);

    LinkOptions options = { NULL };
    if (s_debug)
      options.log_stream = s_log_stream.get();
    result = ReadBinaryLinker(binary, &options);
    if (Failed(result))
      WABT_FATAL("error parsing file: %s\n", input_filename.c_str());
  }

  result = linker.PerformLink();
  return result != Result::Ok;
}

int main(int argc, char** argv) {
  WABT_TRY
  return ProgramMain(argc, argv);
  WABT_CATCH_BAD_ALLOC_AND_EXIT
}
