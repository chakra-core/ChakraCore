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

#include <cassert>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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

#if WITH_EXCEPTIONS
#define WABT_TRY try {
#define WABT_CATCH_BAD_ALLOC_AND_EXIT           \
  }                                             \
  catch (std::bad_alloc&) {                     \
    WABT_FATAL("Memory allocation failure.\n"); \
  }
#else
#define WABT_TRY
#define WABT_CATCH_BAD_ALLOC_AND_EXIT
#endif

#define PRIindex "u"
#define PRIaddress "u"
#define PRIoffset PRIzx

namespace wabt {

typedef uint32_t Index;    // An index into one of the many index spaces.
typedef uint32_t Address;  // An address or size in linear memory.
typedef size_t Offset;     // An offset into a host's file or memory buffer.

static const Address kInvalidAddress = ~0;
static const Index kInvalidIndex = ~0;
static const Offset kInvalidOffset = ~0;

enum class Result {
  Ok,
  Error,
};

#define WABT_SUCCEEDED(x) ((x) == ::wabt::Result::Ok)
#define WABT_FAILED(x) ((x) == ::wabt::Result::Error)

inline std::string WABT_PRINTF_FORMAT(1, 2)
    string_printf(const char* format, ...) {
  va_list args;
  va_list args_copy;
  va_start(args, format);
  va_copy(args_copy, args);
  size_t len = wabt_vsnprintf(nullptr, 0, format, args) + 1;  // For \0.
  std::vector<char> buffer(len);
  va_end(args);
  wabt_vsnprintf(buffer.data(), len, format, args_copy);
  va_end(args_copy);
  return std::string(buffer.data(), len - 1);
}

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

/* matches binary format, do not change */
enum class Type {
  I32 = -0x01,
  I64 = -0x02,
  F32 = -0x03,
  F64 = -0x04,
  Anyfunc = -0x10,
  Func = -0x20,
  Void = -0x40,
  ___ = Void, /* convenient for the opcode table in opcode.h */
  Any = 0,    /* Not actually specified, but useful for type-checking */
};
typedef std::vector<Type> TypeVector;

enum class RelocType {
  FuncIndexLEB = 0,   /* e.g. immediate of call instruction */
  TableIndexSLEB = 1, /* e.g. loading address of function */
  TableIndexI32 = 2,  /* e.g. function address in DATA */
  GlobalAddressLEB = 3,
  GlobalAddressSLEB = 4,
  GlobalAddressI32 = 5,
  TypeIndexLEB = 6, /* e.g immediate type in call_indirect */
  GlobalIndexLEB = 7, /* e.g immediate of get_global inst */

  First = FuncIndexLEB,
  Last = GlobalIndexLEB,
};
static const int kRelocTypeCount = WABT_ENUM_COUNT(RelocType);

struct Reloc {
  Reloc(RelocType, size_t offset, Index index, int32_t addend = 0);

  RelocType type;
  size_t offset;
  Index index;
  int32_t addend;
};

/* matches binary format, do not change */
enum class ExternalKind {
  Func = 0,
  Table = 1,
  Memory = 2,
  Global = 3,
  Except = 4,

  First = Func,
  Last = Except,
};
static const int kExternalKindCount = WABT_ENUM_COUNT(ExternalKind);

struct Limits {
  uint64_t initial;
  uint64_t max;
  bool has_max;
};

enum { WABT_USE_NATURAL_ALIGNMENT = 0xFFFFFFFF };

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
  char* new_data = new char[str.length];
  memcpy(new_data, str.start, str.length);
  result.start = new_data;
  result.length = str.length;
  return result;
}

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

void init_stdio();

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
