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

#include <assert.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>

#include "binary-reader.h"
#include "literal.h"
#include "vector.h"

namespace wabt {

typedef uint32_t Uint32;
WABT_DEFINE_VECTOR(uint32, Uint32);

struct Context {
  ObjdumpOptions* options;
  Stream* out_stream;
  const uint8_t* data;
  size_t size;
  Opcode current_opcode;
  size_t current_opcode_offset;
  size_t last_opcode_end;
  int indent_level;
  bool print_details;
  bool header_printed;
  int section_found;

  uint32_t section_starts[kBinarySectionCount];
  BinarySection reloc_section;

  StringSlice import_module_name;
  StringSlice import_field_name;

  uint32_t next_reloc;
};

static bool should_print_details(Context* ctx) {
  if (ctx->options->mode != ObjdumpMode::Details)
    return false;
  return ctx->print_details;
}

static void WABT_PRINTF_FORMAT(2, 3)
    print_details(Context* ctx, const char* fmt, ...) {
  if (!should_print_details(ctx))
    return;
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
}

static Result begin_section(BinaryReaderContext* ctx,
                            BinarySection section_code,
                            uint32_t size) {
  Context* context = static_cast<Context*>(ctx->user_data);
  context->section_starts[static_cast<size_t>(section_code)] = ctx->offset;

  const char* name = get_section_name(section_code);

  bool section_match = !context->options->section_name ||
                       !strcasecmp(context->options->section_name, name);
  if (section_match)
    context->section_found = true;

  switch (context->options->mode) {
    case ObjdumpMode::Prepass:
      break;
    case ObjdumpMode::Headers:
      printf("%9s start=%#010" PRIzx " end=%#010" PRIzx " (size=%#010x) ", name,
             ctx->offset, ctx->offset + size, size);
      break;
    case ObjdumpMode::Details:
      if (section_match) {
        if (section_code != BinarySection::Code)
          printf("%s:\n", name);
        context->print_details = true;
      } else {
        context->print_details = false;
      }
      break;
    case ObjdumpMode::RawData:
      if (section_match) {
        printf("\nContents of section %s:\n", name);
        write_memory_dump(context->out_stream, context->data + ctx->offset,
                          size, ctx->offset, PrintChars::Yes, nullptr, nullptr);
      }
      break;
    case ObjdumpMode::Disassemble:
      break;
  }
  return Result::Ok;
}

static Result begin_custom_section(BinaryReaderContext* ctx,
                                   uint32_t size,
                                   StringSlice section_name) {
  Context* context = static_cast<Context*>(ctx->user_data);
  print_details(context, " - name: \"" PRIstringslice "\"\n",
                WABT_PRINTF_STRING_SLICE_ARG(section_name));
  if (context->options->mode == ObjdumpMode::Headers) {
    printf("\"" PRIstringslice "\"\n",
           WABT_PRINTF_STRING_SLICE_ARG(section_name));
  }
  return Result::Ok;
}

static Result on_count(uint32_t count, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  if (ctx->options->mode == ObjdumpMode::Headers) {
    printf("count: %d\n", count);
  }
  return Result::Ok;
}

static Result begin_module(uint32_t version, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  if (ctx->options->print_header) {
    const char* basename = strrchr(ctx->options->infile, '/');
    if (basename)
      basename++;
    else
      basename = ctx->options->infile;
    printf("%s:\tfile format wasm %#08x\n", basename, version);
    ctx->header_printed = true;
  }

  switch (ctx->options->mode) {
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
    case ObjdumpMode::RawData:
    case ObjdumpMode::Prepass:
      break;
  }

  return Result::Ok;
}

static Result end_module(void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  if (ctx->options->section_name) {
    if (!ctx->section_found) {
      printf("Section not found: %s\n", ctx->options->section_name);
      return Result::Error;
    }
  }

  return Result::Ok;
}

static Result on_opcode(BinaryReaderContext* ctx, Opcode opcode) {
  Context* context = static_cast<Context*>(ctx->user_data);

  if (context->options->debug) {
    const char* opcode_name = get_opcode_name(opcode);
    printf("on_opcode: %#" PRIzx ": %s\n", ctx->offset, opcode_name);
  }

  if (context->last_opcode_end) {
    if (ctx->offset != context->last_opcode_end + 1) {
      uint8_t missing_opcode = ctx->data[context->last_opcode_end];
      const char* opcode_name =
          get_opcode_name(static_cast<Opcode>(missing_opcode));
      fprintf(stderr, "warning: %#" PRIzx " missing opcode callback at %#" PRIzx
                      " (%#02x=%s)\n",
              ctx->offset, context->last_opcode_end + 1,
              ctx->data[context->last_opcode_end], opcode_name);
      return Result::Error;
    }
  }

  context->current_opcode_offset = ctx->offset;
  context->current_opcode = opcode;
  return Result::Ok;
}

#define IMMEDIATE_OCTET_COUNT 9

static void log_opcode(Context* ctx,
                       const uint8_t* data,
                       size_t data_size,
                       const char* fmt,
                       ...) {
  size_t offset = ctx->current_opcode_offset;

  // Print binary data
  printf(" %06" PRIzx ": %02x", offset - 1,
         static_cast<unsigned>(ctx->current_opcode));
  size_t i;
  for (i = 0; i < data_size && i < IMMEDIATE_OCTET_COUNT; i++, offset++) {
    printf(" %02x", data[offset]);
  }
  for (i = data_size + 1; i < IMMEDIATE_OCTET_COUNT; i++) {
    printf("   ");
  }
  printf(" | ");

  // Print disassemble
  int j;
  int indent_level = ctx->indent_level;
  if (ctx->current_opcode == Opcode::Else)
    indent_level--;
  for (j = 0; j < indent_level; j++) {
    printf("  ");
  }

  const char* opcode_name = get_opcode_name(ctx->current_opcode);
  printf("%s", opcode_name);
  if (fmt) {
    printf(" ");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
  }

  printf("\n");

  ctx->last_opcode_end = ctx->current_opcode_offset + data_size;

  if (ctx->options->relocs) {
    if (ctx->next_reloc < ctx->options->code_relocations.size) {
      Reloc* reloc = &ctx->options->code_relocations.data[ctx->next_reloc];
      size_t code_start =
          ctx->section_starts[static_cast<size_t>(BinarySection::Code)];
      size_t abs_offset = code_start + reloc->offset;
      if (ctx->last_opcode_end > abs_offset) {
        printf("           %06" PRIzx ": %s\n", abs_offset,
               get_reloc_type_name(reloc->type));
        ctx->next_reloc++;
      }
    }
  }
}

static Result on_opcode_bare(BinaryReaderContext* ctx) {
  Context* context = static_cast<Context*>(ctx->user_data);
  log_opcode(context, ctx->data, 0, nullptr);
  return Result::Ok;
}

static Result on_opcode_uint32(BinaryReaderContext* ctx, uint32_t value) {
  Context* context = static_cast<Context*>(ctx->user_data);
  size_t immediate_len = ctx->offset - context->current_opcode_offset;
  log_opcode(context, ctx->data, immediate_len, "%#x", value);
  return Result::Ok;
}

static Result on_opcode_uint32_uint32(BinaryReaderContext* ctx,
                                      uint32_t value,
                                      uint32_t value2) {
  Context* context = static_cast<Context*>(ctx->user_data);
  size_t immediate_len = ctx->offset - context->current_opcode_offset;
  log_opcode(context, ctx->data, immediate_len, "%lu %lu", value, value2);
  return Result::Ok;
}

static Result on_opcode_uint64(BinaryReaderContext* ctx, uint64_t value) {
  Context* context = static_cast<Context*>(ctx->user_data);
  size_t immediate_len = ctx->offset - context->current_opcode_offset;
  log_opcode(context, ctx->data, immediate_len, "%d", value);
  return Result::Ok;
}

static Result on_opcode_f32(BinaryReaderContext* ctx, uint32_t value) {
  Context* context = static_cast<Context*>(ctx->user_data);
  size_t immediate_len = ctx->offset - context->current_opcode_offset;
  char buffer[WABT_MAX_FLOAT_HEX];
  write_float_hex(buffer, sizeof(buffer), value);
  log_opcode(context, ctx->data, immediate_len, buffer);
  return Result::Ok;
}

static Result on_opcode_f64(BinaryReaderContext* ctx, uint64_t value) {
  Context* context = static_cast<Context*>(ctx->user_data);
  size_t immediate_len = ctx->offset - context->current_opcode_offset;
  char buffer[WABT_MAX_DOUBLE_HEX];
  write_double_hex(buffer, sizeof(buffer), value);
  log_opcode(context, ctx->data, immediate_len, buffer);
  return Result::Ok;
}

Result on_br_table_expr(BinaryReaderContext* ctx,
                        uint32_t num_targets,
                        uint32_t* target_depths,
                        uint32_t default_target_depth) {
  Context* context = static_cast<Context*>(ctx->user_data);
  size_t immediate_len = ctx->offset - context->current_opcode_offset;
  /* TODO(sbc): Print targets */
  log_opcode(context, ctx->data, immediate_len, nullptr);
  return Result::Ok;
}

static Result on_end_expr(void* user_data) {
  Context* context = static_cast<Context*>(user_data);
  context->indent_level--;
  assert(context->indent_level >= 0);
  log_opcode(context, nullptr, 0, nullptr);
  return Result::Ok;
}

static const char* type_name(Type type) {
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

static Result on_opcode_block_sig(BinaryReaderContext* ctx,
                                  uint32_t num_types,
                                  Type* sig_types) {
  Context* context = static_cast<Context*>(ctx->user_data);
  if (num_types)
    log_opcode(context, ctx->data, 1, "%s", type_name(*sig_types));
  else
    log_opcode(context, ctx->data, 1, nullptr);
  context->indent_level++;
  return Result::Ok;
}

static Result on_signature(uint32_t index,
                           uint32_t param_count,
                           Type* param_types,
                           uint32_t result_count,
                           Type* result_types,
                           void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);

  if (!should_print_details(ctx))
    return Result::Ok;
  printf(" - [%d] (", index);
  uint32_t i;
  for (i = 0; i < param_count; i++) {
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

static Result on_function_signature(uint32_t index,
                                    uint32_t sig_index,
                                    void* user_data) {
  print_details(static_cast<Context*>(user_data), " - func[%d] sig=%d\n", index,
                sig_index);
  return Result::Ok;
}

static Result begin_function_body(BinaryReaderContext* context,
                                  uint32_t index) {
  Context* ctx = static_cast<Context*>(context->user_data);

  if (ctx->options->mode == ObjdumpMode::Disassemble) {
    if (index < ctx->options->function_names.size &&
        !string_slice_is_empty(&ctx->options->function_names.data[index]))
      printf("%06" PRIzx " <" PRIstringslice ">:\n", context->offset,
             WABT_PRINTF_STRING_SLICE_ARG(
                 ctx->options->function_names.data[index]));
    else
      printf("%06" PRIzx " func[%d]:\n", context->offset, index);
  }

  ctx->last_opcode_end = 0;
  return Result::Ok;
}

static Result on_import(uint32_t index,
                        StringSlice module_name,
                        StringSlice field_name,
                        void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  ctx->import_module_name = module_name;
  ctx->import_field_name = field_name;
  return Result::Ok;
}

static Result on_import_func(uint32_t import_index,
                             uint32_t func_index,
                             uint32_t sig_index,
                             void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  print_details(ctx,
                " - func[%d] sig=%d <- " PRIstringslice "." PRIstringslice "\n",
                func_index, sig_index,
                WABT_PRINTF_STRING_SLICE_ARG(ctx->import_module_name),
                WABT_PRINTF_STRING_SLICE_ARG(ctx->import_field_name));
  return Result::Ok;
}

static Result on_import_table(uint32_t import_index,
                              uint32_t table_index,
                              Type elem_type,
                              const Limits* elem_limits,
                              void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  print_details(
      ctx, " - " PRIstringslice "." PRIstringslice
           " -> table elem_type=%s init=%" PRId64 " max=%" PRId64 "\n",
      WABT_PRINTF_STRING_SLICE_ARG(ctx->import_module_name),
      WABT_PRINTF_STRING_SLICE_ARG(ctx->import_field_name),
      get_type_name(elem_type), elem_limits->initial, elem_limits->max);
  return Result::Ok;
}

static Result on_import_memory(uint32_t import_index,
                               uint32_t memory_index,
                               const Limits* page_limits,
                               void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  print_details(ctx, " - " PRIstringslice "." PRIstringslice " -> memory\n",
                WABT_PRINTF_STRING_SLICE_ARG(ctx->import_module_name),
                WABT_PRINTF_STRING_SLICE_ARG(ctx->import_field_name));
  return Result::Ok;
}

static Result on_import_global(uint32_t import_index,
                               uint32_t global_index,
                               Type type,
                               bool mutable_,
                               void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  print_details(ctx, " - global[%d] %s mutable=%d <- " PRIstringslice
                     "." PRIstringslice "\n",
                global_index, get_type_name(type), mutable_,
                WABT_PRINTF_STRING_SLICE_ARG(ctx->import_module_name),
                WABT_PRINTF_STRING_SLICE_ARG(ctx->import_field_name));
  return Result::Ok;
}

static Result on_memory(uint32_t index,
                        const Limits* page_limits,
                        void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  print_details(ctx, " - memory[%d] pages: initial=%" PRId64, index,
                page_limits->initial);
  if (page_limits->has_max)
    print_details(ctx, " max=%" PRId64, page_limits->max);
  print_details(ctx, "\n");
  return Result::Ok;
}

static Result on_table(uint32_t index,
                       Type elem_type,
                       const Limits* elem_limits,
                       void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  print_details(ctx, " - table[%d] type=%s initial=%" PRId64, index,
                get_type_name(elem_type), elem_limits->initial);
  if (elem_limits->has_max)
    print_details(ctx, " max=%" PRId64, elem_limits->max);
  print_details(ctx, "\n");
  return Result::Ok;
}

static Result on_export(uint32_t index,
                        ExternalKind kind,
                        uint32_t item_index,
                        StringSlice name,
                        void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  print_details(ctx, " - %s[%d] ", get_kind_name(kind), item_index);
  print_details(ctx, PRIstringslice, WABT_PRINTF_STRING_SLICE_ARG(name));
  print_details(ctx, "\n");
  return Result::Ok;
}

static Result on_elem_segment_function_index(uint32_t index,
                                             uint32_t func_index,
                                             void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  print_details(ctx, "  - func[%d]\n", func_index);
  return Result::Ok;
}

static Result begin_elem_segment(uint32_t index,
                                 uint32_t table_index,
                                 void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  print_details(ctx, " - segment[%d] table=%d\n", index, table_index);
  return Result::Ok;
}

static Result begin_global(uint32_t index,
                           Type type,
                           bool mutable_,
                           void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  print_details(ctx, " - global[%d] %s mutable=%d", index, get_type_name(type),
                mutable_);
  return Result::Ok;
}

static Result on_init_expr_f32_const_expr(uint32_t index,
                                          uint32_t value,
                                          void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  char buffer[WABT_MAX_FLOAT_HEX];
  write_float_hex(buffer, sizeof(buffer), value);
  print_details(ctx, " - init f32=%s\n", buffer);
  return Result::Ok;
}

static Result on_init_expr_f64_const_expr(uint32_t index,
                                          uint64_t value,
                                          void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  char buffer[WABT_MAX_DOUBLE_HEX];
  write_float_hex(buffer, sizeof(buffer), value);
  print_details(ctx, " - init f64=%s\n", buffer);
  return Result::Ok;
}

static Result on_init_expr_get_global_expr(uint32_t index,
                                           uint32_t global_index,
                                           void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  print_details(ctx, " - init global=%d\n", global_index);
  return Result::Ok;
}

static Result on_init_expr_i32_const_expr(uint32_t index,
                                          uint32_t value,
                                          void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  print_details(ctx, " - init i32=%d\n", value);
  return Result::Ok;
}

static Result on_init_expr_i64_const_expr(uint32_t index,
                                          uint64_t value,
                                          void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  print_details(ctx, " - init i64=%" PRId64 "\n", value);
  return Result::Ok;
}

static Result on_function_name(uint32_t index,
                               StringSlice name,
                               void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  print_details(ctx, " - func[%d] " PRIstringslice "\n", index,
                WABT_PRINTF_STRING_SLICE_ARG(name));
  if (ctx->options->mode == ObjdumpMode::Prepass) {
    while (ctx->options->function_names.size < index) {
      StringSlice empty = empty_string_slice();
      append_string_slice_value(&ctx->options->function_names, &empty);
    }
    if (ctx->options->function_names.size == index) {
      append_string_slice_value(&ctx->options->function_names, &name);
    }
  }
  return Result::Ok;
}

static Result on_local_name(uint32_t func_index,
                            uint32_t local_index,
                            StringSlice name,
                            void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  if (name.length) {
    print_details(ctx, "  - local[%d] " PRIstringslice "\n", local_index,
                  WABT_PRINTF_STRING_SLICE_ARG(name));
  }
  return Result::Ok;
}

Result on_reloc_count(uint32_t count,
                      BinarySection section_code,
                      StringSlice section_name,
                      void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  ctx->reloc_section = section_code;
  print_details(ctx, "  - section: %s\n", get_section_name(section_code));
  return Result::Ok;
}

Result on_reloc(RelocType type, uint32_t offset, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  uint32_t total_offset =
      ctx->section_starts[static_cast<size_t>(ctx->reloc_section)] + offset;
  print_details(ctx, "   - %-18s offset=%#x (%#x)\n", get_reloc_type_name(type),
                total_offset, offset);
  if (ctx->options->mode == ObjdumpMode::Prepass &&
      ctx->reloc_section == BinarySection::Code) {
    Reloc reloc;
    reloc.offset = offset;
    reloc.type = type;
    append_reloc_value(&ctx->options->code_relocations, &reloc);
  }
  return Result::Ok;
}

static void on_error(BinaryReaderContext* ctx, const char* message) {
  DefaultErrorHandlerInfo info;
  info.header = "error reading binary";
  info.out_file = stdout;
  info.print_header = PrintErrorHeader::Once;
  default_binary_error_callback(ctx->offset, message, &info);
}

static Result begin_data_segment(uint32_t index,
                                 uint32_t memory_index,
                                 void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  print_details(ctx, " - memory[%d]", memory_index);
  return Result::Ok;
}

static Result on_data_segment_data(uint32_t index,
                                   const void* src_data,
                                   uint32_t size,
                                   void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  if (should_print_details(ctx)) {
    write_memory_dump(ctx->out_stream, src_data, size, 0, PrintChars::Yes,
                      "  - ", nullptr);
  }
  return Result::Ok;
}

Result read_binary_objdump(const uint8_t* data,
                           size_t size,
                           ObjdumpOptions* options) {
  Context context;
  WABT_ZERO_MEMORY(context);
  context.header_printed = false;
  context.print_details = false;
  context.section_found = false;
  context.data = data;
  context.size = size;
  context.options = options;
  context.out_stream = init_stdout_stream();

  BinaryReader reader;
  WABT_ZERO_MEMORY(reader);
  if (options->mode == ObjdumpMode::Prepass) {
    reader.on_function_name = on_function_name;
    reader.on_reloc_count = on_reloc_count;
    reader.on_reloc = on_reloc;
  } else {
    reader.begin_module = begin_module;
    reader.end_module = end_module;
    reader.on_error = on_error;

    reader.begin_section = begin_section;

    // User section
    reader.begin_custom_section = begin_custom_section;

    // Signature section
    reader.on_signature_count = on_count;
    reader.on_signature = on_signature;

    // Import section
    reader.on_import_count = on_count;
    reader.on_import = on_import;
    reader.on_import_func = on_import_func;
    reader.on_import_table = on_import_table;
    reader.on_import_memory = on_import_memory;
    reader.on_import_global = on_import_global;

    // Function sigs section
    reader.on_function_signatures_count = on_count;
    reader.on_function_signature = on_function_signature;

    // Table section
    reader.on_table_count = on_count;
    reader.on_table = on_table;

    // Memory section
    reader.on_memory_count = on_count;
    reader.on_memory = on_memory;

    // Globl seciont
    reader.begin_global = begin_global;
    reader.on_global_count = on_count;

    // Export section
    reader.on_export_count = on_count;
    reader.on_export = on_export;

    // Body section
    reader.on_function_bodies_count = on_count;
    reader.begin_function_body = begin_function_body;

    // Elems section
    reader.begin_elem_segment = begin_elem_segment;
    reader.on_elem_segment_count = on_count;
    reader.on_elem_segment_function_index = on_elem_segment_function_index;

    // Data section
    reader.begin_data_segment = begin_data_segment;
    reader.on_data_segment_data = on_data_segment_data;
    reader.on_data_segment_count = on_count;

    // Known "User" sections:
    // - Names section
    reader.on_function_name = on_function_name;
    reader.on_local_name = on_local_name;

    reader.on_reloc_count = on_reloc_count;
    reader.on_reloc = on_reloc;

    reader.on_init_expr_i32_const_expr = on_init_expr_i32_const_expr;
    reader.on_init_expr_i64_const_expr = on_init_expr_i64_const_expr;
    reader.on_init_expr_f32_const_expr = on_init_expr_f32_const_expr;
    reader.on_init_expr_f64_const_expr = on_init_expr_f64_const_expr;
    reader.on_init_expr_get_global_expr = on_init_expr_get_global_expr;
  }

  if (options->mode == ObjdumpMode::Disassemble) {
    reader.on_opcode = on_opcode;
    reader.on_opcode_bare = on_opcode_bare;
    reader.on_opcode_uint32 = on_opcode_uint32;
    reader.on_opcode_uint32_uint32 = on_opcode_uint32_uint32;
    reader.on_opcode_uint64 = on_opcode_uint64;
    reader.on_opcode_f32 = on_opcode_f32;
    reader.on_opcode_f64 = on_opcode_f64;
    reader.on_opcode_block_sig = on_opcode_block_sig;
    reader.on_end_expr = on_end_expr;
    reader.on_br_table_expr = on_br_table_expr;
  }

  reader.user_data = &context;

  ReadBinaryOptions read_options = WABT_READ_BINARY_OPTIONS_DEFAULT;
  read_options.read_debug_names = true;
  return read_binary(data, size, &reader, 1, &read_options);
}

}  // namespace wabt
