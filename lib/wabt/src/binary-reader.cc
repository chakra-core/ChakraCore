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

#include "binary-reader.h"

#include <assert.h>
#include <inttypes.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <vector>

#include "binary.h"
#include "config.h"
#include "stream.h"

#if HAVE_ALLOCA
#include <alloca.h>
#endif

#define INDENT_SIZE 2

#define INITIAL_PARAM_TYPES_CAPACITY 128
#define INITIAL_BR_TABLE_TARGET_CAPACITY 1000

namespace wabt {

namespace {

#define CALLBACK_CTX(member, ...)                                       \
  RAISE_ERROR_UNLESS(                                                   \
      WABT_SUCCEEDED(                                                   \
          ctx->reader->member                                           \
              ? ctx->reader->member(get_user_context(ctx), __VA_ARGS__) \
              : Result::Ok),                                            \
      #member " callback failed")

#define CALLBACK_CTX0(member)                                         \
  RAISE_ERROR_UNLESS(                                                 \
      WABT_SUCCEEDED(ctx->reader->member                              \
                         ? ctx->reader->member(get_user_context(ctx)) \
                         : Result::Ok),                               \
      #member " callback failed")

#define CALLBACK_SECTION(member, section_size) \
  CALLBACK_CTX(member, section_size)

#define CALLBACK0(member)                                              \
  RAISE_ERROR_UNLESS(                                                  \
      WABT_SUCCEEDED(ctx->reader->member                               \
                         ? ctx->reader->member(ctx->reader->user_data) \
                         : Result::Ok),                                \
      #member " callback failed")

#define CALLBACK(member, ...)                                            \
  RAISE_ERROR_UNLESS(                                                    \
      WABT_SUCCEEDED(                                                    \
          ctx->reader->member                                            \
              ? ctx->reader->member(__VA_ARGS__, ctx->reader->user_data) \
              : Result::Ok),                                             \
      #member " callback failed")

#define FORWARD0(member)                                                   \
  return ctx->reader->member ? ctx->reader->member(ctx->reader->user_data) \
                             : Result::Ok

#define FORWARD_CTX0(member)                  \
  if (!ctx->reader->member)                   \
    return Result::Ok;                        \
  BinaryReaderContext new_ctx = *context;     \
  new_ctx.user_data = ctx->reader->user_data; \
  return ctx->reader->member(&new_ctx);

#define FORWARD_CTX(member, ...)              \
  if (!ctx->reader->member)                   \
    return Result::Ok;                        \
  BinaryReaderContext new_ctx = *context;     \
  new_ctx.user_data = ctx->reader->user_data; \
  return ctx->reader->member(&new_ctx, __VA_ARGS__);

#define FORWARD(member, ...)                                            \
  return ctx->reader->member                                            \
             ? ctx->reader->member(__VA_ARGS__, ctx->reader->user_data) \
             : Result::Ok

#define RAISE_ERROR(...) raise_error(ctx, __VA_ARGS__)

#define RAISE_ERROR_UNLESS(cond, ...) \
  if (!(cond))                        \
    RAISE_ERROR(__VA_ARGS__);

struct Context {
  const uint8_t* data = nullptr;
  size_t data_size = 0;
  size_t offset = 0;
  size_t read_end = 0; /* Either the section end or data_size. */
  BinaryReaderContext user_ctx;
  BinaryReader* reader = nullptr;
  jmp_buf error_jmp_buf;
  TypeVector param_types;
  std::vector<uint32_t> target_depths;
  const ReadBinaryOptions* options = nullptr;
  BinarySection last_known_section = BinarySection::Invalid;
  uint32_t num_signatures = 0;
  uint32_t num_imports = 0;
  uint32_t num_func_imports = 0;
  uint32_t num_table_imports = 0;
  uint32_t num_memory_imports = 0;
  uint32_t num_global_imports = 0;
  uint32_t num_function_signatures = 0;
  uint32_t num_tables = 0;
  uint32_t num_memories = 0;
  uint32_t num_globals = 0;
  uint32_t num_exports = 0;
  uint32_t num_function_bodies = 0;
};

struct LoggingContext {
  Stream* stream;
  BinaryReader* reader;
  int indent;
};

}  // namespace

static BinaryReaderContext* get_user_context(Context* ctx) {
  ctx->user_ctx.user_data = ctx->reader->user_data;
  ctx->user_ctx.data = ctx->data;
  ctx->user_ctx.size = ctx->data_size;
  ctx->user_ctx.offset = ctx->offset;
  return &ctx->user_ctx;
}

static void WABT_PRINTF_FORMAT(2, 3)
    raise_error(Context* ctx, const char* format, ...) {
  WABT_SNPRINTF_ALLOCA(buffer, length, format);
  bool handled = false;
  if (ctx->reader->on_error) {
    handled = ctx->reader->on_error(get_user_context(ctx), buffer);
  }

  if (!handled) {
    /* Not great to just print, but we don't want to eat the error either. */
    fprintf(stderr, "*ERROR*: @0x%08zx: %s\n", ctx->offset, buffer);
  }
  longjmp(ctx->error_jmp_buf, 1);
}

#define IN_SIZE(type)                                       \
  if (ctx->offset + sizeof(type) > ctx->read_end) {         \
    RAISE_ERROR("unable to read " #type ": %s", desc);      \
  }                                                         \
  memcpy(out_value, ctx->data + ctx->offset, sizeof(type)); \
  ctx->offset += sizeof(type)

static void in_u8(Context* ctx, uint8_t* out_value, const char* desc) {
  IN_SIZE(uint8_t);
}

static void in_u32(Context* ctx, uint32_t* out_value, const char* desc) {
  IN_SIZE(uint32_t);
}

static void in_f32(Context* ctx, uint32_t* out_value, const char* desc) {
  IN_SIZE(float);
}

static void in_f64(Context* ctx, uint64_t* out_value, const char* desc) {
  IN_SIZE(double);
}

#undef IN_SIZE

#define BYTE_AT(type, i, shift) ((static_cast<type>(p[i]) & 0x7f) << (shift))

#define LEB128_1(type) (BYTE_AT(type, 0, 0))
#define LEB128_2(type) (BYTE_AT(type, 1, 7) | LEB128_1(type))
#define LEB128_3(type) (BYTE_AT(type, 2, 14) | LEB128_2(type))
#define LEB128_4(type) (BYTE_AT(type, 3, 21) | LEB128_3(type))
#define LEB128_5(type) (BYTE_AT(type, 4, 28) | LEB128_4(type))
#define LEB128_6(type) (BYTE_AT(type, 5, 35) | LEB128_5(type))
#define LEB128_7(type) (BYTE_AT(type, 6, 42) | LEB128_6(type))
#define LEB128_8(type) (BYTE_AT(type, 7, 49) | LEB128_7(type))
#define LEB128_9(type) (BYTE_AT(type, 8, 56) | LEB128_8(type))
#define LEB128_10(type) (BYTE_AT(type, 9, 63) | LEB128_9(type))

#define SHIFT_AMOUNT(type, sign_bit) (sizeof(type) * 8 - 1 - (sign_bit))
#define SIGN_EXTEND(type, value, sign_bit)                       \
  (static_cast<type>((value) << SHIFT_AMOUNT(type, sign_bit)) >> \
   SHIFT_AMOUNT(type, sign_bit))

size_t read_u32_leb128(const uint8_t* p,
                       const uint8_t* end,
                       uint32_t* out_value) {
  if (p < end && (p[0] & 0x80) == 0) {
    *out_value = LEB128_1(uint32_t);
    return 1;
  } else if (p + 1 < end && (p[1] & 0x80) == 0) {
    *out_value = LEB128_2(uint32_t);
    return 2;
  } else if (p + 2 < end && (p[2] & 0x80) == 0) {
    *out_value = LEB128_3(uint32_t);
    return 3;
  } else if (p + 3 < end && (p[3] & 0x80) == 0) {
    *out_value = LEB128_4(uint32_t);
    return 4;
  } else if (p + 4 < end && (p[4] & 0x80) == 0) {
    /* the top bits set represent values > 32 bits */
    if (p[4] & 0xf0)
      return 0;
    *out_value = LEB128_5(uint32_t);
    return 5;
  } else {
    /* past the end */
    *out_value = 0;
    return 0;
  }
}

static void in_u32_leb128(Context* ctx, uint32_t* out_value, const char* desc) {
  const uint8_t* p = ctx->data + ctx->offset;
  const uint8_t* end = ctx->data + ctx->read_end;
  size_t bytes_read = read_u32_leb128(p, end, out_value);
  if (!bytes_read)
    RAISE_ERROR("unable to read u32 leb128: %s", desc);
  ctx->offset += bytes_read;
}

size_t read_i32_leb128(const uint8_t* p,
                       const uint8_t* end,
                       uint32_t* out_value) {
  if (p < end && (p[0] & 0x80) == 0) {
    uint32_t result = LEB128_1(uint32_t);
    *out_value = SIGN_EXTEND(int32_t, result, 6);
    return 1;
  } else if (p + 1 < end && (p[1] & 0x80) == 0) {
    uint32_t result = LEB128_2(uint32_t);
    *out_value = SIGN_EXTEND(int32_t, result, 13);
    return 2;
  } else if (p + 2 < end && (p[2] & 0x80) == 0) {
    uint32_t result = LEB128_3(uint32_t);
    *out_value = SIGN_EXTEND(int32_t, result, 20);
    return 3;
  } else if (p + 3 < end && (p[3] & 0x80) == 0) {
    uint32_t result = LEB128_4(uint32_t);
    *out_value = SIGN_EXTEND(int32_t, result, 27);
    return 4;
  } else if (p + 4 < end && (p[4] & 0x80) == 0) {
    /* the top bits should be a sign-extension of the sign bit */
    bool sign_bit_set = (p[4] & 0x8);
    int top_bits = p[4] & 0xf0;
    if ((sign_bit_set && top_bits != 0x70) ||
        (!sign_bit_set && top_bits != 0)) {
      return 0;
    }
    uint32_t result = LEB128_5(uint32_t);
    *out_value = result;
    return 5;
  } else {
    /* past the end */
    return 0;
  }
}

static void in_i32_leb128(Context* ctx, uint32_t* out_value, const char* desc) {
  const uint8_t* p = ctx->data + ctx->offset;
  const uint8_t* end = ctx->data + ctx->read_end;
  size_t bytes_read = read_i32_leb128(p, end, out_value);
  if (!bytes_read)
    RAISE_ERROR("unable to read i32 leb128: %s", desc);
  ctx->offset += bytes_read;
}

static void in_i64_leb128(Context* ctx, uint64_t* out_value, const char* desc) {
  const uint8_t* p = ctx->data + ctx->offset;
  const uint8_t* end = ctx->data + ctx->read_end;

  if (p < end && (p[0] & 0x80) == 0) {
    uint64_t result = LEB128_1(uint64_t);
    *out_value = SIGN_EXTEND(int64_t, result, 6);
    ctx->offset += 1;
  } else if (p + 1 < end && (p[1] & 0x80) == 0) {
    uint64_t result = LEB128_2(uint64_t);
    *out_value = SIGN_EXTEND(int64_t, result, 13);
    ctx->offset += 2;
  } else if (p + 2 < end && (p[2] & 0x80) == 0) {
    uint64_t result = LEB128_3(uint64_t);
    *out_value = SIGN_EXTEND(int64_t, result, 20);
    ctx->offset += 3;
  } else if (p + 3 < end && (p[3] & 0x80) == 0) {
    uint64_t result = LEB128_4(uint64_t);
    *out_value = SIGN_EXTEND(int64_t, result, 27);
    ctx->offset += 4;
  } else if (p + 4 < end && (p[4] & 0x80) == 0) {
    uint64_t result = LEB128_5(uint64_t);
    *out_value = SIGN_EXTEND(int64_t, result, 34);
    ctx->offset += 5;
  } else if (p + 5 < end && (p[5] & 0x80) == 0) {
    uint64_t result = LEB128_6(uint64_t);
    *out_value = SIGN_EXTEND(int64_t, result, 41);
    ctx->offset += 6;
  } else if (p + 6 < end && (p[6] & 0x80) == 0) {
    uint64_t result = LEB128_7(uint64_t);
    *out_value = SIGN_EXTEND(int64_t, result, 48);
    ctx->offset += 7;
  } else if (p + 7 < end && (p[7] & 0x80) == 0) {
    uint64_t result = LEB128_8(uint64_t);
    *out_value = SIGN_EXTEND(int64_t, result, 55);
    ctx->offset += 8;
  } else if (p + 8 < end && (p[8] & 0x80) == 0) {
    uint64_t result = LEB128_9(uint64_t);
    *out_value = SIGN_EXTEND(int64_t, result, 62);
    ctx->offset += 9;
  } else if (p + 9 < end && (p[9] & 0x80) == 0) {
    /* the top bits should be a sign-extension of the sign bit */
    bool sign_bit_set = (p[9] & 0x1);
    int top_bits = p[9] & 0xfe;
    if ((sign_bit_set && top_bits != 0x7e) ||
        (!sign_bit_set && top_bits != 0)) {
      RAISE_ERROR("invalid i64 leb128: %s", desc);
    }
    uint64_t result = LEB128_10(uint64_t);
    *out_value = result;
    ctx->offset += 10;
  } else {
    /* past the end */
    RAISE_ERROR("unable to read i64 leb128: %s", desc);
  }
}

#undef BYTE_AT
#undef LEB128_1
#undef LEB128_2
#undef LEB128_3
#undef LEB128_4
#undef LEB128_5
#undef LEB128_6
#undef LEB128_7
#undef LEB128_8
#undef LEB128_9
#undef LEB128_10
#undef SHIFT_AMOUNT
#undef SIGN_EXTEND

static void in_type(Context* ctx, Type* out_value, const char* desc) {
  uint32_t type = 0;
  in_i32_leb128(ctx, &type, desc);
  /* Must be in the vs7 range: [-128, 127). */
  if (static_cast<int32_t>(type) < -128 || static_cast<int32_t>(type) > 127)
    RAISE_ERROR("invalid type: %d", type);
  *out_value = static_cast<Type>(type);
}

static void in_str(Context* ctx, StringSlice* out_str, const char* desc) {
  uint32_t str_len = 0;
  in_u32_leb128(ctx, &str_len, "string length");

  if (ctx->offset + str_len > ctx->read_end)
    RAISE_ERROR("unable to read string: %s", desc);

  out_str->start = reinterpret_cast<const char*>(ctx->data) + ctx->offset;
  out_str->length = str_len;
  ctx->offset += str_len;
}

static void in_bytes(Context* ctx,
                     const void** out_data,
                     uint32_t* out_data_size,
                     const char* desc) {
  uint32_t data_size = 0;
  in_u32_leb128(ctx, &data_size, "data size");

  if (ctx->offset + data_size > ctx->read_end)
    RAISE_ERROR("unable to read data: %s", desc);

  *out_data = static_cast<const uint8_t*>(ctx->data) + ctx->offset;
  *out_data_size = data_size;
  ctx->offset += data_size;
}

static bool is_valid_external_kind(uint8_t kind) {
  return kind < kExternalKindCount;
}

static bool is_concrete_type(Type type) {
  switch (type) {
    case Type::I32:
    case Type::I64:
    case Type::F32:
    case Type::F64:
      return true;

    default:
      return false;
  }
}

static bool is_inline_sig_type(Type type) {
  return is_concrete_type(type) || type == Type::Void;
}

static uint32_t num_total_funcs(Context* ctx) {
  return ctx->num_func_imports + ctx->num_function_signatures;
}

static uint32_t num_total_tables(Context* ctx) {
  return ctx->num_table_imports + ctx->num_tables;
}

static uint32_t num_total_memories(Context* ctx) {
  return ctx->num_memory_imports + ctx->num_memories;
}

static uint32_t num_total_globals(Context* ctx) {
  return ctx->num_global_imports + ctx->num_globals;
}

/* Logging */

static void indent(LoggingContext* ctx) {
  ctx->indent += INDENT_SIZE;
}

static void dedent(LoggingContext* ctx) {
  ctx->indent -= INDENT_SIZE;
  assert(ctx->indent >= 0);
}

static void write_indent(LoggingContext* ctx) {
  static char s_indent[] =
      "                                                                       "
      "                                                                       ";
  static size_t s_indent_len = sizeof(s_indent) - 1;
  size_t indent = ctx->indent;
  while (indent > s_indent_len) {
    write_data(ctx->stream, s_indent, s_indent_len, nullptr);
    indent -= s_indent_len;
  }
  if (indent > 0) {
    write_data(ctx->stream, s_indent, indent, nullptr);
  }
}

#define LOGF_NOINDENT(...) writef(ctx->stream, __VA_ARGS__)

#define LOGF(...)               \
  do {                          \
    write_indent(ctx);          \
    LOGF_NOINDENT(__VA_ARGS__); \
  } while (0)

static bool logging_on_error(BinaryReaderContext* context,
                             const char* message) {
  LoggingContext* ctx = static_cast<LoggingContext*>(context->user_data);
  // Can't use FORWARD_CTX because it returns Result by default.
  if (!ctx->reader->on_error)
    return false;
  BinaryReaderContext new_ctx = *context;
  new_ctx.user_data = ctx->reader->user_data;
  return ctx->reader->on_error(&new_ctx, message);
}

static Result logging_begin_section(BinaryReaderContext* context,
                                    BinarySection section_type,
                                    uint32_t size) {
  LoggingContext* ctx = static_cast<LoggingContext*>(context->user_data);
  FORWARD_CTX(begin_section, section_type, size);
}

static Result logging_begin_custom_section(BinaryReaderContext* context,
                                           uint32_t size,
                                           StringSlice section_name) {
  LoggingContext* ctx = static_cast<LoggingContext*>(context->user_data);
  LOGF("begin_custom_section: '" PRIstringslice "' size=%d\n",
       WABT_PRINTF_STRING_SLICE_ARG(section_name), size);
  indent(ctx);
  FORWARD_CTX(begin_custom_section, size, section_name);
}

#define LOGGING_BEGIN(name)                                                 \
  static Result logging_begin_##name(BinaryReaderContext* context,          \
                                     uint32_t size) {                       \
    LoggingContext* ctx = static_cast<LoggingContext*>(context->user_data); \
    LOGF("begin_" #name "(%u)\n", size);                                              \
    indent(ctx);                                                            \
    FORWARD_CTX(begin_##name, size);                                        \
  }

#define LOGGING_END(name)                                                   \
  static Result logging_end_##name(BinaryReaderContext* context) {          \
    LoggingContext* ctx = static_cast<LoggingContext*>(context->user_data); \
    dedent(ctx);                                                            \
    LOGF("end_" #name "\n");                                                \
    FORWARD_CTX0(end_##name);                                               \
  }

#define LOGGING_UINT32(name)                                       \
  static Result logging_##name(uint32_t value, void* user_data) {  \
    LoggingContext* ctx = static_cast<LoggingContext*>(user_data); \
    LOGF(#name "(%u)\n", value);                                   \
    FORWARD(name, value);                                          \
  }

#define LOGGING_UINT32_CTX(name)                                               \
  static Result logging_##name(BinaryReaderContext* context, uint32_t value) { \
    LoggingContext* ctx = static_cast<LoggingContext*>(context->user_data);    \
    LOGF(#name "(%u)\n", value);                                               \
    FORWARD_CTX(name, value);                                                  \
  }

#define LOGGING_UINT32_DESC(name, desc)                            \
  static Result logging_##name(uint32_t value, void* user_data) {  \
    LoggingContext* ctx = static_cast<LoggingContext*>(user_data); \
    LOGF(#name "(" desc ": %u)\n", value);                         \
    FORWARD(name, value);                                          \
  }

#define LOGGING_UINT32_UINT32(name, desc0, desc1)                   \
  static Result logging_##name(uint32_t value0, uint32_t value1,    \
                               void* user_data) {                   \
    LoggingContext* ctx = static_cast<LoggingContext*>(user_data);  \
    LOGF(#name "(" desc0 ": %u, " desc1 ": %u)\n", value0, value1); \
    FORWARD(name, value0, value1);                                  \
  }

#define LOGGING_UINT32_UINT32_CTX(name, desc0, desc1)                         \
  static Result logging_##name(BinaryReaderContext* context, uint32_t value0, \
                               uint32_t value1) {                             \
    LoggingContext* ctx = static_cast<LoggingContext*>(context->user_data);   \
    LOGF(#name "(" desc0 ": %u, " desc1 ": %u)\n", value0, value1);           \
    FORWARD_CTX(name, value0, value1);                                        \
  }

#define LOGGING_OPCODE(name)                                       \
  static Result logging_##name(Opcode opcode, void* user_data) {   \
    LoggingContext* ctx = static_cast<LoggingContext*>(user_data); \
    LOGF(#name "(\"%s\" (%u))\n", get_opcode_name(opcode),         \
         static_cast<unsigned>(opcode));                           \
    FORWARD(name, opcode);                                         \
  }

#define LOGGING0(name)                                             \
  static Result logging_##name(void* user_data) {                  \
    LoggingContext* ctx = static_cast<LoggingContext*>(user_data); \
    LOGF(#name "\n");                                              \
    FORWARD0(name);                                                \
  }

LOGGING_UINT32(begin_module)
LOGGING0(end_module)
LOGGING_END(custom_section)
LOGGING_BEGIN(signature_section)
LOGGING_UINT32(on_signature_count)
LOGGING_END(signature_section)
LOGGING_BEGIN(import_section)
LOGGING_UINT32(on_import_count)
LOGGING_END(import_section)
LOGGING_BEGIN(function_signatures_section)
LOGGING_UINT32(on_function_signatures_count)
LOGGING_UINT32_UINT32(on_function_signature, "index", "sig_index")
LOGGING_END(function_signatures_section)
LOGGING_BEGIN(table_section)
LOGGING_UINT32(on_table_count)
LOGGING_END(table_section)
LOGGING_BEGIN(memory_section)
LOGGING_UINT32(on_memory_count)
LOGGING_END(memory_section)
LOGGING_BEGIN(global_section)
LOGGING_UINT32(on_global_count)
LOGGING_UINT32(begin_global_init_expr)
LOGGING_UINT32(end_global_init_expr)
LOGGING_UINT32(end_global)
LOGGING_END(global_section)
LOGGING_BEGIN(export_section)
LOGGING_UINT32(on_export_count)
LOGGING_END(export_section)
LOGGING_BEGIN(start_section)
LOGGING_UINT32(on_start_function)
LOGGING_END(start_section)
LOGGING_BEGIN(function_bodies_section)
LOGGING_UINT32(on_function_bodies_count)
LOGGING_UINT32_CTX(begin_function_body)
LOGGING_UINT32(end_function_body)
LOGGING_UINT32(on_local_decl_count)
LOGGING_OPCODE(on_binary_expr)
LOGGING_UINT32_DESC(on_call_expr, "func_index")
LOGGING_UINT32_DESC(on_call_import_expr, "import_index")
LOGGING_UINT32_DESC(on_call_indirect_expr, "sig_index")
LOGGING_OPCODE(on_compare_expr)
LOGGING_OPCODE(on_convert_expr)
LOGGING0(on_current_memory_expr)
LOGGING0(on_drop_expr)
LOGGING0(on_else_expr)
LOGGING0(on_end_expr)
LOGGING_UINT32_DESC(on_get_global_expr, "index")
LOGGING_UINT32_DESC(on_get_local_expr, "index")
LOGGING0(on_grow_memory_expr)
LOGGING0(on_nop_expr)
LOGGING0(on_return_expr)
LOGGING0(on_select_expr)
LOGGING_UINT32_DESC(on_set_global_expr, "index")
LOGGING_UINT32_DESC(on_set_local_expr, "index")
LOGGING_UINT32_DESC(on_tee_local_expr, "index")
LOGGING0(on_unreachable_expr)
LOGGING_OPCODE(on_unary_expr)
LOGGING_END(function_bodies_section)
LOGGING_BEGIN(elem_section)
LOGGING_UINT32(on_elem_segment_count)
LOGGING_UINT32_UINT32(begin_elem_segment, "index", "table_index")
LOGGING_UINT32(begin_elem_segment_init_expr)
LOGGING_UINT32(end_elem_segment_init_expr)
LOGGING_UINT32_UINT32_CTX(on_elem_segment_function_index_count,
                          "index",
                          "count")
LOGGING_UINT32_UINT32(on_elem_segment_function_index, "index", "func_index")
LOGGING_UINT32(end_elem_segment)
LOGGING_END(elem_section)
LOGGING_BEGIN(data_section)
LOGGING_UINT32(on_data_segment_count)
LOGGING_UINT32_UINT32(begin_data_segment, "index", "memory_index")
LOGGING_UINT32(begin_data_segment_init_expr)
LOGGING_UINT32(end_data_segment_init_expr)
LOGGING_UINT32(end_data_segment)
LOGGING_END(data_section)
LOGGING_BEGIN(names_section)
LOGGING_UINT32(on_function_names_count)
LOGGING_UINT32(on_local_name_function_count)
LOGGING_UINT32_UINT32(on_local_name_local_count, "index", "count")
LOGGING_END(names_section)
LOGGING_BEGIN(reloc_section)
LOGGING_END(reloc_section)
LOGGING_UINT32_UINT32(on_init_expr_get_global_expr, "index", "global_index")

static void sprint_limits(char* dst, size_t size, const Limits* limits) {
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

static void log_types(LoggingContext* ctx, uint32_t type_count, Type* types) {
  LOGF_NOINDENT("[");
  for (uint32_t i = 0; i < type_count; ++i) {
    LOGF_NOINDENT("%s", get_type_name(types[i]));
    if (i != type_count - 1)
      LOGF_NOINDENT(", ");
  }
  LOGF_NOINDENT("]");
}

static Result logging_on_signature(uint32_t index,
                                   uint32_t param_count,
                                   Type* param_types,
                                   uint32_t result_count,
                                   Type* result_types,
                                   void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF("on_signature(index: %u, params: ", index);
  log_types(ctx, param_count, param_types);
  LOGF_NOINDENT(", results: ");
  log_types(ctx, result_count, result_types);
  LOGF_NOINDENT(")\n");
  FORWARD(on_signature, index, param_count, param_types, result_count,
          result_types);
}

static Result logging_on_import(uint32_t index,
                                StringSlice module_name,
                                StringSlice field_name,
                                void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF("on_import(index: %u, module: \"" PRIstringslice
       "\", field: \"" PRIstringslice "\")\n",
       index, WABT_PRINTF_STRING_SLICE_ARG(module_name),
       WABT_PRINTF_STRING_SLICE_ARG(field_name));
  FORWARD(on_import, index, module_name, field_name);
}

static Result logging_on_import_func(uint32_t import_index,
                                     StringSlice module_name,
                                     StringSlice field_name,
                                     uint32_t func_index,
                                     uint32_t sig_index,
                                     void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF("on_import_func(import_index: %u, func_index: %u, sig_index: %u)\n",
       import_index, func_index, sig_index);
  FORWARD(on_import_func, import_index, module_name, field_name,
          func_index, sig_index);
}

static Result logging_on_import_table(uint32_t import_index,
                                      StringSlice module_name,
                                      StringSlice field_name,
                                      uint32_t table_index,
                                      Type elem_type,
                                      const Limits* elem_limits,
                                      void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  char buf[100];
  sprint_limits(buf, sizeof(buf), elem_limits);
  LOGF(
      "on_import_table(import_index: %u, table_index: %u, elem_type: %s, %s)\n",
      import_index, table_index, get_type_name(elem_type), buf);
  FORWARD(on_import_table, import_index, module_name, field_name,
          table_index, elem_type, elem_limits);
}

static Result logging_on_import_memory(uint32_t import_index,
                                       StringSlice module_name,
                                       StringSlice field_name,
                                       uint32_t memory_index,
                                       const Limits* page_limits,
                                       void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  char buf[100];
  sprint_limits(buf, sizeof(buf), page_limits);
  LOGF("on_import_memory(import_index: %u, memory_index: %u, %s)\n",
       import_index, memory_index, buf);
  FORWARD(on_import_memory, import_index, module_name, field_name,
          memory_index, page_limits);
}

static Result logging_on_import_global(uint32_t import_index,
                                       StringSlice module_name,
                                       StringSlice field_name,
                                       uint32_t global_index,
                                       Type type,
                                       bool mutable_,
                                       void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF(
      "on_import_global(import_index: %u, global_index: %u, type: %s, mutable: "
      "%s)\n",
      import_index, global_index, get_type_name(type),
      mutable_ ? "true" : "false");
  FORWARD(on_import_global, import_index, module_name, field_name,
          global_index, type, mutable_);
}

static Result logging_on_table(uint32_t index,
                               Type elem_type,
                               const Limits* elem_limits,
                               void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  char buf[100];
  sprint_limits(buf, sizeof(buf), elem_limits);
  LOGF("on_table(index: %u, elem_type: %s, %s)\n", index,
       get_type_name(elem_type), buf);
  FORWARD(on_table, index, elem_type, elem_limits);
}

static Result logging_on_memory(uint32_t index,
                                const Limits* page_limits,
                                void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  char buf[100];
  sprint_limits(buf, sizeof(buf), page_limits);
  LOGF("on_memory(index: %u, %s)\n", index, buf);
  FORWARD(on_memory, index, page_limits);
}

static Result logging_begin_global(uint32_t index,
                                   Type type,
                                   bool mutable_,
                                   void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF("begin_global(index: %u, type: %s, mutable: %s)\n", index,
       get_type_name(type), mutable_ ? "true" : "false");
  FORWARD(begin_global, index, type, mutable_);
}

static Result logging_on_export(uint32_t index,
                                ExternalKind kind,
                                uint32_t item_index,
                                StringSlice name,
                                void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF("on_export(index: %u, kind: %s, item_index: %u, name: \"" PRIstringslice
       "\")\n",
       index, get_kind_name(kind), item_index,
       WABT_PRINTF_STRING_SLICE_ARG(name));
  FORWARD(on_export, index, kind, item_index, name);
}

static Result logging_begin_function_body_pass(uint32_t index,
                                               uint32_t pass,
                                               void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF("begin_function_body_pass(index: %u, pass: %u)\n", index, pass);
  indent(ctx);
  FORWARD(begin_function_body_pass, index, pass);
}

static Result logging_on_local_decl(uint32_t decl_index,
                                    uint32_t count,
                                    Type type,
                                    void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF("on_local_decl(index: %u, count: %u, type: %s)\n", decl_index, count,
       get_type_name(type));
  FORWARD(on_local_decl, decl_index, count, type);
}

static Result logging_on_block_expr(uint32_t num_types,
                                    Type* sig_types,
                                    void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF("on_block_expr(sig: ");
  log_types(ctx, num_types, sig_types);
  LOGF_NOINDENT(")\n");
  FORWARD(on_block_expr, num_types, sig_types);
}

static Result logging_on_br_expr(uint32_t depth, void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF("on_br_expr(depth: %u)\n", depth);
  FORWARD(on_br_expr, depth);
}

static Result logging_on_br_if_expr(uint32_t depth, void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF("on_br_if_expr(depth: %u)\n", depth);
  FORWARD(on_br_if_expr, depth);
}

static Result logging_on_br_table_expr(BinaryReaderContext* context,
                                       uint32_t num_targets,
                                       uint32_t* target_depths,
                                       uint32_t default_target_depth) {
  LoggingContext* ctx = static_cast<LoggingContext*>(context->user_data);
  LOGF("on_br_table_expr(num_targets: %u, depths: [", num_targets);
  for (uint32_t i = 0; i < num_targets; ++i) {
    LOGF_NOINDENT("%u", target_depths[i]);
    if (i != num_targets - 1)
      LOGF_NOINDENT(", ");
  }
  LOGF_NOINDENT("], default: %u)\n", default_target_depth);
  FORWARD_CTX(on_br_table_expr, num_targets, target_depths,
              default_target_depth);
}

static Result logging_on_f32_const_expr(uint32_t value_bits, void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  float value;
  memcpy(&value, &value_bits, sizeof(value));
  LOGF("on_f32_const_expr(%g (0x04%x))\n", value, value_bits);
  FORWARD(on_f32_const_expr, value_bits);
}

static Result logging_on_f64_const_expr(uint64_t value_bits, void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  double value;
  memcpy(&value, &value_bits, sizeof(value));
  LOGF("on_f64_const_expr(%g (0x08%" PRIx64 "))\n", value, value_bits);
  FORWARD(on_f64_const_expr, value_bits);
}

static Result logging_on_i32_const_expr(uint32_t value, void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF("on_i32_const_expr(%u (0x%x))\n", value, value);
  FORWARD(on_i32_const_expr, value);
}

static Result logging_on_i64_const_expr(uint64_t value, void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF("on_i64_const_expr(%" PRIu64 " (0x%" PRIx64 "))\n", value, value);
  FORWARD(on_i64_const_expr, value);
}

static Result logging_on_if_expr(uint32_t num_types,
                                 Type* sig_types,
                                 void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF("on_if_expr(sig: ");
  log_types(ctx, num_types, sig_types);
  LOGF_NOINDENT(")\n");
  FORWARD(on_if_expr, num_types, sig_types);
}

static Result logging_on_load_expr(Opcode opcode,
                                   uint32_t alignment_log2,
                                   uint32_t offset,
                                   void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF("on_load_expr(opcode: \"%s\" (%u), align log2: %u, offset: %u)\n",
       get_opcode_name(opcode), static_cast<unsigned>(opcode), alignment_log2,
       offset);
  FORWARD(on_load_expr, opcode, alignment_log2, offset);
}

static Result logging_on_loop_expr(uint32_t num_types,
                                   Type* sig_types,
                                   void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF("on_loop_expr(sig: ");
  log_types(ctx, num_types, sig_types);
  LOGF_NOINDENT(")\n");
  FORWARD(on_loop_expr, num_types, sig_types);
}

static Result logging_on_store_expr(Opcode opcode,
                                    uint32_t alignment_log2,
                                    uint32_t offset,
                                    void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF("on_store_expr(opcode: \"%s\" (%u), align log2: %u, offset: %u)\n",
       get_opcode_name(opcode), static_cast<unsigned>(opcode), alignment_log2,
       offset);
  FORWARD(on_store_expr, opcode, alignment_log2, offset);
}

static Result logging_end_function_body_pass(uint32_t index,
                                             uint32_t pass,
                                             void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  dedent(ctx);
  LOGF("end_function_body_pass(index: %u, pass: %u)\n", index, pass);
  FORWARD(end_function_body_pass, index, pass);
}

static Result logging_on_data_segment_data(uint32_t index,
                                           const void* data,
                                           uint32_t size,
                                           void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF("on_data_segment_data(index:%u, size:%u)\n", index, size);
  FORWARD(on_data_segment_data, index, data, size);
}

static Result logging_on_function_name_subsection(uint32_t index,
                                                  uint32_t name_type,
                                                  uint32_t subsection_size,
                                                  void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF("on_function_name_subsection(index:%u, nametype:%u, size:%u)\n", index, name_type, subsection_size);
  FORWARD(on_function_name_subsection, index, name_type, subsection_size);
}

static Result logging_on_function_name(uint32_t index,
                                       StringSlice name,
                                       void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF("on_function_name(index: %u, name: \"" PRIstringslice "\")\n", index,
       WABT_PRINTF_STRING_SLICE_ARG(name));
  FORWARD(on_function_name, index, name);
}

static Result logging_on_local_name_subsection(uint32_t index,
                                               uint32_t name_type,
                                               uint32_t subsection_size,
                                               void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF("on_local_name_subsection(index:%u, nametype:%u, size:%u)\n", index, name_type, subsection_size);
  FORWARD(on_local_name_subsection, index, name_type, subsection_size);
}

static Result logging_on_local_name(uint32_t func_index,
                                    uint32_t local_index,
                                    StringSlice name,
                                    void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF("on_local_name(func_index: %u, local_index: %u, name: \"" PRIstringslice
       "\")\n",
       func_index, local_index, WABT_PRINTF_STRING_SLICE_ARG(name));
  FORWARD(on_local_name, func_index, local_index, name);
}

static Result logging_on_init_expr_f32_const_expr(uint32_t index,
                                                  uint32_t value_bits,
                                                  void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  float value;
  memcpy(&value, &value_bits, sizeof(value));
  LOGF("on_init_expr_f32_const_expr(index: %u, value: %g (0x04%x))\n", index,
       value, value_bits);
  FORWARD(on_init_expr_f32_const_expr, index, value_bits);
}

static Result logging_on_init_expr_f64_const_expr(uint32_t index,
                                                  uint64_t value_bits,
                                                  void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  double value;
  memcpy(&value, &value_bits, sizeof(value));
  LOGF("on_init_expr_f64_const_expr(index: %u value: %g (0x08%" PRIx64 "))\n",
       index, value, value_bits);
  FORWARD(on_init_expr_f64_const_expr, index, value_bits);
}

static Result logging_on_init_expr_i32_const_expr(uint32_t index,
                                                  uint32_t value,
                                                  void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF("on_init_expr_i32_const_expr(index: %u, value: %u)\n", index, value);
  FORWARD(on_init_expr_i32_const_expr, index, value);
}

static Result logging_on_init_expr_i64_const_expr(uint32_t index,
                                                  uint64_t value,
                                                  void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF("on_init_expr_i64_const_expr(index: %u, value: %" PRIu64 ")\n", index,
       value);
  FORWARD(on_init_expr_i64_const_expr, index, value);
}

static Result logging_on_reloc_count(uint32_t count,
                                     BinarySection section_code,
                                     StringSlice section_name,
                                     void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF("on_reloc_count(count: %d, section: %s, section_name: " PRIstringslice
       ")\n",
       count, get_section_name(section_code),
       WABT_PRINTF_STRING_SLICE_ARG(section_name));
  FORWARD(on_reloc_count, count, section_code, section_name);
}

static Result logging_on_reloc(RelocType type,
                               uint32_t offset,
                               uint32_t index,
                               int32_t addend,
                               void* user_data) {
  LoggingContext* ctx = static_cast<LoggingContext*>(user_data);
  LOGF("on_reloc(type: %s, offset: %u, index: %u, addend: %d)\n",
       get_reloc_type_name(type), offset, index, addend);
  FORWARD(on_reloc, type, offset, index, addend);
}

static void read_init_expr(Context* ctx, uint32_t index) {
  uint8_t opcode;
  in_u8(ctx, &opcode, "opcode");
  switch (static_cast<Opcode>(opcode)) {
    case Opcode::I32Const: {
      uint32_t value = 0;
      in_i32_leb128(ctx, &value, "init_expr i32.const value");
      CALLBACK(on_init_expr_i32_const_expr, index, value);
      break;
    }

    case Opcode::I64Const: {
      uint64_t value = 0;
      in_i64_leb128(ctx, &value, "init_expr i64.const value");
      CALLBACK(on_init_expr_i64_const_expr, index, value);
      break;
    }

    case Opcode::F32Const: {
      uint32_t value_bits = 0;
      in_f32(ctx, &value_bits, "init_expr f32.const value");
      CALLBACK(on_init_expr_f32_const_expr, index, value_bits);
      break;
    }

    case Opcode::F64Const: {
      uint64_t value_bits = 0;
      in_f64(ctx, &value_bits, "init_expr f64.const value");
      CALLBACK(on_init_expr_f64_const_expr, index, value_bits);
      break;
    }

    case Opcode::GetGlobal: {
      uint32_t global_index;
      in_u32_leb128(ctx, &global_index, "init_expr get_global index");
      CALLBACK(on_init_expr_get_global_expr, index, global_index);
      break;
    }

    case Opcode::End:
      return;

    default:
      RAISE_ERROR("unexpected opcode in initializer expression: %d (0x%x)",
                  opcode, opcode);
      break;
  }

  in_u8(ctx, &opcode, "opcode");
  RAISE_ERROR_UNLESS(static_cast<Opcode>(opcode) == Opcode::End,
                     "expected END opcode after initializer expression");
}

static void read_table(Context* ctx,
                       Type* out_elem_type,
                       Limits* out_elem_limits) {
  in_type(ctx, out_elem_type, "table elem type");
  RAISE_ERROR_UNLESS(*out_elem_type == Type::Anyfunc,
                     "table elem type must by anyfunc");

  uint32_t flags;
  uint32_t initial;
  uint32_t max = 0;
  in_u32_leb128(ctx, &flags, "table flags");
  in_u32_leb128(ctx, &initial, "table initial elem count");
  bool has_max = flags & WABT_BINARY_LIMITS_HAS_MAX_FLAG;
  if (has_max) {
    in_u32_leb128(ctx, &max, "table max elem count");
    RAISE_ERROR_UNLESS(initial <= max,
                       "table initial elem count must be <= max elem count");
  }

  out_elem_limits->has_max = has_max;
  out_elem_limits->initial = initial;
  out_elem_limits->max = max;
}

static void read_memory(Context* ctx, Limits* out_page_limits) {
  uint32_t flags;
  uint32_t initial;
  uint32_t max = 0;
  in_u32_leb128(ctx, &flags, "memory flags");
  in_u32_leb128(ctx, &initial, "memory initial page count");
  bool has_max = flags & WABT_BINARY_LIMITS_HAS_MAX_FLAG;
  RAISE_ERROR_UNLESS(initial <= WABT_MAX_PAGES, "invalid memory initial size");
  if (has_max) {
    in_u32_leb128(ctx, &max, "memory max page count");
    RAISE_ERROR_UNLESS(max <= WABT_MAX_PAGES, "invalid memory max size");
    RAISE_ERROR_UNLESS(initial <= max,
                       "memory initial size must be <= max size");
  }

  out_page_limits->has_max = has_max;
  out_page_limits->initial = initial;
  out_page_limits->max = max;
}

static void read_global_header(Context* ctx,
                               Type* out_type,
                               bool* out_mutable) {
  Type global_type;
  uint8_t mutable_;
  in_type(ctx, &global_type, "global type");
  RAISE_ERROR_UNLESS(is_concrete_type(global_type),
                     "invalid global type: %#x", static_cast<int>(global_type));

  in_u8(ctx, &mutable_, "global mutability");
  RAISE_ERROR_UNLESS(mutable_ <= 1, "global mutability must be 0 or 1");

  *out_type = global_type;
  *out_mutable = mutable_;
}

static void read_function_body(Context* ctx, uint32_t end_offset) {
  bool seen_end_opcode = false;
  while (ctx->offset < end_offset) {
    uint8_t opcode_u8;
    in_u8(ctx, &opcode_u8, "opcode");
    Opcode opcode = static_cast<Opcode>(opcode_u8);
    CALLBACK_CTX(on_opcode, opcode);
    switch (opcode) {
      case Opcode::Unreachable:
        CALLBACK0(on_unreachable_expr);
        CALLBACK_CTX0(on_opcode_bare);
        break;

      case Opcode::Block: {
        Type sig_type;
        in_type(ctx, &sig_type, "block signature type");
        RAISE_ERROR_UNLESS(is_inline_sig_type(sig_type),
                           "expected valid block signature type");
        uint32_t num_types = sig_type == Type::Void ? 0 : 1;
        CALLBACK(on_block_expr, num_types, &sig_type);
        CALLBACK_CTX(on_opcode_block_sig, num_types, &sig_type);
        break;
      }

      case Opcode::Loop: {
        Type sig_type;
        in_type(ctx, &sig_type, "loop signature type");
        RAISE_ERROR_UNLESS(is_inline_sig_type(sig_type),
                           "expected valid block signature type");
        uint32_t num_types = sig_type == Type::Void ? 0 : 1;
        CALLBACK(on_loop_expr, num_types, &sig_type);
        CALLBACK_CTX(on_opcode_block_sig, num_types, &sig_type);
        break;
      }

      case Opcode::If: {
        Type sig_type;
        in_type(ctx, &sig_type, "if signature type");
        RAISE_ERROR_UNLESS(is_inline_sig_type(sig_type),
                           "expected valid block signature type");
        uint32_t num_types = sig_type == Type::Void ? 0 : 1;
        CALLBACK(on_if_expr, num_types, &sig_type);
        CALLBACK_CTX(on_opcode_block_sig, num_types, &sig_type);
        break;
      }

      case Opcode::Else:
        CALLBACK0(on_else_expr);
        CALLBACK_CTX0(on_opcode_bare);
        break;

      case Opcode::Select:
        CALLBACK0(on_select_expr);
        CALLBACK_CTX0(on_opcode_bare);
        break;

      case Opcode::Br: {
        uint32_t depth;
        in_u32_leb128(ctx, &depth, "br depth");
        CALLBACK(on_br_expr, depth);
        CALLBACK_CTX(on_opcode_uint32, depth);
        break;
      }

      case Opcode::BrIf: {
        uint32_t depth;
        in_u32_leb128(ctx, &depth, "br_if depth");
        CALLBACK(on_br_if_expr, depth);
        CALLBACK_CTX(on_opcode_uint32, depth);
        break;
      }

      case Opcode::BrTable: {
        uint32_t num_targets;
        in_u32_leb128(ctx, &num_targets, "br_table target count");
        ctx->target_depths.resize(num_targets);

        for (uint32_t i = 0; i < num_targets; ++i) {
          uint32_t target_depth;
          in_u32_leb128(ctx, &target_depth, "br_table target depth");
          ctx->target_depths[i] = target_depth;
        }

        uint32_t default_target_depth;
        in_u32_leb128(ctx, &default_target_depth,
                      "br_table default target depth");

        uint32_t* target_depths =
            num_targets ? ctx->target_depths.data() : nullptr;

        CALLBACK_CTX(on_br_table_expr, num_targets, target_depths,
                     default_target_depth);
        break;
      }

      case Opcode::Return:
        CALLBACK0(on_return_expr);
        CALLBACK_CTX0(on_opcode_bare);
        break;

      case Opcode::Nop:
        CALLBACK0(on_nop_expr);
        CALLBACK_CTX0(on_opcode_bare);
        break;

      case Opcode::Drop:
        CALLBACK0(on_drop_expr);
        CALLBACK_CTX0(on_opcode_bare);
        break;

      case Opcode::End:
        if (ctx->offset == end_offset) {
          seen_end_opcode = true;
          CALLBACK0(on_end_func);
        } else {
          CALLBACK0(on_end_expr);
        }
        break;

      case Opcode::I32Const: {
        uint32_t value = 0;
        in_i32_leb128(ctx, &value, "i32.const value");
        CALLBACK(on_i32_const_expr, value);
        CALLBACK_CTX(on_opcode_uint32, value);
        break;
      }

      case Opcode::I64Const: {
        uint64_t value = 0;
        in_i64_leb128(ctx, &value, "i64.const value");
        CALLBACK(on_i64_const_expr, value);
        CALLBACK_CTX(on_opcode_uint64, value);
        break;
      }

      case Opcode::F32Const: {
        uint32_t value_bits = 0;
        in_f32(ctx, &value_bits, "f32.const value");
        CALLBACK(on_f32_const_expr, value_bits);
        CALLBACK_CTX(on_opcode_f32, value_bits);
        break;
      }

      case Opcode::F64Const: {
        uint64_t value_bits = 0;
        in_f64(ctx, &value_bits, "f64.const value");
        CALLBACK(on_f64_const_expr, value_bits);
        CALLBACK_CTX(on_opcode_f64, value_bits);
        break;
      }

      case Opcode::GetGlobal: {
        uint32_t global_index;
        in_u32_leb128(ctx, &global_index, "get_global global index");
        CALLBACK(on_get_global_expr, global_index);
        CALLBACK_CTX(on_opcode_uint32, global_index);
        break;
      }

      case Opcode::GetLocal: {
        uint32_t local_index;
        in_u32_leb128(ctx, &local_index, "get_local local index");
        CALLBACK(on_get_local_expr, local_index);
        CALLBACK_CTX(on_opcode_uint32, local_index);
        break;
      }

      case Opcode::SetGlobal: {
        uint32_t global_index;
        in_u32_leb128(ctx, &global_index, "set_global global index");
        CALLBACK(on_set_global_expr, global_index);
        CALLBACK_CTX(on_opcode_uint32, global_index);
        break;
      }

      case Opcode::SetLocal: {
        uint32_t local_index;
        in_u32_leb128(ctx, &local_index, "set_local local index");
        CALLBACK(on_set_local_expr, local_index);
        CALLBACK_CTX(on_opcode_uint32, local_index);
        break;
      }

      case Opcode::Call: {
        uint32_t func_index;
        in_u32_leb128(ctx, &func_index, "call function index");
        RAISE_ERROR_UNLESS(func_index < num_total_funcs(ctx),
                           "invalid call function index");
        CALLBACK(on_call_expr, func_index);
        CALLBACK_CTX(on_opcode_uint32, func_index);
        break;
      }

      case Opcode::CallIndirect: {
        uint32_t sig_index;
        in_u32_leb128(ctx, &sig_index, "call_indirect signature index");
        RAISE_ERROR_UNLESS(sig_index < ctx->num_signatures,
                           "invalid call_indirect signature index");
        uint32_t reserved;
        in_u32_leb128(ctx, &reserved, "call_indirect reserved");
        RAISE_ERROR_UNLESS(reserved == 0,
                           "call_indirect reserved value must be 0");
        CALLBACK(on_call_indirect_expr, sig_index);
        CALLBACK_CTX(on_opcode_uint32_uint32, sig_index, reserved);
        break;
      }

      case Opcode::TeeLocal: {
        uint32_t local_index;
        in_u32_leb128(ctx, &local_index, "tee_local local index");
        CALLBACK(on_tee_local_expr, local_index);
        CALLBACK_CTX(on_opcode_uint32, local_index);
        break;
      }

      case Opcode::I32Load8S:
      case Opcode::I32Load8U:
      case Opcode::I32Load16S:
      case Opcode::I32Load16U:
      case Opcode::I64Load8S:
      case Opcode::I64Load8U:
      case Opcode::I64Load16S:
      case Opcode::I64Load16U:
      case Opcode::I64Load32S:
      case Opcode::I64Load32U:
      case Opcode::I32Load:
      case Opcode::I64Load:
      case Opcode::F32Load:
      case Opcode::F64Load: {
        uint32_t alignment_log2;
        in_u32_leb128(ctx, &alignment_log2, "load alignment");
        uint32_t offset;
        in_u32_leb128(ctx, &offset, "load offset");

        CALLBACK(on_load_expr, opcode, alignment_log2, offset);
        CALLBACK_CTX(on_opcode_uint32_uint32, alignment_log2, offset);
        break;
      }

      case Opcode::I32Store8:
      case Opcode::I32Store16:
      case Opcode::I64Store8:
      case Opcode::I64Store16:
      case Opcode::I64Store32:
      case Opcode::I32Store:
      case Opcode::I64Store:
      case Opcode::F32Store:
      case Opcode::F64Store: {
        uint32_t alignment_log2;
        in_u32_leb128(ctx, &alignment_log2, "store alignment");
        uint32_t offset;
        in_u32_leb128(ctx, &offset, "store offset");

        CALLBACK(on_store_expr, opcode, alignment_log2, offset);
        CALLBACK_CTX(on_opcode_uint32_uint32, alignment_log2, offset);
        break;
      }

      case Opcode::CurrentMemory: {
        uint32_t reserved;
        in_u32_leb128(ctx, &reserved, "current_memory reserved");
        RAISE_ERROR_UNLESS(reserved == 0,
                           "current_memory reserved value must be 0");
        CALLBACK0(on_current_memory_expr);
        CALLBACK_CTX(on_opcode_uint32, reserved);
        break;
      }

      case Opcode::GrowMemory: {
        uint32_t reserved;
        in_u32_leb128(ctx, &reserved, "grow_memory reserved");
        RAISE_ERROR_UNLESS(reserved == 0,
                           "grow_memory reserved value must be 0");
        CALLBACK0(on_grow_memory_expr);
        CALLBACK_CTX(on_opcode_uint32, reserved);
        break;
      }

      case Opcode::I32Add:
      case Opcode::I32Sub:
      case Opcode::I32Mul:
      case Opcode::I32DivS:
      case Opcode::I32DivU:
      case Opcode::I32RemS:
      case Opcode::I32RemU:
      case Opcode::I32And:
      case Opcode::I32Or:
      case Opcode::I32Xor:
      case Opcode::I32Shl:
      case Opcode::I32ShrU:
      case Opcode::I32ShrS:
      case Opcode::I32Rotr:
      case Opcode::I32Rotl:
      case Opcode::I64Add:
      case Opcode::I64Sub:
      case Opcode::I64Mul:
      case Opcode::I64DivS:
      case Opcode::I64DivU:
      case Opcode::I64RemS:
      case Opcode::I64RemU:
      case Opcode::I64And:
      case Opcode::I64Or:
      case Opcode::I64Xor:
      case Opcode::I64Shl:
      case Opcode::I64ShrU:
      case Opcode::I64ShrS:
      case Opcode::I64Rotr:
      case Opcode::I64Rotl:
      case Opcode::F32Add:
      case Opcode::F32Sub:
      case Opcode::F32Mul:
      case Opcode::F32Div:
      case Opcode::F32Min:
      case Opcode::F32Max:
      case Opcode::F32Copysign:
      case Opcode::F64Add:
      case Opcode::F64Sub:
      case Opcode::F64Mul:
      case Opcode::F64Div:
      case Opcode::F64Min:
      case Opcode::F64Max:
      case Opcode::F64Copysign:
        CALLBACK(on_binary_expr, opcode);
        CALLBACK_CTX0(on_opcode_bare);
        break;

      case Opcode::I32Eq:
      case Opcode::I32Ne:
      case Opcode::I32LtS:
      case Opcode::I32LeS:
      case Opcode::I32LtU:
      case Opcode::I32LeU:
      case Opcode::I32GtS:
      case Opcode::I32GeS:
      case Opcode::I32GtU:
      case Opcode::I32GeU:
      case Opcode::I64Eq:
      case Opcode::I64Ne:
      case Opcode::I64LtS:
      case Opcode::I64LeS:
      case Opcode::I64LtU:
      case Opcode::I64LeU:
      case Opcode::I64GtS:
      case Opcode::I64GeS:
      case Opcode::I64GtU:
      case Opcode::I64GeU:
      case Opcode::F32Eq:
      case Opcode::F32Ne:
      case Opcode::F32Lt:
      case Opcode::F32Le:
      case Opcode::F32Gt:
      case Opcode::F32Ge:
      case Opcode::F64Eq:
      case Opcode::F64Ne:
      case Opcode::F64Lt:
      case Opcode::F64Le:
      case Opcode::F64Gt:
      case Opcode::F64Ge:
        CALLBACK(on_compare_expr, opcode);
        CALLBACK_CTX0(on_opcode_bare);
        break;

      case Opcode::I32Clz:
      case Opcode::I32Ctz:
      case Opcode::I32Popcnt:
      case Opcode::I64Clz:
      case Opcode::I64Ctz:
      case Opcode::I64Popcnt:
      case Opcode::F32Abs:
      case Opcode::F32Neg:
      case Opcode::F32Ceil:
      case Opcode::F32Floor:
      case Opcode::F32Trunc:
      case Opcode::F32Nearest:
      case Opcode::F32Sqrt:
      case Opcode::F64Abs:
      case Opcode::F64Neg:
      case Opcode::F64Ceil:
      case Opcode::F64Floor:
      case Opcode::F64Trunc:
      case Opcode::F64Nearest:
      case Opcode::F64Sqrt:
        CALLBACK(on_unary_expr, opcode);
        CALLBACK_CTX0(on_opcode_bare);
        break;

      case Opcode::I32TruncSF32:
      case Opcode::I32TruncSF64:
      case Opcode::I32TruncUF32:
      case Opcode::I32TruncUF64:
      case Opcode::I32WrapI64:
      case Opcode::I64TruncSF32:
      case Opcode::I64TruncSF64:
      case Opcode::I64TruncUF32:
      case Opcode::I64TruncUF64:
      case Opcode::I64ExtendSI32:
      case Opcode::I64ExtendUI32:
      case Opcode::F32ConvertSI32:
      case Opcode::F32ConvertUI32:
      case Opcode::F32ConvertSI64:
      case Opcode::F32ConvertUI64:
      case Opcode::F32DemoteF64:
      case Opcode::F32ReinterpretI32:
      case Opcode::F64ConvertSI32:
      case Opcode::F64ConvertUI32:
      case Opcode::F64ConvertSI64:
      case Opcode::F64ConvertUI64:
      case Opcode::F64PromoteF32:
      case Opcode::F64ReinterpretI64:
      case Opcode::I32ReinterpretF32:
      case Opcode::I64ReinterpretF64:
      case Opcode::I32Eqz:
      case Opcode::I64Eqz:
        CALLBACK(on_convert_expr, opcode);
        CALLBACK_CTX0(on_opcode_bare);
        break;

      default:
        RAISE_ERROR("unexpected opcode: %d (0x%x)", static_cast<int>(opcode),
                    static_cast<unsigned>(opcode));
    }
  }
  RAISE_ERROR_UNLESS(ctx->offset == end_offset,
                     "function body longer than given size");
  RAISE_ERROR_UNLESS(seen_end_opcode, "function body must end with END opcode");
}

static void read_custom_section(Context* ctx, uint32_t section_size) {
  StringSlice section_name;
  in_str(ctx, &section_name, "section name");
  CALLBACK_CTX(begin_custom_section, section_size, section_name);

  bool name_section_ok = ctx->last_known_section >= BinarySection::Import;
  if (ctx->options->read_debug_names && name_section_ok &&
      strncmp(section_name.start, WABT_BINARY_SECTION_NAME,
              section_name.length) == 0) {
    CALLBACK_SECTION(begin_names_section, section_size);
    uint32_t i = 0;
    while (ctx->offset < ctx->read_end) {
      uint32_t name_type;
      uint32_t subsection_size;
      in_u32_leb128(ctx, &name_type, "name type");
      in_u32_leb128(ctx, &subsection_size, "subsection size");

      switch (static_cast<NameSectionSubsection>(name_type)) {
      case NameSectionSubsection::Function:
        CALLBACK(on_function_name_subsection, i, name_type, subsection_size);
        if (subsection_size) {
          uint32_t num_names;
          in_u32_leb128(ctx, &num_names, "name count");
          CALLBACK(on_function_names_count, num_names);
          for (uint32_t j = 0; j < num_names; ++j) {
            uint32_t function_index;
            StringSlice function_name;

            in_u32_leb128(ctx, &function_index, "function index");
            in_str(ctx, &function_name, "function name");
            CALLBACK(on_function_name, function_index, function_name);
          }
        }
        ++i;
        break;
      case NameSectionSubsection::Local:
        CALLBACK(on_local_name_subsection, i, name_type, subsection_size);
        if (subsection_size) {
          uint32_t num_funcs;
          in_u32_leb128(ctx, &num_funcs, "function count");
          CALLBACK(on_local_name_function_count, num_funcs);
          for (uint32_t j = 0; j < num_funcs; ++j) {
            uint32_t function_index;
            in_u32_leb128(ctx, &function_index, "function index");
            uint32_t num_locals;
            in_u32_leb128(ctx, &num_locals, "local count");
            CALLBACK(on_local_name_local_count, function_index, num_locals);
            for (uint32_t k = 0; k < num_locals; ++k) {
              uint32_t local_index;
              StringSlice local_name;

              in_u32_leb128(ctx, &local_index, "named index");
              in_str(ctx, &local_name, "name");
              CALLBACK(on_local_name, function_index, local_index, local_name);
            }
          }
        }
        ++i;
        break;
      default:
        /* unknown subsection, skip rest of section */
        ctx->offset = ctx->read_end;
        break;
      }
    }
    CALLBACK_CTX0(end_names_section);
  } else if (strncmp(section_name.start, WABT_BINARY_SECTION_RELOC,
                     strlen(WABT_BINARY_SECTION_RELOC)) == 0) {
    CALLBACK_SECTION(begin_reloc_section, section_size);
    uint32_t num_relocs, section;
    in_u32_leb128(ctx, &section, "section");
    WABT_ZERO_MEMORY(section_name);
    if (static_cast<BinarySection>(section) == BinarySection::Custom)
      in_str(ctx, &section_name, "section name");
    in_u32_leb128(ctx, &num_relocs, "relocation count");
    CALLBACK(on_reloc_count, num_relocs, static_cast<BinarySection>(section),
             section_name);
    for (uint32_t i = 0; i < num_relocs; ++i) {
      uint32_t reloc_type, offset, index, addend = 0;
      in_u32_leb128(ctx, &reloc_type, "relocation type");
      in_u32_leb128(ctx, &offset, "offset");
      in_u32_leb128(ctx, &index, "index");
      RelocType type = static_cast<RelocType>(reloc_type);
      switch (type) {
        case RelocType::MemoryAddressLEB:
        case RelocType::MemoryAddressSLEB:
        case RelocType::MemoryAddressI32:
          in_u32_leb128(ctx, &addend, "addend");
          break;
        default:
          break;
      }
      CALLBACK(on_reloc, type, offset, index, addend);
    }
    CALLBACK_CTX0(end_reloc_section);
  } else {
    /* This is an unknown custom section, skip it. */
    ctx->offset = ctx->read_end;
  }
  CALLBACK_CTX0(end_custom_section);
}

static void read_type_section(Context* ctx, uint32_t section_size) {
  CALLBACK_SECTION(begin_signature_section, section_size);
  in_u32_leb128(ctx, &ctx->num_signatures, "type count");
  CALLBACK(on_signature_count, ctx->num_signatures);

  for (uint32_t i = 0; i < ctx->num_signatures; ++i) {
    Type form;
    in_type(ctx, &form, "type form");
    RAISE_ERROR_UNLESS(form == Type::Func, "unexpected type form: %d",
                       static_cast<int>(form));

    uint32_t num_params;
    in_u32_leb128(ctx, &num_params, "function param count");

    ctx->param_types.resize(num_params);

    for (uint32_t j = 0; j < num_params; ++j) {
      Type param_type;
      in_type(ctx, &param_type, "function param type");
      RAISE_ERROR_UNLESS(is_concrete_type(param_type),
                         "expected valid param type (got %d)",
                         static_cast<int>(param_type));
      ctx->param_types[j] = param_type;
    }

    uint32_t num_results;
    in_u32_leb128(ctx, &num_results, "function result count");
    RAISE_ERROR_UNLESS(num_results <= 1, "result count must be 0 or 1");

    Type result_type = Type::Void;
    if (num_results) {
      in_type(ctx, &result_type, "function result type");
      RAISE_ERROR_UNLESS(is_concrete_type(result_type),
                         "expected valid result type: %d",
                         static_cast<int>(result_type));
    }

    Type* param_types = num_params ? ctx->param_types.data() : nullptr;

    CALLBACK(on_signature, i, num_params, param_types, num_results,
             &result_type);
  }
  CALLBACK_CTX0(end_signature_section);
}

static void read_import_section(Context* ctx, uint32_t section_size) {
  CALLBACK_SECTION(begin_import_section, section_size);
  in_u32_leb128(ctx, &ctx->num_imports, "import count");
  CALLBACK(on_import_count, ctx->num_imports);
  for (uint32_t i = 0; i < ctx->num_imports; ++i) {
    StringSlice module_name;
    in_str(ctx, &module_name, "import module name");
    StringSlice field_name;
    in_str(ctx, &field_name, "import field name");

    uint32_t kind;
    in_u32_leb128(ctx, &kind, "import kind");
    switch (static_cast<ExternalKind>(kind)) {
      case ExternalKind::Func: {
        uint32_t sig_index;
        in_u32_leb128(ctx, &sig_index, "import signature index");
        RAISE_ERROR_UNLESS(sig_index < ctx->num_signatures,
                           "invalid import signature index");
        CALLBACK(on_import, i, module_name, field_name);
        CALLBACK(on_import_func, i, module_name, field_name,
                 ctx->num_func_imports, sig_index);
        ctx->num_func_imports++;
        break;
      }

      case ExternalKind::Table: {
        Type elem_type;
        Limits elem_limits;
        read_table(ctx, &elem_type, &elem_limits);
        CALLBACK(on_import, i, module_name, field_name);
        CALLBACK(on_import_table, i, module_name, field_name,
                 ctx->num_table_imports, elem_type, &elem_limits);
        ctx->num_table_imports++;
        break;
      }

      case ExternalKind::Memory: {
        Limits page_limits;
        read_memory(ctx, &page_limits);
        CALLBACK(on_import, i, module_name, field_name);
        CALLBACK(on_import_memory, i, module_name, field_name,
                 ctx->num_memory_imports, &page_limits);
        ctx->num_memory_imports++;
        break;
      }

      case ExternalKind::Global: {
        Type type;
        bool mutable_;
        read_global_header(ctx, &type, &mutable_);
        CALLBACK(on_import, i, module_name, field_name);
        CALLBACK(on_import_global, i, module_name, field_name,
                 ctx->num_global_imports, type, mutable_);
        ctx->num_global_imports++;
        break;
      }

      default:
        RAISE_ERROR("invalid import kind: %d", kind);
    }
  }
  CALLBACK_CTX0(end_import_section);
}

static void read_function_section(Context* ctx, uint32_t section_size) {
  CALLBACK_SECTION(begin_function_signatures_section, section_size);
  in_u32_leb128(ctx, &ctx->num_function_signatures, "function signature count");
  CALLBACK(on_function_signatures_count, ctx->num_function_signatures);
  for (uint32_t i = 0; i < ctx->num_function_signatures; ++i) {
    uint32_t func_index = ctx->num_func_imports + i;
    uint32_t sig_index;
    in_u32_leb128(ctx, &sig_index, "function signature index");
    RAISE_ERROR_UNLESS(sig_index < ctx->num_signatures,
                       "invalid function signature index: %d", sig_index);
    CALLBACK(on_function_signature, func_index, sig_index);
  }
  CALLBACK_CTX0(end_function_signatures_section);
}

static void read_table_section(Context* ctx, uint32_t section_size) {
  CALLBACK_SECTION(begin_table_section, section_size);
  in_u32_leb128(ctx, &ctx->num_tables, "table count");
  RAISE_ERROR_UNLESS(ctx->num_tables <= 1, "table count (%d) must be 0 or 1",
                     ctx->num_tables);
  CALLBACK(on_table_count, ctx->num_tables);
  for (uint32_t i = 0; i < ctx->num_tables; ++i) {
    uint32_t table_index = ctx->num_table_imports + i;
    Type elem_type;
    Limits elem_limits;
    read_table(ctx, &elem_type, &elem_limits);
    CALLBACK(on_table, table_index, elem_type, &elem_limits);
  }
  CALLBACK_CTX0(end_table_section);
}

static void read_memory_section(Context* ctx, uint32_t section_size) {
  CALLBACK_SECTION(begin_memory_section, section_size);
  in_u32_leb128(ctx, &ctx->num_memories, "memory count");
  RAISE_ERROR_UNLESS(ctx->num_memories <= 1, "memory count must be 0 or 1");
  CALLBACK(on_memory_count, ctx->num_memories);
  for (uint32_t i = 0; i < ctx->num_memories; ++i) {
    uint32_t memory_index = ctx->num_memory_imports + i;
    Limits page_limits;
    read_memory(ctx, &page_limits);
    CALLBACK(on_memory, memory_index, &page_limits);
  }
  CALLBACK_CTX0(end_memory_section);
}

static void read_global_section(Context* ctx, uint32_t section_size) {
  CALLBACK_SECTION(begin_global_section, section_size);
  in_u32_leb128(ctx, &ctx->num_globals, "global count");
  CALLBACK(on_global_count, ctx->num_globals);
  for (uint32_t i = 0; i < ctx->num_globals; ++i) {
    uint32_t global_index = ctx->num_global_imports + i;
    Type global_type;
    bool mutable_;
    read_global_header(ctx, &global_type, &mutable_);
    CALLBACK(begin_global, global_index, global_type, mutable_);
    CALLBACK(begin_global_init_expr, global_index);
    read_init_expr(ctx, global_index);
    CALLBACK(end_global_init_expr, global_index);
    CALLBACK(end_global, global_index);
  }
  CALLBACK_CTX0(end_global_section);
}

static void read_export_section(Context* ctx, uint32_t section_size) {
  CALLBACK_SECTION(begin_export_section, section_size);
  in_u32_leb128(ctx, &ctx->num_exports, "export count");
  CALLBACK(on_export_count, ctx->num_exports);
  for (uint32_t i = 0; i < ctx->num_exports; ++i) {
    StringSlice name;
    in_str(ctx, &name, "export item name");

    uint8_t external_kind;
    in_u8(ctx, &external_kind, "export external kind");
    RAISE_ERROR_UNLESS(is_valid_external_kind(external_kind),
                       "invalid export external kind");

    uint32_t item_index;
    in_u32_leb128(ctx, &item_index, "export item index");
    switch (static_cast<ExternalKind>(external_kind)) {
      case ExternalKind::Func:
        RAISE_ERROR_UNLESS(item_index < num_total_funcs(ctx),
                           "invalid export func index: %d", item_index);
        break;
      case ExternalKind::Table:
        RAISE_ERROR_UNLESS(item_index < num_total_tables(ctx),
                           "invalid export table index");
        break;
      case ExternalKind::Memory:
        RAISE_ERROR_UNLESS(item_index < num_total_memories(ctx),
                           "invalid export memory index");
        break;
      case ExternalKind::Global:
        RAISE_ERROR_UNLESS(item_index < num_total_globals(ctx),
                           "invalid export global index");
        break;
    }

    CALLBACK(on_export, i, static_cast<ExternalKind>(external_kind), item_index,
             name);
  }
  CALLBACK_CTX0(end_export_section);
}

static void read_start_section(Context* ctx, uint32_t section_size) {
  CALLBACK_SECTION(begin_start_section, section_size);
  uint32_t func_index;
  in_u32_leb128(ctx, &func_index, "start function index");
  RAISE_ERROR_UNLESS(func_index < num_total_funcs(ctx),
                     "invalid start function index");
  CALLBACK(on_start_function, func_index);
  CALLBACK_CTX0(end_start_section);
}

static void read_elem_section(Context* ctx, uint32_t section_size) {
  CALLBACK_SECTION(begin_elem_section, section_size);
  uint32_t num_elem_segments;
  in_u32_leb128(ctx, &num_elem_segments, "elem segment count");
  CALLBACK(on_elem_segment_count, num_elem_segments);
  RAISE_ERROR_UNLESS(num_elem_segments == 0 || num_total_tables(ctx) > 0,
                     "elem section without table section");
  for (uint32_t i = 0; i < num_elem_segments; ++i) {
    uint32_t table_index;
    in_u32_leb128(ctx, &table_index, "elem segment table index");
    CALLBACK(begin_elem_segment, i, table_index);
    CALLBACK(begin_elem_segment_init_expr, i);
    read_init_expr(ctx, i);
    CALLBACK(end_elem_segment_init_expr, i);

    uint32_t num_function_indexes;
    in_u32_leb128(ctx, &num_function_indexes,
                  "elem segment function index count");
    CALLBACK_CTX(on_elem_segment_function_index_count, i, num_function_indexes);
    for (uint32_t j = 0; j < num_function_indexes; ++j) {
      uint32_t func_index;
      in_u32_leb128(ctx, &func_index, "elem segment function index");
      CALLBACK(on_elem_segment_function_index, i, func_index);
    }
    CALLBACK(end_elem_segment, i);
  }
  CALLBACK_CTX0(end_elem_section);
}

static void read_code_section(Context* ctx, uint32_t section_size) {
  CALLBACK_SECTION(begin_function_bodies_section, section_size);
  in_u32_leb128(ctx, &ctx->num_function_bodies, "function body count");
  RAISE_ERROR_UNLESS(ctx->num_function_signatures == ctx->num_function_bodies,
                     "function signature count != function body count");
  CALLBACK(on_function_bodies_count, ctx->num_function_bodies);
  for (uint32_t i = 0; i < ctx->num_function_bodies; ++i) {
    uint32_t func_index = ctx->num_func_imports + i;
    uint32_t func_offset = ctx->offset;
    ctx->offset = func_offset;
    CALLBACK_CTX(begin_function_body, func_index);
    uint32_t body_size;
    in_u32_leb128(ctx, &body_size, "function body size");
    uint32_t body_start_offset = ctx->offset;
    uint32_t end_offset = body_start_offset + body_size;

    uint32_t num_local_decls;
    in_u32_leb128(ctx, &num_local_decls, "local declaration count");
    CALLBACK(on_local_decl_count, num_local_decls);
    for (uint32_t k = 0; k < num_local_decls; ++k) {
      uint32_t num_local_types;
      in_u32_leb128(ctx, &num_local_types, "local type count");
      Type local_type;
      in_type(ctx, &local_type, "local type");
      RAISE_ERROR_UNLESS(is_concrete_type(local_type),
                         "expected valid local type");
      CALLBACK(on_local_decl, k, num_local_types, local_type);
    }

    read_function_body(ctx, end_offset);

    CALLBACK(end_function_body, func_index);
  }
  CALLBACK_CTX0(end_function_bodies_section);
}

static void read_data_section(Context* ctx, uint32_t section_size) {
  CALLBACK_SECTION(begin_data_section, section_size);
  uint32_t num_data_segments;
  in_u32_leb128(ctx, &num_data_segments, "data segment count");
  CALLBACK(on_data_segment_count, num_data_segments);
  RAISE_ERROR_UNLESS(num_data_segments == 0 || num_total_memories(ctx) > 0,
                     "data section without memory section");
  for (uint32_t i = 0; i < num_data_segments; ++i) {
    uint32_t memory_index;
    in_u32_leb128(ctx, &memory_index, "data segment memory index");
    CALLBACK(begin_data_segment, i, memory_index);
    CALLBACK(begin_data_segment_init_expr, i);
    read_init_expr(ctx, i);
    CALLBACK(end_data_segment_init_expr, i);

    uint32_t data_size;
    const void* data;
    in_bytes(ctx, &data, &data_size, "data segment data");
    CALLBACK(on_data_segment_data, i, data, data_size);
    CALLBACK(end_data_segment, i);
  }
  CALLBACK_CTX0(end_data_section);
}

static void read_sections(Context* ctx) {
  while (ctx->offset < ctx->data_size) {
    uint32_t section_code;
    uint32_t section_size;
    /* Temporarily reset read_end to the full data size so the next section
     * can be read. */
    ctx->read_end = ctx->data_size;
    in_u32_leb128(ctx, &section_code, "section code");
    in_u32_leb128(ctx, &section_size, "section size");
    ctx->read_end = ctx->offset + section_size;
    if (section_code >= kBinarySectionCount) {
      RAISE_ERROR("invalid section code: %u; max is %u", section_code,
                  kBinarySectionCount - 1);
    }

    BinarySection section = static_cast<BinarySection>(section_code);

    if (ctx->read_end > ctx->data_size)
      RAISE_ERROR("invalid section size: extends past end");

    if (ctx->last_known_section != BinarySection::Invalid &&
        section != BinarySection::Custom &&
        section <= ctx->last_known_section) {
      RAISE_ERROR("section %s out of order", get_section_name(section));
    }

    CALLBACK_CTX(begin_section, section, section_size);

#define V(Name, name, code)                   \
  case BinarySection::Name:                   \
    read_##name##_section(ctx, section_size); \
    break;

    switch (section) {
      WABT_FOREACH_BINARY_SECTION(V)

      default:
        assert(0);
        break;
    }

#undef V

    if (ctx->offset != ctx->read_end) {
      RAISE_ERROR("unfinished section (expected end: 0x%" PRIzx ")",
                  ctx->read_end);
    }

    if (section != BinarySection::Custom)
      ctx->last_known_section = section;
  }
}

Result read_binary(const void* data,
                   size_t size,
                   BinaryReader* reader,
                   uint32_t num_function_passes,
                   const ReadBinaryOptions* options) {
  LoggingContext logging_context;
  WABT_ZERO_MEMORY(logging_context);
  logging_context.reader = reader;
  logging_context.stream = options->log_stream;

  BinaryReader logging_reader;
  WABT_ZERO_MEMORY(logging_reader);
  logging_reader.user_data = &logging_context;

  logging_reader.on_error = logging_on_error;
  logging_reader.begin_section = logging_begin_section;
  logging_reader.begin_module = logging_begin_module;
  logging_reader.end_module = logging_end_module;

  logging_reader.begin_custom_section = logging_begin_custom_section;
  logging_reader.end_custom_section = logging_end_custom_section;

  logging_reader.begin_signature_section = logging_begin_signature_section;
  logging_reader.on_signature_count = logging_on_signature_count;
  logging_reader.on_signature = logging_on_signature;
  logging_reader.end_signature_section = logging_end_signature_section;

  logging_reader.begin_import_section = logging_begin_import_section;
  logging_reader.on_import_count = logging_on_import_count;
  logging_reader.on_import = logging_on_import;
  logging_reader.on_import_func = logging_on_import_func;
  logging_reader.on_import_table = logging_on_import_table;
  logging_reader.on_import_memory = logging_on_import_memory;
  logging_reader.on_import_global = logging_on_import_global;
  logging_reader.end_import_section = logging_end_import_section;

  logging_reader.begin_function_signatures_section =
      logging_begin_function_signatures_section;
  logging_reader.on_function_signatures_count =
      logging_on_function_signatures_count;
  logging_reader.on_function_signature = logging_on_function_signature;
  logging_reader.end_function_signatures_section =
      logging_end_function_signatures_section;

  logging_reader.begin_table_section = logging_begin_table_section;
  logging_reader.on_table_count = logging_on_table_count;
  logging_reader.on_table = logging_on_table;
  logging_reader.end_table_section = logging_end_table_section;

  logging_reader.begin_memory_section = logging_begin_memory_section;
  logging_reader.on_memory_count = logging_on_memory_count;
  logging_reader.on_memory = logging_on_memory;
  logging_reader.end_memory_section = logging_end_memory_section;

  logging_reader.begin_global_section = logging_begin_global_section;
  logging_reader.on_global_count = logging_on_global_count;
  logging_reader.begin_global = logging_begin_global;
  logging_reader.begin_global_init_expr = logging_begin_global_init_expr;
  logging_reader.end_global_init_expr = logging_end_global_init_expr;
  logging_reader.end_global = logging_end_global;
  logging_reader.end_global_section = logging_end_global_section;

  logging_reader.begin_export_section = logging_begin_export_section;
  logging_reader.on_export_count = logging_on_export_count;
  logging_reader.on_export = logging_on_export;
  logging_reader.end_export_section = logging_end_export_section;

  logging_reader.begin_start_section = logging_begin_start_section;
  logging_reader.on_start_function = logging_on_start_function;
  logging_reader.end_start_section = logging_end_start_section;

  logging_reader.begin_function_bodies_section =
      logging_begin_function_bodies_section;
  logging_reader.on_function_bodies_count = logging_on_function_bodies_count;
  logging_reader.begin_function_body_pass = logging_begin_function_body_pass;
  logging_reader.begin_function_body = logging_begin_function_body;
  logging_reader.on_local_decl_count = logging_on_local_decl_count;
  logging_reader.on_local_decl = logging_on_local_decl;
  logging_reader.on_binary_expr = logging_on_binary_expr;
  logging_reader.on_block_expr = logging_on_block_expr;
  logging_reader.on_br_expr = logging_on_br_expr;
  logging_reader.on_br_if_expr = logging_on_br_if_expr;
  logging_reader.on_br_table_expr = logging_on_br_table_expr;
  logging_reader.on_call_expr = logging_on_call_expr;
  logging_reader.on_call_import_expr = logging_on_call_import_expr;
  logging_reader.on_call_indirect_expr = logging_on_call_indirect_expr;
  logging_reader.on_compare_expr = logging_on_compare_expr;
  logging_reader.on_convert_expr = logging_on_convert_expr;
  logging_reader.on_drop_expr = logging_on_drop_expr;
  logging_reader.on_else_expr = logging_on_else_expr;
  logging_reader.on_end_expr = logging_on_end_expr;
  logging_reader.on_f32_const_expr = logging_on_f32_const_expr;
  logging_reader.on_f64_const_expr = logging_on_f64_const_expr;
  logging_reader.on_get_global_expr = logging_on_get_global_expr;
  logging_reader.on_get_local_expr = logging_on_get_local_expr;
  logging_reader.on_grow_memory_expr = logging_on_grow_memory_expr;
  logging_reader.on_i32_const_expr = logging_on_i32_const_expr;
  logging_reader.on_i64_const_expr = logging_on_i64_const_expr;
  logging_reader.on_if_expr = logging_on_if_expr;
  logging_reader.on_load_expr = logging_on_load_expr;
  logging_reader.on_loop_expr = logging_on_loop_expr;
  logging_reader.on_current_memory_expr = logging_on_current_memory_expr;
  logging_reader.on_nop_expr = logging_on_nop_expr;
  logging_reader.on_return_expr = logging_on_return_expr;
  logging_reader.on_select_expr = logging_on_select_expr;
  logging_reader.on_set_global_expr = logging_on_set_global_expr;
  logging_reader.on_set_local_expr = logging_on_set_local_expr;
  logging_reader.on_store_expr = logging_on_store_expr;
  logging_reader.on_tee_local_expr = logging_on_tee_local_expr;
  logging_reader.on_unary_expr = logging_on_unary_expr;
  logging_reader.on_unreachable_expr = logging_on_unreachable_expr;
  logging_reader.end_function_body = logging_end_function_body;
  logging_reader.end_function_body_pass = logging_end_function_body_pass;
  logging_reader.end_function_bodies_section =
      logging_end_function_bodies_section;

  logging_reader.begin_elem_section = logging_begin_elem_section;
  logging_reader.on_elem_segment_count = logging_on_elem_segment_count;
  logging_reader.begin_elem_segment = logging_begin_elem_segment;
  logging_reader.begin_elem_segment_init_expr =
      logging_begin_elem_segment_init_expr;
  logging_reader.end_elem_segment_init_expr =
      logging_end_elem_segment_init_expr;
  logging_reader.on_elem_segment_function_index_count =
      logging_on_elem_segment_function_index_count;
  logging_reader.on_elem_segment_function_index =
      logging_on_elem_segment_function_index;
  logging_reader.end_elem_segment = logging_end_elem_segment;
  logging_reader.end_elem_section = logging_end_elem_section;

  logging_reader.begin_data_section = logging_begin_data_section;
  logging_reader.on_data_segment_count = logging_on_data_segment_count;
  logging_reader.begin_data_segment = logging_begin_data_segment;
  logging_reader.begin_data_segment_init_expr =
      logging_begin_data_segment_init_expr;
  logging_reader.end_data_segment_init_expr =
      logging_end_data_segment_init_expr;
  logging_reader.on_data_segment_data = logging_on_data_segment_data;
  logging_reader.end_data_segment = logging_end_data_segment;
  logging_reader.end_data_section = logging_end_data_section;

  logging_reader.begin_names_section = logging_begin_names_section;
  logging_reader.on_function_name_subsection = logging_on_function_name_subsection;
  logging_reader.on_function_names_count = logging_on_function_names_count;
  logging_reader.on_function_name = logging_on_function_name;
  logging_reader.on_local_name_subsection = logging_on_local_name_subsection;
  logging_reader.on_local_name_function_count = logging_on_local_name_function_count;
  logging_reader.on_local_name_local_count = logging_on_local_name_local_count;
  logging_reader.on_local_name = logging_on_local_name;
  logging_reader.end_names_section = logging_end_names_section;

  logging_reader.begin_reloc_section = logging_begin_reloc_section;
  logging_reader.on_reloc_count = logging_on_reloc_count;
  logging_reader.on_reloc = logging_on_reloc;
  logging_reader.end_reloc_section = logging_end_reloc_section;

  logging_reader.on_init_expr_f32_const_expr =
      logging_on_init_expr_f32_const_expr;
  logging_reader.on_init_expr_f64_const_expr =
      logging_on_init_expr_f64_const_expr;
  logging_reader.on_init_expr_get_global_expr =
      logging_on_init_expr_get_global_expr;
  logging_reader.on_init_expr_i32_const_expr =
      logging_on_init_expr_i32_const_expr;
  logging_reader.on_init_expr_i64_const_expr =
      logging_on_init_expr_i64_const_expr;

  Context context;
  /* all the macros assume a Context* named ctx */
  Context* ctx = &context;
  ctx->data = static_cast<const uint8_t*>(data);
  ctx->data_size = ctx->read_end = size;
  ctx->reader = options->log_stream ? &logging_reader : reader;
  ctx->options = options;
  ctx->last_known_section = BinarySection::Invalid;

  if (setjmp(ctx->error_jmp_buf) == 1) {
    return Result::Error;
  }

  uint32_t magic;
  in_u32(ctx, &magic, "magic");
  RAISE_ERROR_UNLESS(magic == WABT_BINARY_MAGIC, "bad magic value");
  uint32_t version;
  in_u32(ctx, &version, "version");
  RAISE_ERROR_UNLESS(version == WABT_BINARY_VERSION,
                     "bad wasm file version: %#x (expected %#x)", version,
                     WABT_BINARY_VERSION);

  CALLBACK(begin_module, version);
  read_sections(ctx);
  CALLBACK0(end_module);
  return Result::Ok;
}

}  // namespace wabt
