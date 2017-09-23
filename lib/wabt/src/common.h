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

#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "config.h"

#include "src/make-unique.h"
#include "src/result.h"
#include "src/string-view.h"

#define WABT_FATAL(...) fprintf(stderr, __VA_ARGS__), exit(1)
#define WABT_ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define WABT_USE(x) static_cast<void>(x)

#define WABT_PAGE_SIZE 0x10000 /* 64k */
#define WABT_MAX_PAGES 0x10000 /* # of pages that fit in 32-bit address space */
#define WABT_BYTES_TO_PAGES(x) ((x) >> 16)
#define WABT_ALIGN_UP_TO_PAGE(x) \
  (((x) + WABT_PAGE_SIZE - 1) & ~(WABT_PAGE_SIZE - 1))

#define PRIstringview "%.*s"
#define WABT_PRINTF_STRING_VIEW_ARG(x) \
  static_cast<int>((x).length()), (x).data()

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

template <typename Dst, typename Src>
Dst Bitcast(Src value) {
  static_assert(sizeof(Src) == sizeof(Dst), "Bitcast sizes must match.");
  Dst result;
  memcpy(&result, &value, sizeof(result));
  return result;
}

template<typename T>
void ZeroMemory(T& v) {
  WABT_STATIC_ASSERT(std::is_pod<T>::value);
  memset(&v, 0, sizeof(v));
}

// Placement construct
template <typename T, typename... Args>
void Construct(T& placement, Args&&... args) {
  new (&placement) T(std::forward<Args>(args)...);
}

// Placement destruct
template <typename T>
void Destruct(T& placement) {
  placement.~T();
}

// Calls data() on vector, string, etc. but will return nullptr if the
// container is empty.
// TODO(binji): this should probably be removed when there is a more direct way
// to represent a memory slice (e.g. something similar to GSL's span)
template <typename T>
typename T::value_type* DataOrNull(T& container) {
  return container.empty() ? nullptr : container.data();
}

inline std::string WABT_PRINTF_FORMAT(1, 2)
    StringPrintf(const char* format, ...) {
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
  Try,
  Catch,

  First = Func,
  Last = Catch,
};
static const int kLabelTypeCount = WABT_ENUM_COUNT(LabelType);

struct Location {
  enum class Type {
    Text,
    Binary,
  };

  Location() : line(0), first_column(0), last_column(0) {}
  Location(const char* filename, int line, int first_column, int last_column)
      : filename(filename),
        line(line),
        first_column(first_column),
        last_column(last_column) {}
  explicit Location(size_t offset) : offset(offset) {}

  const char* filename = nullptr;
  union {
    // For text files.
    struct {
      int line;
      int first_column;
      int last_column;
    };
    // For binary files.
    struct {
      size_t offset;
    };
  };
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
  FuncIndexLEB = 0,       // e.g. Immediate of call instruction
  TableIndexSLEB = 1,     // e.g. Loading address of function
  TableIndexI32 = 2,      // e.g. Function address in DATA
  MemoryAddressLEB = 3,   // e.g. Memory address in load/store offset immediate
  MemoryAddressSLEB = 4,  // e.g. Memory address in i32.const
  MemoryAddressI32 = 5,   // e.g. Memory address in DATA
  TypeIndexLEB = 6,       // e.g. Immediate type in call_indirect
  GlobalIndexLEB = 7,     // e.g. Immediate of get_global inst

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

enum class LinkingEntryType {
  StackPointer = 1,
  SymbolInfo = 2,
  DataSize = 3,
  DataAlignment = 4,
};

enum class SymbolBinding {
  Global = 0,
  Weak = 1,
  Local = 2,
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
  uint64_t initial = 0;
  uint64_t max = 0;
  bool has_max = false;
};

enum { WABT_USE_NATURAL_ALIGNMENT = 0xFFFFFFFF };

enum class NameSectionSubsection {
  Function = 1,
  Local = 2,
};

Result ReadFile(const char* filename, std::vector<uint8_t>* out_data);

void InitStdio();

/* external kind */

extern const char* g_kind_name[];

static WABT_INLINE const char* GetKindName(ExternalKind kind) {
  assert(static_cast<int>(kind) < kExternalKindCount);
  return g_kind_name[static_cast<size_t>(kind)];
}

/* reloc */

extern const char* g_reloc_type_name[];

static WABT_INLINE const char* GetRelocTypeName(RelocType reloc) {
  assert(static_cast<int>(reloc) < kRelocTypeCount);
  return g_reloc_type_name[static_cast<size_t>(reloc)];
}

/* type */

static WABT_INLINE const char* GetTypeName(Type type) {
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
  }
  WABT_UNREACHABLE;
}

template <typename T>
void ConvertBackslashToSlash(T begin, T end) {
  std::replace(begin, end, '\\', '/');
}

inline void ConvertBackslashToSlash(char* s, size_t length) {
  ConvertBackslashToSlash(s, s + length);
}

inline void ConvertBackslashToSlash(char* s) {
  ConvertBackslashToSlash(s, strlen(s));
}

inline void ConvertBackslashToSlash(std::string* s) {
  ConvertBackslashToSlash(s->begin(), s->end());
}

}  // namespace wabt

#endif // WABT_COMMON_H_
