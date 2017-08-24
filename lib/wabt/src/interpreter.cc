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

#include "interpreter.h"

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cmath>
#include <limits>
#include <type_traits>
#include <vector>

#include "stream.h"

namespace wabt {
namespace interpreter {

static const char* s_opcode_name[] = {
#define WABT_OPCODE(rtype, type1, type2, mem_size, prefix, code, NAME, text) \
  text,
#include "interpreter-opcode.def"
#undef WABT_OPCODE

  "<invalid>",
};

#define CHECK_RESULT(expr)  \
  do {                      \
    if (WABT_FAILED(expr))  \
      return Result::Error; \
  } while (0)

// Differs from CHECK_RESULT since it can return different traps, not just
// Error. Also uses __VA_ARGS__ so templates can be passed without surrounding
// parentheses.
#define CHECK_TRAP(...)            \
  do {                             \
    Result result = (__VA_ARGS__); \
    if (result != Result::Ok) {    \
      return result;               \
    }                              \
  } while (0)

static const char* GetOpcodeName(Opcode opcode) {
  int value = static_cast<int>(opcode);
  return value < static_cast<int>(WABT_ARRAY_SIZE(s_opcode_name))
             ? s_opcode_name[value]
             : "<invalid>";
}

Environment::Environment() : istream_(new OutputBuffer()) {}

Index Environment::FindModuleIndex(string_view name) const {
  auto iter = module_bindings_.find(name.to_string());
  if (iter == module_bindings_.end())
    return kInvalidIndex;
  return iter->second.index;
}

Module* Environment::FindModule(string_view name) {
  Index index = FindModuleIndex(name);
  return index == kInvalidIndex ? nullptr : modules_[index].get();
}

Module* Environment::FindRegisteredModule(string_view name) {
  auto iter = registered_module_bindings_.find(name.to_string());
  if (iter == registered_module_bindings_.end())
    return nullptr;
  return modules_[iter->second.index].get();
}

Thread::Options::Options(uint32_t value_stack_size,
                         uint32_t call_stack_size,
                         IstreamOffset pc)
    : value_stack_size(value_stack_size),
      call_stack_size(call_stack_size),
      pc(pc) {}

Thread::Thread(Environment* env, const Options& options)
    : env_(env),
      value_stack_(options.value_stack_size),
      call_stack_(options.call_stack_size),
      value_stack_top_(value_stack_.data()),
      value_stack_end_(value_stack_.data() + value_stack_.size()),
      call_stack_top_(call_stack_.data()),
      call_stack_end_(call_stack_.data() + call_stack_.size()),
      pc_(options.pc) {}

FuncSignature::FuncSignature(Index param_count,
                             Type* param_types,
                             Index result_count,
                             Type* result_types)
    : param_types(param_types, param_types + param_count),
      result_types(result_types, result_types + result_count) {}

Import::Import() : kind(ExternalKind::Func) {
  func.sig_index = kInvalidIndex;
}

Import::Import(Import&& other) {
  *this = std::move(other);
}

Import& Import::operator=(Import&& other) {
  kind = other.kind;
  module_name = std::move(other.module_name);
  field_name = std::move(other.field_name);
  switch (kind) {
    case ExternalKind::Func:
      func.sig_index = other.func.sig_index;
      break;
    case ExternalKind::Table:
      table.limits = other.table.limits;
      break;
    case ExternalKind::Memory:
      memory.limits = other.memory.limits;
      break;
    case ExternalKind::Global:
      global.type = other.global.type;
      global.mutable_ = other.global.mutable_;
      break;
    case ExternalKind::Except:
      // TODO(karlschimpf) Define
      WABT_FATAL("Import::operator=() not implemented for exceptions");
      break;
  }
  return *this;
}

Module::Module(bool is_host)
    : memory_index(kInvalidIndex),
      table_index(kInvalidIndex),
      is_host(is_host) {}

Module::Module(string_view name, bool is_host)
    : name(name.to_string()),
      memory_index(kInvalidIndex),
      table_index(kInvalidIndex),
      is_host(is_host) {}

Export* Module::GetExport(string_view name) {
  int field_index = export_bindings.FindIndex(name);
  if (field_index < 0)
    return nullptr;
  return &exports[field_index];
}

DefinedModule::DefinedModule()
    : Module(false),
      start_func_index(kInvalidIndex),
      istream_start(kInvalidIstreamOffset),
      istream_end(kInvalidIstreamOffset) {}

HostModule::HostModule(string_view name) : Module(name, true) {}

Environment::MarkPoint Environment::Mark() {
  MarkPoint mark;
  mark.modules_size = modules_.size();
  mark.sigs_size = sigs_.size();
  mark.funcs_size = funcs_.size();
  mark.memories_size = memories_.size();
  mark.tables_size = tables_.size();
  mark.globals_size = globals_.size();
  mark.istream_size = istream_->data.size();
  return mark;
}

void Environment::ResetToMarkPoint(const MarkPoint& mark) {
  // Destroy entries in the binding hash.
  for (size_t i = mark.modules_size; i < modules_.size(); ++i) {
    std::string name = modules_[i]->name;
    if (!name.empty())
      module_bindings_.erase(name);
  }

  // registered_module_bindings_ maps from an arbitrary name to a module index,
  // so we have to iterate through the entire table to find entries to remove.
  auto iter = registered_module_bindings_.begin();
  while (iter != registered_module_bindings_.end()) {
    if (iter->second.index >= mark.modules_size)
      iter = registered_module_bindings_.erase(iter);
    else
      ++iter;
  }

  modules_.erase(modules_.begin() + mark.modules_size, modules_.end());
  sigs_.erase(sigs_.begin() + mark.sigs_size, sigs_.end());
  funcs_.erase(funcs_.begin() + mark.funcs_size, funcs_.end());
  memories_.erase(memories_.begin() + mark.memories_size, memories_.end());
  tables_.erase(tables_.begin() + mark.tables_size, tables_.end());
  globals_.erase(globals_.begin() + mark.globals_size, globals_.end());
  istream_->data.resize(mark.istream_size);
}

HostModule* Environment::AppendHostModule(string_view name) {
  HostModule* module = new HostModule(name);
  modules_.emplace_back(module);
  registered_module_bindings_.emplace(name.to_string(),
                                      Binding(modules_.size() - 1));
  return module;
}

Result Thread::PushArgs(const FuncSignature* sig,
                        const std::vector<TypedValue>& args) {
  if (sig->param_types.size() != args.size())
    return interpreter::Result::ArgumentTypeMismatch;

  for (size_t i = 0; i < sig->param_types.size(); ++i) {
    if (sig->param_types[i] != args[i].type)
      return interpreter::Result::ArgumentTypeMismatch;

    interpreter::Result iresult = Push(args[i].value);
    if (iresult != interpreter::Result::Ok) {
      value_stack_top_ = value_stack_.data();
      return iresult;
    }
  }
  return interpreter::Result::Ok;
}

void Thread::CopyResults(const FuncSignature* sig,
                         std::vector<TypedValue>* out_results) {
  size_t expected_results = sig->result_types.size();
  size_t value_stack_depth = value_stack_top_ - value_stack_.data();
  WABT_USE(value_stack_depth);
  assert(expected_results == value_stack_depth);

  out_results->clear();
  for (size_t i = 0; i < expected_results; ++i)
    out_results->emplace_back(sig->result_types[i], value_stack_[i]);
}

template <typename Dst, typename Src>
Dst Bitcast(Src value) {
  static_assert(sizeof(Src) == sizeof(Dst), "Bitcast sizes must match.");
  Dst result;
  memcpy(&result, &value, sizeof(result));
  return result;
}

uint32_t ToRep(bool x) { return x ? 1 : 0; }
uint32_t ToRep(uint32_t x) { return x; }
uint64_t ToRep(uint64_t x) { return x; }
uint32_t ToRep(int32_t x) { return Bitcast<uint32_t>(x); }
uint64_t ToRep(int64_t x) { return Bitcast<uint64_t>(x); }
uint32_t ToRep(float x) { return Bitcast<uint32_t>(x); }
uint64_t ToRep(double x) { return Bitcast<uint64_t>(x); }

template <typename Dst, typename Src>
Dst FromRep(Src x);

template <>
uint32_t FromRep<uint32_t>(uint32_t x) { return x; }
template <>
uint64_t FromRep<uint64_t>(uint64_t x) { return x; }
template <>
int32_t FromRep<int32_t>(uint32_t x) { return Bitcast<int32_t>(x); }
template <>
int64_t FromRep<int64_t>(uint64_t x) { return Bitcast<int64_t>(x); }
template <>
float FromRep<float>(uint32_t x) { return Bitcast<float>(x); }
template <>
double FromRep<double>(uint64_t x) { return Bitcast<double>(x); }

template <typename T>
struct FloatTraits;

template <typename R, typename T>
bool IsConversionInRange(ValueTypeRep<T> bits);

/* 3 32222222 222...00
 * 1 09876543 210...10
 * -------------------
 * 0 00000000 000...00 => 0x00000000 => 0
 * 0 10011101 111...11 => 0x4effffff => 2147483520                  (~INT32_MAX)
 * 0 10011110 000...00 => 0x4f000000 => 2147483648
 * 0 10011110 111...11 => 0x4f7fffff => 4294967040                 (~UINT32_MAX)
 * 0 10111110 111...11 => 0x5effffff => 9223371487098961920         (~INT64_MAX)
 * 0 10111110 000...00 => 0x5f000000 => 9223372036854775808
 * 0 10111111 111...11 => 0x5f7fffff => 18446742974197923840       (~UINT64_MAX)
 * 0 10111111 000...00 => 0x5f800000 => 18446744073709551616
 * 0 11111111 000...00 => 0x7f800000 => inf
 * 0 11111111 000...01 => 0x7f800001 => nan(0x1)
 * 0 11111111 111...11 => 0x7fffffff => nan(0x7fffff)
 * 1 00000000 000...00 => 0x80000000 => -0
 * 1 01111110 111...11 => 0xbf7fffff => -1 + ulp      (~UINT32_MIN, ~UINT64_MIN)
 * 1 01111111 000...00 => 0xbf800000 => -1
 * 1 10011110 000...00 => 0xcf000000 => -2147483648                  (INT32_MIN)
 * 1 10111110 000...00 => 0xdf000000 => -9223372036854775808         (INT64_MIN)
 * 1 11111111 000...00 => 0xff800000 => -inf
 * 1 11111111 000...01 => 0xff800001 => -nan(0x1)
 * 1 11111111 111...11 => 0xffffffff => -nan(0x7fffff)
 */

template <>
struct FloatTraits<float> {
  static const uint32_t kMax = 0x7f7fffffU;
  static const uint32_t kInf = 0x7f800000U;
  static const uint32_t kNegMax = 0xff7fffffU;
  static const uint32_t kNegInf = 0xff800000U;
  static const uint32_t kNegOne = 0xbf800000U;
  static const uint32_t kNegZero =0x80000000U;
  static const uint32_t kQuietNan = 0x7fc00000U;
  static const uint32_t kQuietNegNan = 0xffc00000U;
  static const uint32_t kQuietNanBit = 0x00400000U;
  static const int kSigBits = 23;
  static const uint32_t kSigMask = 0x7fffff;
  static const uint32_t kSignMask = 0x80000000U;

  static bool IsNan(uint32_t bits) {
    return (bits > kInf && bits < kNegZero) || (bits > kNegInf);
  }

  static bool IsZero(uint32_t bits) {
    return bits == 0 || bits == kNegZero;
  }

  static bool IsCanonicalNan(uint32_t bits) {
    return bits == kQuietNan || bits == kQuietNegNan;
  }

  static bool IsArithmeticNan(uint32_t bits) {
    return (bits & kQuietNan) == kQuietNan;
  }
};

bool IsCanonicalNan(uint32_t bits) {
  return FloatTraits<float>::IsCanonicalNan(bits);
}

bool IsArithmeticNan(uint32_t bits) {
  return FloatTraits<float>::IsArithmeticNan(bits);
}

template <>
bool IsConversionInRange<int32_t, float>(uint32_t bits) {
  return (bits < 0x4f000000U) ||
         (bits >= FloatTraits<float>::kNegZero && bits <= 0xcf000000U);
}

template <>
bool IsConversionInRange<int64_t, float>(uint32_t bits) {
  return (bits < 0x5f000000U) ||
         (bits >= FloatTraits<float>::kNegZero && bits <= 0xdf000000U);
}

template <>
bool IsConversionInRange<uint32_t, float>(uint32_t bits) {
  return (bits < 0x4f800000U) || (bits >= FloatTraits<float>::kNegZero &&
                                  bits < FloatTraits<float>::kNegOne);
}

template <>
bool IsConversionInRange<uint64_t, float>(uint32_t bits) {
  return (bits < 0x5f800000U) || (bits >= FloatTraits<float>::kNegZero &&
                                  bits < FloatTraits<float>::kNegOne);
}

/*
 * 6 66655555555 5544..2..222221...000
 * 3 21098765432 1098..9..432109...210
 * -----------------------------------
 * 0 00000000000 0000..0..000000...000 0x0000000000000000 => 0
 * 0 10000011101 1111..1..111000...000 0x41dfffffffc00000 => 2147483647           (INT32_MAX)
 * 0 10000011110 1111..1..111100...000 0x41efffffffe00000 => 4294967295           (UINT32_MAX)
 * 0 10000111101 1111..1..111111...111 0x43dfffffffffffff => 9223372036854774784  (~INT64_MAX)
 * 0 10000111110 0000..0..000000...000 0x43e0000000000000 => 9223372036854775808
 * 0 10000111110 1111..1..111111...111 0x43efffffffffffff => 18446744073709549568 (~UINT64_MAX)
 * 0 10000111111 0000..0..000000...000 0x43f0000000000000 => 18446744073709551616
 * 0 10001111110 1111..1..000000...000 0x47efffffe0000000 => 3.402823e+38         (FLT_MAX)
 * 0 11111111111 0000..0..000000...000 0x7ff0000000000000 => inf
 * 0 11111111111 0000..0..000000...001 0x7ff0000000000001 => nan(0x1)
 * 0 11111111111 1111..1..111111...111 0x7fffffffffffffff => nan(0xfff...)
 * 1 00000000000 0000..0..000000...000 0x8000000000000000 => -0
 * 1 01111111110 1111..1..111111...111 0xbfefffffffffffff => -1 + ulp             (~UINT32_MIN, ~UINT64_MIN)
 * 1 01111111111 0000..0..000000...000 0xbff0000000000000 => -1
 * 1 10000011110 0000..0..000000...000 0xc1e0000000000000 => -2147483648          (INT32_MIN)
 * 1 10000111110 0000..0..000000...000 0xc3e0000000000000 => -9223372036854775808 (INT64_MIN)
 * 1 10001111110 1111..1..000000...000 0xc7efffffe0000000 => -3.402823e+38        (-FLT_MAX)
 * 1 11111111111 0000..0..000000...000 0xfff0000000000000 => -inf
 * 1 11111111111 0000..0..000000...001 0xfff0000000000001 => -nan(0x1)
 * 1 11111111111 1111..1..111111...111 0xffffffffffffffff => -nan(0xfff...)
 */

template <>
struct FloatTraits<double> {
  static const uint64_t kInf = 0x7ff0000000000000ULL;
  static const uint64_t kNegInf = 0xfff0000000000000ULL;
  static const uint64_t kNegOne = 0xbff0000000000000ULL;
  static const uint64_t kNegZero = 0x8000000000000000ULL;
  static const uint64_t kQuietNan = 0x7ff8000000000000ULL;
  static const uint64_t kQuietNegNan = 0xfff8000000000000ULL;
  static const uint64_t kQuietNanBit = 0x0008000000000000ULL;
  static const int kSigBits = 52;
  static const uint64_t kSigMask = 0xfffffffffffffULL;
  static const uint64_t kSignMask = 0x8000000000000000ULL;

  static bool IsNan(uint64_t bits) {
    return (bits > kInf && bits < kNegZero) || (bits > kNegInf);
  }

  static bool IsZero(uint64_t bits) {
    return bits == 0 || bits == kNegZero;
  }

  static bool IsCanonicalNan(uint64_t bits) {
    return bits == kQuietNan || bits == kQuietNegNan;
  }

  static bool IsArithmeticNan(uint64_t bits) {
    return (bits & kQuietNan) == kQuietNan;
  }
};

bool IsCanonicalNan(uint64_t bits) {
  return FloatTraits<double>::IsCanonicalNan(bits);
}

bool IsArithmeticNan(uint64_t bits) {
  return FloatTraits<double>::IsArithmeticNan(bits);
}

template <>
bool IsConversionInRange<int32_t, double>(uint64_t bits) {
  return (bits <= 0x41dfffffffc00000ULL) ||
         (bits >= FloatTraits<double>::kNegZero &&
          bits <= 0xc1e0000000000000ULL);
}

template <>
bool IsConversionInRange<int64_t, double>(uint64_t bits) {
  return (bits < 0x43e0000000000000ULL) ||
         (bits >= FloatTraits<double>::kNegZero &&
          bits <= 0xc3e0000000000000ULL);
}

template <>
bool IsConversionInRange<uint32_t, double>(uint64_t bits) {
  return (bits <= 0x41efffffffe00000ULL) ||
         (bits >= FloatTraits<double>::kNegZero &&
          bits < FloatTraits<double>::kNegOne);
}

template <>
bool IsConversionInRange<uint64_t, double>(uint64_t bits) {
  return (bits < 0x43f0000000000000ULL) ||
         (bits >= FloatTraits<double>::kNegZero &&
          bits < FloatTraits<double>::kNegOne);
}

template <>
bool IsConversionInRange<float, double>(uint64_t bits) {
  return (bits <= 0x47efffffe0000000ULL) ||
         (bits >= FloatTraits<double>::kNegZero &&
          bits <= 0xc7efffffe0000000ULL);
}

// The WebAssembly rounding mode means that these values (which are > F32_MAX)
// should be rounded to F32_MAX and not set to infinity. Unfortunately, UBSAN
// complains that the value is not representable as a float, so we'll special
// case them.
bool IsInRangeF64DemoteF32RoundToF32Max(uint64_t bits) {
  return bits > 0x47efffffe0000000ULL && bits < 0x47effffff0000000ULL;
}

bool IsInRangeF64DemoteF32RoundToNegF32Max(uint64_t bits) {
  return bits > 0xc7efffffe0000000ULL && bits < 0xc7effffff0000000ULL;
}

template <typename T, typename MemType> struct ExtendMemType;
template<> struct ExtendMemType<uint32_t, uint8_t> { typedef uint32_t type; };
template<> struct ExtendMemType<uint32_t, int8_t> { typedef int32_t type; };
template<> struct ExtendMemType<uint32_t, uint16_t> { typedef uint32_t type; };
template<> struct ExtendMemType<uint32_t, int16_t> { typedef int32_t type; };
template<> struct ExtendMemType<uint32_t, uint32_t> { typedef uint32_t type; };
template<> struct ExtendMemType<uint32_t, int32_t> { typedef int32_t type; };
template<> struct ExtendMemType<uint64_t, uint8_t> { typedef uint64_t type; };
template<> struct ExtendMemType<uint64_t, int8_t> { typedef int64_t type; };
template<> struct ExtendMemType<uint64_t, uint16_t> { typedef uint64_t type; };
template<> struct ExtendMemType<uint64_t, int16_t> { typedef int64_t type; };
template<> struct ExtendMemType<uint64_t, uint32_t> { typedef uint64_t type; };
template<> struct ExtendMemType<uint64_t, int32_t> { typedef int64_t type; };
template<> struct ExtendMemType<uint64_t, uint64_t> { typedef uint64_t type; };
template<> struct ExtendMemType<uint64_t, int64_t> { typedef int64_t type; };
template<> struct ExtendMemType<float, float> { typedef float type; };
template<> struct ExtendMemType<double, double> { typedef double type; };

template <typename T, typename MemType> struct WrapMemType;
template<> struct WrapMemType<uint32_t, uint8_t> { typedef uint8_t type; };
template<> struct WrapMemType<uint32_t, uint16_t> { typedef uint16_t type; };
template<> struct WrapMemType<uint32_t, uint32_t> { typedef uint32_t type; };
template<> struct WrapMemType<uint64_t, uint8_t> { typedef uint8_t type; };
template<> struct WrapMemType<uint64_t, uint16_t> { typedef uint16_t type; };
template<> struct WrapMemType<uint64_t, uint32_t> { typedef uint32_t type; };
template<> struct WrapMemType<uint64_t, uint64_t> { typedef uint64_t type; };
template<> struct WrapMemType<float, float> { typedef uint32_t type; };
template<> struct WrapMemType<double, double> { typedef uint64_t type; };

template <typename T>
Value MakeValue(ValueTypeRep<T>);

template <>
Value MakeValue<uint32_t>(uint32_t v) {
  Value result;
  result.i32 = v;
  return result;
}

template <>
Value MakeValue<int32_t>(uint32_t v) {
  Value result;
  result.i32 = v;
  return result;
}

template <>
Value MakeValue<uint64_t>(uint64_t v) {
  Value result;
  result.i64 = v;
  return result;
}

template <>
Value MakeValue<int64_t>(uint64_t v) {
  Value result;
  result.i64 = v;
  return result;
}

template <>
Value MakeValue<float>(uint32_t v) {
  Value result;
  result.f32_bits = v;
  return result;
}

template <>
Value MakeValue<double>(uint64_t v) {
  Value result;
  result.f64_bits = v;
  return result;
}

template <typename T> ValueTypeRep<T> GetValue(Value);
template<> uint32_t GetValue<int32_t>(Value v) { return v.i32; }
template<> uint32_t GetValue<uint32_t>(Value v) { return v.i32; }
template<> uint64_t GetValue<int64_t>(Value v) { return v.i64; }
template<> uint64_t GetValue<uint64_t>(Value v) { return v.i64; }
template<> uint32_t GetValue<float>(Value v) { return v.f32_bits; }
template<> uint64_t GetValue<double>(Value v) { return v.f64_bits; }

#define TRAP(type) return Result::Trap##type
#define TRAP_UNLESS(cond, type) TRAP_IF(!(cond), type)
#define TRAP_IF(cond, type)  \
  do {                       \
    if (WABT_UNLIKELY(cond)) \
      TRAP(type);            \
  } while (0)

#define CHECK_STACK() \
  TRAP_IF(value_stack_top_ >= value_stack_end_, ValueStackExhausted)

#define PUSH_NEG_1_AND_BREAK_IF(cond) \
  if (WABT_UNLIKELY(cond)) {          \
    CHECK_TRAP(Push<int32_t>(-1));  \
    break;                            \
  }

#define GOTO(offset) pc = &istream[offset]

static WABT_INLINE uint32_t read_u32_at(const uint8_t* pc) {
  uint32_t result;
  memcpy(&result, pc, sizeof(uint32_t));
  return result;
}

static WABT_INLINE uint32_t read_u32(const uint8_t** pc) {
  uint32_t result = read_u32_at(*pc);
  *pc += sizeof(uint32_t);
  return result;
}

static WABT_INLINE uint64_t read_u64_at(const uint8_t* pc) {
  uint64_t result;
  memcpy(&result, pc, sizeof(uint64_t));
  return result;
}

static WABT_INLINE uint64_t read_u64(const uint8_t** pc) {
  uint64_t result = read_u64_at(*pc);
  *pc += sizeof(uint64_t);
  return result;
}

static WABT_INLINE void read_table_entry_at(const uint8_t* pc,
                                            IstreamOffset* out_offset,
                                            uint32_t* out_drop,
                                            uint8_t* out_keep) {
  *out_offset = read_u32_at(pc + WABT_TABLE_ENTRY_OFFSET_OFFSET);
  *out_drop = read_u32_at(pc + WABT_TABLE_ENTRY_DROP_OFFSET);
  *out_keep = *(pc + WABT_TABLE_ENTRY_KEEP_OFFSET);
}

Memory* Thread::ReadMemory(const uint8_t** pc) {
  Index memory_index = read_u32(pc);
  return &env_->memories_[memory_index];
}

Value& Thread::Top() {
  return Pick(1);
}

Value& Thread::Pick(Index depth) {
  return *(value_stack_top_ - depth);
}

Result Thread::Push(Value value) {
  CHECK_STACK();
  *value_stack_top_++ = value;
  return Result::Ok;
}

Value Thread::Pop() {
  return *--value_stack_top_;
}

template <typename T>
Result Thread::Push(T value) {
  return PushRep<T>(ToRep(value));
}

template <typename T>
T Thread::Pop() {
  return FromRep<T>(PopRep<T>());
}

template <typename T>
Result Thread::PushRep(ValueTypeRep<T> value) {
  return Push(MakeValue<T>(value));
}

template <typename T>
ValueTypeRep<T> Thread::PopRep() {
  return GetValue<T>(Pop());
}

void Thread::DropKeep(uint32_t drop_count, uint8_t keep_count) {
  assert(keep_count <= 1);
  if (keep_count == 1)
    Pick(drop_count + 1) = Top();
  value_stack_top_ -= drop_count;
}

Result Thread::PushCall(const uint8_t* pc) {
  TRAP_IF(call_stack_top_ >= call_stack_end_, CallStackExhausted);
  *call_stack_top_++ = pc - GetIstream();
  return Result::Ok;
}

IstreamOffset Thread::PopCall() {
  return *--call_stack_top_;
}

template <typename MemType, typename ResultType>
Result Thread::Load(const uint8_t** pc) {
  typedef typename ExtendMemType<ResultType, MemType>::type ExtendedType;
  static_assert(std::is_floating_point<MemType>::value ==
                    std::is_floating_point<ExtendedType>::value,
                "Extended type should be float iff MemType is float");

  Memory* memory = ReadMemory(pc);
  uint64_t offset = static_cast<uint64_t>(Pop<uint32_t>()) + read_u32(pc);
  MemType value;
  TRAP_IF(offset + sizeof(value) > memory->data.size(),
          MemoryAccessOutOfBounds);
  void* src = memory->data.data() + static_cast<IstreamOffset>(offset);
  memcpy(&value, src, sizeof(value));
  return Push<ResultType>(static_cast<ExtendedType>(value));
}

template <typename MemType, typename ResultType>
Result Thread::Store(const uint8_t** pc) {
  typedef typename WrapMemType<ResultType, MemType>::type WrappedType;
  Memory* memory = ReadMemory(pc);
  WrappedType value = PopRep<ResultType>();
  uint64_t offset = static_cast<uint64_t>(Pop<uint32_t>()) + read_u32(pc);
  TRAP_IF(offset + sizeof(value) > memory->data.size(),
          MemoryAccessOutOfBounds);
  void* dst = memory->data.data() + static_cast<IstreamOffset>(offset);
  memcpy(dst, &value, sizeof(value));
  return Result::Ok;
}

template <typename R, typename T>
Result Thread::Unop(UnopFunc<R, T> func) {
  auto value = PopRep<T>();
  return PushRep<R>(func(value));
}

template <typename R, typename T>
Result Thread::UnopTrap(UnopTrapFunc<R, T> func) {
  auto value = PopRep<T>();
  ValueTypeRep<R> result_value;
  CHECK_TRAP(func(value, &result_value));
  return PushRep<R>(result_value);
}

template <typename R, typename T>
Result Thread::Binop(BinopFunc<R, T> func) {
  auto rhs_rep = PopRep<T>();
  auto lhs_rep = PopRep<T>();
  return PushRep<R>(func(lhs_rep, rhs_rep));
}

template <typename R, typename T>
Result Thread::BinopTrap(BinopTrapFunc<R, T> func) {
  auto rhs_rep = PopRep<T>();
  auto lhs_rep = PopRep<T>();
  ValueTypeRep<R> result_value;
  CHECK_TRAP(func(lhs_rep, rhs_rep, &result_value));
  return PushRep<R>(result_value);
}

// {i,f}{32,64}.add
template <typename T>
ValueTypeRep<T> Add(ValueTypeRep<T> lhs_rep, ValueTypeRep<T> rhs_rep) {
  return ToRep(FromRep<T>(lhs_rep) + FromRep<T>(rhs_rep));
}

// {i,f}{32,64}.sub
template <typename T>
ValueTypeRep<T> Sub(ValueTypeRep<T> lhs_rep, ValueTypeRep<T> rhs_rep) {
  return ToRep(FromRep<T>(lhs_rep) - FromRep<T>(rhs_rep));
}

// {i,f}{32,64}.mul
template <typename T>
ValueTypeRep<T> Mul(ValueTypeRep<T> lhs_rep, ValueTypeRep<T> rhs_rep) {
  return ToRep(FromRep<T>(lhs_rep) * FromRep<T>(rhs_rep));
}

// i{32,64}.{div,rem}_s are special-cased because they trap when dividing the
// max signed value by -1. The modulo operation on x86 uses the same
// instruction to generate the quotient and the remainder.
template <typename T>
bool IsNormalDivRemS(T lhs, T rhs) {
  static_assert(std::is_signed<T>::value, "T should be a signed type.");
  return !(lhs == std::numeric_limits<T>::min() && rhs == -1);
}

// i{32,64}.div_s
template <typename T>
Result IntDivS(ValueTypeRep<T> lhs_rep,
               ValueTypeRep<T> rhs_rep,
               ValueTypeRep<T>* out_result) {
  auto lhs = FromRep<T>(lhs_rep);
  auto rhs = FromRep<T>(rhs_rep);
  TRAP_IF(rhs == 0, IntegerDivideByZero);
  TRAP_UNLESS(IsNormalDivRemS(lhs, rhs), IntegerOverflow);
  *out_result = ToRep(lhs / rhs);
  return Result::Ok;
}

// i{32,64}.rem_s
template <typename T>
Result IntRemS(ValueTypeRep<T> lhs_rep,
               ValueTypeRep<T> rhs_rep,
               ValueTypeRep<T>* out_result) {
  auto lhs = FromRep<T>(lhs_rep);
  auto rhs = FromRep<T>(rhs_rep);
  TRAP_IF(rhs == 0, IntegerDivideByZero);
  if (WABT_LIKELY(IsNormalDivRemS(lhs, rhs))) {
    *out_result = ToRep(lhs % rhs);
  } else {
    *out_result = 0;
  }
  return Result::Ok;
}

// i{32,64}.div_u
template <typename T>
Result IntDivU(ValueTypeRep<T> lhs_rep,
               ValueTypeRep<T> rhs_rep,
               ValueTypeRep<T>* out_result) {
  auto lhs = FromRep<T>(lhs_rep);
  auto rhs = FromRep<T>(rhs_rep);
  TRAP_IF(rhs == 0, IntegerDivideByZero);
  *out_result = ToRep(lhs / rhs);
  return Result::Ok;
}

// i{32,64}.rem_u
template <typename T>
Result IntRemU(ValueTypeRep<T> lhs_rep,
               ValueTypeRep<T> rhs_rep,
               ValueTypeRep<T>* out_result) {
  auto lhs = FromRep<T>(lhs_rep);
  auto rhs = FromRep<T>(rhs_rep);
  TRAP_IF(rhs == 0, IntegerDivideByZero);
  *out_result = ToRep(lhs % rhs);
  return Result::Ok;
}

// f{32,64}.div
template <typename T>
ValueTypeRep<T> FloatDiv(ValueTypeRep<T> lhs_rep, ValueTypeRep<T> rhs_rep) {
  typedef FloatTraits<T> Traits;
  ValueTypeRep<T> result;
  if (WABT_UNLIKELY(Traits::IsZero(rhs_rep))) {
    if (Traits::IsNan(lhs_rep)) {
      result = lhs_rep | Traits::kQuietNan;
    } else if (Traits::IsZero(lhs_rep)) {
      result = Traits::kQuietNan;
    } else {
      auto sign = (lhs_rep & Traits::kSignMask) ^ (rhs_rep & Traits::kSignMask);
      result = sign | Traits::kInf;
    }
  } else {
    result = ToRep(FromRep<T>(lhs_rep) / FromRep<T>(rhs_rep));
  }
  return result;
}

// i{32,64}.and
template <typename T>
ValueTypeRep<T> IntAnd(ValueTypeRep<T> lhs_rep, ValueTypeRep<T> rhs_rep) {
  return ToRep(FromRep<T>(lhs_rep) & FromRep<T>(rhs_rep));
}

// i{32,64}.or
template <typename T>
ValueTypeRep<T> IntOr(ValueTypeRep<T> lhs_rep, ValueTypeRep<T> rhs_rep) {
  return ToRep(FromRep<T>(lhs_rep) | FromRep<T>(rhs_rep));
}

// i{32,64}.xor
template <typename T>
ValueTypeRep<T> IntXor(ValueTypeRep<T> lhs_rep, ValueTypeRep<T> rhs_rep) {
  return ToRep(FromRep<T>(lhs_rep) ^ FromRep<T>(rhs_rep));
}

// i{32,64}.shl
template <typename T>
ValueTypeRep<T> IntShl(ValueTypeRep<T> lhs_rep, ValueTypeRep<T> rhs_rep) {
  const int mask = sizeof(T) * 8 - 1;
  return ToRep(FromRep<T>(lhs_rep) << (FromRep<T>(rhs_rep) & mask));
}

// i{32,64}.shr_{s,u}
template <typename T>
ValueTypeRep<T> IntShr(ValueTypeRep<T> lhs_rep, ValueTypeRep<T> rhs_rep) {
  const int mask = sizeof(T) * 8 - 1;
  return ToRep(FromRep<T>(lhs_rep) >> (FromRep<T>(rhs_rep) & mask));
}

// i{32,64}.rotl
template <typename T>
ValueTypeRep<T> IntRotl(ValueTypeRep<T> lhs_rep, ValueTypeRep<T> rhs_rep) {
  const int mask = sizeof(T) * 8 - 1;
  int amount = FromRep<T>(rhs_rep) & mask;
  auto lhs = FromRep<T>(lhs_rep);
  if (amount == 0) {
    return ToRep(lhs);
  } else {
    return ToRep((lhs << amount) | (lhs >> (mask + 1 - amount)));
  }
}

// i{32,64}.rotr
template <typename T>
ValueTypeRep<T> IntRotr(ValueTypeRep<T> lhs_rep, ValueTypeRep<T> rhs_rep) {
  const int mask = sizeof(T) * 8 - 1;
  int amount = FromRep<T>(rhs_rep) & mask;
  auto lhs = FromRep<T>(lhs_rep);
  if (amount == 0) {
    return ToRep(lhs);
  } else {
    return ToRep((lhs >> amount) | (lhs << (mask + 1 - amount)));
  }
}

// i{32,64}.eqz
template <typename R, typename T>
ValueTypeRep<R> IntEqz(ValueTypeRep<T> v_rep) {
  return ToRep(v_rep == 0);
}

// f{32,64}.abs
template <typename T>
ValueTypeRep<T> FloatAbs(ValueTypeRep<T> v_rep) {
  return v_rep & ~FloatTraits<T>::kSignMask;
}

// f{32,64}.neg
template <typename T>
ValueTypeRep<T> FloatNeg(ValueTypeRep<T> v_rep) {
  return v_rep ^ FloatTraits<T>::kSignMask;
}

// f{32,64}.ceil
template <typename T>
ValueTypeRep<T> FloatCeil(ValueTypeRep<T> v_rep) {
  auto result = ToRep(std::ceil(FromRep<T>(v_rep)));
  if (WABT_UNLIKELY(FloatTraits<T>::IsNan(result))) {
    result |= FloatTraits<T>::kQuietNanBit;
  }
  return result;
}

// f{32,64}.floor
template <typename T>
ValueTypeRep<T> FloatFloor(ValueTypeRep<T> v_rep) {
  auto result = ToRep(std::floor(FromRep<T>(v_rep)));
  if (WABT_UNLIKELY(FloatTraits<T>::IsNan(result))) {
    result |= FloatTraits<T>::kQuietNanBit;
  }
  return result;
}

// f{32,64}.trunc
template <typename T>
ValueTypeRep<T> FloatTrunc(ValueTypeRep<T> v_rep) {
  auto result = ToRep(std::trunc(FromRep<T>(v_rep)));
  if (WABT_UNLIKELY(FloatTraits<T>::IsNan(result))) {
    result |= FloatTraits<T>::kQuietNanBit;
  }
  return result;
}

// f{32,64}.nearest
template <typename T>
ValueTypeRep<T> FloatNearest(ValueTypeRep<T> v_rep) {
  auto result = ToRep(std::nearbyint(FromRep<T>(v_rep)));
  if (WABT_UNLIKELY(FloatTraits<T>::IsNan(result))) {
    result |= FloatTraits<T>::kQuietNanBit;
  }
  return result;
}

// f{32,64}.sqrt
template <typename T>
ValueTypeRep<T> FloatSqrt(ValueTypeRep<T> v_rep) {
  auto result = ToRep(std::sqrt(FromRep<T>(v_rep)));
  if (WABT_UNLIKELY(FloatTraits<T>::IsNan(result))) {
    result |= FloatTraits<T>::kQuietNanBit;
  }
  return result;
}

// f{32,64}.min
template <typename T>
ValueTypeRep<T> FloatMin(ValueTypeRep<T> lhs_rep, ValueTypeRep<T> rhs_rep) {
  typedef FloatTraits<T> Traits;

  if (WABT_UNLIKELY(Traits::IsNan(lhs_rep))) {
    return lhs_rep | Traits::kQuietNanBit;
  } else if (WABT_UNLIKELY(Traits::IsNan(rhs_rep))) {
    return rhs_rep | Traits::kQuietNanBit;
  } else if (WABT_UNLIKELY(Traits::IsZero(lhs_rep) &&
                           Traits::IsZero(rhs_rep))) {
    // min(0.0, -0.0) == -0.0, but std::min won't produce the correct result.
    // We can instead compare using the unsigned integer representation, but
    // just max instead (since the sign bit makes the value larger).
    return std::max(lhs_rep, rhs_rep);
  } else {
    return ToRep(std::min(FromRep<T>(lhs_rep), FromRep<T>(rhs_rep)));
  }
}

// f{32,64}.max
template <typename T>
ValueTypeRep<T> FloatMax(ValueTypeRep<T> lhs_rep, ValueTypeRep<T> rhs_rep) {
  typedef FloatTraits<T> Traits;

  if (WABT_UNLIKELY(Traits::IsNan(lhs_rep))) {
    return lhs_rep | Traits::kQuietNanBit;
  } else if (WABT_UNLIKELY(Traits::IsNan(rhs_rep))) {
    return rhs_rep | Traits::kQuietNanBit;
  } else if (WABT_UNLIKELY(Traits::IsZero(lhs_rep) &&
                           Traits::IsZero(rhs_rep))) {
    // min(0.0, -0.0) == -0.0, but std::min won't produce the correct result.
    // We can instead compare using the unsigned integer representation, but
    // just max instead (since the sign bit makes the value larger).
    return std::min(lhs_rep, rhs_rep);
  } else {
    return ToRep(std::max(FromRep<T>(lhs_rep), FromRep<T>(rhs_rep)));
  }
}

// f{32,64}.copysign
template <typename T>
ValueTypeRep<T> FloatCopySign(ValueTypeRep<T> lhs_rep,
                              ValueTypeRep<T> rhs_rep) {
  typedef FloatTraits<T> Traits;
  return (lhs_rep & ~Traits::kSignMask) | (rhs_rep & Traits::kSignMask);
}

// {i,f}{32,64}.eq
template <typename T>
uint32_t Eq(ValueTypeRep<T> lhs_rep, ValueTypeRep<T> rhs_rep) {
  return ToRep(FromRep<T>(lhs_rep) == FromRep<T>(rhs_rep));
}

// {i,f}{32,64}.ne
template <typename T>
uint32_t Ne(ValueTypeRep<T> lhs_rep, ValueTypeRep<T> rhs_rep) {
  return ToRep(FromRep<T>(lhs_rep) != FromRep<T>(rhs_rep));
}

// f{32,64}.lt | i{32,64}.lt_{s,u}
template <typename T>
uint32_t Lt(ValueTypeRep<T> lhs_rep, ValueTypeRep<T> rhs_rep) {
  return ToRep(FromRep<T>(lhs_rep) < FromRep<T>(rhs_rep));
}

// f{32,64}.le | i{32,64}.le_{s,u}
template <typename T>
uint32_t Le(ValueTypeRep<T> lhs_rep, ValueTypeRep<T> rhs_rep) {
  return ToRep(FromRep<T>(lhs_rep) <= FromRep<T>(rhs_rep));
}

// f{32,64}.gt | i{32,64}.gt_{s,u}
template <typename T>
uint32_t Gt(ValueTypeRep<T> lhs_rep, ValueTypeRep<T> rhs_rep) {
  return ToRep(FromRep<T>(lhs_rep) > FromRep<T>(rhs_rep));
}

// f{32,64}.ge | i{32,64}.ge_{s,u}
template <typename T>
uint32_t Ge(ValueTypeRep<T> lhs_rep, ValueTypeRep<T> rhs_rep) {
  return ToRep(FromRep<T>(lhs_rep) >= FromRep<T>(rhs_rep));
}

// i{32,64}.trunc_{s,u}/f{32,64}
template <typename R, typename T>
Result IntTrunc(ValueTypeRep<T> v_rep, ValueTypeRep<R>* out_result) {
  TRAP_IF(FloatTraits<T>::IsNan(v_rep), InvalidConversionToInteger);
  TRAP_UNLESS((IsConversionInRange<R, T>(v_rep)), IntegerOverflow);
  *out_result = ToRep(static_cast<R>(FromRep<T>(v_rep)));
  return Result::Ok;
}

// i{32,64}.trunc_{s,u}:sat/f{32,64}
template <typename R, typename T>
ValueTypeRep<R> IntTruncSat(ValueTypeRep<T> v_rep) {
  typedef FloatTraits<T> Traits;
  if (WABT_UNLIKELY(Traits::IsNan(v_rep))) {
    return 0;
  } else if (WABT_UNLIKELY((!IsConversionInRange<R, T>(v_rep)))) {
    if (v_rep & Traits::kSignMask) {
      return ToRep(std::numeric_limits<R>::min());
    } else {
      return ToRep(std::numeric_limits<R>::max());
    }
  } else {
    return ToRep(static_cast<R>(FromRep<T>(v_rep)));
  }
}

bool Environment::FuncSignaturesAreEqual(Index sig_index_0,
                                         Index sig_index_1) const {
  if (sig_index_0 == sig_index_1)
    return true;
  const FuncSignature* sig_0 = &sigs_[sig_index_0];
  const FuncSignature* sig_1 = &sigs_[sig_index_1];
  return sig_0->param_types == sig_1->param_types &&
         sig_0->result_types == sig_1->result_types;
}

Result Thread::RunFunction(Index func_index,
                           const std::vector<TypedValue>& args,
                           std::vector<TypedValue>* out_results) {
  Func* func = env_->GetFunc(func_index);
  FuncSignature* sig = env_->GetFuncSignature(func->sig_index);

  Result result = PushArgs(sig, args);
  if (result == Result::Ok) {
    result = func->is_host ? CallHost(func->as_host())
                           : RunDefinedFunction(func->as_defined()->offset);
    if (result == Result::Ok)
      CopyResults(sig, out_results);
  }

  // Always reset the value and call stacks.
  value_stack_top_ = value_stack_.data();
  call_stack_top_ = call_stack_.data();
  return result;
}

Result Thread::TraceFunction(Index func_index,
                             Stream* stream,
                             const std::vector<TypedValue>& args,
                             std::vector<TypedValue>* out_results) {
  Func* func = env_->GetFunc(func_index);
  FuncSignature* sig = env_->GetFuncSignature(func->sig_index);

  Result result = PushArgs(sig, args);
  if (result == Result::Ok) {
    result = func->is_host
                 ? CallHost(func->as_host())
                 : TraceDefinedFunction(func->as_defined()->offset, stream);
    if (result == Result::Ok)
      CopyResults(sig, out_results);
  }

  // Always reset the value and call stacks.
  value_stack_top_ = value_stack_.data();
  call_stack_top_ = call_stack_.data();
  return result;
}

Result Thread::RunDefinedFunction(IstreamOffset function_offset) {
  const int kNumInstructions = 1000;
  Result result = Result::Ok;
  pc_ = function_offset;
  IstreamOffset* call_stack_return_top = call_stack_top_;
  while (result == Result::Ok) {
    result = Run(kNumInstructions, call_stack_return_top);
  }
  if (result != Result::Returned)
    return result;
  // Use OK instead of RETURNED for consistency.
  return Result::Ok;
}

Result Thread::TraceDefinedFunction(IstreamOffset function_offset,
                                    Stream* stream) {
  const int kNumInstructions = 1;
  Result result = Result::Ok;
  pc_ = function_offset;
  IstreamOffset* call_stack_return_top = call_stack_top_;
  while (result == Result::Ok) {
    Trace(stream);
    result = Run(kNumInstructions, call_stack_return_top);
  }
  if (result != Result::Returned)
    return result;
  // Use OK instead of RETURNED for consistency.
  return Result::Ok;
}

Result Thread::CallHost(HostFunc* func) {
  FuncSignature* sig = &env_->sigs_[func->sig_index];

  size_t num_params = sig->param_types.size();
  size_t num_results = sig->result_types.size();
  // + 1 is a workaround for using data() below; UBSAN doesn't like calling
  // data() with an empty vector.
  std::vector<TypedValue> params(num_params + 1);
  std::vector<TypedValue> results(num_results + 1);

  for (size_t i = num_params; i > 0; --i) {
    params[i - 1].value = Pop();
    params[i - 1].type = sig->param_types[i - 1];
  }

  Result call_result =
      func->callback(func, sig, num_params, params.data(), num_results,
                     results.data(), func->user_data);
  TRAP_IF(call_result != Result::Ok, HostTrapped);

  for (size_t i = 0; i < num_results; ++i) {
    TRAP_IF(results[i].type != sig->result_types[i], HostResultTypeMismatch);
    CHECK_TRAP(Push(results[i].value));
  }

  return Result::Ok;
}

Result Thread::Run(int num_instructions, IstreamOffset* call_stack_return_top) {
  Result result = Result::Ok;
  assert(call_stack_return_top < call_stack_end_);

  const uint8_t* istream = GetIstream();
  const uint8_t* pc = &istream[pc_];
  for (int i = 0; i < num_instructions; ++i) {
    Opcode opcode = static_cast<Opcode>(*pc++);
    switch (opcode) {
      case Opcode::Select: {
        uint32_t cond = Pop<uint32_t>();
        Value false_ = Pop();
        Value true_ = Pop();
        CHECK_TRAP(Push(cond ? true_ : false_));
        break;
      }

      case Opcode::Br:
        GOTO(read_u32(&pc));
        break;

      case Opcode::BrIf: {
        IstreamOffset new_pc = read_u32(&pc);
        if (Pop<uint32_t>())
          GOTO(new_pc);
        break;
      }

      case Opcode::BrTable: {
        Index num_targets = read_u32(&pc);
        IstreamOffset table_offset = read_u32(&pc);
        uint32_t key = Pop<uint32_t>();
        IstreamOffset key_offset =
            (key >= num_targets ? num_targets : key) * WABT_TABLE_ENTRY_SIZE;
        const uint8_t* entry = istream + table_offset + key_offset;
        IstreamOffset new_pc;
        uint32_t drop_count;
        uint8_t keep_count;
        read_table_entry_at(entry, &new_pc, &drop_count, &keep_count);
        DropKeep(drop_count, keep_count);
        GOTO(new_pc);
        break;
      }

      case Opcode::Return:
        if (call_stack_top_ == call_stack_return_top) {
          result = Result::Returned;
          goto exit_loop;
        }
        GOTO(PopCall());
        break;

      case Opcode::Unreachable:
        TRAP(Unreachable);
        break;

      case Opcode::I32Const:
        CHECK_TRAP(Push<uint32_t>(read_u32(&pc)));
        break;

      case Opcode::I64Const:
        CHECK_TRAP(Push<uint64_t>(read_u64(&pc)));
        break;

      case Opcode::F32Const:
        CHECK_TRAP(PushRep<float>(read_u32(&pc)));
        break;

      case Opcode::F64Const:
        CHECK_TRAP(PushRep<double>(read_u64(&pc)));
        break;

      case Opcode::GetGlobal: {
        Index index = read_u32(&pc);
        assert(index < env_->globals_.size());
        CHECK_TRAP(Push(env_->globals_[index].typed_value.value));
        break;
      }

      case Opcode::SetGlobal: {
        Index index = read_u32(&pc);
        assert(index < env_->globals_.size());
        env_->globals_[index].typed_value.value = Pop();
        break;
      }

      case Opcode::GetLocal: {
        Value value = Pick(read_u32(&pc));
        CHECK_TRAP(Push(value));
        break;
      }

      case Opcode::SetLocal: {
        Value value = Pop();
        Pick(read_u32(&pc)) = value;
        break;
      }

      case Opcode::TeeLocal:
        Pick(read_u32(&pc)) = Top();
        break;

      case Opcode::Call: {
        IstreamOffset offset = read_u32(&pc);
        CHECK_TRAP(PushCall(pc));
        GOTO(offset);
        break;
      }

      case Opcode::CallIndirect: {
        Index table_index = read_u32(&pc);
        Table* table = &env_->tables_[table_index];
        Index sig_index = read_u32(&pc);
        Index entry_index = Pop<uint32_t>();
        TRAP_IF(entry_index >= table->func_indexes.size(), UndefinedTableIndex);
        Index func_index = table->func_indexes[entry_index];
        TRAP_IF(func_index == kInvalidIndex, UninitializedTableElement);
        Func* func = env_->funcs_[func_index].get();
        TRAP_UNLESS(env_->FuncSignaturesAreEqual(func->sig_index, sig_index),
                    IndirectCallSignatureMismatch);
        if (func->is_host) {
          CallHost(func->as_host());
        } else {
          CHECK_TRAP(PushCall(pc));
          GOTO(func->as_defined()->offset);
        }
        break;
      }

      case Opcode::CallHost: {
        Index func_index = read_u32(&pc);
        CallHost(env_->funcs_[func_index]->as_host());
        break;
      }

      case Opcode::I32Load8S:
        CHECK_TRAP(Load<int8_t, uint32_t>(&pc));
        break;

      case Opcode::I32Load8U:
        CHECK_TRAP(Load<uint8_t, uint32_t>(&pc));
        break;

      case Opcode::I32Load16S:
        CHECK_TRAP(Load<int16_t, uint32_t>(&pc));
        break;

      case Opcode::I32Load16U:
        CHECK_TRAP(Load<uint16_t, uint32_t>(&pc));
        break;

      case Opcode::I64Load8S:
        CHECK_TRAP(Load<int8_t, uint64_t>(&pc));
        break;

      case Opcode::I64Load8U:
        CHECK_TRAP(Load<uint8_t, uint64_t>(&pc));
        break;

      case Opcode::I64Load16S:
        CHECK_TRAP(Load<int16_t, uint64_t>(&pc));
        break;

      case Opcode::I64Load16U:
        CHECK_TRAP(Load<uint16_t, uint64_t>(&pc));
        break;

      case Opcode::I64Load32S:
        CHECK_TRAP(Load<int32_t, uint64_t>(&pc));
        break;

      case Opcode::I64Load32U:
        CHECK_TRAP(Load<uint32_t, uint64_t>(&pc));
        break;

      case Opcode::I32Load:
        CHECK_TRAP(Load<uint32_t>(&pc));
        break;

      case Opcode::I64Load:
        CHECK_TRAP(Load<uint64_t>(&pc));
        break;

      case Opcode::F32Load:
        CHECK_TRAP(Load<float>(&pc));
        break;

      case Opcode::F64Load:
        CHECK_TRAP(Load<double>(&pc));
        break;

      case Opcode::I32Store8:
        CHECK_TRAP(Store<uint8_t, uint32_t>(&pc));
        break;

      case Opcode::I32Store16:
        CHECK_TRAP(Store<uint16_t, uint32_t>(&pc));
        break;

      case Opcode::I64Store8:
        CHECK_TRAP(Store<uint8_t, uint64_t>(&pc));
        break;

      case Opcode::I64Store16:
        CHECK_TRAP(Store<uint16_t, uint64_t>(&pc));
        break;

      case Opcode::I64Store32:
        CHECK_TRAP(Store<uint32_t, uint64_t>(&pc));
        break;

      case Opcode::I32Store:
        CHECK_TRAP(Store<uint32_t>(&pc));
        break;

      case Opcode::I64Store:
        CHECK_TRAP(Store<uint64_t>(&pc));
        break;

      case Opcode::F32Store:
        CHECK_TRAP(Store<float>(&pc));
        break;

      case Opcode::F64Store:
        CHECK_TRAP(Store<double>(&pc));
        break;

      case Opcode::CurrentMemory:
        CHECK_TRAP(Push<uint32_t>(ReadMemory(&pc)->page_limits.initial));
        break;

      case Opcode::GrowMemory: {
        Memory* memory = ReadMemory(&pc);
        uint32_t old_page_size = memory->page_limits.initial;
        uint32_t grow_pages = Pop<uint32_t>();
        uint32_t new_page_size = old_page_size + grow_pages;
        uint32_t max_page_size = memory->page_limits.has_max
                                     ? memory->page_limits.max
                                     : WABT_MAX_PAGES;
        PUSH_NEG_1_AND_BREAK_IF(new_page_size > max_page_size);
        PUSH_NEG_1_AND_BREAK_IF(
            static_cast<uint64_t>(new_page_size) * WABT_PAGE_SIZE > UINT32_MAX);
        memory->data.resize(new_page_size * WABT_PAGE_SIZE);
        memory->page_limits.initial = new_page_size;
        CHECK_TRAP(Push<uint32_t>(old_page_size));
        break;
      }

      case Opcode::I32Add:
        CHECK_TRAP(Binop(Add<uint32_t>));
        break;

      case Opcode::I32Sub:
        CHECK_TRAP(Binop(Sub<uint32_t>));
        break;

      case Opcode::I32Mul:
        CHECK_TRAP(Binop(Mul<uint32_t>));
        break;

      case Opcode::I32DivS:
        CHECK_TRAP(BinopTrap(IntDivS<int32_t>));
        break;

      case Opcode::I32DivU:
        CHECK_TRAP(BinopTrap(IntDivU<uint32_t>));
        break;

      case Opcode::I32RemS:
        CHECK_TRAP(BinopTrap(IntRemS<int32_t>));
        break;

      case Opcode::I32RemU:
        CHECK_TRAP(BinopTrap(IntRemU<uint32_t>));
        break;

      case Opcode::I32And:
        CHECK_TRAP(Binop(IntAnd<uint32_t>));
        break;

      case Opcode::I32Or:
        CHECK_TRAP(Binop(IntOr<uint32_t>));
        break;

      case Opcode::I32Xor:
        CHECK_TRAP(Binop(IntXor<uint32_t>));
        break;

      case Opcode::I32Shl:
        CHECK_TRAP(Binop(IntShl<uint32_t>));
        break;

      case Opcode::I32ShrU:
        CHECK_TRAP(Binop(IntShr<uint32_t>));
        break;

      case Opcode::I32ShrS:
        CHECK_TRAP(Binop(IntShr<int32_t>));
        break;

      case Opcode::I32Eq:
        CHECK_TRAP(Binop(Eq<uint32_t>));
        break;

      case Opcode::I32Ne:
        CHECK_TRAP(Binop(Ne<uint32_t>));
        break;

      case Opcode::I32LtS:
        CHECK_TRAP(Binop(Lt<int32_t>));
        break;

      case Opcode::I32LeS:
        CHECK_TRAP(Binop(Le<int32_t>));
        break;

      case Opcode::I32LtU:
        CHECK_TRAP(Binop(Lt<uint32_t>));
        break;

      case Opcode::I32LeU:
        CHECK_TRAP(Binop(Le<uint32_t>));
        break;

      case Opcode::I32GtS:
        CHECK_TRAP(Binop(Gt<int32_t>));
        break;

      case Opcode::I32GeS:
        CHECK_TRAP(Binop(Ge<int32_t>));
        break;

      case Opcode::I32GtU:
        CHECK_TRAP(Binop(Gt<uint32_t>));
        break;

      case Opcode::I32GeU:
        CHECK_TRAP(Binop(Ge<uint32_t>));
        break;

      case Opcode::I32Clz: {
        uint32_t value = Pop<uint32_t>();
        CHECK_TRAP(Push<uint32_t>(value != 0 ? wabt_clz_u32(value) : 32));
        break;
      }

      case Opcode::I32Ctz: {
        uint32_t value = Pop<uint32_t>();
        CHECK_TRAP(Push<uint32_t>(value != 0 ? wabt_ctz_u32(value) : 32));
        break;
      }

      case Opcode::I32Popcnt: {
        uint32_t value = Pop<uint32_t>();
        CHECK_TRAP(Push<uint32_t>(wabt_popcount_u32(value)));
        break;
      }

      case Opcode::I32Eqz:
        CHECK_TRAP(Unop(IntEqz<uint32_t, uint32_t>));
        break;

      case Opcode::I64Add:
        CHECK_TRAP(Binop(Add<uint64_t>));
        break;

      case Opcode::I64Sub:
        CHECK_TRAP(Binop(Sub<uint64_t>));
        break;

      case Opcode::I64Mul:
        CHECK_TRAP(Binop(Mul<uint64_t>));
        break;

      case Opcode::I64DivS:
        CHECK_TRAP(BinopTrap(IntDivS<int64_t>));
        break;

      case Opcode::I64DivU:
        CHECK_TRAP(BinopTrap(IntDivU<uint64_t>));
        break;

      case Opcode::I64RemS:
        CHECK_TRAP(BinopTrap(IntRemS<int64_t>));
        break;

      case Opcode::I64RemU:
        CHECK_TRAP(BinopTrap(IntRemU<uint64_t>));
        break;

      case Opcode::I64And:
        CHECK_TRAP(Binop(IntAnd<uint64_t>));
        break;

      case Opcode::I64Or:
        CHECK_TRAP(Binop(IntOr<uint64_t>));
        break;

      case Opcode::I64Xor:
        CHECK_TRAP(Binop(IntXor<uint64_t>));
        break;

      case Opcode::I64Shl:
        CHECK_TRAP(Binop(IntShl<uint64_t>));
        break;

      case Opcode::I64ShrU:
        CHECK_TRAP(Binop(IntShr<uint64_t>));
        break;

      case Opcode::I64ShrS:
        CHECK_TRAP(Binop(IntShr<int64_t>));
        break;

      case Opcode::I64Eq:
        CHECK_TRAP(Binop(Eq<uint64_t>));
        break;

      case Opcode::I64Ne:
        CHECK_TRAP(Binop(Ne<uint64_t>));
        break;

      case Opcode::I64LtS:
        CHECK_TRAP(Binop(Lt<int64_t>));
        break;

      case Opcode::I64LeS:
        CHECK_TRAP(Binop(Le<int64_t>));
        break;

      case Opcode::I64LtU:
        CHECK_TRAP(Binop(Lt<uint64_t>));
        break;

      case Opcode::I64LeU:
        CHECK_TRAP(Binop(Le<uint64_t>));
        break;

      case Opcode::I64GtS:
        CHECK_TRAP(Binop(Gt<int64_t>));
        break;

      case Opcode::I64GeS:
        CHECK_TRAP(Binop(Ge<int64_t>));
        break;

      case Opcode::I64GtU:
        CHECK_TRAP(Binop(Gt<uint64_t>));
        break;

      case Opcode::I64GeU:
        CHECK_TRAP(Binop(Ge<uint64_t>));
        break;

      case Opcode::I64Clz: {
        uint64_t value = Pop<uint64_t>();
        CHECK_TRAP(Push<uint64_t>(value != 0 ? wabt_clz_u64(value) : 64));
        break;
      }

      case Opcode::I64Ctz: {
        uint64_t value = Pop<uint64_t>();
        CHECK_TRAP(Push<uint64_t>(value != 0 ? wabt_ctz_u64(value) : 64));
        break;
      }

      case Opcode::I64Popcnt:
        CHECK_TRAP(Push<uint64_t>(wabt_popcount_u64(Pop<uint64_t>())));
        break;

      case Opcode::F32Add:
        CHECK_TRAP(Binop(Add<float>));
        break;

      case Opcode::F32Sub:
        CHECK_TRAP(Binop(Sub<float>));
        break;

      case Opcode::F32Mul:
        CHECK_TRAP(Binop(Mul<float>));
        break;

      case Opcode::F32Div:
        CHECK_TRAP(Binop(FloatDiv<float>));
        break;

      case Opcode::F32Min:
        CHECK_TRAP(Binop(FloatMin<float>));
        break;

      case Opcode::F32Max:
        CHECK_TRAP(Binop(FloatMax<float>));
        break;

      case Opcode::F32Abs:
        CHECK_TRAP(Unop(FloatAbs<float>));
        break;

      case Opcode::F32Neg:
        CHECK_TRAP(Unop(FloatNeg<float>));
        break;

      case Opcode::F32Copysign:
        CHECK_TRAP(Binop(FloatCopySign<float>));
        break;

      case Opcode::F32Ceil:
        CHECK_TRAP(Unop(FloatCeil<float>));
        break;

      case Opcode::F32Floor:
        CHECK_TRAP(Unop(FloatFloor<float>));
        break;

      case Opcode::F32Trunc:
        CHECK_TRAP(Unop(FloatTrunc<float>));
        break;

      case Opcode::F32Nearest:
        CHECK_TRAP(Unop(FloatNearest<float>));
        break;

      case Opcode::F32Sqrt:
        CHECK_TRAP(Unop(FloatSqrt<float>));
        break;

      case Opcode::F32Eq:
        CHECK_TRAP(Binop(Eq<float>));
        break;

      case Opcode::F32Ne:
        CHECK_TRAP(Binop(Ne<float>));
        break;

      case Opcode::F32Lt:
        CHECK_TRAP(Binop(Lt<float>));
        break;

      case Opcode::F32Le:
        CHECK_TRAP(Binop(Le<float>));
        break;

      case Opcode::F32Gt:
        CHECK_TRAP(Binop(Gt<float>));
        break;

      case Opcode::F32Ge:
        CHECK_TRAP(Binop(Ge<float>));
        break;

      case Opcode::F64Add:
        CHECK_TRAP(Binop(Add<double>));
        break;

      case Opcode::F64Sub:
        CHECK_TRAP(Binop(Sub<double>));
        break;

      case Opcode::F64Mul:
        CHECK_TRAP(Binop(Mul<double>));
        break;

      case Opcode::F64Div:
        CHECK_TRAP(Binop(FloatDiv<double>));
        break;

      case Opcode::F64Min:
        CHECK_TRAP(Binop(FloatMin<double>));
        break;

      case Opcode::F64Max:
        CHECK_TRAP(Binop(FloatMax<double>));
        break;

      case Opcode::F64Abs:
        CHECK_TRAP(Unop(FloatAbs<double>));
        break;

      case Opcode::F64Neg:
        CHECK_TRAP(Unop(FloatNeg<double>));
        break;

      case Opcode::F64Copysign:
        CHECK_TRAP(Binop(FloatCopySign<double>));
        break;

      case Opcode::F64Ceil:
        CHECK_TRAP(Unop(FloatCeil<double>));
        break;

      case Opcode::F64Floor:
        CHECK_TRAP(Unop(FloatFloor<double>));
        break;

      case Opcode::F64Trunc:
        CHECK_TRAP(Unop(FloatTrunc<double>));
        break;

      case Opcode::F64Nearest:
        CHECK_TRAP(Unop(FloatNearest<double>));
        break;

      case Opcode::F64Sqrt:
        CHECK_TRAP(Unop(FloatSqrt<double>));
        break;

      case Opcode::F64Eq:
        CHECK_TRAP(Binop(Eq<double>));
        break;

      case Opcode::F64Ne:
        CHECK_TRAP(Binop(Ne<double>));
        break;

      case Opcode::F64Lt:
        CHECK_TRAP(Binop(Lt<double>));
        break;

      case Opcode::F64Le:
        CHECK_TRAP(Binop(Le<double>));
        break;

      case Opcode::F64Gt:
        CHECK_TRAP(Binop(Gt<double>));
        break;

      case Opcode::F64Ge:
        CHECK_TRAP(Binop(Ge<double>));
        break;

      case Opcode::I32TruncSF32:
        CHECK_TRAP(UnopTrap(IntTrunc<int32_t, float>));
        break;

      case Opcode::I32TruncSSatF32:
        CHECK_TRAP(Unop(IntTruncSat<int32_t, float>));
        break;

      case Opcode::I32TruncSF64:
        CHECK_TRAP(UnopTrap(IntTrunc<int32_t, double>));
        break;

      case Opcode::I32TruncSSatF64:
        CHECK_TRAP(Unop(IntTruncSat<int32_t, double>));
        break;

      case Opcode::I32TruncUF32:
        CHECK_TRAP(UnopTrap(IntTrunc<uint32_t, float>));
        break;

      case Opcode::I32TruncUSatF32:
        CHECK_TRAP(Unop(IntTruncSat<uint32_t, float>));
        break;

      case Opcode::I32TruncUF64:
        CHECK_TRAP(UnopTrap(IntTrunc<uint32_t, double>));
        break;

      case Opcode::I32TruncUSatF64:
        CHECK_TRAP(Unop(IntTruncSat<uint32_t, double>));
        break;

      case Opcode::I32WrapI64:
        CHECK_TRAP(Push<uint32_t>(Pop<uint64_t>()));
        break;

      case Opcode::I64TruncSF32:
        CHECK_TRAP(UnopTrap(IntTrunc<int64_t, float>));
        break;

      case Opcode::I64TruncSSatF32:
        CHECK_TRAP(Unop(IntTruncSat<int64_t, float>));
        break;

      case Opcode::I64TruncSF64:
        CHECK_TRAP(UnopTrap(IntTrunc<int64_t, double>));
        break;

      case Opcode::I64TruncSSatF64:
        CHECK_TRAP(Unop(IntTruncSat<int64_t, double>));
        break;

      case Opcode::I64TruncUF32:
        CHECK_TRAP(UnopTrap(IntTrunc<uint64_t, float>));
        break;

      case Opcode::I64TruncUSatF32:
        CHECK_TRAP(Unop(IntTruncSat<uint64_t, float>));
        break;

      case Opcode::I64TruncUF64:
        CHECK_TRAP(UnopTrap(IntTrunc<uint64_t, double>));
        break;

      case Opcode::I64TruncUSatF64:
        CHECK_TRAP(Unop(IntTruncSat<uint64_t, double>));
        break;

      case Opcode::I64ExtendSI32:
        CHECK_TRAP(Push<uint64_t>(Pop<int32_t>()));
        break;

      case Opcode::I64ExtendUI32:
        CHECK_TRAP(Push<uint64_t>(Pop<uint32_t>()));
        break;

      case Opcode::F32ConvertSI32:
        CHECK_TRAP(Push<float>(Pop<int32_t>()));
        break;

      case Opcode::F32ConvertUI32:
        CHECK_TRAP(Push<float>(Pop<uint32_t>()));
        break;

      case Opcode::F32ConvertSI64:
        CHECK_TRAP(Push<float>(Pop<int64_t>()));
        break;

      case Opcode::F32ConvertUI64:
        CHECK_TRAP(Push<float>(wabt_convert_uint64_to_float(Pop<uint64_t>())));
        break;

      case Opcode::F32DemoteF64: {
        typedef FloatTraits<float> F32Traits;
        typedef FloatTraits<double> F64Traits;

        uint64_t value = PopRep<double>();
        if (WABT_LIKELY((IsConversionInRange<float, double>(value)))) {
          CHECK_TRAP(Push<float>(FromRep<double>(value)));
        } else if (IsInRangeF64DemoteF32RoundToF32Max(value)) {
          CHECK_TRAP(PushRep<float>(F32Traits::kMax));
        } else if (IsInRangeF64DemoteF32RoundToNegF32Max(value)) {
          CHECK_TRAP(PushRep<float>(F32Traits::kNegMax));
        } else {
          uint32_t sign = (value >> 32) & F32Traits::kSignMask;
          uint32_t tag = 0;
          if (F64Traits::IsNan(value)) {
            tag = F32Traits::kQuietNanBit |
                  ((value >> (F64Traits::kSigBits - F32Traits::kSigBits)) &
                   F32Traits::kSigMask);
          }
          CHECK_TRAP(PushRep<float>(sign | F32Traits::kInf | tag));
        }
        break;
      }

      case Opcode::F32ReinterpretI32:
        CHECK_TRAP(PushRep<float>(Pop<uint32_t>()));
        break;

      case Opcode::F64ConvertSI32:
        CHECK_TRAP(Push<double>(Pop<int32_t>()));
        break;

      case Opcode::F64ConvertUI32:
        CHECK_TRAP(Push<double>(Pop<uint32_t>()));
        break;

      case Opcode::F64ConvertSI64:
        CHECK_TRAP(Push<double>(Pop<int64_t>()));
        break;

      case Opcode::F64ConvertUI64:
        CHECK_TRAP(
            Push<double>(wabt_convert_uint64_to_double(Pop<uint64_t>())));
        break;

      case Opcode::F64PromoteF32:
        CHECK_TRAP(Push<double>(Pop<float>()));
        break;

      case Opcode::F64ReinterpretI64:
        CHECK_TRAP(PushRep<double>(Pop<uint64_t>()));
        break;

      case Opcode::I32ReinterpretF32:
        CHECK_TRAP(Push<uint32_t>(PopRep<float>()));
        break;

      case Opcode::I64ReinterpretF64:
        CHECK_TRAP(Push<uint64_t>(PopRep<double>()));
        break;

      case Opcode::I32Rotr:
        CHECK_TRAP(Binop(IntRotr<uint32_t>));
        break;

      case Opcode::I32Rotl:
        CHECK_TRAP(Binop(IntRotl<uint32_t>));
        break;

      case Opcode::I64Rotr:
        CHECK_TRAP(Binop(IntRotr<uint64_t>));
        break;

      case Opcode::I64Rotl:
        CHECK_TRAP(Binop(IntRotl<uint64_t>));
        break;

      case Opcode::I64Eqz:
        CHECK_TRAP(Unop(IntEqz<uint32_t, uint64_t>));
        break;

      case Opcode::Alloca: {
        Value* old_value_stack_top = value_stack_top_;
        value_stack_top_ += read_u32(&pc);
        CHECK_STACK();
        memset(old_value_stack_top, 0,
               (value_stack_top_ - old_value_stack_top) * sizeof(Value));
        break;
      }

      case Opcode::BrUnless: {
        IstreamOffset new_pc = read_u32(&pc);
        if (!Pop<uint32_t>())
          GOTO(new_pc);
        break;
      }

      case Opcode::Drop:
        (void)Pop();
        break;

      case Opcode::DropKeep: {
        uint32_t drop_count = read_u32(&pc);
        uint8_t keep_count = *pc++;
        DropKeep(drop_count, keep_count);
        break;
      }

      case Opcode::Data:
        /* shouldn't ever execute this */
        assert(0);
        break;

      case Opcode::Nop:
        break;

      default:
        assert(0);
        break;
    }
  }

exit_loop:
  pc_ = pc - istream;
  return result;
}

void Thread::Trace(Stream* stream) {
  const uint8_t* istream = GetIstream();
  const uint8_t* pc = &istream[pc_];
  size_t value_stack_depth = value_stack_top_ - value_stack_.data();
  size_t call_stack_depth = call_stack_top_ - call_stack_.data();

  stream->Writef("#%" PRIzd ". %4" PRIzd ": V:%-3" PRIzd "| ", call_stack_depth,
                 pc - istream, value_stack_depth);

  Opcode opcode = static_cast<Opcode>(*pc++);
  switch (opcode) {
    case Opcode::Select:
      stream->Writef("%s %u, %" PRIu64 ", %" PRIu64 "\n", GetOpcodeName(opcode),
                     Pick(3).i32, Pick(2).i64, Pick(1).i64);
      break;

    case Opcode::Br:
      stream->Writef("%s @%u\n", GetOpcodeName(opcode), read_u32_at(pc));
      break;

    case Opcode::BrIf:
      stream->Writef("%s @%u, %u\n", GetOpcodeName(opcode), read_u32_at(pc),
                     Top().i32);
      break;

    case Opcode::BrTable: {
      Index num_targets = read_u32_at(pc);
      IstreamOffset table_offset = read_u32_at(pc + 4);
      uint32_t key = Top().i32;
      stream->Writef("%s %u, $#%" PRIindex ", table:$%u\n",
                     GetOpcodeName(opcode), key, num_targets, table_offset);
      break;
    }

    case Opcode::Nop:
    case Opcode::Return:
    case Opcode::Unreachable:
    case Opcode::Drop:
      stream->Writef("%s\n", GetOpcodeName(opcode));
      break;

    case Opcode::CurrentMemory: {
      Index memory_index = read_u32(&pc);
      stream->Writef("%s $%" PRIindex "\n", GetOpcodeName(opcode),
                     memory_index);
      break;
    }

    case Opcode::I32Const:
      stream->Writef("%s $%u\n", GetOpcodeName(opcode), read_u32_at(pc));
      break;

    case Opcode::I64Const:
      stream->Writef("%s $%" PRIu64 "\n", GetOpcodeName(opcode),
                     read_u64_at(pc));
      break;

    case Opcode::F32Const:
      stream->Writef("%s $%g\n", GetOpcodeName(opcode),
                     Bitcast<float>(read_u32_at(pc)));
      break;

    case Opcode::F64Const:
      stream->Writef("%s $%g\n", GetOpcodeName(opcode),
                     Bitcast<double>(read_u64_at(pc)));
      break;

    case Opcode::GetLocal:
    case Opcode::GetGlobal:
      stream->Writef("%s $%u\n", GetOpcodeName(opcode), read_u32_at(pc));
      break;

    case Opcode::SetLocal:
    case Opcode::SetGlobal:
    case Opcode::TeeLocal:
      stream->Writef("%s $%u, %u\n", GetOpcodeName(opcode), read_u32_at(pc),
                     Top().i32);
      break;

    case Opcode::Call:
      stream->Writef("%s @%u\n", GetOpcodeName(opcode), read_u32_at(pc));
      break;

    case Opcode::CallIndirect:
      stream->Writef("%s $%u, %u\n", GetOpcodeName(opcode), read_u32_at(pc),
                     Top().i32);
      break;

    case Opcode::CallHost:
      stream->Writef("%s $%u\n", GetOpcodeName(opcode), read_u32_at(pc));
      break;

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
      Index memory_index = read_u32(&pc);
      stream->Writef("%s $%" PRIindex ":%u+$%u\n", GetOpcodeName(opcode),
                     memory_index, Top().i32, read_u32_at(pc));
      break;
    }

    case Opcode::I32Store8:
    case Opcode::I32Store16:
    case Opcode::I32Store: {
      Index memory_index = read_u32(&pc);
      stream->Writef("%s $%" PRIindex ":%u+$%u, %u\n", GetOpcodeName(opcode),
                     memory_index, Pick(2).i32, read_u32_at(pc), Pick(1).i32);
      break;
    }

    case Opcode::I64Store8:
    case Opcode::I64Store16:
    case Opcode::I64Store32:
    case Opcode::I64Store: {
      Index memory_index = read_u32(&pc);
      stream->Writef("%s $%" PRIindex ":%u+$%u, %" PRIu64 "\n",
                     GetOpcodeName(opcode), memory_index, Pick(2).i32,
                     read_u32_at(pc), Pick(1).i64);
      break;
    }

    case Opcode::F32Store: {
      Index memory_index = read_u32(&pc);
      stream->Writef("%s $%" PRIindex ":%u+$%u, %g\n", GetOpcodeName(opcode),
                     memory_index, Pick(2).i32, read_u32_at(pc),
                     Bitcast<float>(Pick(1).f32_bits));
      break;
    }

    case Opcode::F64Store: {
      Index memory_index = read_u32(&pc);
      stream->Writef("%s $%" PRIindex ":%u+$%u, %g\n", GetOpcodeName(opcode),
                     memory_index, Pick(2).i32, read_u32_at(pc),
                     Bitcast<double>(Pick(1).f64_bits));
      break;
    }

    case Opcode::GrowMemory: {
      Index memory_index = read_u32(&pc);
      stream->Writef("%s $%" PRIindex ":%u\n", GetOpcodeName(opcode),
                     memory_index, Top().i32);
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
    case Opcode::I32Rotr:
    case Opcode::I32Rotl:
      stream->Writef("%s %u, %u\n", GetOpcodeName(opcode), Pick(2).i32,
                     Pick(1).i32);
      break;

    case Opcode::I32Clz:
    case Opcode::I32Ctz:
    case Opcode::I32Popcnt:
    case Opcode::I32Eqz:
      stream->Writef("%s %u\n", GetOpcodeName(opcode), Top().i32);
      break;

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
    case Opcode::I64Rotr:
    case Opcode::I64Rotl:
      stream->Writef("%s %" PRIu64 ", %" PRIu64 "\n", GetOpcodeName(opcode),
                     Pick(2).i64, Pick(1).i64);
      break;

    case Opcode::I64Clz:
    case Opcode::I64Ctz:
    case Opcode::I64Popcnt:
    case Opcode::I64Eqz:
      stream->Writef("%s %" PRIu64 "\n", GetOpcodeName(opcode), Top().i64);
      break;

    case Opcode::F32Add:
    case Opcode::F32Sub:
    case Opcode::F32Mul:
    case Opcode::F32Div:
    case Opcode::F32Min:
    case Opcode::F32Max:
    case Opcode::F32Copysign:
    case Opcode::F32Eq:
    case Opcode::F32Ne:
    case Opcode::F32Lt:
    case Opcode::F32Le:
    case Opcode::F32Gt:
    case Opcode::F32Ge:
      stream->Writef("%s %g, %g\n", GetOpcodeName(opcode),
                     Bitcast<float>(Pick(2).i32), Bitcast<float>(Pick(1).i32));
      break;

    case Opcode::F32Abs:
    case Opcode::F32Neg:
    case Opcode::F32Ceil:
    case Opcode::F32Floor:
    case Opcode::F32Trunc:
    case Opcode::F32Nearest:
    case Opcode::F32Sqrt:
      stream->Writef("%s %g\n", GetOpcodeName(opcode),
                     Bitcast<float>(Top().i32));
      break;

    case Opcode::F64Add:
    case Opcode::F64Sub:
    case Opcode::F64Mul:
    case Opcode::F64Div:
    case Opcode::F64Min:
    case Opcode::F64Max:
    case Opcode::F64Copysign:
    case Opcode::F64Eq:
    case Opcode::F64Ne:
    case Opcode::F64Lt:
    case Opcode::F64Le:
    case Opcode::F64Gt:
    case Opcode::F64Ge:
      stream->Writef("%s %g, %g\n", GetOpcodeName(opcode),
                     Bitcast<double>(Pick(2).i64),
                     Bitcast<double>(Pick(1).i64));
      break;

    case Opcode::F64Abs:
    case Opcode::F64Neg:
    case Opcode::F64Ceil:
    case Opcode::F64Floor:
    case Opcode::F64Trunc:
    case Opcode::F64Nearest:
    case Opcode::F64Sqrt:
      stream->Writef("%s %g\n", GetOpcodeName(opcode),
                     Bitcast<double>(Top().i64));
      break;

    case Opcode::I32TruncSF32:
    case Opcode::I32TruncUF32:
    case Opcode::I64TruncSF32:
    case Opcode::I64TruncUF32:
    case Opcode::F64PromoteF32:
    case Opcode::I32ReinterpretF32:
    case Opcode::I32TruncSSatF32:
    case Opcode::I32TruncUSatF32:
    case Opcode::I64TruncSSatF32:
    case Opcode::I64TruncUSatF32:
      stream->Writef("%s %g\n", GetOpcodeName(opcode),
                     Bitcast<float>(Top().i32));
      break;

    case Opcode::I32TruncSF64:
    case Opcode::I32TruncUF64:
    case Opcode::I64TruncSF64:
    case Opcode::I64TruncUF64:
    case Opcode::F32DemoteF64:
    case Opcode::I64ReinterpretF64:
    case Opcode::I32TruncSSatF64:
    case Opcode::I32TruncUSatF64:
    case Opcode::I64TruncSSatF64:
    case Opcode::I64TruncUSatF64:
      stream->Writef("%s %g\n", GetOpcodeName(opcode),
                     Bitcast<double>(Top().i64));
      break;

    case Opcode::I32WrapI64:
    case Opcode::F32ConvertSI64:
    case Opcode::F32ConvertUI64:
    case Opcode::F64ConvertSI64:
    case Opcode::F64ConvertUI64:
    case Opcode::F64ReinterpretI64:
      stream->Writef("%s %" PRIu64 "\n", GetOpcodeName(opcode), Top().i64);
      break;

    case Opcode::I64ExtendSI32:
    case Opcode::I64ExtendUI32:
    case Opcode::F32ConvertSI32:
    case Opcode::F32ConvertUI32:
    case Opcode::F32ReinterpretI32:
    case Opcode::F64ConvertSI32:
    case Opcode::F64ConvertUI32:
      stream->Writef("%s %u\n", GetOpcodeName(opcode), Top().i32);
      break;

    case Opcode::Alloca:
      stream->Writef("%s $%u\n", GetOpcodeName(opcode), read_u32_at(pc));
      break;

    case Opcode::BrUnless:
      stream->Writef("%s @%u, %u\n", GetOpcodeName(opcode), read_u32_at(pc),
                     Top().i32);
      break;

    case Opcode::DropKeep:
      stream->Writef("%s $%u $%u\n", GetOpcodeName(opcode), read_u32_at(pc),
                     *(pc + 4));
      break;

    case Opcode::Data:
      /* shouldn't ever execute this */
      assert(0);
      break;

    default:
      assert(0);
      break;
  }
}

void Environment::Disassemble(Stream* stream,
                              IstreamOffset from,
                              IstreamOffset to) {
  /* TODO(binji): mark function entries */
  /* TODO(binji): track value stack size */
  if (from >= istream_->data.size())
    return;
  to = std::min<IstreamOffset>(to, istream_->data.size());
  const uint8_t* istream = istream_->data.data();
  const uint8_t* pc = &istream[from];

  while (static_cast<IstreamOffset>(pc - istream) < to) {
    stream->Writef("%4" PRIzd "| ", pc - istream);

    Opcode opcode = static_cast<Opcode>(*pc++);
    switch (opcode) {
      case Opcode::Select:
        stream->Writef("%s %%[-3], %%[-2], %%[-1]\n", GetOpcodeName(opcode));
        break;

      case Opcode::Br:
        stream->Writef("%s @%u\n", GetOpcodeName(opcode), read_u32(&pc));
        break;

      case Opcode::BrIf:
        stream->Writef("%s @%u, %%[-1]\n", GetOpcodeName(opcode),
                       read_u32(&pc));
        break;

      case Opcode::BrTable: {
        Index num_targets = read_u32(&pc);
        IstreamOffset table_offset = read_u32(&pc);
        stream->Writef("%s %%[-1], $#%" PRIindex ", table:$%u\n",
                       GetOpcodeName(opcode), num_targets, table_offset);
        break;
      }

      case Opcode::Nop:
      case Opcode::Return:
      case Opcode::Unreachable:
      case Opcode::Drop:
        stream->Writef("%s\n", GetOpcodeName(opcode));
        break;

      case Opcode::CurrentMemory: {
        Index memory_index = read_u32(&pc);
        stream->Writef("%s $%" PRIindex "\n", GetOpcodeName(opcode),
                       memory_index);
        break;
      }

      case Opcode::I32Const:
        stream->Writef("%s $%u\n", GetOpcodeName(opcode), read_u32(&pc));
        break;

      case Opcode::I64Const:
        stream->Writef("%s $%" PRIu64 "\n", GetOpcodeName(opcode),
                       read_u64(&pc));
        break;

      case Opcode::F32Const:
        stream->Writef("%s $%g\n", GetOpcodeName(opcode),
                       Bitcast<float>(read_u32(&pc)));
        break;

      case Opcode::F64Const:
        stream->Writef("%s $%g\n", GetOpcodeName(opcode),
                       Bitcast<double>(read_u64(&pc)));
        break;

      case Opcode::GetLocal:
      case Opcode::GetGlobal:
        stream->Writef("%s $%u\n", GetOpcodeName(opcode), read_u32(&pc));
        break;

      case Opcode::SetLocal:
      case Opcode::SetGlobal:
      case Opcode::TeeLocal:
        stream->Writef("%s $%u, %%[-1]\n", GetOpcodeName(opcode),
                       read_u32(&pc));
        break;

      case Opcode::Call:
        stream->Writef("%s @%u\n", GetOpcodeName(opcode), read_u32(&pc));
        break;

      case Opcode::CallIndirect: {
        Index table_index = read_u32(&pc);
        stream->Writef("%s $%" PRIindex ":%u, %%[-1]\n", GetOpcodeName(opcode),
                       table_index, read_u32(&pc));
        break;
      }

      case Opcode::CallHost:
        stream->Writef("%s $%u\n", GetOpcodeName(opcode), read_u32(&pc));
        break;

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
        Index memory_index = read_u32(&pc);
        stream->Writef("%s $%" PRIindex ":%%[-1]+$%u\n", GetOpcodeName(opcode),
                       memory_index, read_u32(&pc));
        break;
      }

      case Opcode::I32Store8:
      case Opcode::I32Store16:
      case Opcode::I32Store:
      case Opcode::I64Store8:
      case Opcode::I64Store16:
      case Opcode::I64Store32:
      case Opcode::I64Store:
      case Opcode::F32Store:
      case Opcode::F64Store: {
        Index memory_index = read_u32(&pc);
        stream->Writef("%s %%[-2]+$%" PRIindex ", $%u:%%[-1]\n",
                       GetOpcodeName(opcode), memory_index, read_u32(&pc));
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
      case Opcode::I32Rotr:
      case Opcode::I32Rotl:
      case Opcode::F32Add:
      case Opcode::F32Sub:
      case Opcode::F32Mul:
      case Opcode::F32Div:
      case Opcode::F32Min:
      case Opcode::F32Max:
      case Opcode::F32Copysign:
      case Opcode::F32Eq:
      case Opcode::F32Ne:
      case Opcode::F32Lt:
      case Opcode::F32Le:
      case Opcode::F32Gt:
      case Opcode::F32Ge:
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
      case Opcode::I64Rotr:
      case Opcode::I64Rotl:
      case Opcode::F64Add:
      case Opcode::F64Sub:
      case Opcode::F64Mul:
      case Opcode::F64Div:
      case Opcode::F64Min:
      case Opcode::F64Max:
      case Opcode::F64Copysign:
      case Opcode::F64Eq:
      case Opcode::F64Ne:
      case Opcode::F64Lt:
      case Opcode::F64Le:
      case Opcode::F64Gt:
      case Opcode::F64Ge:
        stream->Writef("%s %%[-2], %%[-1]\n", GetOpcodeName(opcode));
        break;

      case Opcode::I32Clz:
      case Opcode::I32Ctz:
      case Opcode::I32Popcnt:
      case Opcode::I32Eqz:
      case Opcode::I64Clz:
      case Opcode::I64Ctz:
      case Opcode::I64Popcnt:
      case Opcode::I64Eqz:
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
      case Opcode::I32TruncSF32:
      case Opcode::I32TruncUF32:
      case Opcode::I64TruncSF32:
      case Opcode::I64TruncUF32:
      case Opcode::F64PromoteF32:
      case Opcode::I32ReinterpretF32:
      case Opcode::I32TruncSF64:
      case Opcode::I32TruncUF64:
      case Opcode::I64TruncSF64:
      case Opcode::I64TruncUF64:
      case Opcode::F32DemoteF64:
      case Opcode::I64ReinterpretF64:
      case Opcode::I32WrapI64:
      case Opcode::F32ConvertSI64:
      case Opcode::F32ConvertUI64:
      case Opcode::F64ConvertSI64:
      case Opcode::F64ConvertUI64:
      case Opcode::F64ReinterpretI64:
      case Opcode::I64ExtendSI32:
      case Opcode::I64ExtendUI32:
      case Opcode::F32ConvertSI32:
      case Opcode::F32ConvertUI32:
      case Opcode::F32ReinterpretI32:
      case Opcode::F64ConvertSI32:
      case Opcode::F64ConvertUI32:
      case Opcode::I32TruncSSatF32:
      case Opcode::I32TruncUSatF32:
      case Opcode::I64TruncSSatF32:
      case Opcode::I64TruncUSatF32:
      case Opcode::I32TruncSSatF64:
      case Opcode::I32TruncUSatF64:
      case Opcode::I64TruncSSatF64:
      case Opcode::I64TruncUSatF64:
        stream->Writef("%s %%[-1]\n", GetOpcodeName(opcode));
        break;

      case Opcode::GrowMemory: {
        Index memory_index = read_u32(&pc);
        stream->Writef("%s $%" PRIindex ":%%[-1]\n", GetOpcodeName(opcode),
                       memory_index);
        break;
      }

      case Opcode::Alloca:
        stream->Writef("%s $%u\n", GetOpcodeName(opcode), read_u32(&pc));
        break;

      case Opcode::BrUnless:
        stream->Writef("%s @%u, %%[-1]\n", GetOpcodeName(opcode),
                       read_u32(&pc));
        break;

      case Opcode::DropKeep: {
        uint32_t drop = read_u32(&pc);
        uint8_t keep = *pc++;
        stream->Writef("%s $%u $%u\n", GetOpcodeName(opcode), drop, keep);
        break;
      }

      case Opcode::Data: {
        uint32_t num_bytes = read_u32(&pc);
        stream->Writef("%s $%u\n", GetOpcodeName(opcode), num_bytes);
        /* for now, the only reason this is emitted is for br_table, so display
         * it as a list of table entries */
        if (num_bytes % WABT_TABLE_ENTRY_SIZE == 0) {
          Index num_entries = num_bytes / WABT_TABLE_ENTRY_SIZE;
          for (Index i = 0; i < num_entries; ++i) {
            stream->Writef("%4" PRIzd "| ", pc - istream);
            IstreamOffset offset;
            uint32_t drop;
            uint8_t keep;
            read_table_entry_at(pc, &offset, &drop, &keep);
            stream->Writef("  entry %" PRIindex
                           ": offset: %u drop: %u keep: %u\n",
                           i, offset, drop, keep);
            pc += WABT_TABLE_ENTRY_SIZE;
          }
        } else {
          /* just skip those data bytes */
          pc += num_bytes;
        }

        break;
      }

      default:
        assert(0);
        break;
    }
  }
}

void Environment::DisassembleModule(Stream* stream, Module* module) {
  assert(!module->is_host);
  Disassemble(stream, module->as_defined()->istream_start,
              module->as_defined()->istream_end);
}

}  // namespace interpreter
}  // namespace wabt
