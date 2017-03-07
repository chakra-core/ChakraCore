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

#include "binary-reader-linker.h"

#include "binary-reader.h"
#include "wasm-link.h"

#define RELOC_SIZE 5

namespace wabt {

struct Context {
  LinkerInputBinary* binary;

  Section* reloc_section;

  StringSlice import_name;
  Section* current_section;
};

static Result on_reloc_count(uint32_t count,
                             BinarySection section_code,
                             StringSlice section_name,
                             void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  LinkerInputBinary* binary = ctx->binary;
  if (section_code == BinarySection::Custom) {
    WABT_FATAL("relocation for custom sections not yet supported\n");
  }

  uint32_t i;
  for (i = 0; i < binary->sections.size; i++) {
    Section* sec = &binary->sections.data[i];
    if (sec->section_code != section_code)
      continue;
    ctx->reloc_section = sec;
    return Result::Ok;
  }

  WABT_FATAL("section not found: %d\n", static_cast<int>(section_code));
  return Result::Error;
}

static Result on_reloc(RelocType type, uint32_t offset, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);

  if (offset + RELOC_SIZE > ctx->reloc_section->size) {
    WABT_FATAL("invalid relocation offset: %#x\n", offset);
  }

  Reloc* reloc = append_reloc(&ctx->reloc_section->relocations);
  reloc->type = type;
  reloc->offset = offset;

  return Result::Ok;
}

static Result on_import(uint32_t index,
                        StringSlice module_name,
                        StringSlice field_name,
                        void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  if (!string_slice_eq_cstr(&module_name, WABT_LINK_MODULE_NAME)) {
    WABT_FATAL("unsupported import module: " PRIstringslice,
               WABT_PRINTF_STRING_SLICE_ARG(module_name));
  }
  ctx->import_name = field_name;
  return Result::Ok;
}

static Result on_import_func(uint32_t import_index,
                             uint32_t global_index,
                             uint32_t sig_index,
                             void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  FunctionImport* import =
      append_function_import(&ctx->binary->function_imports);
  import->name = ctx->import_name;
  import->sig_index = sig_index;
  import->active = true;
  ctx->binary->active_function_imports++;
  return Result::Ok;
}

static Result on_import_global(uint32_t import_index,
                               uint32_t global_index,
                               Type type,
                               bool mutable_,
                               void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  GlobalImport* import = append_global_import(&ctx->binary->global_imports);
  import->name = ctx->import_name;
  import->type = type;
  import->mutable_ = mutable_;
  ctx->binary->active_global_imports++;
  return Result::Ok;
}

static Result begin_section(BinaryReaderContext* ctx,
                            BinarySection section_code,
                            uint32_t size) {
  Context* context = static_cast<Context*>(ctx->user_data);
  LinkerInputBinary* binary = context->binary;
  Section* sec = append_section(&binary->sections);
  context->current_section = sec;
  sec->section_code = section_code;
  sec->size = size;
  sec->offset = ctx->offset;
  sec->binary = binary;

  if (sec->section_code != BinarySection::Custom &&
      sec->section_code != BinarySection::Start) {
    size_t bytes_read = read_u32_leb128(
        &binary->data[sec->offset], &binary->data[binary->size], &sec->count);
    if (bytes_read == 0)
      WABT_FATAL("error reading section element count\n");
    sec->payload_offset = sec->offset + bytes_read;
    sec->payload_size = sec->size - bytes_read;
  }
  return Result::Ok;
}

static Result begin_custom_section(BinaryReaderContext* ctx,
                                   uint32_t size,
                                   StringSlice section_name) {
  Context* context = static_cast<Context*>(ctx->user_data);
  LinkerInputBinary* binary = context->binary;
  Section* sec = context->current_section;
  sec->data_custom.name = section_name;

  /* Modify section size and offset to not include the name itself. */
  size_t delta = ctx->offset - sec->offset;
  sec->offset = sec->offset + delta;
  sec->size = sec->size - delta;
  sec->payload_offset = sec->offset;
  sec->payload_size = sec->size;

  /* Special handling for certain CUSTOM sections */
  if (string_slice_eq_cstr(&section_name, "name")) {
    uint32_t name_type;
    size_t bytes_read = read_u32_leb128(
        &binary->data[sec->offset], &binary->data[binary->size], &name_type);

    if (static_cast<NameSectionSubsection>(name_type) != NameSectionSubsection::Function) {
      WABT_FATAL("no function name section");
    }

    sec->payload_offset += bytes_read;
    sec->payload_size -= bytes_read;

    uint32_t subsection_size;
    bytes_read = read_u32_leb128(
        &binary->data[sec->offset], &binary->data[binary->size], &subsection_size);

    sec->payload_offset += bytes_read;
    sec->payload_size -= bytes_read;

    bytes_read = read_u32_leb128(
        &binary->data[sec->payload_offset], &binary->data[binary->size], &sec->count);
    sec->payload_offset += bytes_read;
    sec->payload_size -= bytes_read;

    /* We don't currently support merging name sections unless they contain
     * a name for every function. */
    size_t i;
    uint32_t total_funcs = binary->function_imports.size;
    for (i = 0; i < binary->sections.size; i++) {
      if (binary->sections.data[i].section_code == BinarySection::Function) {
        total_funcs += binary->sections.data[i].count;
        break;
      }
    }
    if (total_funcs != sec->count) {
      WABT_FATAL("name section count (%d) does not match function count (%d)\n",
                 sec->count, total_funcs);
    }
  }

  return Result::Ok;
}

static Result on_table(uint32_t index,
                       Type elem_type,
                       const Limits* elem_limits,
                       void* user_data) {
  if (elem_limits->has_max && (elem_limits->max != elem_limits->initial))
    WABT_FATAL("Tables with max != initial not supported by wabt-link\n");

  Context* ctx = static_cast<Context*>(user_data);
  ctx->binary->table_elem_count = elem_limits->initial;
  return Result::Ok;
}

static Result on_elem_segment_function_index_count(BinaryReaderContext* ctx,
                                                   uint32_t index,
                                                   uint32_t count) {
  Context* context = static_cast<Context*>(ctx->user_data);
  Section* sec = context->current_section;

  /* Modify the payload to include only the actual function indexes */
  size_t delta = ctx->offset - sec->payload_offset;
  sec->payload_offset += delta;
  sec->payload_size -= delta;
  return Result::Ok;
}

static Result on_memory(uint32_t index,
                        const Limits* page_limits,
                        void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Section* sec = ctx->current_section;
  sec->memory_limits = *page_limits;
  ctx->binary->memory_page_count = page_limits->initial;
  return Result::Ok;
}

static Result begin_data_segment(uint32_t index,
                                 uint32_t memory_index,
                                 void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Section* sec = ctx->current_section;
  DataSegment* segment = append_data_segment(&sec->data_segments);
  segment->memory_index = memory_index;
  return Result::Ok;
}

static Result on_init_expr_i32_const_expr(uint32_t index,
                                          uint32_t value,
                                          void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Section* sec = ctx->current_section;
  if (sec->section_code != BinarySection::Data)
    return Result::Ok;
  DataSegment* segment = &sec->data_segments.data[sec->data_segments.size - 1];
  segment->offset = value;
  return Result::Ok;
}

static Result on_data_segment_data(uint32_t index,
                                   const void* src_data,
                                   uint32_t size,
                                   void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Section* sec = ctx->current_section;
  DataSegment* segment = &sec->data_segments.data[sec->data_segments.size - 1];
  segment->data = static_cast<const uint8_t*>(src_data);
  segment->size = size;
  return Result::Ok;
}

static Result on_export(uint32_t index,
                        ExternalKind kind,
                        uint32_t item_index,
                        StringSlice name,
                        void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  Export* export_ = append_export(&ctx->binary->exports);
  export_->name = name;
  export_->kind = kind;
  export_->index = item_index;
  return Result::Ok;
}

static Result on_function_name(uint32_t index,
                               StringSlice name,
                               void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  while (ctx->binary->debug_names.size < index) {
    StringSlice empty = empty_string_slice();
    append_string_slice_value(&ctx->binary->debug_names, &empty);
  }
  if (ctx->binary->debug_names.size == index) {
    append_string_slice_value(&ctx->binary->debug_names, &name);
  }
  return Result::Ok;
}

Result read_binary_linker(LinkerInputBinary* input_info) {
  Context context;
  WABT_ZERO_MEMORY(context);
  context.binary = input_info;

  BinaryReader reader;
  WABT_ZERO_MEMORY(reader);
  reader.user_data = &context;
  reader.begin_section = begin_section;
  reader.begin_custom_section = begin_custom_section;

  reader.on_reloc_count = on_reloc_count;
  reader.on_reloc = on_reloc;

  reader.on_import = on_import;
  reader.on_import_func = on_import_func;
  reader.on_import_global = on_import_global;

  reader.on_export = on_export;

  reader.on_table = on_table;

  reader.on_memory = on_memory;

  reader.begin_data_segment = begin_data_segment;
  reader.on_init_expr_i32_const_expr = on_init_expr_i32_const_expr;
  reader.on_data_segment_data = on_data_segment_data;

  reader.on_elem_segment_function_index_count =
      on_elem_segment_function_index_count;

  reader.on_function_name = on_function_name;

  ReadBinaryOptions read_options = WABT_READ_BINARY_OPTIONS_DEFAULT;
  read_options.read_debug_names = true;
  return read_binary(input_info->data, input_info->size, &reader, 1,
                     &read_options);
}

}  // namespace wabt
