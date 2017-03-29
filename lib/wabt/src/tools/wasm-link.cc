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

#include "binary-reader.h"
#include "binding-hash.h"
#include "binary-writer.h"
#include "option-parser.h"
#include "stream.h"
#include "writer.h"
#include "binary-reader-linker.h"

#include <memory>
#include <vector>

#define PROGRAM_NAME "wasm-link"
#define NOPE HasArgument::No
#define YEP HasArgument::Yes
#define FIRST_KNOWN_SECTION static_cast<size_t>(BinarySection::Type)

using namespace wabt;
using namespace wabt::link;

enum { FLAG_DEBUG, FLAG_OUTPUT, FLAG_RELOCATABLE, FLAG_HELP, NUM_FLAGS };

static const char s_description[] =
    "  link one or more wasm binary modules into a single binary module."
    "\n"
    "  $ wasm-link m1.wasm m2.wasm -o out.wasm\n";

static Option s_options[] = {
    {FLAG_DEBUG, '\0', "debug", nullptr, NOPE,
     "log extra information when reading and writing wasm files"},
    {FLAG_OUTPUT, 'o', "output", "FILE", YEP, "output wasm binary file"},
    {FLAG_RELOCATABLE, 'r', "relocatable", nullptr, NOPE,
     "output a relocatable object file"},
    {FLAG_HELP, 'h', "help", nullptr, NOPE, "print this help message"},
};
WABT_STATIC_ASSERT(NUM_FLAGS == WABT_ARRAY_SIZE(s_options));

static bool s_debug;
static bool s_relocatable;
static const char* s_outfile = "a.wasm";
static std::vector<std::string> s_infiles;
static FileWriter s_log_stream_writer;
static Stream s_log_stream;

struct Context {
  WABT_DISALLOW_COPY_AND_ASSIGN(Context);
  Context() {
    WABT_ZERO_MEMORY(stream);
  }

  Stream stream;
  std::vector<std::unique_ptr<LinkerInputBinary>> inputs;
  ssize_t current_section_payload_offset = 0;
};

static void on_option(struct OptionParser* parser,
                      struct Option* option,
                      const char* argument) {
  switch (option->id) {
    case FLAG_DEBUG:
      s_debug = true;
      init_file_writer_existing(&s_log_stream_writer, stdout);
      init_stream(&s_log_stream, &s_log_stream_writer.base, nullptr);
      break;

    case FLAG_OUTPUT:
      s_outfile = argument;
      break;

    case FLAG_RELOCATABLE:
      s_relocatable = true;
      break;

    case FLAG_HELP:
      print_help(parser, PROGRAM_NAME);
      exit(0);
      break;
  }
}

static void on_argument(struct OptionParser* parser, const char* argument) {
  s_infiles.emplace_back(argument);
}

static void on_option_error(struct OptionParser* parser, const char* message) {
  WABT_FATAL("%s\n", message);
}

static void parse_options(int argc, char** argv) {
  OptionParser parser;
  WABT_ZERO_MEMORY(parser);
  parser.description = s_description;
  parser.options = s_options;
  parser.num_options = WABT_ARRAY_SIZE(s_options);
  parser.on_option = on_option;
  parser.on_argument = on_argument;
  parser.on_error = on_option_error;
  parse_options(&parser, argc, argv);

  if (!s_infiles.size()) {
    print_help(&parser, PROGRAM_NAME);
    WABT_FATAL("No inputs files specified.\n");
  }
}

Section::Section()
    : binary(nullptr),
      section_code(BinarySection::Invalid),
      size(0),
      offset(0),
      payload_size(0),
      payload_offset(0),
      count(0),
      output_payload_offset(0) {}

Section::~Section() {
  if (section_code == BinarySection::Data) {
    delete data_segments;
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

static uint32_t relocate_func_index(LinkerInputBinary* binary,
                                    uint32_t function_index) {
  uint32_t offset;
  if (function_index >= binary->function_imports.size()) {
    /* locally declared function call */
    offset = binary->function_index_offset;
    if (s_debug)
      writef(&s_log_stream, "func reloc %d + %d\n", function_index, offset);
  } else {
    /* imported function call */
    FunctionImport* import = &binary->function_imports[function_index];
    offset = binary->imported_function_index_offset;
    if (!import->active) {
      function_index = import->foreign_index;
      offset = import->foreign_binary->function_index_offset;
      if (s_debug)
        writef(&s_log_stream,
               "reloc for disabled import. new index = %d + %d\n",
               function_index, offset);
    }
  }
  return function_index + offset;
}

static void apply_relocation(Section* section, Reloc* r) {
  LinkerInputBinary* binary = section->binary;
  uint8_t* section_data = &binary->data[section->offset];
  size_t section_size = section->size;

  uint32_t cur_value = 0, new_value = 0;
  read_u32_leb128(section_data + r->offset, section_data + section_size,
                  &cur_value);

  uint32_t offset = 0;
  switch (r->type) {
    case RelocType::FuncIndexLEB:
      new_value = relocate_func_index(binary, cur_value);
      break;
    case RelocType::TableIndexSLEB:
      printf("%s: table index reloc: %d offset=%d\n", binary->filename,
             cur_value, binary->table_index_offset);
      offset = binary->table_index_offset;
      new_value = cur_value + offset;
      break;
    case RelocType::GlobalIndexLEB:
      if (cur_value >= binary->global_imports.size()) {
        offset = binary->global_index_offset;
      } else {
        offset = binary->imported_global_index_offset;
      }
      new_value = cur_value + offset;
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

  if (s_debug)
    writef(&s_log_stream, "apply_relocations: %s\n",
           get_section_name(section->section_code));

  /* Perform relocations in-place */
  for (Reloc& reloc: section->relocations) {
    apply_relocation(section, &reloc);
  }
}

static void write_section_payload(Context* ctx, Section* sec) {
  assert(ctx->current_section_payload_offset != -1);

  sec->output_payload_offset =
      ctx->stream.offset - ctx->current_section_payload_offset;

  uint8_t* payload = &sec->binary->data[sec->payload_offset];
  write_data(&ctx->stream, payload, sec->payload_size, "section content");
}

static void write_c_str(Stream* stream, const char* str, const char* desc) {
  write_str(stream, str, strlen(str), PrintChars::Yes, desc);
}

static void write_slice(Stream* stream, StringSlice str, const char* desc) {
  write_str(stream, str.start, str.length, PrintChars::Yes, desc);
}

static void write_string(Stream* stream,
                         const std::string& str,
                         const char* desc) {
  write_str(stream, str.data(), str.length(), PrintChars::Yes, desc);
}

#define WRITE_UNKNOWN_SIZE(STREAM)                        \
  {                                                       \
  uint32_t fixup_offset = (STREAM)->offset;               \
  write_fixed_u32_leb128(STREAM, 0, "unknown size");      \
  ctx->current_section_payload_offset = (STREAM)->offset; \
  uint32_t start = (STREAM)->offset;

#define FIXUP_SIZE(STREAM)                                                  \
  write_fixed_u32_leb128_at(STREAM, fixup_offset, (STREAM)->offset - start, \
                            "fixup size");                              \
  }

static void write_table_section(Context* ctx,
                                const SectionPtrVector& sections) {
  /* Total section size includes the element count leb128 which is
   * always 1 in the current spec */
  uint32_t table_count = 1;
  uint32_t flags = WABT_BINARY_LIMITS_HAS_MAX_FLAG;
  uint32_t elem_count = 0;

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
  uint32_t total_exports = 0;
  for (const std::unique_ptr<LinkerInputBinary>& binary: ctx->inputs) {
    total_exports += binary->exports.size();
  }

  Stream* stream = &ctx->stream;
  WRITE_UNKNOWN_SIZE(stream);
  write_u32_leb128(stream, total_exports, "export count");

  for (const std::unique_ptr<LinkerInputBinary>& binary : ctx->inputs) {
    for (const Export& export_ : binary->exports) {
      write_slice(stream, export_.name, "export name");
      write_u8_enum(stream, export_.kind, "export kind");
      uint32_t index = export_.index;
      switch (export_.kind) {
        case ExternalKind::Func:
          index = relocate_func_index(binary.get(), index);
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

  uint32_t total_elem_count = 0;
  for (Section* section : sections) {
    total_elem_count += section->binary->table_elem_count;
  }

  write_u32_leb128(stream, 1, "segment count");
  write_u32_leb128(stream, 0, "table index");
  write_opcode(&ctx->stream, Opcode::I32Const);
  write_i32_leb128(&ctx->stream, 0, "elem init literal");
  write_opcode(&ctx->stream, Opcode::End);
  write_u32_leb128(stream, total_elem_count, "num elements");

  ctx->current_section_payload_offset = stream->offset;

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
    limits.initial += sec->memory_limits.initial;
  }
  limits.max = limits.initial;
  write_limits(stream, &limits);

  FIXUP_SIZE(stream);
}

static void write_function_import(Context* ctx,
                                  FunctionImport* import,
                                  uint32_t offset) {
  write_c_str(&ctx->stream, WABT_LINK_MODULE_NAME, "import module name");
  write_slice(&ctx->stream, import->name, "import field name");
  write_u8_enum(&ctx->stream, ExternalKind::Func, "import kind");
  write_u32_leb128(&ctx->stream, import->sig_index + offset,
                   "import signature index");
}

static void write_global_import(Context* ctx, GlobalImport* import) {
  write_c_str(&ctx->stream, WABT_LINK_MODULE_NAME, "import module name");
  write_slice(&ctx->stream, import->name, "import field name");
  write_u8_enum(&ctx->stream, ExternalKind::Global, "import kind");
  write_type(&ctx->stream, import->type);
  write_u8(&ctx->stream, import->mutable_, "global mutability");
}

static void write_import_section(Context* ctx) {
  uint32_t num_imports = 0;
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
                                   uint32_t total_count) {
  Stream* stream = &ctx->stream;
  WRITE_UNKNOWN_SIZE(stream);

  write_u32_leb128(stream, total_count, "function count");

  for (size_t i = 0; i < sections.size(); i++) {
    Section* sec = sections[i];
    uint32_t count = sec->count;
    uint32_t input_offset = 0;
    uint32_t sig_index = 0;
    const uint8_t* start = &sec->binary->data[sec->payload_offset];
    const uint8_t* end =
        &sec->binary->data[sec->payload_offset + sec->payload_size];
    while (count--) {
      input_offset += read_u32_leb128(start + input_offset, end, &sig_index);
      sig_index += sec->binary->type_index_offset;
      write_u32_leb128(stream, sig_index, "sig");
    }
  }

  FIXUP_SIZE(stream);
}

static void write_data_segment(Stream* stream,
                               const DataSegment& segment,
                               uint32_t offset) {
  assert(segment.memory_index == 0);
  write_u32_leb128(stream, segment.memory_index, "memory index");
  write_opcode(stream, Opcode::I32Const);
  write_u32_leb128(stream, segment.offset + offset, "offset");
  write_opcode(stream, Opcode::End);
  write_u32_leb128(stream, segment.size, "segment size");
  write_data(stream, segment.data, segment.size, "segment data");
}

static void write_data_section(Context* ctx,
                               const SectionPtrVector& sections,
                               uint32_t total_count) {
  Stream* stream = &ctx->stream;
  WRITE_UNKNOWN_SIZE(stream);

  write_u32_leb128(stream, total_count, "data segment count");
  for (size_t i = 0; i < sections.size(); i++) {
    Section* sec = sections[i];
    for (size_t j = 0; j < sec->data_segments->size(); j++) {
      const DataSegment& segment = (*sec->data_segments)[j];
      write_data_segment(stream, segment,
                         sec->binary->memory_page_offset * WABT_PAGE_SIZE);
    }
  }

  FIXUP_SIZE(stream);
}

static void write_names_section(Context* ctx) {
  uint32_t total_count = 0;
  size_t k;
  for (size_t i = 0; i < ctx->inputs.size(); i++) {
    LinkerInputBinary* binary = ctx->inputs[i].get();
    for (size_t j = 0; j < binary->debug_names.size(); j++) {
      if (j < binary->function_imports.size()) {
        if (!binary->function_imports[j].active)
          continue;
      }
      total_count++;
    }
  }

  if (!total_count)
    return;

  Stream* stream = &ctx->stream;
  write_u8_enum(stream, BinarySection::Custom, "section code");
  WRITE_UNKNOWN_SIZE(stream);
  write_c_str(stream, "name", "custom section name");
  write_u8_enum(stream, NameSectionSubsection::Function, "subsection code");
  WRITE_UNKNOWN_SIZE(stream);
  write_u32_leb128(stream, total_count, "element count");

  k = 0;
  for (size_t i = 0; i < ctx->inputs.size(); i++) {
    LinkerInputBinary* binary = ctx->inputs[i].get();
    for (size_t j = 0; j < binary->debug_names.size(); j++) {
      if (j < binary->function_imports.size()) {
        if (!binary->function_imports[j].active)
          continue;
      }
      write_u32_leb128(stream, k++, "function index");
      write_string(stream, binary->debug_names[j], "function name");
    }
  }

  FIXUP_SIZE(stream);
  FIXUP_SIZE(stream);
}

static void write_reloc_section(Context* ctx,
                                BinarySection section_code,
                                const SectionPtrVector& sections) {
  uint32_t total_relocs = 0;

  /* First pass to know total reloc count */
  for (size_t i = 0; i < sections.size(); i++) {
    Section* sec = sections[i];
    total_relocs += sec->relocations.size();
  }

  if (!total_relocs)
    return;

  char section_name[128];
  snprintf(section_name, sizeof(section_name), "%s.%s",
           WABT_BINARY_SECTION_RELOC, get_section_name(section_code));

  Stream* stream = &ctx->stream;
  write_u8_enum(stream, BinarySection::Custom, "section code");
  WRITE_UNKNOWN_SIZE(stream);
  write_c_str(stream, section_name, "reloc section name");
  write_u32_leb128_enum(&ctx->stream, section_code, "reloc section");
  write_u32_leb128(&ctx->stream, total_relocs, "num relocs");

  for (size_t i = 0; i < sections.size(); i++) {
    Section* sec = sections[i];
    const std::vector<Reloc>& relocs = sec->relocations;
    for (size_t j = 0; j < relocs.size(); j++) {
      write_u32_leb128_enum(&ctx->stream, relocs[j].type, "reloc type");
      uint32_t new_offset = relocs[j].offset + sec->output_payload_offset;
      write_u32_leb128(&ctx->stream, new_offset, "reloc offset");
      write_u32_leb128(&ctx->stream, relocs[j].index, "reloc index");
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

  uint32_t total_count = 0;
  uint32_t total_size = 0;

  /* Sum section size and element count */
  for (size_t i = 0; i < sections.size(); i++) {
    Section* sec = sections[i];
    total_size += sec->payload_size;
    total_count += sec->count;
  }

  write_u8_enum(&ctx->stream, section_code, "section code");
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
      ctx->current_section_payload_offset = ctx->stream.offset;
      for (size_t i = 0; i < sections.size(); i++) {
        Section* sec = sections[i];
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
      int export_index = export_map.find_index(import->name);
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
  uint32_t memory_page_offset = 0;
  uint32_t type_count = 0;
  uint32_t global_count = 0;
  uint32_t function_count = 0;
  uint32_t table_elem_count = 0;
  uint32_t total_function_imports = 0;
  uint32_t total_global_imports = 0;
  for (size_t i = 0; i < ctx->inputs.size(); i++) {
    LinkerInputBinary* binary = ctx->inputs[i].get();
    /* The imported_function_index_offset is the sum of all the function
     * imports from objects that precede this one.  i.e. the current running
     * total */
    binary->imported_function_index_offset = total_function_imports;
    binary->imported_global_index_offset = total_global_imports;
    binary->memory_page_offset = memory_page_offset;
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
  write_u32(&ctx->stream, WABT_BINARY_MAGIC, "WABT_BINARY_MAGIC");
  write_u32(&ctx->stream, WABT_BINARY_VERSION, "WABT_BINARY_VERSION");

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
    for (uint32_t i = 0; i < ctx->inputs.size(); i++) {
      LinkerInputBinary* binary = ctx->inputs[i].get();
      writef(&s_log_stream, "Relocation info for: %s\n", binary->filename);
      writef(&s_log_stream, " - type index offset       : %d\n",
             binary->type_index_offset);
      writef(&s_log_stream, " - mem page offset         : %d\n",
             binary->memory_page_offset);
      writef(&s_log_stream, " - function index offset   : %d\n",
             binary->function_index_offset);
      writef(&s_log_stream, " - global index offset     : %d\n",
             binary->global_index_offset);
      writef(&s_log_stream, " - imported function offset: %d\n",
             binary->imported_function_index_offset);
      writef(&s_log_stream, " - imported global offset  : %d\n",
             binary->imported_global_index_offset);
    }
  }
}

static Result perform_link(Context* ctx) {
  MemoryWriter writer;
  WABT_ZERO_MEMORY(writer);
  if (WABT_FAILED(init_mem_writer(&writer)))
    WABT_FATAL("unable to open memory writer for writing\n");

  Stream* log_stream = nullptr;
  if (s_debug)
    log_stream = &s_log_stream;

  if (s_debug)
    writef(&s_log_stream, "writing file: %s\n", s_outfile);

  calculate_reloc_offsets(ctx);
  resolve_symbols(ctx);
  calculate_reloc_offsets(ctx);
  dump_reloc_offsets(ctx);
  init_stream(&ctx->stream, &writer.base, log_stream);
  write_binary(ctx);

  if (WABT_FAILED(write_output_buffer_to_file(&writer.buf, s_outfile)))
    WABT_FATAL("error writing linked output to file\n");

  close_mem_writer(&writer);
  return Result::Ok;
}

int main(int argc, char** argv) {
  init_stdio();

  Context context;

  parse_options(argc, argv);

  Result result = Result::Ok;
  for (size_t i = 0; i < s_infiles.size(); i++) {
    const std::string& input_filename = s_infiles[i];
    if (s_debug)
      writef(&s_log_stream, "reading file: %s\n", input_filename.c_str());
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
      options.log_stream = &s_log_stream;
    result = read_binary_linker(b, &options);
    if (WABT_FAILED(result))
      WABT_FATAL("error parsing file: %s\n", input_filename.c_str());
  }

  result = perform_link(&context);
  return result != Result::Ok;
}
