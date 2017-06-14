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
#include <vector>

#include "stream.h"

namespace wabt {
namespace interpreter {

static const char* s_opcode_name[256] = {

#define WABT_OPCODE(rtype, type1, type2, mem_size, code, NAME, text) text,
#include "interpreter-opcode.def"
#undef WABT_OPCODE

};

#define CHECK_RESULT(expr)  \
  do {                      \
    if (WABT_FAILED(expr))  \
      return Result::Error; \
  } while (0)

static const char* get_opcode_name(Opcode opcode) {
  return s_opcode_name[static_cast<int>(opcode)];
}

Environment::Environment() : istream_(new OutputBuffer()) {}

Index Environment::FindModuleIndex(StringSlice name) const {
  auto iter = module_bindings_.find(string_slice_to_string(name));
  if (iter == module_bindings_.end())
    return kInvalidIndex;
  return iter->second.index;
}

Module* Environment::FindModule(StringSlice name) {
  Index index = FindModuleIndex(name);
  return index == kInvalidIndex ? nullptr : modules_[index].get();
}

Module* Environment::FindRegisteredModule(StringSlice name) {
  auto iter = registered_module_bindings_.find(string_slice_to_string(name));
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
  WABT_ZERO_MEMORY(module_name);
  WABT_ZERO_MEMORY(field_name);
  WABT_ZERO_MEMORY(func.sig_index);
}

Import::Import(Import&& other) {
  *this = std::move(other);
}

Import& Import::operator=(Import&& other) {
  kind = other.kind;
  module_name = other.module_name;
  WABT_ZERO_MEMORY(other.module_name);
  field_name = other.field_name;
  WABT_ZERO_MEMORY(other.field_name);
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

Import::~Import() {
  destroy_string_slice(&module_name);
  destroy_string_slice(&field_name);
}

Export::Export(Export&& other)
    : name(other.name), kind(other.kind), index(other.index) {
  WABT_ZERO_MEMORY(other.name);
}

Export& Export::operator=(Export&& other) {
  name = other.name;
  kind = other.kind;
  index = other.index;
  WABT_ZERO_MEMORY(other.name);
  return *this;
}

Export::~Export() {
  destroy_string_slice(&name);
}

Module::Module(bool is_host)
    : memory_index(kInvalidIndex),
      table_index(kInvalidIndex),
      is_host(is_host) {
  WABT_ZERO_MEMORY(name);
}

Module::Module(const StringSlice& name, bool is_host)
    : name(name),
      memory_index(kInvalidIndex),
      table_index(kInvalidIndex),
      is_host(is_host) {}

Module::~Module() {
  destroy_string_slice(&name);
}

Export* Module::GetExport(StringSlice name) {
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

HostModule::HostModule(const StringSlice& name) : Module(name, true) {}

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
    const StringSlice* name = &modules_[i]->name;
    if (!string_slice_is_empty(name))
      module_bindings_.erase(string_slice_to_string(*name));
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

HostModule* Environment::AppendHostModule(StringSlice name) {
  HostModule* module = new HostModule(dup_string_slice(name));
  modules_.emplace_back(module);
  registered_module_bindings_.emplace(string_slice_to_string(name),
                                      Binding(modules_.size() - 1));
  return module;
}

Result Thread::PushValue(Value value) {
  if (value_stack_top_ >= value_stack_end_)
    return Result::TrapValueStackExhausted;
  *value_stack_top_++ = value;
  return Result::Ok;
}

Result Thread::PushArgs(const FuncSignature* sig,
                        const std::vector<TypedValue>& args) {
  if (sig->param_types.size() != args.size())
    return interpreter::Result::ArgumentTypeMismatch;

  for (size_t i = 0; i < sig->param_types.size(); ++i) {
    if (sig->param_types[i] != args[i].type)
      return interpreter::Result::ArgumentTypeMismatch;

    interpreter::Result iresult = PushValue(args[i].value);
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

#define F32_MAX 0x7f7fffffU
#define F32_INF 0x7f800000U
#define F32_NEG_MAX 0xff7fffffU
#define F32_NEG_INF 0xff800000U
#define F32_NEG_ONE 0xbf800000U
#define F32_NEG_ZERO 0x80000000U
#define F32_QUIET_NAN 0x7fc00000U
#define F32_QUIET_NEG_NAN 0xffc00000U
#define F32_QUIET_NAN_BIT 0x00400000U
#define F32_SIG_BITS 23
#define F32_SIG_MASK 0x7fffff
#define F32_SIGN_MASK 0x80000000U

static bool is_nan_f32(uint32_t f32_bits) {
  return (f32_bits > F32_INF && f32_bits < F32_NEG_ZERO) ||
         (f32_bits > F32_NEG_INF);
}

bool is_canonical_nan_f32(uint32_t f32_bits) {
  return f32_bits == F32_QUIET_NAN || f32_bits == F32_QUIET_NEG_NAN;
}

bool is_arithmetic_nan_f32(uint32_t f32_bits) {
  return (f32_bits & F32_QUIET_NAN) == F32_QUIET_NAN;
}

static WABT_INLINE bool is_zero_f32(uint32_t f32_bits) {
  return f32_bits == 0 || f32_bits == F32_NEG_ZERO;
}

static WABT_INLINE bool is_in_range_i32_trunc_s_f32(uint32_t f32_bits) {
  return (f32_bits < 0x4f000000U) ||
         (f32_bits >= F32_NEG_ZERO && f32_bits <= 0xcf000000U);
}

static WABT_INLINE bool is_in_range_i64_trunc_s_f32(uint32_t f32_bits) {
  return (f32_bits < 0x5f000000U) ||
         (f32_bits >= F32_NEG_ZERO && f32_bits <= 0xdf000000U);
}

static WABT_INLINE bool is_in_range_i32_trunc_u_f32(uint32_t f32_bits) {
  return (f32_bits < 0x4f800000U) ||
         (f32_bits >= F32_NEG_ZERO && f32_bits < F32_NEG_ONE);
}

static WABT_INLINE bool is_in_range_i64_trunc_u_f32(uint32_t f32_bits) {
  return (f32_bits < 0x5f800000U) ||
         (f32_bits >= F32_NEG_ZERO && f32_bits < F32_NEG_ONE);
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

#define F64_INF 0x7ff0000000000000ULL
#define F64_NEG_INF 0xfff0000000000000ULL
#define F64_NEG_ONE 0xbff0000000000000ULL
#define F64_NEG_ZERO 0x8000000000000000ULL
#define F64_QUIET_NAN 0x7ff8000000000000ULL
#define F64_QUIET_NEG_NAN 0xfff8000000000000ULL
#define F64_QUIET_NAN_BIT 0x0008000000000000ULL
#define F64_SIG_BITS 52
#define F64_SIG_MASK 0xfffffffffffffULL
#define F64_SIGN_MASK 0x8000000000000000ULL

static bool is_nan_f64(uint64_t f64_bits) {
  return (f64_bits > F64_INF && f64_bits < F64_NEG_ZERO) ||
         (f64_bits > F64_NEG_INF);
}

bool is_canonical_nan_f64(uint64_t f64_bits) {
  return f64_bits == F64_QUIET_NAN || f64_bits == F64_QUIET_NEG_NAN;
}

bool is_arithmetic_nan_f64(uint64_t f64_bits) {
  return (f64_bits & F64_QUIET_NAN) == F64_QUIET_NAN;
}

static WABT_INLINE bool is_zero_f64(uint64_t f64_bits) {
  return f64_bits == 0 || f64_bits == F64_NEG_ZERO;
}

static WABT_INLINE bool is_in_range_i32_trunc_s_f64(uint64_t f64_bits) {
  return (f64_bits <= 0x41dfffffffc00000ULL) ||
         (f64_bits >= F64_NEG_ZERO && f64_bits <= 0xc1e0000000000000ULL);
}

static WABT_INLINE bool is_in_range_i32_trunc_u_f64(uint64_t f64_bits) {
  return (f64_bits <= 0x41efffffffe00000ULL) ||
         (f64_bits >= F64_NEG_ZERO && f64_bits < F64_NEG_ONE);
}

static WABT_INLINE bool is_in_range_i64_trunc_s_f64(uint64_t f64_bits) {
  return (f64_bits < 0x43e0000000000000ULL) ||
         (f64_bits >= F64_NEG_ZERO && f64_bits <= 0xc3e0000000000000ULL);
}

static WABT_INLINE bool is_in_range_i64_trunc_u_f64(uint64_t f64_bits) {
  return (f64_bits < 0x43f0000000000000ULL) ||
         (f64_bits >= F64_NEG_ZERO && f64_bits < F64_NEG_ONE);
}

static WABT_INLINE bool is_in_range_f64_demote_f32(uint64_t f64_bits) {
  return (f64_bits <= 0x47efffffe0000000ULL) ||
         (f64_bits >= F64_NEG_ZERO && f64_bits <= 0xc7efffffe0000000ULL);
}

/* The WebAssembly rounding mode means that these values (which are > F32_MAX)
 * should be rounded to F32_MAX and not set to infinity. Unfortunately, UBSAN
 * complains that the value is not representable as a float, so we'll special
 * case them. */
static WABT_INLINE bool is_in_range_f64_demote_f32_round_to_f32_max(
    uint64_t f64_bits) {
  return f64_bits > 0x47efffffe0000000ULL && f64_bits < 0x47effffff0000000ULL;
}

static WABT_INLINE bool is_in_range_f64_demote_f32_round_to_neg_f32_max(
    uint64_t f64_bits) {
  return f64_bits > 0xc7efffffe0000000ULL && f64_bits < 0xc7effffff0000000ULL;
}

#define IS_NAN_F32 is_nan_f32
#define IS_NAN_F64 is_nan_f64
#define IS_ZERO_F32 is_zero_f32
#define IS_ZERO_F64 is_zero_f64

#define DEFINE_BITCAST(name, src, dst) \
  static WABT_INLINE dst name(src x) { \
    dst result;                        \
    memcpy(&result, &x, sizeof(dst));  \
    return result;                     \
  }

DEFINE_BITCAST(bitcast_u32_to_i32, uint32_t, int32_t)
DEFINE_BITCAST(bitcast_u64_to_i64, uint64_t, int64_t)
DEFINE_BITCAST(bitcast_f32_to_u32, float, uint32_t)
DEFINE_BITCAST(bitcast_u32_to_f32, uint32_t, float)
DEFINE_BITCAST(bitcast_f64_to_u64, double, uint64_t)
DEFINE_BITCAST(bitcast_u64_to_f64, uint64_t, double)

#define bitcast_i32_to_u32(x) (static_cast<uint32_t>(x))
#define bitcast_i64_to_u64(x) (static_cast<uint64_t>(x))

#define VALUE_TYPE_I32 uint32_t
#define VALUE_TYPE_I64 uint64_t
#define VALUE_TYPE_F32 uint32_t
#define VALUE_TYPE_F64 uint64_t

#define VALUE_TYPE_SIGNED_MAX_I32 (0x80000000U)
#define VALUE_TYPE_UNSIGNED_MAX_I32 (0xFFFFFFFFU)
#define VALUE_TYPE_SIGNED_MAX_I64 static_cast<uint64_t>(0x8000000000000000ULL)
#define VALUE_TYPE_UNSIGNED_MAX_I64 static_cast<uint64_t>(0xFFFFFFFFFFFFFFFFULL)

#define FLOAT_TYPE_F32 float
#define FLOAT_TYPE_F64 double

#define MEM_TYPE_I8 int8_t
#define MEM_TYPE_U8 uint8_t
#define MEM_TYPE_I16 int16_t
#define MEM_TYPE_U16 uint16_t
#define MEM_TYPE_I32 int32_t
#define MEM_TYPE_U32 uint32_t
#define MEM_TYPE_I64 int64_t
#define MEM_TYPE_U64 uint64_t
#define MEM_TYPE_F32 uint32_t
#define MEM_TYPE_F64 uint64_t

#define MEM_TYPE_EXTEND_I32_I8 int32_t
#define MEM_TYPE_EXTEND_I32_U8 uint32_t
#define MEM_TYPE_EXTEND_I32_I16 int32_t
#define MEM_TYPE_EXTEND_I32_U16 uint32_t
#define MEM_TYPE_EXTEND_I32_I32 int32_t
#define MEM_TYPE_EXTEND_I32_U32 uint32_t

#define MEM_TYPE_EXTEND_I64_I8 int64_t
#define MEM_TYPE_EXTEND_I64_U8 uint64_t
#define MEM_TYPE_EXTEND_I64_I16 int64_t
#define MEM_TYPE_EXTEND_I64_U16 uint64_t
#define MEM_TYPE_EXTEND_I64_I32 int64_t
#define MEM_TYPE_EXTEND_I64_U32 uint64_t
#define MEM_TYPE_EXTEND_I64_I64 int64_t
#define MEM_TYPE_EXTEND_I64_U64 uint64_t

#define MEM_TYPE_EXTEND_F32_F32 uint32_t
#define MEM_TYPE_EXTEND_F64_F64 uint64_t

#define BITCAST_I32_TO_SIGNED bitcast_u32_to_i32
#define BITCAST_I64_TO_SIGNED bitcast_u64_to_i64
#define BITCAST_I32_TO_UNSIGNED bitcast_i32_to_u32
#define BITCAST_I64_TO_UNSIGNED bitcast_i64_to_u64

#define BITCAST_TO_F32 bitcast_u32_to_f32
#define BITCAST_TO_F64 bitcast_u64_to_f64
#define BITCAST_FROM_F32 bitcast_f32_to_u32
#define BITCAST_FROM_F64 bitcast_f64_to_u64

#define TYPE_FIELD_NAME_I32 i32
#define TYPE_FIELD_NAME_I64 i64
#define TYPE_FIELD_NAME_F32 f32_bits
#define TYPE_FIELD_NAME_F64 f64_bits

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
    PUSH_I32(-1);                     \
    break;                            \
  }

#define PUSH(v)                  \
  do {                           \
    CHECK_STACK();               \
    (*value_stack_top_++) = (v); \
  } while (0)

#define PUSH_TYPE(type, v)                         \
  do {                                             \
    CHECK_STACK();                                 \
    (*value_stack_top_++).TYPE_FIELD_NAME_##type = \
        static_cast<VALUE_TYPE_##type>(v);         \
  } while (0)

#define PUSH_I32(v) PUSH_TYPE(I32, (v))
#define PUSH_I64(v) PUSH_TYPE(I64, (v))
#define PUSH_F32(v) PUSH_TYPE(F32, (v))
#define PUSH_F64(v) PUSH_TYPE(F64, (v))

#define PICK(depth) (*(value_stack_top_ - (depth)))
#define TOP() (PICK(1))
#define POP() (*--value_stack_top_)
#define POP_I32() (POP().i32)
#define POP_I64() (POP().i64)
#define POP_F32() (POP().f32_bits)
#define POP_F64() (POP().f64_bits)
#define DROP_KEEP(drop, keep)   \
  do {                          \
    assert((keep) <= 1);        \
    if ((keep) == 1)            \
      PICK((drop) + 1) = TOP(); \
    value_stack_top_ -= (drop); \
  } while (0)

#define GOTO(offset) pc = &istream[offset]

#define PUSH_CALL()                                                  \
  do {                                                               \
    TRAP_IF(call_stack_top_ >= call_stack_end_, CallStackExhausted); \
    (*call_stack_top_++) = (pc - istream);                           \
  } while (0)

#define POP_CALL() (*--call_stack_top_)

#define GET_MEMORY(var)               \
  Index memory_index = read_u32(&pc); \
  Memory* var = &env_->memories_[memory_index]

#define LOAD(type, mem_type)                                              \
  do {                                                                    \
    GET_MEMORY(memory);                                                   \
    uint64_t offset = static_cast<uint64_t>(POP_I32()) + read_u32(&pc);   \
    MEM_TYPE_##mem_type value;                                            \
    TRAP_IF(offset + sizeof(value) > memory->data.size(),                 \
            MemoryAccessOutOfBounds);                                     \
    void* src = memory->data.data() + static_cast<IstreamOffset>(offset); \
    memcpy(&value, src, sizeof(MEM_TYPE_##mem_type));                     \
    PUSH_##type(static_cast<MEM_TYPE_EXTEND_##type##_##mem_type>(value)); \
  } while (0)

#define STORE(type, mem_type)                                             \
  do {                                                                    \
    GET_MEMORY(memory);                                                   \
    VALUE_TYPE_##type value = POP_##type();                               \
    uint64_t offset = static_cast<uint64_t>(POP_I32()) + read_u32(&pc);   \
    MEM_TYPE_##mem_type src = static_cast<MEM_TYPE_##mem_type>(value);    \
    TRAP_IF(offset + sizeof(src) > memory->data.size(),                   \
            MemoryAccessOutOfBounds);                                     \
    void* dst = memory->data.data() + static_cast<IstreamOffset>(offset); \
    memcpy(dst, &src, sizeof(MEM_TYPE_##mem_type));                       \
  } while (0)

#define BINOP(rtype, type, op)            \
  do {                                    \
    VALUE_TYPE_##type rhs = POP_##type(); \
    VALUE_TYPE_##type lhs = POP_##type(); \
    PUSH_##rtype(lhs op rhs);             \
  } while (0)

#define BINOP_SIGNED(rtype, type, op)                     \
  do {                                                    \
    VALUE_TYPE_##type rhs = POP_##type();                 \
    VALUE_TYPE_##type lhs = POP_##type();                 \
    PUSH_##rtype(BITCAST_##type##_TO_SIGNED(lhs)          \
                     op BITCAST_##type##_TO_SIGNED(rhs)); \
  } while (0)

#define SHIFT_MASK_I32 31
#define SHIFT_MASK_I64 63

#define BINOP_SHIFT(type, op, sign)                                          \
  do {                                                                       \
    VALUE_TYPE_##type rhs = POP_##type();                                    \
    VALUE_TYPE_##type lhs = POP_##type();                                    \
    PUSH_##type(BITCAST_##type##_TO_##sign(lhs) op(rhs& SHIFT_MASK_##type)); \
  } while (0)

#define ROT_LEFT_0_SHIFT_OP <<
#define ROT_LEFT_1_SHIFT_OP >>
#define ROT_RIGHT_0_SHIFT_OP >>
#define ROT_RIGHT_1_SHIFT_OP <<

#define BINOP_ROT(type, dir)                                               \
  do {                                                                     \
    VALUE_TYPE_##type rhs = POP_##type();                                  \
    VALUE_TYPE_##type lhs = POP_##type();                                  \
    uint32_t amount = rhs & SHIFT_MASK_##type;                             \
    if (WABT_LIKELY(amount != 0)) {                                        \
      PUSH_##type(                                                         \
          (lhs ROT_##dir##_0_SHIFT_OP amount) |                            \
          (lhs ROT_##dir##_1_SHIFT_OP((SHIFT_MASK_##type + 1) - amount))); \
    } else {                                                               \
      PUSH_##type(lhs);                                                    \
    }                                                                      \
  } while (0)

#define BINOP_DIV_REM_U(type, op)                          \
  do {                                                     \
    VALUE_TYPE_##type rhs = POP_##type();                  \
    VALUE_TYPE_##type lhs = POP_##type();                  \
    TRAP_IF(rhs == 0, IntegerDivideByZero);                \
    PUSH_##type(BITCAST_##type##_TO_UNSIGNED(lhs)          \
                    op BITCAST_##type##_TO_UNSIGNED(rhs)); \
  } while (0)

/* {i32,i64}.{div,rem}_s are special-cased because they trap when dividing the
 * max signed value by -1. The modulo operation on x86 uses the same
 * instruction to generate the quotient and the remainder. */
#define BINOP_DIV_S(type)                              \
  do {                                                 \
    VALUE_TYPE_##type rhs = POP_##type();              \
    VALUE_TYPE_##type lhs = POP_##type();              \
    TRAP_IF(rhs == 0, IntegerDivideByZero);            \
    TRAP_IF(lhs == VALUE_TYPE_SIGNED_MAX_##type &&     \
                rhs == VALUE_TYPE_UNSIGNED_MAX_##type, \
            IntegerOverflow);                          \
    PUSH_##type(BITCAST_##type##_TO_SIGNED(lhs) /      \
                BITCAST_##type##_TO_SIGNED(rhs));      \
  } while (0)

#define BINOP_REM_S(type)                                       \
  do {                                                          \
    VALUE_TYPE_##type rhs = POP_##type();                       \
    VALUE_TYPE_##type lhs = POP_##type();                       \
    TRAP_IF(rhs == 0, IntegerDivideByZero);                     \
    if (WABT_UNLIKELY(lhs == VALUE_TYPE_SIGNED_MAX_##type &&    \
                      rhs == VALUE_TYPE_UNSIGNED_MAX_##type)) { \
      PUSH_##type(0);                                           \
    } else {                                                    \
      PUSH_##type(BITCAST_##type##_TO_SIGNED(lhs) %             \
                  BITCAST_##type##_TO_SIGNED(rhs));             \
    }                                                           \
  } while (0)

#define UNOP_FLOAT(type, func)                                   \
  do {                                                           \
    FLOAT_TYPE_##type value = BITCAST_TO_##type(POP_##type());   \
    VALUE_TYPE_##type result = BITCAST_FROM_##type(func(value)); \
    if (WABT_UNLIKELY(IS_NAN_##type(result))) {                  \
      result |= type##_QUIET_NAN_BIT;                            \
    }                                                            \
    PUSH_##type(result);                                         \
    break;                                                       \
  } while (0)

#define BINOP_FLOAT(type, op)                                \
  do {                                                       \
    FLOAT_TYPE_##type rhs = BITCAST_TO_##type(POP_##type()); \
    FLOAT_TYPE_##type lhs = BITCAST_TO_##type(POP_##type()); \
    PUSH_##type(BITCAST_FROM_##type(lhs op rhs));            \
  } while (0)

#define BINOP_FLOAT_DIV(type)                                    \
  do {                                                           \
    VALUE_TYPE_##type rhs = POP_##type();                        \
    VALUE_TYPE_##type lhs = POP_##type();                        \
    if (WABT_UNLIKELY(IS_ZERO_##type(rhs))) {                    \
      if (IS_NAN_##type(lhs)) {                                  \
        PUSH_##type(lhs | type##_QUIET_NAN);                     \
      } else if (IS_ZERO_##type(lhs)) {                          \
        PUSH_##type(type##_QUIET_NAN);                           \
      } else {                                                   \
        VALUE_TYPE_##type sign =                                 \
            (lhs & type##_SIGN_MASK) ^ (rhs & type##_SIGN_MASK); \
        PUSH_##type(sign | type##_INF);                          \
      }                                                          \
    } else {                                                     \
      PUSH_##type(BITCAST_FROM_##type(BITCAST_TO_##type(lhs) /   \
                                      BITCAST_TO_##type(rhs)));  \
    }                                                            \
  } while (0)

#define BINOP_FLOAT_COMPARE(type, op)                        \
  do {                                                       \
    FLOAT_TYPE_##type rhs = BITCAST_TO_##type(POP_##type()); \
    FLOAT_TYPE_##type lhs = BITCAST_TO_##type(POP_##type()); \
    PUSH_I32(lhs op rhs);                                    \
  } while (0)

#define MIN_OP <
#define MAX_OP >

#define MINMAX_FLOAT(type, op)                                                 \
  do {                                                                         \
    VALUE_TYPE_##type rhs = POP_##type();                                      \
    VALUE_TYPE_##type lhs = POP_##type();                                      \
    VALUE_TYPE_##type result;                                                  \
    if (WABT_UNLIKELY(IS_NAN_##type(lhs))) {                                   \
      result = lhs | type##_QUIET_NAN_BIT;                                     \
    } else if (WABT_UNLIKELY(IS_NAN_##type(rhs))) {                            \
      result = rhs | type##_QUIET_NAN_BIT;                                     \
    } else if ((lhs ^ rhs) & type##_SIGN_MASK) {                               \
      /* min(-0.0, 0.0) => -0.0; since we know the sign bits are different, we \
       * can just use the inverse integer comparison (because the sign bit is  \
       * set when the value is negative) */                                    \
      result = !(lhs op##_OP rhs) ? lhs : rhs;                                 \
    } else {                                                                   \
      FLOAT_TYPE_##type float_rhs = BITCAST_TO_##type(rhs);                    \
      FLOAT_TYPE_##type float_lhs = BITCAST_TO_##type(lhs);                    \
      result = BITCAST_FROM_##type(float_lhs op##_OP float_rhs ? float_lhs     \
                                                               : float_rhs);   \
    }                                                                          \
    PUSH_##type(result);                                                       \
  } while (0)

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
    params[i - 1].value = POP();
    params[i - 1].type = sig->param_types[i - 1];
  }

  Result call_result =
      func->callback(func, sig, num_params, params.data(), num_results,
                     results.data(), func->user_data);
  TRAP_IF(call_result != Result::Ok, HostTrapped);

  for (size_t i = 0; i < num_results; ++i) {
    TRAP_IF(results[i].type != sig->result_types[i], HostResultTypeMismatch);
    PUSH(results[i].value);
  }

  return Result::Ok;
}

Result Thread::Run(int num_instructions, IstreamOffset* call_stack_return_top) {
  Result result = Result::Ok;
  assert(call_stack_return_top < call_stack_end_);

  const uint8_t* istream = env_->istream_->data.data();
  const uint8_t* pc = &istream[pc_];
  for (int i = 0; i < num_instructions; ++i) {
    Opcode opcode = static_cast<Opcode>(*pc++);
    switch (opcode) {
      case Opcode::Select: {
        VALUE_TYPE_I32 cond = POP_I32();
        Value false_ = POP();
        Value true_ = POP();
        PUSH(cond ? true_ : false_);
        break;
      }

      case Opcode::Br:
        GOTO(read_u32(&pc));
        break;

      case Opcode::BrIf: {
        IstreamOffset new_pc = read_u32(&pc);
        if (POP_I32())
          GOTO(new_pc);
        break;
      }

      case Opcode::BrTable: {
        Index num_targets = read_u32(&pc);
        IstreamOffset table_offset = read_u32(&pc);
        VALUE_TYPE_I32 key = POP_I32();
        IstreamOffset key_offset =
            (key >= num_targets ? num_targets : key) * WABT_TABLE_ENTRY_SIZE;
        const uint8_t* entry = istream + table_offset + key_offset;
        IstreamOffset new_pc;
        uint32_t drop_count;
        uint8_t keep_count;
        read_table_entry_at(entry, &new_pc, &drop_count, &keep_count);
        DROP_KEEP(drop_count, keep_count);
        GOTO(new_pc);
        break;
      }

      case Opcode::Return:
        if (call_stack_top_ == call_stack_return_top) {
          result = Result::Returned;
          goto exit_loop;
        }
        GOTO(POP_CALL());
        break;

      case Opcode::Unreachable:
        TRAP(Unreachable);
        break;

      case Opcode::I32Const:
        PUSH_I32(read_u32(&pc));
        break;

      case Opcode::I64Const:
        PUSH_I64(read_u64(&pc));
        break;

      case Opcode::F32Const:
        PUSH_F32(read_u32(&pc));
        break;

      case Opcode::F64Const:
        PUSH_F64(read_u64(&pc));
        break;

      case Opcode::GetGlobal: {
        Index index = read_u32(&pc);
        assert(index < env_->globals_.size());
        PUSH(env_->globals_[index].typed_value.value);
        break;
      }

      case Opcode::SetGlobal: {
        Index index = read_u32(&pc);
        assert(index < env_->globals_.size());
        env_->globals_[index].typed_value.value = POP();
        break;
      }

      case Opcode::GetLocal: {
        Value value = PICK(read_u32(&pc));
        PUSH(value);
        break;
      }

      case Opcode::SetLocal: {
        Value value = POP();
        PICK(read_u32(&pc)) = value;
        break;
      }

      case Opcode::TeeLocal:
        PICK(read_u32(&pc)) = TOP();
        break;

      case Opcode::Call: {
        IstreamOffset offset = read_u32(&pc);
        PUSH_CALL();
        GOTO(offset);
        break;
      }

      case Opcode::CallIndirect: {
        Index table_index = read_u32(&pc);
        Table* table = &env_->tables_[table_index];
        Index sig_index = read_u32(&pc);
        VALUE_TYPE_I32 entry_index = POP_I32();
        TRAP_IF(entry_index >= table->func_indexes.size(), UndefinedTableIndex);
        Index func_index = table->func_indexes[entry_index];
        TRAP_IF(func_index == kInvalidIndex, UninitializedTableElement);
        Func* func = env_->funcs_[func_index].get();
        TRAP_UNLESS(env_->FuncSignaturesAreEqual(func->sig_index, sig_index),
                    IndirectCallSignatureMismatch);
        if (func->is_host) {
          CallHost(func->as_host());
        } else {
          PUSH_CALL();
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
        LOAD(I32, I8);
        break;

      case Opcode::I32Load8U:
        LOAD(I32, U8);
        break;

      case Opcode::I32Load16S:
        LOAD(I32, I16);
        break;

      case Opcode::I32Load16U:
        LOAD(I32, U16);
        break;

      case Opcode::I64Load8S:
        LOAD(I64, I8);
        break;

      case Opcode::I64Load8U:
        LOAD(I64, U8);
        break;

      case Opcode::I64Load16S:
        LOAD(I64, I16);
        break;

      case Opcode::I64Load16U:
        LOAD(I64, U16);
        break;

      case Opcode::I64Load32S:
        LOAD(I64, I32);
        break;

      case Opcode::I64Load32U:
        LOAD(I64, U32);
        break;

      case Opcode::I32Load:
        LOAD(I32, U32);
        break;

      case Opcode::I64Load:
        LOAD(I64, U64);
        break;

      case Opcode::F32Load:
        LOAD(F32, F32);
        break;

      case Opcode::F64Load:
        LOAD(F64, F64);
        break;

      case Opcode::I32Store8:
        STORE(I32, U8);
        break;

      case Opcode::I32Store16:
        STORE(I32, U16);
        break;

      case Opcode::I64Store8:
        STORE(I64, U8);
        break;

      case Opcode::I64Store16:
        STORE(I64, U16);
        break;

      case Opcode::I64Store32:
        STORE(I64, U32);
        break;

      case Opcode::I32Store:
        STORE(I32, U32);
        break;

      case Opcode::I64Store:
        STORE(I64, U64);
        break;

      case Opcode::F32Store:
        STORE(F32, F32);
        break;

      case Opcode::F64Store:
        STORE(F64, F64);
        break;

      case Opcode::CurrentMemory: {
        GET_MEMORY(memory);
        PUSH_I32(memory->page_limits.initial);
        break;
      }

      case Opcode::GrowMemory: {
        GET_MEMORY(memory);
        uint32_t old_page_size = memory->page_limits.initial;
        VALUE_TYPE_I32 grow_pages = POP_I32();
        uint32_t new_page_size = old_page_size + grow_pages;
        uint32_t max_page_size = memory->page_limits.has_max
                                     ? memory->page_limits.max
                                     : WABT_MAX_PAGES;
        PUSH_NEG_1_AND_BREAK_IF(new_page_size > max_page_size);
        PUSH_NEG_1_AND_BREAK_IF(
            static_cast<uint64_t>(new_page_size) * WABT_PAGE_SIZE > UINT32_MAX);
        memory->data.resize(new_page_size * WABT_PAGE_SIZE);
        memory->page_limits.initial = new_page_size;
        PUSH_I32(old_page_size);
        break;
      }

      case Opcode::I32Add:
        BINOP(I32, I32, +);
        break;

      case Opcode::I32Sub:
        BINOP(I32, I32, -);
        break;

      case Opcode::I32Mul:
        BINOP(I32, I32, *);
        break;

      case Opcode::I32DivS:
        BINOP_DIV_S(I32);
        break;

      case Opcode::I32DivU:
        BINOP_DIV_REM_U(I32, /);
        break;

      case Opcode::I32RemS:
        BINOP_REM_S(I32);
        break;

      case Opcode::I32RemU:
        BINOP_DIV_REM_U(I32, %);
        break;

      case Opcode::I32And:
        BINOP(I32, I32, &);
        break;

      case Opcode::I32Or:
        BINOP(I32, I32, |);
        break;

      case Opcode::I32Xor:
        BINOP(I32, I32, ^);
        break;

      case Opcode::I32Shl:
        BINOP_SHIFT(I32, <<, UNSIGNED);
        break;

      case Opcode::I32ShrU:
        BINOP_SHIFT(I32, >>, UNSIGNED);
        break;

      case Opcode::I32ShrS:
        BINOP_SHIFT(I32, >>, SIGNED);
        break;

      case Opcode::I32Eq:
        BINOP(I32, I32, ==);
        break;

      case Opcode::I32Ne:
        BINOP(I32, I32, !=);
        break;

      case Opcode::I32LtS:
        BINOP_SIGNED(I32, I32, <);
        break;

      case Opcode::I32LeS:
        BINOP_SIGNED(I32, I32, <=);
        break;

      case Opcode::I32LtU:
        BINOP(I32, I32, <);
        break;

      case Opcode::I32LeU:
        BINOP(I32, I32, <=);
        break;

      case Opcode::I32GtS:
        BINOP_SIGNED(I32, I32, >);
        break;

      case Opcode::I32GeS:
        BINOP_SIGNED(I32, I32, >=);
        break;

      case Opcode::I32GtU:
        BINOP(I32, I32, >);
        break;

      case Opcode::I32GeU:
        BINOP(I32, I32, >=);
        break;

      case Opcode::I32Clz: {
        VALUE_TYPE_I32 value = POP_I32();
        PUSH_I32(value != 0 ? wabt_clz_u32(value) : 32);
        break;
      }

      case Opcode::I32Ctz: {
        VALUE_TYPE_I32 value = POP_I32();
        PUSH_I32(value != 0 ? wabt_ctz_u32(value) : 32);
        break;
      }

      case Opcode::I32Popcnt: {
        VALUE_TYPE_I32 value = POP_I32();
        PUSH_I32(wabt_popcount_u32(value));
        break;
      }

      case Opcode::I32Eqz: {
        VALUE_TYPE_I32 value = POP_I32();
        PUSH_I32(value == 0);
        break;
      }

      case Opcode::I64Add:
        BINOP(I64, I64, +);
        break;

      case Opcode::I64Sub:
        BINOP(I64, I64, -);
        break;

      case Opcode::I64Mul:
        BINOP(I64, I64, *);
        break;

      case Opcode::I64DivS:
        BINOP_DIV_S(I64);
        break;

      case Opcode::I64DivU:
        BINOP_DIV_REM_U(I64, /);
        break;

      case Opcode::I64RemS:
        BINOP_REM_S(I64);
        break;

      case Opcode::I64RemU:
        BINOP_DIV_REM_U(I64, %);
        break;

      case Opcode::I64And:
        BINOP(I64, I64, &);
        break;

      case Opcode::I64Or:
        BINOP(I64, I64, |);
        break;

      case Opcode::I64Xor:
        BINOP(I64, I64, ^);
        break;

      case Opcode::I64Shl:
        BINOP_SHIFT(I64, <<, UNSIGNED);
        break;

      case Opcode::I64ShrU:
        BINOP_SHIFT(I64, >>, UNSIGNED);
        break;

      case Opcode::I64ShrS:
        BINOP_SHIFT(I64, >>, SIGNED);
        break;

      case Opcode::I64Eq:
        BINOP(I32, I64, ==);
        break;

      case Opcode::I64Ne:
        BINOP(I32, I64, !=);
        break;

      case Opcode::I64LtS:
        BINOP_SIGNED(I32, I64, <);
        break;

      case Opcode::I64LeS:
        BINOP_SIGNED(I32, I64, <=);
        break;

      case Opcode::I64LtU:
        BINOP(I32, I64, <);
        break;

      case Opcode::I64LeU:
        BINOP(I32, I64, <=);
        break;

      case Opcode::I64GtS:
        BINOP_SIGNED(I32, I64, >);
        break;

      case Opcode::I64GeS:
        BINOP_SIGNED(I32, I64, >=);
        break;

      case Opcode::I64GtU:
        BINOP(I32, I64, >);
        break;

      case Opcode::I64GeU:
        BINOP(I32, I64, >=);
        break;

      case Opcode::I64Clz: {
        VALUE_TYPE_I64 value = POP_I64();
        PUSH_I64(value != 0 ? wabt_clz_u64(value) : 64);
        break;
      }

      case Opcode::I64Ctz: {
        VALUE_TYPE_I64 value = POP_I64();
        PUSH_I64(value != 0 ? wabt_ctz_u64(value) : 64);
        break;
      }

      case Opcode::I64Popcnt: {
        VALUE_TYPE_I64 value = POP_I64();
        PUSH_I64(wabt_popcount_u64(value));
        break;
      }

      case Opcode::F32Add:
        BINOP_FLOAT(F32, +);
        break;

      case Opcode::F32Sub:
        BINOP_FLOAT(F32, -);
        break;

      case Opcode::F32Mul:
        BINOP_FLOAT(F32, *);
        break;

      case Opcode::F32Div:
        BINOP_FLOAT_DIV(F32);
        break;

      case Opcode::F32Min:
        MINMAX_FLOAT(F32, MIN);
        break;

      case Opcode::F32Max:
        MINMAX_FLOAT(F32, MAX);
        break;

      case Opcode::F32Abs:
        TOP().f32_bits &= ~F32_SIGN_MASK;
        break;

      case Opcode::F32Neg:
        TOP().f32_bits ^= F32_SIGN_MASK;
        break;

      case Opcode::F32Copysign: {
        VALUE_TYPE_F32 rhs = POP_F32();
        VALUE_TYPE_F32 lhs = POP_F32();
        PUSH_F32((lhs & ~F32_SIGN_MASK) | (rhs & F32_SIGN_MASK));
        break;
      }

      case Opcode::F32Ceil:
        UNOP_FLOAT(F32, ceilf);
        break;

      case Opcode::F32Floor:
        UNOP_FLOAT(F32, floorf);
        break;

      case Opcode::F32Trunc:
        UNOP_FLOAT(F32, truncf);
        break;

      case Opcode::F32Nearest:
        UNOP_FLOAT(F32, nearbyintf);
        break;

      case Opcode::F32Sqrt:
        UNOP_FLOAT(F32, sqrtf);
        break;

      case Opcode::F32Eq:
        BINOP_FLOAT_COMPARE(F32, ==);
        break;

      case Opcode::F32Ne:
        BINOP_FLOAT_COMPARE(F32, !=);
        break;

      case Opcode::F32Lt:
        BINOP_FLOAT_COMPARE(F32, <);
        break;

      case Opcode::F32Le:
        BINOP_FLOAT_COMPARE(F32, <=);
        break;

      case Opcode::F32Gt:
        BINOP_FLOAT_COMPARE(F32, >);
        break;

      case Opcode::F32Ge:
        BINOP_FLOAT_COMPARE(F32, >=);
        break;

      case Opcode::F64Add:
        BINOP_FLOAT(F64, +);
        break;

      case Opcode::F64Sub:
        BINOP_FLOAT(F64, -);
        break;

      case Opcode::F64Mul:
        BINOP_FLOAT(F64, *);
        break;

      case Opcode::F64Div:
        BINOP_FLOAT_DIV(F64);
        break;

      case Opcode::F64Min:
        MINMAX_FLOAT(F64, MIN);
        break;

      case Opcode::F64Max:
        MINMAX_FLOAT(F64, MAX);
        break;

      case Opcode::F64Abs:
        TOP().f64_bits &= ~F64_SIGN_MASK;
        break;

      case Opcode::F64Neg:
        TOP().f64_bits ^= F64_SIGN_MASK;
        break;

      case Opcode::F64Copysign: {
        VALUE_TYPE_F64 rhs = POP_F64();
        VALUE_TYPE_F64 lhs = POP_F64();
        PUSH_F64((lhs & ~F64_SIGN_MASK) | (rhs & F64_SIGN_MASK));
        break;
      }

      case Opcode::F64Ceil:
        UNOP_FLOAT(F64, ceil);
        break;

      case Opcode::F64Floor:
        UNOP_FLOAT(F64, floor);
        break;

      case Opcode::F64Trunc:
        UNOP_FLOAT(F64, trunc);
        break;

      case Opcode::F64Nearest:
        UNOP_FLOAT(F64, nearbyint);
        break;

      case Opcode::F64Sqrt:
        UNOP_FLOAT(F64, sqrt);
        break;

      case Opcode::F64Eq:
        BINOP_FLOAT_COMPARE(F64, ==);
        break;

      case Opcode::F64Ne:
        BINOP_FLOAT_COMPARE(F64, !=);
        break;

      case Opcode::F64Lt:
        BINOP_FLOAT_COMPARE(F64, <);
        break;

      case Opcode::F64Le:
        BINOP_FLOAT_COMPARE(F64, <=);
        break;

      case Opcode::F64Gt:
        BINOP_FLOAT_COMPARE(F64, >);
        break;

      case Opcode::F64Ge:
        BINOP_FLOAT_COMPARE(F64, >=);
        break;

      case Opcode::I32TruncSF32: {
        VALUE_TYPE_F32 value = POP_F32();
        TRAP_IF(is_nan_f32(value), InvalidConversionToInteger);
        TRAP_UNLESS(is_in_range_i32_trunc_s_f32(value), IntegerOverflow);
        PUSH_I32(static_cast<int32_t>(BITCAST_TO_F32(value)));
        break;
      }

      case Opcode::I32TruncSF64: {
        VALUE_TYPE_F64 value = POP_F64();
        TRAP_IF(is_nan_f64(value), InvalidConversionToInteger);
        TRAP_UNLESS(is_in_range_i32_trunc_s_f64(value), IntegerOverflow);
        PUSH_I32(static_cast<int32_t>(BITCAST_TO_F64(value)));
        break;
      }

      case Opcode::I32TruncUF32: {
        VALUE_TYPE_F32 value = POP_F32();
        TRAP_IF(is_nan_f32(value), InvalidConversionToInteger);
        TRAP_UNLESS(is_in_range_i32_trunc_u_f32(value), IntegerOverflow);
        PUSH_I32(static_cast<uint32_t>(BITCAST_TO_F32(value)));
        break;
      }

      case Opcode::I32TruncUF64: {
        VALUE_TYPE_F64 value = POP_F64();
        TRAP_IF(is_nan_f64(value), InvalidConversionToInteger);
        TRAP_UNLESS(is_in_range_i32_trunc_u_f64(value), IntegerOverflow);
        PUSH_I32(static_cast<uint32_t>(BITCAST_TO_F64(value)));
        break;
      }

      case Opcode::I32WrapI64: {
        VALUE_TYPE_I64 value = POP_I64();
        PUSH_I32(static_cast<uint32_t>(value));
        break;
      }

      case Opcode::I64TruncSF32: {
        VALUE_TYPE_F32 value = POP_F32();
        TRAP_IF(is_nan_f32(value), InvalidConversionToInteger);
        TRAP_UNLESS(is_in_range_i64_trunc_s_f32(value), IntegerOverflow);
        PUSH_I64(static_cast<int64_t>(BITCAST_TO_F32(value)));
        break;
      }

      case Opcode::I64TruncSF64: {
        VALUE_TYPE_F64 value = POP_F64();
        TRAP_IF(is_nan_f64(value), InvalidConversionToInteger);
        TRAP_UNLESS(is_in_range_i64_trunc_s_f64(value), IntegerOverflow);
        PUSH_I64(static_cast<int64_t>(BITCAST_TO_F64(value)));
        break;
      }

      case Opcode::I64TruncUF32: {
        VALUE_TYPE_F32 value = POP_F32();
        TRAP_IF(is_nan_f32(value), InvalidConversionToInteger);
        TRAP_UNLESS(is_in_range_i64_trunc_u_f32(value), IntegerOverflow);
        PUSH_I64(static_cast<uint64_t>(BITCAST_TO_F32(value)));
        break;
      }

      case Opcode::I64TruncUF64: {
        VALUE_TYPE_F64 value = POP_F64();
        TRAP_IF(is_nan_f64(value), InvalidConversionToInteger);
        TRAP_UNLESS(is_in_range_i64_trunc_u_f64(value), IntegerOverflow);
        PUSH_I64(static_cast<uint64_t>(BITCAST_TO_F64(value)));
        break;
      }

      case Opcode::I64ExtendSI32: {
        VALUE_TYPE_I32 value = POP_I32();
        PUSH_I64(static_cast<int64_t>(BITCAST_I32_TO_SIGNED(value)));
        break;
      }

      case Opcode::I64ExtendUI32: {
        VALUE_TYPE_I32 value = POP_I32();
        PUSH_I64(static_cast<uint64_t>(value));
        break;
      }

      case Opcode::F32ConvertSI32: {
        VALUE_TYPE_I32 value = POP_I32();
        PUSH_F32(
            BITCAST_FROM_F32(static_cast<float>(BITCAST_I32_TO_SIGNED(value))));
        break;
      }

      case Opcode::F32ConvertUI32: {
        VALUE_TYPE_I32 value = POP_I32();
        PUSH_F32(BITCAST_FROM_F32(static_cast<float>(value)));
        break;
      }

      case Opcode::F32ConvertSI64: {
        VALUE_TYPE_I64 value = POP_I64();
        PUSH_F32(
            BITCAST_FROM_F32(static_cast<float>(BITCAST_I64_TO_SIGNED(value))));
        break;
      }

      case Opcode::F32ConvertUI64: {
        VALUE_TYPE_I64 value = POP_I64();
        PUSH_F32(BITCAST_FROM_F32(wabt_convert_uint64_to_float(value)));
        break;
      }

      case Opcode::F32DemoteF64: {
        VALUE_TYPE_F64 value = POP_F64();
        if (WABT_LIKELY(is_in_range_f64_demote_f32(value))) {
          PUSH_F32(BITCAST_FROM_F32(static_cast<float>(BITCAST_TO_F64(value))));
        } else if (is_in_range_f64_demote_f32_round_to_f32_max(value)) {
          PUSH_F32(F32_MAX);
        } else if (is_in_range_f64_demote_f32_round_to_neg_f32_max(value)) {
          PUSH_F32(F32_NEG_MAX);
        } else {
          uint32_t sign = (value >> 32) & F32_SIGN_MASK;
          uint32_t tag = 0;
          if (IS_NAN_F64(value)) {
            tag = F32_QUIET_NAN_BIT |
                  ((value >> (F64_SIG_BITS - F32_SIG_BITS)) & F32_SIG_MASK);
          }
          PUSH_F32(sign | F32_INF | tag);
        }
        break;
      }

      case Opcode::F32ReinterpretI32: {
        VALUE_TYPE_I32 value = POP_I32();
        PUSH_F32(value);
        break;
      }

      case Opcode::F64ConvertSI32: {
        VALUE_TYPE_I32 value = POP_I32();
        PUSH_F64(BITCAST_FROM_F64(
            static_cast<double>(BITCAST_I32_TO_SIGNED(value))));
        break;
      }

      case Opcode::F64ConvertUI32: {
        VALUE_TYPE_I32 value = POP_I32();
        PUSH_F64(BITCAST_FROM_F64(static_cast<double>(value)));
        break;
      }

      case Opcode::F64ConvertSI64: {
        VALUE_TYPE_I64 value = POP_I64();
        PUSH_F64(BITCAST_FROM_F64(
            static_cast<double>(BITCAST_I64_TO_SIGNED(value))));
        break;
      }

      case Opcode::F64ConvertUI64: {
        VALUE_TYPE_I64 value = POP_I64();
        PUSH_F64(BITCAST_FROM_F64(wabt_convert_uint64_to_double(value)));
        break;
      }

      case Opcode::F64PromoteF32: {
        VALUE_TYPE_F32 value = POP_F32();
        PUSH_F64(BITCAST_FROM_F64(static_cast<double>(BITCAST_TO_F32(value))));
        break;
      }

      case Opcode::F64ReinterpretI64: {
        VALUE_TYPE_I64 value = POP_I64();
        PUSH_F64(value);
        break;
      }

      case Opcode::I32ReinterpretF32: {
        VALUE_TYPE_F32 value = POP_F32();
        PUSH_I32(value);
        break;
      }

      case Opcode::I64ReinterpretF64: {
        VALUE_TYPE_F64 value = POP_F64();
        PUSH_I64(value);
        break;
      }

      case Opcode::I32Rotr:
        BINOP_ROT(I32, RIGHT);
        break;

      case Opcode::I32Rotl:
        BINOP_ROT(I32, LEFT);
        break;

      case Opcode::I64Rotr:
        BINOP_ROT(I64, RIGHT);
        break;

      case Opcode::I64Rotl:
        BINOP_ROT(I64, LEFT);
        break;

      case Opcode::I64Eqz: {
        VALUE_TYPE_I64 value = POP_I64();
        PUSH_I64(value == 0);
        break;
      }

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
        if (!POP_I32())
          GOTO(new_pc);
        break;
      }

      case Opcode::Drop:
        (void)POP();
        break;

      case Opcode::DropKeep: {
        uint32_t drop_count = read_u32(&pc);
        uint8_t keep_count = *pc++;
        DROP_KEEP(drop_count, keep_count);
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
  const uint8_t* istream = env_->istream_->data.data();
  const uint8_t* pc = &istream[pc_];
  size_t value_stack_depth = value_stack_top_ - value_stack_.data();
  size_t call_stack_depth = call_stack_top_ - call_stack_.data();

  stream->Writef("#%" PRIzd ". %4" PRIzd ": V:%-3" PRIzd "| ", call_stack_depth,
                 pc - env_->istream_->data.data(), value_stack_depth);

  Opcode opcode = static_cast<Opcode>(*pc++);
  switch (opcode) {
    case Opcode::Select:
      stream->Writef("%s %u, %" PRIu64 ", %" PRIu64 "\n",
                     get_opcode_name(opcode), PICK(3).i32, PICK(2).i64,
                     PICK(1).i64);
      break;

    case Opcode::Br:
      stream->Writef("%s @%u\n", get_opcode_name(opcode), read_u32_at(pc));
      break;

    case Opcode::BrIf:
      stream->Writef("%s @%u, %u\n", get_opcode_name(opcode), read_u32_at(pc),
                     TOP().i32);
      break;

    case Opcode::BrTable: {
      Index num_targets = read_u32_at(pc);
      IstreamOffset table_offset = read_u32_at(pc + 4);
      VALUE_TYPE_I32 key = TOP().i32;
      stream->Writef("%s %u, $#%" PRIindex ", table:$%u\n",
                     get_opcode_name(opcode), key, num_targets, table_offset);
      break;
    }

    case Opcode::Nop:
    case Opcode::Return:
    case Opcode::Unreachable:
    case Opcode::Drop:
      stream->Writef("%s\n", get_opcode_name(opcode));
      break;

    case Opcode::CurrentMemory: {
      Index memory_index = read_u32(&pc);
      stream->Writef("%s $%" PRIindex "\n", get_opcode_name(opcode),
                     memory_index);
      break;
    }

    case Opcode::I32Const:
      stream->Writef("%s $%u\n", get_opcode_name(opcode), read_u32_at(pc));
      break;

    case Opcode::I64Const:
      stream->Writef("%s $%" PRIu64 "\n", get_opcode_name(opcode),
                     read_u64_at(pc));
      break;

    case Opcode::F32Const:
      stream->Writef("%s $%g\n", get_opcode_name(opcode),
                     bitcast_u32_to_f32(read_u32_at(pc)));
      break;

    case Opcode::F64Const:
      stream->Writef("%s $%g\n", get_opcode_name(opcode),
                     bitcast_u64_to_f64(read_u64_at(pc)));
      break;

    case Opcode::GetLocal:
    case Opcode::GetGlobal:
      stream->Writef("%s $%u\n", get_opcode_name(opcode), read_u32_at(pc));
      break;

    case Opcode::SetLocal:
    case Opcode::SetGlobal:
    case Opcode::TeeLocal:
      stream->Writef("%s $%u, %u\n", get_opcode_name(opcode), read_u32_at(pc),
                     TOP().i32);
      break;

    case Opcode::Call:
      stream->Writef("%s @%u\n", get_opcode_name(opcode), read_u32_at(pc));
      break;

    case Opcode::CallIndirect:
      stream->Writef("%s $%u, %u\n", get_opcode_name(opcode), read_u32_at(pc),
                     TOP().i32);
      break;

    case Opcode::CallHost:
      stream->Writef("%s $%u\n", get_opcode_name(opcode), read_u32_at(pc));
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
      stream->Writef("%s $%" PRIindex ":%u+$%u\n", get_opcode_name(opcode),
                     memory_index, TOP().i32, read_u32_at(pc));
      break;
    }

    case Opcode::I32Store8:
    case Opcode::I32Store16:
    case Opcode::I32Store: {
      Index memory_index = read_u32(&pc);
      stream->Writef("%s $%" PRIindex ":%u+$%u, %u\n", get_opcode_name(opcode),
                     memory_index, PICK(2).i32, read_u32_at(pc), PICK(1).i32);
      break;
    }

    case Opcode::I64Store8:
    case Opcode::I64Store16:
    case Opcode::I64Store32:
    case Opcode::I64Store: {
      Index memory_index = read_u32(&pc);
      stream->Writef("%s $%" PRIindex ":%u+$%u, %" PRIu64 "\n",
                     get_opcode_name(opcode), memory_index, PICK(2).i32,
                     read_u32_at(pc), PICK(1).i64);
      break;
    }

    case Opcode::F32Store: {
      Index memory_index = read_u32(&pc);
      stream->Writef("%s $%" PRIindex ":%u+$%u, %g\n", get_opcode_name(opcode),
                     memory_index, PICK(2).i32, read_u32_at(pc),
                     bitcast_u32_to_f32(PICK(1).f32_bits));
      break;
    }

    case Opcode::F64Store: {
      Index memory_index = read_u32(&pc);
      stream->Writef("%s $%" PRIindex ":%u+$%u, %g\n", get_opcode_name(opcode),
                     memory_index, PICK(2).i32, read_u32_at(pc),
                     bitcast_u64_to_f64(PICK(1).f64_bits));
      break;
    }

    case Opcode::GrowMemory: {
      Index memory_index = read_u32(&pc);
      stream->Writef("%s $%" PRIindex ":%u\n", get_opcode_name(opcode),
                     memory_index, TOP().i32);
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
      stream->Writef("%s %u, %u\n", get_opcode_name(opcode), PICK(2).i32,
                     PICK(1).i32);
      break;

    case Opcode::I32Clz:
    case Opcode::I32Ctz:
    case Opcode::I32Popcnt:
    case Opcode::I32Eqz:
      stream->Writef("%s %u\n", get_opcode_name(opcode), TOP().i32);
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
      stream->Writef("%s %" PRIu64 ", %" PRIu64 "\n", get_opcode_name(opcode),
                     PICK(2).i64, PICK(1).i64);
      break;

    case Opcode::I64Clz:
    case Opcode::I64Ctz:
    case Opcode::I64Popcnt:
    case Opcode::I64Eqz:
      stream->Writef("%s %" PRIu64 "\n", get_opcode_name(opcode), TOP().i64);
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
      stream->Writef("%s %g, %g\n", get_opcode_name(opcode),
                     bitcast_u32_to_f32(PICK(2).i32),
                     bitcast_u32_to_f32(PICK(1).i32));
      break;

    case Opcode::F32Abs:
    case Opcode::F32Neg:
    case Opcode::F32Ceil:
    case Opcode::F32Floor:
    case Opcode::F32Trunc:
    case Opcode::F32Nearest:
    case Opcode::F32Sqrt:
      stream->Writef("%s %g\n", get_opcode_name(opcode),
                     bitcast_u32_to_f32(TOP().i32));
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
      stream->Writef("%s %g, %g\n", get_opcode_name(opcode),
                     bitcast_u64_to_f64(PICK(2).i64),
                     bitcast_u64_to_f64(PICK(1).i64));
      break;

    case Opcode::F64Abs:
    case Opcode::F64Neg:
    case Opcode::F64Ceil:
    case Opcode::F64Floor:
    case Opcode::F64Trunc:
    case Opcode::F64Nearest:
    case Opcode::F64Sqrt:
      stream->Writef("%s %g\n", get_opcode_name(opcode),
                     bitcast_u64_to_f64(TOP().i64));
      break;

    case Opcode::I32TruncSF32:
    case Opcode::I32TruncUF32:
    case Opcode::I64TruncSF32:
    case Opcode::I64TruncUF32:
    case Opcode::F64PromoteF32:
    case Opcode::I32ReinterpretF32:
      stream->Writef("%s %g\n", get_opcode_name(opcode),
                     bitcast_u32_to_f32(TOP().i32));
      break;

    case Opcode::I32TruncSF64:
    case Opcode::I32TruncUF64:
    case Opcode::I64TruncSF64:
    case Opcode::I64TruncUF64:
    case Opcode::F32DemoteF64:
    case Opcode::I64ReinterpretF64:
      stream->Writef("%s %g\n", get_opcode_name(opcode),
                     bitcast_u64_to_f64(TOP().i64));
      break;

    case Opcode::I32WrapI64:
    case Opcode::F32ConvertSI64:
    case Opcode::F32ConvertUI64:
    case Opcode::F64ConvertSI64:
    case Opcode::F64ConvertUI64:
    case Opcode::F64ReinterpretI64:
      stream->Writef("%s %" PRIu64 "\n", get_opcode_name(opcode), TOP().i64);
      break;

    case Opcode::I64ExtendSI32:
    case Opcode::I64ExtendUI32:
    case Opcode::F32ConvertSI32:
    case Opcode::F32ConvertUI32:
    case Opcode::F32ReinterpretI32:
    case Opcode::F64ConvertSI32:
    case Opcode::F64ConvertUI32:
      stream->Writef("%s %u\n", get_opcode_name(opcode), TOP().i32);
      break;

    case Opcode::Alloca:
      stream->Writef("%s $%u\n", get_opcode_name(opcode), read_u32_at(pc));
      break;

    case Opcode::BrUnless:
      stream->Writef("%s @%u, %u\n", get_opcode_name(opcode), read_u32_at(pc),
                     TOP().i32);
      break;

    case Opcode::DropKeep:
      stream->Writef("%s $%u $%u\n", get_opcode_name(opcode), read_u32_at(pc),
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
        stream->Writef("%s %%[-3], %%[-2], %%[-1]\n", get_opcode_name(opcode));
        break;

      case Opcode::Br:
        stream->Writef("%s @%u\n", get_opcode_name(opcode), read_u32(&pc));
        break;

      case Opcode::BrIf:
        stream->Writef("%s @%u, %%[-1]\n", get_opcode_name(opcode),
                       read_u32(&pc));
        break;

      case Opcode::BrTable: {
        Index num_targets = read_u32(&pc);
        IstreamOffset table_offset = read_u32(&pc);
        stream->Writef("%s %%[-1], $#%" PRIindex ", table:$%u\n",
                       get_opcode_name(opcode), num_targets, table_offset);
        break;
      }

      case Opcode::Nop:
      case Opcode::Return:
      case Opcode::Unreachable:
      case Opcode::Drop:
        stream->Writef("%s\n", get_opcode_name(opcode));
        break;

      case Opcode::CurrentMemory: {
        Index memory_index = read_u32(&pc);
        stream->Writef("%s $%" PRIindex "\n", get_opcode_name(opcode),
                       memory_index);
        break;
      }

      case Opcode::I32Const:
        stream->Writef("%s $%u\n", get_opcode_name(opcode), read_u32(&pc));
        break;

      case Opcode::I64Const:
        stream->Writef("%s $%" PRIu64 "\n", get_opcode_name(opcode),
                       read_u64(&pc));
        break;

      case Opcode::F32Const:
        stream->Writef("%s $%g\n", get_opcode_name(opcode),
                       bitcast_u32_to_f32(read_u32(&pc)));
        break;

      case Opcode::F64Const:
        stream->Writef("%s $%g\n", get_opcode_name(opcode),
                       bitcast_u64_to_f64(read_u64(&pc)));
        break;

      case Opcode::GetLocal:
      case Opcode::GetGlobal:
        stream->Writef("%s $%u\n", get_opcode_name(opcode), read_u32(&pc));
        break;

      case Opcode::SetLocal:
      case Opcode::SetGlobal:
      case Opcode::TeeLocal:
        stream->Writef("%s $%u, %%[-1]\n", get_opcode_name(opcode),
                       read_u32(&pc));
        break;

      case Opcode::Call:
        stream->Writef("%s @%u\n", get_opcode_name(opcode), read_u32(&pc));
        break;

      case Opcode::CallIndirect: {
        Index table_index = read_u32(&pc);
        stream->Writef("%s $%" PRIindex ":%u, %%[-1]\n",
                       get_opcode_name(opcode), table_index, read_u32(&pc));
        break;
      }

      case Opcode::CallHost:
        stream->Writef("%s $%u\n", get_opcode_name(opcode), read_u32(&pc));
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
        stream->Writef("%s $%" PRIindex ":%%[-1]+$%u\n",
                       get_opcode_name(opcode), memory_index, read_u32(&pc));
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
                       get_opcode_name(opcode), memory_index, read_u32(&pc));
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
        stream->Writef("%s %%[-2], %%[-1]\n", get_opcode_name(opcode));
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
        stream->Writef("%s %%[-1]\n", get_opcode_name(opcode));
        break;

      case Opcode::GrowMemory: {
        Index memory_index = read_u32(&pc);
        stream->Writef("%s $%" PRIindex ":%%[-1]\n", get_opcode_name(opcode),
                       memory_index);
        break;
      }

      case Opcode::Alloca:
        stream->Writef("%s $%u\n", get_opcode_name(opcode), read_u32(&pc));
        break;

      case Opcode::BrUnless:
        stream->Writef("%s @%u, %%[-1]\n", get_opcode_name(opcode),
                       read_u32(&pc));
        break;

      case Opcode::DropKeep: {
        uint32_t drop = read_u32(&pc);
        uint8_t keep = *pc++;
        stream->Writef("%s $%u $%u\n", get_opcode_name(opcode), drop, keep);
        break;
      }

      case Opcode::Data: {
        uint32_t num_bytes = read_u32(&pc);
        stream->Writef("%s $%u\n", get_opcode_name(opcode), num_bytes);
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
