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

#ifndef WABT_COMMON_H_
#define WABT_COMMON_H_

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <type_traits>
#include <vector>

#include "config.h"

#define WABT_FATAL(...) fprintf(stderr, __VA_ARGS__), exit(1)
#define WABT_ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define WABT_ZERO_MEMORY(var)                                          \
  WABT_STATIC_ASSERT(                                                  \
      std::is_pod<std::remove_reference<decltype(var)>::type>::value); \
  memset(static_cast<void*>(&(var)), 0, sizeof(var))

#define WABT_USE(x) static_cast<void>(x)

#define WABT_UNKNOWN_OFFSET (static_cast<uint32_t>(~0))
#define WABT_PAGE_SIZE 0x10000 /* 64k */
#define WABT_MAX_PAGES 0x10000 /* # of pages that fit in 32-bit address space */
#define WABT_BYTES_TO_PAGES(x) ((x) >> 16)
#define WABT_ALIGN_UP_TO_PAGE(x) \
  (((x) + WABT_PAGE_SIZE - 1) & ~(WABT_PAGE_SIZE - 1))

#define PRIstringslice "%.*s"
#define WABT_PRINTF_STRING_SLICE_ARG(x) static_cast<int>((x).length), (x).start

#define WABT_DEFAULT_SNPRINTF_ALLOCA_BUFSIZE 128
#define WABT_SNPRINTF_ALLOCA(buffer, len, format)                          \
  va_list args;                                                            \
  va_list args_copy;                                                       \
  va_start(args, format);                                                  \
  va_copy(args_copy, args);                                                \
  char fixed_buf[WABT_DEFAULT_SNPRINTF_ALLOCA_BUFSIZE];                    \
  char* buffer = fixed_buf;                                                \
  size_t len = wabt_vsnprintf(fixed_buf, sizeof(fixed_buf), format, args); \
  va_end(args);                                                            \
  if (len + 1 > sizeof(fixed_buf)) {                                       \
    buffer = static_cast<char*>(alloca(len + 1));                          \
    len = wabt_vsnprintf(buffer, len + 1, format, args_copy);              \
  }                                                                        \
  va_end(args_copy)

#define WABT_ENUM_COUNT(name) \
  (static_cast<int>(name::Last) - static_cast<int>(name::First) + 1)

#define WABT_DISALLOW_COPY_AND_ASSIGN(type) \
  type(const type&) = delete;               \
  type& operator=(const type&) = delete;

namespace wabt {

enum class Result {
  Ok,
  Error,
};

#define WABT_SUCCEEDED(x) ((x) == Result::Ok)
#define WABT_FAILED(x) ((x) == Result::Error)

enum class LabelType {
  Func,
  Block,
  Loop,
  If,
  Else,

  First = Func,
  Last = Else,
};
static const int kLabelTypeCount = WABT_ENUM_COUNT(LabelType);

struct StringSlice {
  const char* start;
  size_t length;
};

struct Location {
  const char* filename;
  int line;
  int first_column;
  int last_column;
};

/* Returns true if the error was handled. */
typedef bool (*SourceErrorCallback)(const Location*,
                                    const char* error,
                                    const char* source_line,
                                    size_t source_line_length,
                                    size_t source_line_column_offset,
                                    void* user_data);

struct SourceErrorHandler {
  SourceErrorCallback on_error;
  /* on_error will be called with with source_line trimmed to this length */
  size_t source_line_max_length;
  void* user_data;
};

#define WABT_SOURCE_LINE_MAX_LENGTH_DEFAULT 80
#define WABT_SOURCE_ERROR_HANDLER_DEFAULT                               \
  {                                                                     \
    default_source_error_callback, WABT_SOURCE_LINE_MAX_LENGTH_DEFAULT, \
        nullptr                                                         \
  }

/* Returns true if the error was handled. */
typedef bool (*BinaryErrorCallback)(uint32_t offset,
                                    const char* error,
                                    void* user_data);

struct BinaryErrorHandler {
  BinaryErrorCallback on_error;
  void* user_data;
};

#define WABT_BINARY_ERROR_HANDLER_DEFAULT \
  { default_binary_error_callback, nullptr }

/* This data structure is not required; it is just used by the default error
 * handler callbacks. */
enum class PrintErrorHeader {
  Never,
  Once,
  Always,
};

struct DefaultErrorHandlerInfo {
  const char* header;
  FILE* out_file;
  PrintErrorHeader print_header;
};

/* matches binary format, do not change */
enum class Type {
  I32 = -0x01,
  I64 = -0x02,
  F32 = -0x03,
  F64 = -0x04,
  Anyfunc = -0x10,
  Func = -0x20,
  Void = -0x40,
  ___ = Void, /* convenient for the opcode table below */
  Any = 0,    /* Not actually specified, but useful for type-checking */
};
typedef std::vector<Type> TypeVector;

enum class RelocType {
  FuncIndexLEB = 0,   /* e.g. immediate of call instruction */
  TableIndexSLEB = 1, /* e.g. loading address of function */
  TableIndexI32 = 2,  /* e.g. function address in DATA */
  MemoryAddressLEB = 3,
  MemoryAddressSLEB = 4,
  MemoryAddressI32 = 5,
  TypeIndexLEB = 6, /* e.g immediate type in call_indirect */
  GlobalIndexLEB = 7, /* e.g immediate of get_global inst */

  First = FuncIndexLEB,
  Last = GlobalIndexLEB,
};
static const int kRelocTypeCount = WABT_ENUM_COUNT(RelocType);

struct Reloc {
  Reloc(RelocType, size_t offset, uint32_t index, int32_t addend = 0);

  RelocType type;
  size_t offset;
  uint32_t index;
  int32_t addend;
};

/* matches binary format, do not change */
enum class ExternalKind {
  Func = 0,
  Table = 1,
  Memory = 2,
  Global = 3,

  First = Func,
  Last = Global,
};
static const int kExternalKindCount = WABT_ENUM_COUNT(ExternalKind);

struct Limits {
  uint64_t initial;
  uint64_t max;
  bool has_max;
};

enum { WABT_USE_NATURAL_ALIGNMENT = 0xFFFFFFFF };

/*
 *   tr: result type
 *   t1: type of the 1st parameter
 *   t2: type of the 2nd parameter
 *    m: memory size of the operation, if any
 * code: opcode
 * NAME: used to generate the opcode enum
 * text: a string of the opcode name in the AST format
 *
 *  tr  t1    t2   m  code  NAME text
 *  ============================ */
#define WABT_FOREACH_OPCODE(V)                                        \
  V(___, ___, ___, 0, 0x00, Unreachable, "unreachable")               \
  V(___, ___, ___, 0, 0x01, Nop, "nop")                               \
  V(___, ___, ___, 0, 0x02, Block, "block")                           \
  V(___, ___, ___, 0, 0x03, Loop, "loop")                             \
  V(___, ___, ___, 0, 0x04, If, "if")                                 \
  V(___, ___, ___, 0, 0x05, Else, "else")                             \
  V(___, ___, ___, 0, 0x0b, End, "end")                               \
  V(___, ___, ___, 0, 0x0c, Br, "br")                                 \
  V(___, ___, ___, 0, 0x0d, BrIf, "br_if")                            \
  V(___, ___, ___, 0, 0x0e, BrTable, "br_table")                      \
  V(___, ___, ___, 0, 0x0f, Return, "return")                         \
  V(___, ___, ___, 0, 0x10, Call, "call")                             \
  V(___, ___, ___, 0, 0x11, CallIndirect, "call_indirect")            \
  V(___, ___, ___, 0, 0x1a, Drop, "drop")                             \
  V(___, ___, ___, 0, 0x1b, Select, "select")                         \
  V(___, ___, ___, 0, 0x20, GetLocal, "get_local")                    \
  V(___, ___, ___, 0, 0x21, SetLocal, "set_local")                    \
  V(___, ___, ___, 0, 0x22, TeeLocal, "tee_local")                    \
  V(___, ___, ___, 0, 0x23, GetGlobal, "get_global")                  \
  V(___, ___, ___, 0, 0x24, SetGlobal, "set_global")                  \
  V(I32, I32, ___, 4, 0x28, I32Load, "i32.load")                      \
  V(I64, I32, ___, 8, 0x29, I64Load, "i64.load")                      \
  V(F32, I32, ___, 4, 0x2a, F32Load, "f32.load")                      \
  V(F64, I32, ___, 8, 0x2b, F64Load, "f64.load")                      \
  V(I32, I32, ___, 1, 0x2c, I32Load8S, "i32.load8_s")                 \
  V(I32, I32, ___, 1, 0x2d, I32Load8U, "i32.load8_u")                 \
  V(I32, I32, ___, 2, 0x2e, I32Load16S, "i32.load16_s")               \
  V(I32, I32, ___, 2, 0x2f, I32Load16U, "i32.load16_u")               \
  V(I64, I32, ___, 1, 0x30, I64Load8S, "i64.load8_s")                 \
  V(I64, I32, ___, 1, 0x31, I64Load8U, "i64.load8_u")                 \
  V(I64, I32, ___, 2, 0x32, I64Load16S, "i64.load16_s")               \
  V(I64, I32, ___, 2, 0x33, I64Load16U, "i64.load16_u")               \
  V(I64, I32, ___, 4, 0x34, I64Load32S, "i64.load32_s")               \
  V(I64, I32, ___, 4, 0x35, I64Load32U, "i64.load32_u")               \
  V(___, I32, I32, 4, 0x36, I32Store, "i32.store")                    \
  V(___, I32, I64, 8, 0x37, I64Store, "i64.store")                    \
  V(___, I32, F32, 4, 0x38, F32Store, "f32.store")                    \
  V(___, I32, F64, 8, 0x39, F64Store, "f64.store")                    \
  V(___, I32, I32, 1, 0x3a, I32Store8, "i32.store8")                  \
  V(___, I32, I32, 2, 0x3b, I32Store16, "i32.store16")                \
  V(___, I32, I64, 1, 0x3c, I64Store8, "i64.store8")                  \
  V(___, I32, I64, 2, 0x3d, I64Store16, "i64.store16")                \
  V(___, I32, I64, 4, 0x3e, I64Store32, "i64.store32")                \
  V(I32, ___, ___, 0, 0x3f, CurrentMemory, "current_memory")          \
  V(I32, I32, ___, 0, 0x40, GrowMemory, "grow_memory")                \
  V(I32, ___, ___, 0, 0x41, I32Const, "i32.const")                    \
  V(I64, ___, ___, 0, 0x42, I64Const, "i64.const")                    \
  V(F32, ___, ___, 0, 0x43, F32Const, "f32.const")                    \
  V(F64, ___, ___, 0, 0x44, F64Const, "f64.const")                    \
  V(I32, I32, ___, 0, 0x45, I32Eqz, "i32.eqz")                        \
  V(I32, I32, I32, 0, 0x46, I32Eq, "i32.eq")                          \
  V(I32, I32, I32, 0, 0x47, I32Ne, "i32.ne")                          \
  V(I32, I32, I32, 0, 0x48, I32LtS, "i32.lt_s")                       \
  V(I32, I32, I32, 0, 0x49, I32LtU, "i32.lt_u")                       \
  V(I32, I32, I32, 0, 0x4a, I32GtS, "i32.gt_s")                       \
  V(I32, I32, I32, 0, 0x4b, I32GtU, "i32.gt_u")                       \
  V(I32, I32, I32, 0, 0x4c, I32LeS, "i32.le_s")                       \
  V(I32, I32, I32, 0, 0x4d, I32LeU, "i32.le_u")                       \
  V(I32, I32, I32, 0, 0x4e, I32GeS, "i32.ge_s")                       \
  V(I32, I32, I32, 0, 0x4f, I32GeU, "i32.ge_u")                       \
  V(I32, I64, ___, 0, 0x50, I64Eqz, "i64.eqz")                        \
  V(I32, I64, I64, 0, 0x51, I64Eq, "i64.eq")                          \
  V(I32, I64, I64, 0, 0x52, I64Ne, "i64.ne")                          \
  V(I32, I64, I64, 0, 0x53, I64LtS, "i64.lt_s")                       \
  V(I32, I64, I64, 0, 0x54, I64LtU, "i64.lt_u")                       \
  V(I32, I64, I64, 0, 0x55, I64GtS, "i64.gt_s")                       \
  V(I32, I64, I64, 0, 0x56, I64GtU, "i64.gt_u")                       \
  V(I32, I64, I64, 0, 0x57, I64LeS, "i64.le_s")                       \
  V(I32, I64, I64, 0, 0x58, I64LeU, "i64.le_u")                       \
  V(I32, I64, I64, 0, 0x59, I64GeS, "i64.ge_s")                       \
  V(I32, I64, I64, 0, 0x5a, I64GeU, "i64.ge_u")                       \
  V(I32, F32, F32, 0, 0x5b, F32Eq, "f32.eq")                          \
  V(I32, F32, F32, 0, 0x5c, F32Ne, "f32.ne")                          \
  V(I32, F32, F32, 0, 0x5d, F32Lt, "f32.lt")                          \
  V(I32, F32, F32, 0, 0x5e, F32Gt, "f32.gt")                          \
  V(I32, F32, F32, 0, 0x5f, F32Le, "f32.le")                          \
  V(I32, F32, F32, 0, 0x60, F32Ge, "f32.ge")                          \
  V(I32, F64, F64, 0, 0x61, F64Eq, "f64.eq")                          \
  V(I32, F64, F64, 0, 0x62, F64Ne, "f64.ne")                          \
  V(I32, F64, F64, 0, 0x63, F64Lt, "f64.lt")                          \
  V(I32, F64, F64, 0, 0x64, F64Gt, "f64.gt")                          \
  V(I32, F64, F64, 0, 0x65, F64Le, "f64.le")                          \
  V(I32, F64, F64, 0, 0x66, F64Ge, "f64.ge")                          \
  V(I32, I32, ___, 0, 0x67, I32Clz, "i32.clz")                        \
  V(I32, I32, ___, 0, 0x68, I32Ctz, "i32.ctz")                        \
  V(I32, I32, ___, 0, 0x69, I32Popcnt, "i32.popcnt")                  \
  V(I32, I32, I32, 0, 0x6a, I32Add, "i32.add")                        \
  V(I32, I32, I32, 0, 0x6b, I32Sub, "i32.sub")                        \
  V(I32, I32, I32, 0, 0x6c, I32Mul, "i32.mul")                        \
  V(I32, I32, I32, 0, 0x6d, I32DivS, "i32.div_s")                     \
  V(I32, I32, I32, 0, 0x6e, I32DivU, "i32.div_u")                     \
  V(I32, I32, I32, 0, 0x6f, I32RemS, "i32.rem_s")                     \
  V(I32, I32, I32, 0, 0x70, I32RemU, "i32.rem_u")                     \
  V(I32, I32, I32, 0, 0x71, I32And, "i32.and")                        \
  V(I32, I32, I32, 0, 0x72, I32Or, "i32.or")                          \
  V(I32, I32, I32, 0, 0x73, I32Xor, "i32.xor")                        \
  V(I32, I32, I32, 0, 0x74, I32Shl, "i32.shl")                        \
  V(I32, I32, I32, 0, 0x75, I32ShrS, "i32.shr_s")                     \
  V(I32, I32, I32, 0, 0x76, I32ShrU, "i32.shr_u")                     \
  V(I32, I32, I32, 0, 0x77, I32Rotl, "i32.rotl")                      \
  V(I32, I32, I32, 0, 0x78, I32Rotr, "i32.rotr")                      \
  V(I64, I64, I64, 0, 0x79, I64Clz, "i64.clz")                        \
  V(I64, I64, I64, 0, 0x7a, I64Ctz, "i64.ctz")                        \
  V(I64, I64, I64, 0, 0x7b, I64Popcnt, "i64.popcnt")                  \
  V(I64, I64, I64, 0, 0x7c, I64Add, "i64.add")                        \
  V(I64, I64, I64, 0, 0x7d, I64Sub, "i64.sub")                        \
  V(I64, I64, I64, 0, 0x7e, I64Mul, "i64.mul")                        \
  V(I64, I64, I64, 0, 0x7f, I64DivS, "i64.div_s")                     \
  V(I64, I64, I64, 0, 0x80, I64DivU, "i64.div_u")                     \
  V(I64, I64, I64, 0, 0x81, I64RemS, "i64.rem_s")                     \
  V(I64, I64, I64, 0, 0x82, I64RemU, "i64.rem_u")                     \
  V(I64, I64, I64, 0, 0x83, I64And, "i64.and")                        \
  V(I64, I64, I64, 0, 0x84, I64Or, "i64.or")                          \
  V(I64, I64, I64, 0, 0x85, I64Xor, "i64.xor")                        \
  V(I64, I64, I64, 0, 0x86, I64Shl, "i64.shl")                        \
  V(I64, I64, I64, 0, 0x87, I64ShrS, "i64.shr_s")                     \
  V(I64, I64, I64, 0, 0x88, I64ShrU, "i64.shr_u")                     \
  V(I64, I64, I64, 0, 0x89, I64Rotl, "i64.rotl")                      \
  V(I64, I64, I64, 0, 0x8a, I64Rotr, "i64.rotr")                      \
  V(F32, F32, F32, 0, 0x8b, F32Abs, "f32.abs")                        \
  V(F32, F32, F32, 0, 0x8c, F32Neg, "f32.neg")                        \
  V(F32, F32, F32, 0, 0x8d, F32Ceil, "f32.ceil")                      \
  V(F32, F32, F32, 0, 0x8e, F32Floor, "f32.floor")                    \
  V(F32, F32, F32, 0, 0x8f, F32Trunc, "f32.trunc")                    \
  V(F32, F32, F32, 0, 0x90, F32Nearest, "f32.nearest")                \
  V(F32, F32, F32, 0, 0x91, F32Sqrt, "f32.sqrt")                      \
  V(F32, F32, F32, 0, 0x92, F32Add, "f32.add")                        \
  V(F32, F32, F32, 0, 0x93, F32Sub, "f32.sub")                        \
  V(F32, F32, F32, 0, 0x94, F32Mul, "f32.mul")                        \
  V(F32, F32, F32, 0, 0x95, F32Div, "f32.div")                        \
  V(F32, F32, F32, 0, 0x96, F32Min, "f32.min")                        \
  V(F32, F32, F32, 0, 0x97, F32Max, "f32.max")                        \
  V(F32, F32, F32, 0, 0x98, F32Copysign, "f32.copysign")              \
  V(F64, F64, F64, 0, 0x99, F64Abs, "f64.abs")                        \
  V(F64, F64, F64, 0, 0x9a, F64Neg, "f64.neg")                        \
  V(F64, F64, F64, 0, 0x9b, F64Ceil, "f64.ceil")                      \
  V(F64, F64, F64, 0, 0x9c, F64Floor, "f64.floor")                    \
  V(F64, F64, F64, 0, 0x9d, F64Trunc, "f64.trunc")                    \
  V(F64, F64, F64, 0, 0x9e, F64Nearest, "f64.nearest")                \
  V(F64, F64, F64, 0, 0x9f, F64Sqrt, "f64.sqrt")                      \
  V(F64, F64, F64, 0, 0xa0, F64Add, "f64.add")                        \
  V(F64, F64, F64, 0, 0xa1, F64Sub, "f64.sub")                        \
  V(F64, F64, F64, 0, 0xa2, F64Mul, "f64.mul")                        \
  V(F64, F64, F64, 0, 0xa3, F64Div, "f64.div")                        \
  V(F64, F64, F64, 0, 0xa4, F64Min, "f64.min")                        \
  V(F64, F64, F64, 0, 0xa5, F64Max, "f64.max")                        \
  V(F64, F64, F64, 0, 0xa6, F64Copysign, "f64.copysign")              \
  V(I32, I64, ___, 0, 0xa7, I32WrapI64, "i32.wrap/i64")               \
  V(I32, F32, ___, 0, 0xa8, I32TruncSF32, "i32.trunc_s/f32")          \
  V(I32, F32, ___, 0, 0xa9, I32TruncUF32, "i32.trunc_u/f32")          \
  V(I32, F64, ___, 0, 0xaa, I32TruncSF64, "i32.trunc_s/f64")          \
  V(I32, F64, ___, 0, 0xab, I32TruncUF64, "i32.trunc_u/f64")          \
  V(I64, I32, ___, 0, 0xac, I64ExtendSI32, "i64.extend_s/i32")        \
  V(I64, I32, ___, 0, 0xad, I64ExtendUI32, "i64.extend_u/i32")        \
  V(I64, F32, ___, 0, 0xae, I64TruncSF32, "i64.trunc_s/f32")          \
  V(I64, F32, ___, 0, 0xaf, I64TruncUF32, "i64.trunc_u/f32")          \
  V(I64, F64, ___, 0, 0xb0, I64TruncSF64, "i64.trunc_s/f64")          \
  V(I64, F64, ___, 0, 0xb1, I64TruncUF64, "i64.trunc_u/f64")          \
  V(F32, I32, ___, 0, 0xb2, F32ConvertSI32, "f32.convert_s/i32")      \
  V(F32, I32, ___, 0, 0xb3, F32ConvertUI32, "f32.convert_u/i32")      \
  V(F32, I64, ___, 0, 0xb4, F32ConvertSI64, "f32.convert_s/i64")      \
  V(F32, I64, ___, 0, 0xb5, F32ConvertUI64, "f32.convert_u/i64")      \
  V(F32, F64, ___, 0, 0xb6, F32DemoteF64, "f32.demote/f64")           \
  V(F64, I32, ___, 0, 0xb7, F64ConvertSI32, "f64.convert_s/i32")      \
  V(F64, I32, ___, 0, 0xb8, F64ConvertUI32, "f64.convert_u/i32")      \
  V(F64, I64, ___, 0, 0xb9, F64ConvertSI64, "f64.convert_s/i64")      \
  V(F64, I64, ___, 0, 0xba, F64ConvertUI64, "f64.convert_u/i64")      \
  V(F64, F32, ___, 0, 0xbb, F64PromoteF32, "f64.promote/f32")         \
  V(I32, F32, ___, 0, 0xbc, I32ReinterpretF32, "i32.reinterpret/f32") \
  V(I64, F64, ___, 0, 0xbd, I64ReinterpretF64, "i64.reinterpret/f64") \
  V(F32, I32, ___, 0, 0xbe, F32ReinterpretI32, "f32.reinterpret/i32") \
  V(F64, I64, ___, 0, 0xbf, F64ReinterpretI64, "f64.reinterpret/i64")

enum class Opcode {
#define V(rtype, type1, type2, mem_size, code, Name, text) Name = code,
  WABT_FOREACH_OPCODE(V)
#undef V

      First = Unreachable,
  Last = F64ReinterpretI64,
};
static const int kOpcodeCount = WABT_ENUM_COUNT(Opcode);

struct OpcodeInfo {
  const char* name;
  Type result_type;
  Type param1_type;
  Type param2_type;
  int memory_size;
};

enum class LiteralType {
  Int,
  Float,
  Hexfloat,
  Infinity,
  Nan,
};

struct Literal {
  LiteralType type;
  StringSlice text;
};

enum class NameSectionSubsection {
  Function = 1,
  Local = 2,
};

static WABT_INLINE char* wabt_strndup(const char* s, size_t len) {
  size_t real_len = 0;
  const char* p = s;
  while (real_len < len && *p) {
    p++;
    real_len++;
  }

  char* new_s = new char[real_len + 1];
  memcpy(new_s, s, real_len);
  new_s[real_len] = 0;
  return new_s;
}

static WABT_INLINE StringSlice dup_string_slice(StringSlice str) {
  StringSlice result;
  result.start = wabt_strndup(str.start, str.length);
  result.length = str.length;
  return result;
}

/* return 1 if |alignment| matches the alignment of |opcode|, or if |alignment|
 * is WABT_USE_NATURAL_ALIGNMENT */
bool is_naturally_aligned(Opcode opcode, uint32_t alignment);

/* if |alignment| is WABT_USE_NATURAL_ALIGNMENT, return the alignment of
 * |opcode|, else return |alignment| */
uint32_t get_opcode_alignment(Opcode opcode, uint32_t alignment);

StringSlice empty_string_slice(void);
bool string_slice_eq_cstr(const StringSlice* s1, const char* s2);
bool string_slice_startswith(const StringSlice* s1, const char* s2);
StringSlice string_slice_from_cstr(const char* string);
bool string_slice_is_empty(const StringSlice*);
bool string_slices_are_equal(const StringSlice*, const StringSlice*);
void destroy_string_slice(StringSlice*);
Result read_file(const char* filename, char** out_data, size_t* out_size);

inline std::string string_slice_to_string(const StringSlice& ss) {
  return std::string(ss.start, ss.length);
}

inline StringSlice string_to_string_slice(const std::string& s) {
  StringSlice ss;
  ss.start = s.data();
  ss.length = s.length();
  return ss;
}

bool default_source_error_callback(const Location*,
                                   const char* error,
                                   const char* source_line,
                                   size_t source_line_length,
                                   size_t source_line_column_offset,
                                   void* user_data);

bool default_binary_error_callback(uint32_t offset,
                                   const char* error,
                                   void* user_data);

void init_stdio();

/* opcode info */
extern OpcodeInfo g_opcode_info[];
void init_opcode_info(void);

static WABT_INLINE const char* get_opcode_name(Opcode opcode) {
  assert(static_cast<int>(opcode) < kOpcodeCount);
  init_opcode_info();
  return g_opcode_info[static_cast<size_t>(opcode)].name;
}

static WABT_INLINE Type get_opcode_result_type(Opcode opcode) {
  assert(static_cast<int>(opcode) < kOpcodeCount);
  init_opcode_info();
  return g_opcode_info[static_cast<size_t>(opcode)].result_type;
}

static WABT_INLINE Type get_opcode_param_type_1(Opcode opcode) {
  assert(static_cast<int>(opcode) < kOpcodeCount);
  init_opcode_info();
  return g_opcode_info[static_cast<size_t>(opcode)].param1_type;
}

static WABT_INLINE Type get_opcode_param_type_2(Opcode opcode) {
  assert(static_cast<int>(opcode) < kOpcodeCount);
  init_opcode_info();
  return g_opcode_info[static_cast<size_t>(opcode)].param2_type;
}

static WABT_INLINE int get_opcode_memory_size(Opcode opcode) {
  assert(static_cast<int>(opcode) < kOpcodeCount);
  init_opcode_info();
  return g_opcode_info[static_cast<size_t>(opcode)].memory_size;
}

/* external kind */

extern const char* g_kind_name[];

static WABT_INLINE const char* get_kind_name(ExternalKind kind) {
  assert(static_cast<int>(kind) < kExternalKindCount);
  return g_kind_name[static_cast<size_t>(kind)];
}

/* reloc */

extern const char* g_reloc_type_name[];

static WABT_INLINE const char* get_reloc_type_name(RelocType reloc) {
  assert(static_cast<int>(reloc) < kRelocTypeCount);
  return g_reloc_type_name[static_cast<size_t>(reloc)];
}

/* type */

static WABT_INLINE const char* get_type_name(Type type) {
  switch (type) {
    case Type::I32:
      return "i32";
    case Type::I64:
      return "i64";
    case Type::F32:
      return "f32";
    case Type::F64:
      return "f64";
    case Type::Anyfunc:
      return "anyfunc";
    case Type::Func:
      return "func";
    case Type::Void:
      return "void";
    case Type::Any:
      return "any";
    default:
      return nullptr;
  }
}

}  // namespace

#endif /* WABT_COMMON_H_ */
