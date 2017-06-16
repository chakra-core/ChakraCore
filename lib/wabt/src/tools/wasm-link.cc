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

#include "wasm-link.h"

#include <memory>
#include <vector>

#include "binary-reader.h"
#include "binding-hash.h"
#include "binary-writer.h"
#include "option-parser.h"
#include "stream.h"
#include "writer.h"
#include "binary-reader-linker.h"

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

struct Context {
  WABT_DISALLOW_COPY_AND_ASSIGN(Context);
  Context() {}

  MemoryStream stream;
  std::vector<std::unique_ptr<LinkerInputBinary>> inputs;
  ssize_t current_section_payload_offset = 0;
};

static void parse_options(int argc, char** argv) {
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
  WABT_ZERO_MEMORY(data);
}

Section::~Section() {
  if (section_code == BinarySection::Data) {
    delete data.data_segments;
  }
}

LinkerInputBinary::LinkerInputBinary(const char* filename,
                                     uint8_t* data,
                                     size_t size)
    : filename(filename),
      data(data),
      size(size),
      active_function_imports(0),
      active_global_imports(0),
      type_index_offset(0),
      function_index_offset(0),
      imported_function_index_offset(0),
      table_index_offset(0),
      memory_page_count(0),
      memory_page_offset(0),
      table_elem_count(0) {}

LinkerInputBinary::~LinkerInputBinary() {
  delete[] data;
}

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
    /* locally declared function call */
    offset = function_index_offset;
    LOG_DEBUG("func reloc %d + %d\n", function_index, offset);
  } else {
    /* imported function call */
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

static void apply_relocation(Section* section, Reloc* r) {
  LinkerInputBinary* binary = section->binary;
  uint8_t* section_data = &binary->data[section->offset];
  size_t section_size = section->size;

  Index cur_value = 0, new_value = 0;
  read_u32_leb128(section_data + r->offset, section_data + section_size,
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
    default:
      WABT_FATAL("unhandled relocation type: %s\n",
                 get_reloc_type_name(r->type));
      break;
  }

  write_fixed_u32_leb128_raw(section_data + r->offset,
                             section_data + section_size, new_value);
}

static void apply_relocations(Section* section) {
  if (!section->relocations.size())
    return;

  LOG_DEBUG("apply_relocations: %s\n", get_section_name(section->section_code));

  /* Perform relocations in-place */
  for (Reloc& reloc: section->relocations) {
    apply_relocation(section, &reloc);
  }
}

static void write_section_payload(Context* ctx, Section* sec) {
  assert(ctx->current_section_payload_offset != -1);

  sec->output_payload_offset =
      ctx->stream.offset() - ctx->current_section_payload_offset;

  uint8_t* payload = &sec->binary->data[sec->payload_offset];
  ctx->stream.WriteData(payload, sec->payload_size, "section content");
}

static void write_c_str(Stream* stream, const char* str, const char* desc) {
  write_str(stream, str, strlen(str), desc, PrintChars::Yes);
}

static void write_slice(Stream* stream, StringSlice str, const char* desc) {
  write_str(stream, str.start, str.length, desc, PrintChars::Yes);
}

static void write_string(Stream* stream,
                         const std::string& str,
                         const char* desc) {
  write_str(stream, str.data(), str.length(), desc, PrintChars::Yes);
}

#define WRITE_UNKNOWN_SIZE(STREAM)                            \
  {                                                           \
    Offset fixup_offset = (STREAM)->offset();                 \
    write_fixed_u32_leb128(STREAM, 0, "unknown size");        \
    ctx->current_section_payload_offset = (STREAM)->offset(); \
    Offset start = (STREAM)->offset();

#define FIXUP_SIZE(STREAM)                                                    \
  write_fixed_u32_leb128_at(STREAM, fixup_offset, (STREAM)->offset() - start, \
                            "fixup size");                                    \
  }

static void write_table_section(Context* ctx,
                                const SectionPtrVector& sections) {
  /* Total section size includes the element count leb128 which is
   * always 1 in the current spec */
  Index table_count = 1;
  uint32_t flags = WABT_BINARY_LIMITS_HAS_MAX_FLAG;
  Index elem_count = 0;

  for (Section* section: sections) {
    elem_count += section->binary->table_elem_count;
  }

  Stream* stream = &ctx->stream;
  WRITE_UNKNOWN_SIZE(stream);
  write_u32_leb128(stream, table_count, "table count");
  write_type(stream, Type::Anyfunc);
  write_u32_leb128(stream, flags, "table elem flags");
  write_u32_leb128(stream, elem_count, "table initial length");
  write_u32_leb128(stream, elem_count, "table max length");
  FIXUP_SIZE(stream);
}

static void write_export_section(Context* ctx) {
  Index total_exports = 0;
  for (const std::unique_ptr<LinkerInputBinary>& binary: ctx->inputs) {
    total_exports += binary->exports.size();
  }

  Stream* stream = &ctx->stream;
  WRITE_UNKNOWN_SIZE(stream);
  write_u32_leb128(stream, total_exports, "export count");

  for (const std::unique_ptr<LinkerInputBinary>& binary : ctx->inputs) {
    for (const Export& export_ : binary->exports) {
      write_slice(stream, export_.name, "export name");
      stream->WriteU8Enum(export_.kind, "export kind");
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
      write_u32_leb128(stream, index, "export index");
    }
  }
  FIXUP_SIZE(stream);
}

static void write_elem_section(Context* ctx,
                               const SectionPtrVector& sections) {
  Stream* stream = &ctx->stream;
  WRITE_UNKNOWN_SIZE(stream);

  Index total_elem_count = 0;
  for (Section* section : sections) {
    total_elem_count += section->binary->table_elem_count;
  }

  write_u32_leb128(stream, 1, "segment count");
  write_u32_leb128(stream, 0, "table index");
  write_opcode(&ctx->stream, Opcode::I32Const);
  write_i32_leb128(&ctx->stream, 0, "elem init literal");
  write_opcode(&ctx->stream, Opcode::End);
  write_u32_leb128(stream, total_elem_count, "num elements");

  ctx->current_section_payload_offset = stream->offset();

  for (Section* section : sections) {
    apply_relocations(section);
    write_section_payload(ctx, section);
  }

  FIXUP_SIZE(stream);
}

static void write_memory_section(Context* ctx,
                                 const SectionPtrVector& sections) {
  Stream* stream = &ctx->stream;
  WRITE_UNKNOWN_SIZE(stream);

  write_u32_leb128(stream, 1, "memory count");

  Limits limits;
  WABT_ZERO_MEMORY(limits);
  limits.has_max = true;
  for (size_t i = 0; i < sections.size(); i++) {
    Section* sec = sections[i];
    limits.initial += sec->data.memory_limits.initial;
  }
  limits.max = limits.initial;
  write_limits(stream, &limits);

  FIXUP_SIZE(stream);
}

static void write_function_import(Context* ctx,
                                  FunctionImport* import,
                                  Index offset) {
  write_slice(&ctx->stream, import->module_name, "import module name");
  write_slice(&ctx->stream, import->name, "import field name");
  ctx->stream.WriteU8Enum(ExternalKind::Func, "import kind");
  write_u32_leb128(&ctx->stream, import->sig_index + offset,
                   "import signature index");
}

static void write_global_import(Context* ctx, GlobalImport* import) {
  write_slice(&ctx->stream, import->module_name, "import module name");
  write_slice(&ctx->stream, import->name, "import field name");
  ctx->stream.WriteU8Enum(ExternalKind::Global, "import kind");
  write_type(&ctx->stream, import->type);
  ctx->stream.WriteU8(import->mutable_, "global mutability");
}

static void write_import_section(Context* ctx) {
  Index num_imports = 0;
  for (size_t i = 0; i < ctx->inputs.size(); i++) {
    LinkerInputBinary* binary = ctx->inputs[i].get();
    std::vector<FunctionImport>& imports = binary->function_imports;
    for (size_t j = 0; j < imports.size(); j++) {
      FunctionImport* import = &imports[j];
      if (import->active)
        num_imports++;
    }
    num_imports += binary->global_imports.size();
  }

  WRITE_UNKNOWN_SIZE(&ctx->stream);
  write_u32_leb128(&ctx->stream, num_imports, "num imports");

  for (size_t i = 0; i < ctx->inputs.size(); i++) {
    LinkerInputBinary* binary = ctx->inputs[i].get();
    std::vector<FunctionImport>& imports = binary->function_imports;
    for (size_t j = 0; j < imports.size(); j++) {
      FunctionImport* import = &imports[j];
      if (import->active)
        write_function_import(ctx, import, binary->type_index_offset);
    }

    std::vector<GlobalImport>& globals = binary->global_imports;
    for (size_t j = 0; j < globals.size(); j++) {
      write_global_import(ctx, &globals[j]);
    }
  }

  FIXUP_SIZE(&ctx->stream);
}

static void write_function_section(Context* ctx,
                                   const SectionPtrVector& sections,
                                   Index total_count) {
  Stream* stream = &ctx->stream;
  WRITE_UNKNOWN_SIZE(stream);

  write_u32_leb128(stream, total_count, "function count");

  for (size_t i = 0; i < sections.size(); i++) {
    Section* sec = sections[i];
    Index count = sec->count;
    Offset input_offset = 0;
    Index sig_index = 0;
    const uint8_t* start = &sec->binary->data[sec->payload_offset];
    const uint8_t* end =
        &sec->binary->data[sec->payload_offset + sec->payload_size];
    while (count--) {
      input_offset += read_u32_leb128(start + input_offset, end, &sig_index);
      write_u32_leb128(stream, sec->binary->RelocateTypeIndex(sig_index),
                       "sig");
    }
  }

  FIXUP_SIZE(stream);
}

static void write_data_segment(Stream* stream,
                               const DataSegment& segment,
                               Address offset) {
  assert(segment.memory_index == 0);
  write_u32_leb128(stream, segment.memory_index, "memory index");
  write_opcode(stream, Opcode::I32Const);
  write_u32_leb128(stream, segment.offset + offset, "offset");
  write_opcode(stream, Opcode::End);
  write_u32_leb128(stream, segment.size, "segment size");
  stream->WriteData(segment.data, segment.size, "segment data");
}

static void write_data_section(Context* ctx,
                               const SectionPtrVector& sections,
                               Index total_count) {
  Stream* stream = &ctx->stream;
  WRITE_UNKNOWN_SIZE(stream);

  write_u32_leb128(stream, total_count, "data segment count");
  for (size_t i = 0; i < sections.size(); i++) {
    Section* sec = sections[i];
    for (size_t j = 0; j < sec->data.data_segments->size(); j++) {
      const DataSegment& segment = (*sec->data.data_segments)[j];
      write_data_segment(stream, segment,
                         sec->binary->memory_page_offset * WABT_PAGE_SIZE);
    }
  }

  FIXUP_SIZE(stream);
}

static void write_names_section(Context* ctx) {
  Index total_count = 0;
  for (const std::unique_ptr<LinkerInputBinary>& binary: ctx->inputs) {
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

  Stream* stream = &ctx->stream;
  stream->WriteU8Enum(BinarySection::Custom, "section code");
  WRITE_UNKNOWN_SIZE(stream);
  write_c_str(stream, "name", "custom section name");

  stream->WriteU8Enum(NameSectionSubsection::Function, "subsection code");
  WRITE_UNKNOWN_SIZE(stream);
  write_u32_leb128(stream, total_count, "element count");

  // Write import names
  for (const std::unique_ptr<LinkerInputBinary>& binary: ctx->inputs) {
    for (size_t i = 0; i < binary->debug_names.size(); i++) {
      if (binary->debug_names[i].empty() || !binary->IsFunctionImport(i))
        continue;
      if (binary->IsInactiveFunctionImport(i))
        continue;
      write_u32_leb128(stream, binary->RelocateFuncIndex(i), "function index");
      write_string(stream, binary->debug_names[i], "function name");
    }
  }

  // Write non-import names
  for (const std::unique_ptr<LinkerInputBinary>& binary: ctx->inputs) {
    for (size_t i = 0; i < binary->debug_names.size(); i++) {
      if (binary->debug_names[i].empty() || binary->IsFunctionImport(i))
        continue;
      write_u32_leb128(stream, binary->RelocateFuncIndex(i), "function index");
      write_string(stream, binary->debug_names[i], "function name");
    }
  }

  FIXUP_SIZE(stream);

  FIXUP_SIZE(stream);
}

static void write_reloc_section(Context* ctx,
                                BinarySection section_code,
                                const SectionPtrVector& sections) {
  Index total_relocs = 0;

  /* First pass to know total reloc count */
  for (Section* sec: sections)
    total_relocs += sec->relocations.size();

  if (!total_relocs)
    return;

  char section_name[128];
  snprintf(section_name, sizeof(section_name), "%s.%s",
           WABT_BINARY_SECTION_RELOC, get_section_name(section_code));

  Stream* stream = &ctx->stream;
  stream->WriteU8Enum(BinarySection::Custom, "section code");
  WRITE_UNKNOWN_SIZE(stream);
  write_c_str(stream, section_name, "reloc section name");
  write_u32_leb128_enum(&ctx->stream, section_code, "reloc section");
  write_u32_leb128(&ctx->stream, total_relocs, "num relocs");

  for (Section* sec: sections) {
    for (const Reloc& reloc: sec->relocations) {
      write_u32_leb128_enum(&ctx->stream, reloc.type, "reloc type");
      Offset new_offset = reloc.offset + sec->output_payload_offset;
      write_u32_leb128(&ctx->stream, new_offset, "reloc offset");
      Index relocated_index;
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
        // TODO(sbc): Handle other relocation types.
        default:
          WABT_FATAL("Unhandled reloc type: %s\n", get_reloc_type_name(reloc.type));
          break;
      }
      write_u32_leb128(&ctx->stream, relocated_index, "reloc index");
    }
  }

  FIXUP_SIZE(stream);
}

static bool write_combined_section(Context* ctx,
                                   BinarySection section_code,
                                   const SectionPtrVector& sections) {
  if (!sections.size())
    return false;

  if (section_code == BinarySection::Start && sections.size() > 1) {
    WABT_FATAL("Don't know how to combine sections of type: %s\n",
               get_section_name(section_code));
  }

  Index total_count = 0;
  Index total_size = 0;

  /* Sum section size and element count */
  for (Section* sec: sections) {
    total_size += sec->payload_size;
    total_count += sec->count;
  }

  ctx->stream.WriteU8Enum(section_code, "section code");
  ctx->current_section_payload_offset = -1;

  switch (section_code) {
    case BinarySection::Import:
      write_import_section(ctx);
      break;
    case BinarySection::Function:
      write_function_section(ctx, sections, total_count);
      break;
    case BinarySection::Table:
      write_table_section(ctx, sections);
      break;
    case BinarySection::Export:
      write_export_section(ctx);
      break;
    case BinarySection::Elem:
      write_elem_section(ctx, sections);
      break;
    case BinarySection::Memory:
      write_memory_section(ctx, sections);
      break;
    case BinarySection::Data:
      write_data_section(ctx, sections, total_count);
      break;
    default: {
      /* Total section size includes the element count leb128. */
      total_size += u32_leb128_length(total_count);

      /* Write section to stream */
      Stream* stream = &ctx->stream;
      write_u32_leb128(stream, total_size, "section size");
      write_u32_leb128(stream, total_count, "element count");
      ctx->current_section_payload_offset = ctx->stream.offset();
      for (Section* sec: sections) {
        apply_relocations(sec);
        write_section_payload(ctx, sec);
      }
    }
  }

  return true;
}

struct ExportInfo {
  ExportInfo(Export* export_, LinkerInputBinary* binary)
      : export_(export_), binary(binary) {}

  Export* export_;
  LinkerInputBinary* binary;
};

static void resolve_symbols(Context* ctx) {
  /* Create hashmap of all exported symbols from all inputs */
  BindingHash export_map;
  std::vector<ExportInfo> export_list;

  for (size_t i = 0; i < ctx->inputs.size(); i++) {
    LinkerInputBinary* binary = ctx->inputs[i].get();
    for (size_t j = 0; j < binary->exports.size(); j++) {
      Export* export_ = &binary->exports[j];
      export_list.emplace_back(export_, binary);

      /* TODO(sbc): Handle duplicate names */
      export_map.emplace(string_slice_to_string(export_->name),
                         Binding(export_list.size() - 1));
    }
  }

  /*
   * Iterate through all imported functions resolving them against exported
   * ones.
   */
  for (size_t i = 0; i < ctx->inputs.size(); i++) {
    LinkerInputBinary* binary = ctx->inputs[i].get();
    for (size_t j = 0; j < binary->function_imports.size(); j++) {
      FunctionImport* import = &binary->function_imports[j];
      int export_index = export_map.FindIndex(import->name);
      if (export_index == -1) {
        if (!s_relocatable)
          WABT_FATAL("undefined symbol: " PRIstringslice "\n",
                     WABT_PRINTF_STRING_SLICE_ARG(import->name));
        continue;
      }

      /* We found the symbol exported by another module */
      ExportInfo* export_info = &export_list[export_index];

      /* TODO(sbc): verify the foriegn function has the correct signature */
      import->active = false;
      import->foreign_binary = export_info->binary;
      import->foreign_index = export_info->export_->index;
      binary->active_function_imports--;
    }
  }
}

static void calculate_reloc_offsets(Context* ctx) {
  Index memory_page_offset = 0;
  Index type_count = 0;
  Index global_count = 0;
  Index function_count = 0;
  Index table_elem_count = 0;
  Index total_function_imports = 0;
  Index total_global_imports = 0;
  for (size_t i = 0; i < ctx->inputs.size(); i++) {
    LinkerInputBinary* binary = ctx->inputs[i].get();
    /* The imported_function_index_offset is the sum of all the function
     * imports from objects that precede this one.  i.e. the current running
     * total */
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

  for (size_t i = 0; i < ctx->inputs.size(); i++) {
    LinkerInputBinary* binary = ctx->inputs[i].get();
    binary->table_index_offset = table_elem_count;
    table_elem_count += binary->table_elem_count;
    for (size_t j = 0; j < binary->sections.size(); j++) {
      Section* sec = binary->sections[j].get();
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

static void write_binary(Context* ctx) {
  /* Find all the sections of each type */
  SectionPtrVector sections[kBinarySectionCount];

  for (size_t j = 0; j < ctx->inputs.size(); j++) {
    LinkerInputBinary* binary = ctx->inputs[j].get();
    for (size_t i = 0; i < binary->sections.size(); i++) {
      Section* s = binary->sections[i].get();
      SectionPtrVector& sec_list = sections[static_cast<int>(s->section_code)];
      sec_list.push_back(s);
    }
  }

  /* Write the final binary */
  ctx->stream.WriteU32(WABT_BINARY_MAGIC, "WABT_BINARY_MAGIC");
  ctx->stream.WriteU32(WABT_BINARY_VERSION, "WABT_BINARY_VERSION");

  /* Write known sections first */
  for (size_t i = FIRST_KNOWN_SECTION; i < kBinarySectionCount; i++) {
    write_combined_section(ctx, static_cast<BinarySection>(i), sections[i]);
  }

  write_names_section(ctx);

  /* Generate a new set of reloction sections */
  for (size_t i = FIRST_KNOWN_SECTION; i < kBinarySectionCount; i++) {
    write_reloc_section(ctx, static_cast<BinarySection>(i), sections[i]);
  }
}

static void dump_reloc_offsets(Context* ctx) {
  if (s_debug) {
    for (size_t i = 0; i < ctx->inputs.size(); i++) {
      LinkerInputBinary* binary = ctx->inputs[i].get();
      LOG_DEBUG("Relocation info for: %s\n", binary->filename);
      LOG_DEBUG(" - type index offset       : %d\n",
                           binary->type_index_offset);
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

static Result perform_link(Context* ctx) {
  if (s_debug)
    ctx->stream.set_log_stream(s_log_stream.get());

  LOG_DEBUG("writing file: %s\n", s_outfile);
  calculate_reloc_offsets(ctx);
  resolve_symbols(ctx);
  calculate_reloc_offsets(ctx);
  dump_reloc_offsets(ctx);
  write_binary(ctx);

  if (WABT_FAILED(ctx->stream.WriteToFile(s_outfile))) {
    WABT_FATAL("error writing linked output to file\n");
  }

  return Result::Ok;
}

int ProgramMain(int argc, char** argv) {
  init_stdio();

  Context context;

  parse_options(argc, argv);

  Result result = Result::Ok;
  for (size_t i = 0; i < s_infiles.size(); i++) {
    const std::string& input_filename = s_infiles[i];
    LOG_DEBUG("reading file: %s\n", input_filename.c_str());
    char* data;
    size_t size;
    result = read_file(input_filename.c_str(), &data, &size);
    if (WABT_FAILED(result))
      return result != Result::Ok;
    LinkerInputBinary* b = new LinkerInputBinary(
        input_filename.c_str(), reinterpret_cast<uint8_t*>(data), size);
    context.inputs.emplace_back(b);
    LinkOptions options = { NULL };
    if (s_debug)
      options.log_stream = s_log_stream.get();
    result = read_binary_linker(b, &options);
    if (WABT_FAILED(result))
      WABT_FATAL("error parsing file: %s\n", input_filename.c_str());
  }

  result = perform_link(&context);
  return result != Result::Ok;
}

int main(int argc, char** argv) {
  WABT_TRY
  return ProgramMain(argc, argv);
  WABT_CATCH_BAD_ALLOC_AND_EXIT
}
