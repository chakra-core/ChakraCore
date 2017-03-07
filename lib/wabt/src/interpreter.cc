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

#include <assert.h>
#include <inttypes.h>
#include <math.h>

#include "stream.h"

#define INITIAL_ISTREAM_CAPACITY (64 * 1024)

namespace wabt {

static const char* s_interpreter_opcode_name[256];

#define CHECK_RESULT(expr)  \
  do {                      \
    if (WABT_FAILED(expr))  \
      return Result::Error; \
  } while (0)

/* TODO(binji): It's annoying to have to have an initializer function, but it
 * seems to be necessary as g++ doesn't allow non-trival designated
 * initializers (e.g. [314] = "blah") */
static void init_interpreter_opcode_table(void) {
  static bool s_initialized = false;
  if (!s_initialized) {
#define V(rtype, type1, type2, mem_size, code, NAME, text) \
  s_interpreter_opcode_name[code] = text;

    WABT_FOREACH_INTERPRETER_OPCODE(V)

#undef V
  }
}

static const char* get_interpreter_opcode_name(InterpreterOpcode opcode) {
  init_interpreter_opcode_table();
  return s_interpreter_opcode_name[static_cast<int>(opcode)];
}

void init_interpreter_environment(InterpreterEnvironment* env) {
  WABT_ZERO_MEMORY(*env);
  init_output_buffer(&env->istream, INITIAL_ISTREAM_CAPACITY);
}

static void destroy_interpreter_func_signature(InterpreterFuncSignature* sig) {
  destroy_type_vector(&sig->param_types);
  destroy_type_vector(&sig->result_types);
}

static void destroy_interpreter_func(InterpreterFunc* func) {
  if (!func->is_host)
    destroy_type_vector(&func->defined.param_and_local_types);
}

static void destroy_interpreter_memory(InterpreterMemory* memory) {
  delete [] memory->data;
}

static void destroy_interpreter_table(InterpreterTable* table) {
  destroy_uint32_array(&table->func_indexes);
}

static void destroy_interpreter_import(InterpreterImport* import) {
  destroy_string_slice(&import->module_name);
  destroy_string_slice(&import->field_name);
}

static void destroy_interpreter_module(InterpreterModule* module) {
  destroy_interpreter_export_vector(&module->exports);
  destroy_binding_hash(&module->export_bindings);
  destroy_string_slice(&module->name);
  if (!module->is_host) {
    WABT_DESTROY_ARRAY_AND_ELEMENTS(module->defined.imports,
                                    interpreter_import);
  }
}

void destroy_interpreter_environment(InterpreterEnvironment* env) {
  WABT_DESTROY_VECTOR_AND_ELEMENTS(env->modules, interpreter_module);
  WABT_DESTROY_VECTOR_AND_ELEMENTS(env->sigs, interpreter_func_signature);
  WABT_DESTROY_VECTOR_AND_ELEMENTS(env->funcs, interpreter_func);
  WABT_DESTROY_VECTOR_AND_ELEMENTS(env->memories, interpreter_memory);
  WABT_DESTROY_VECTOR_AND_ELEMENTS(env->tables, interpreter_table);
  destroy_interpreter_global_vector(&env->globals);
  destroy_output_buffer(&env->istream);
  destroy_binding_hash(&env->module_bindings);
  destroy_binding_hash(&env->registered_module_bindings);
}

InterpreterEnvironmentMark mark_interpreter_environment(
    InterpreterEnvironment* env) {
  InterpreterEnvironmentMark mark;
  WABT_ZERO_MEMORY(mark);
  mark.modules_size = env->modules.size;
  mark.sigs_size = env->sigs.size;
  mark.funcs_size = env->funcs.size;
  mark.memories_size = env->memories.size;
  mark.tables_size = env->tables.size;
  mark.globals_size = env->globals.size;
  mark.istream_size = env->istream.size;
  return mark;
}

void reset_interpreter_environment_to_mark(InterpreterEnvironment* env,
                                           InterpreterEnvironmentMark mark) {
  size_t i;

#define DESTROY_PAST_MARK(destroy_name, names)                 \
  do {                                                         \
    assert(mark.names##_size <= env->names.size);              \
    for (i = mark.names##_size; i < env->names.size; ++i)      \
      destroy_interpreter_##destroy_name(&env->names.data[i]); \
    env->names.size = mark.names##_size;                       \
  } while (0)

  /* Destroy entries in the binding hash. */
  for (i = mark.modules_size; i < env->modules.size; ++i) {
    const StringSlice* name = &env->modules.data[i].name;
    if (!string_slice_is_empty(name))
      remove_binding(&env->module_bindings, name);
  }

  /* registered_module_bindings maps from an arbitrary name to a module index,
   * so we have to iterate through the entire table to find entries to remove.
   */
  for (i = 0; i < env->registered_module_bindings.entries.capacity; ++i) {
    BindingHashEntry* entry = &env->registered_module_bindings.entries.data[i];
    if (!hash_entry_is_free(entry) &&
        entry->binding.index >= static_cast<int>(mark.modules_size)) {
      remove_binding(&env->registered_module_bindings, &entry->binding.name);
    }
  }

  DESTROY_PAST_MARK(module, modules);
  DESTROY_PAST_MARK(func_signature, sigs);
  DESTROY_PAST_MARK(func, funcs);
  DESTROY_PAST_MARK(memory, memories);
  DESTROY_PAST_MARK(table, tables);
  env->globals.size = mark.globals_size;
  env->istream.size = mark.istream_size;

#undef DESTROY_PAST_MARK
}

InterpreterModule* append_host_module(InterpreterEnvironment* env,
                                      StringSlice name) {
  InterpreterModule* module = append_interpreter_module(&env->modules);
  module->name = dup_string_slice(name);
  module->memory_index = WABT_INVALID_INDEX;
  module->table_index = WABT_INVALID_INDEX;
  module->is_host = true;

  StringSlice dup_name = dup_string_slice(name);
  Binding* binding =
      insert_binding(&env->registered_module_bindings, &dup_name);
  binding->index = env->modules.size - 1;
  return module;
}

void init_interpreter_thread(InterpreterEnvironment* env,
                             InterpreterThread* thread,
                             InterpreterThreadOptions* options) {
  WABT_ZERO_MEMORY(*thread);
  new_interpreter_value_array(&thread->value_stack, options->value_stack_size);
  new_uint32_array(&thread->call_stack, options->call_stack_size);
  thread->env = env;
  thread->value_stack_top = thread->value_stack.data;
  thread->value_stack_end = thread->value_stack.data + thread->value_stack.size;
  thread->call_stack_top = thread->call_stack.data;
  thread->call_stack_end = thread->call_stack.data + thread->call_stack.size;
  thread->pc = options->pc;
}

InterpreterResult push_thread_value(InterpreterThread* thread,
                                    InterpreterValue value) {
  if (thread->value_stack_top >= thread->value_stack_end)
    return InterpreterResult::TrapValueStackExhausted;
  *thread->value_stack_top++ = value;
  return InterpreterResult::Ok;
}

InterpreterExport* get_interpreter_export_by_name(InterpreterModule* module,
                                                  const StringSlice* name) {
  int field_index = find_binding_index_by_name(&module->export_bindings, name);
  if (field_index < 0)
    return nullptr;
  assert(static_cast<size_t>(field_index) < module->exports.size);
  return &module->exports.data[field_index];
}

void destroy_interpreter_thread(InterpreterThread* thread) {
  destroy_interpreter_value_array(&thread->value_stack);
  destroy_uint32_array(&thread->call_stack);
  destroy_interpreter_typed_value_vector(&thread->host_args);
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
#define F32_QUIET_NAN_BIT 0x00400000U
#define F32_SIG_BITS 23
#define F32_SIG_MASK 0x7fffff
#define F32_SIGN_MASK 0x80000000U

bool is_nan_f32(uint32_t f32_bits) {
  return (f32_bits > F32_INF && f32_bits < F32_NEG_ZERO) ||
         (f32_bits > F32_NEG_INF);
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
 * 0 10000011101 1111..1..111000...000 0x41dfffffffc00000 => 2147483647
 * (INT32_MAX)
 * 0 10000011110 1111..1..111100...000 0x41efffffffe00000 => 4294967295
 * (UINT32_MAX)
 * 0 10000111101 1111..1..111111...111 0x43dfffffffffffff => 9223372036854774784
 * (~INT64_MAX)
 * 0 10000111110 0000..0..000000...000 0x43e0000000000000 => 9223372036854775808
 * 0 10000111110 1111..1..111111...111 0x43efffffffffffff =>
 * 18446744073709549568   (~UINT64_MAX)
 * 0 10000111111 0000..0..000000...000 0x43f0000000000000 =>
 * 18446744073709551616
 * 0 10001111110 1111..1..000000...000 0x47efffffe0000000 => 3.402823e+38
 * (FLT_MAX)
 * 0 11111111111 0000..0..000000...000 0x7ff0000000000000 => inf
 * 0 11111111111 0000..0..000000...001 0x7ff0000000000001 => nan(0x1)
 * 0 11111111111 1111..1..111111...111 0x7fffffffffffffff => nan(0xfff...)
 * 1 00000000000 0000..0..000000...000 0x8000000000000000 => -0
 * 1 01111111110 1111..1..111111...111 0xbfefffffffffffff => -1 + ulp
 * (~UINT32_MIN, ~UINT64_MIN)
 * 1 01111111111 0000..0..000000...000 0xbff0000000000000 => -1
 * 1 10000011110 0000..0..000000...000 0xc1e0000000000000 => -2147483648
 * (INT32_MIN)
 * 1 10000111110 0000..0..000000...000 0xc3e0000000000000 =>
 * -9223372036854775808     (INT64_MIN)
 * 1 10001111110 1111..1..000000...000 0xc7efffffe0000000 => -3.402823e+38
 * (-FLT_MAX)
 * 1 11111111111 0000..0..000000...000 0xfff0000000000000 => -inf
 * 1 11111111111 0000..0..000000...001 0xfff0000000000001 => -nan(0x1)
 * 1 11111111111 1111..1..111111...111 0xffffffffffffffff => -nan(0xfff...)
 */

#define F64_INF 0x7ff0000000000000ULL
#define F64_NEG_INF 0xfff0000000000000ULL
#define F64_NEG_ONE 0xbff0000000000000ULL
#define F64_NEG_ZERO 0x8000000000000000ULL
#define F64_QUIET_NAN 0x7ff8000000000000ULL
#define F64_QUIET_NAN_BIT 0x0008000000000000ULL
#define F64_SIG_BITS 52
#define F64_SIG_MASK 0xfffffffffffffULL
#define F64_SIGN_MASK 0x8000000000000000ULL

bool is_nan_f64(uint64_t f64_bits) {
  return (f64_bits > F64_INF && f64_bits < F64_NEG_ZERO) ||
         (f64_bits > F64_NEG_INF);
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

#define TRAP(type) return InterpreterResult::Trap##type
#define TRAP_UNLESS(cond, type) TRAP_IF(!(cond), type)
#define TRAP_IF(cond, type)  \
  do {                       \
    if (WABT_UNLIKELY(cond)) \
      TRAP(type);            \
  } while (0)

#define CHECK_STACK()                                         \
  TRAP_IF(thread->value_stack_top >= thread->value_stack_end, \
          ValueStackExhausted)

#define PUSH_NEG_1_AND_BREAK_IF(cond) \
  if (WABT_UNLIKELY(cond)) {          \
    PUSH_I32(-1);                     \
    break;                            \
  }

#define PUSH(v)                         \
  do {                                  \
    CHECK_STACK();                      \
    (*thread->value_stack_top++) = (v); \
  } while (0)

#define PUSH_TYPE(type, v)                                \
  do {                                                    \
    CHECK_STACK();                                        \
    (*thread->value_stack_top++).TYPE_FIELD_NAME_##type = \
        static_cast<VALUE_TYPE_##type>(v);                \
  } while (0)

#define PUSH_I32(v) PUSH_TYPE(I32, (v))
#define PUSH_I64(v) PUSH_TYPE(I64, (v))
#define PUSH_F32(v) PUSH_TYPE(F32, (v))
#define PUSH_F64(v) PUSH_TYPE(F64, (v))

#define PICK(depth) (*(thread->value_stack_top - (depth)))
#define TOP() (PICK(1))
#define POP() (*--thread->value_stack_top)
#define POP_I32() (POP().i32)
#define POP_I64() (POP().i64)
#define POP_F32() (POP().f32_bits)
#define POP_F64() (POP().f64_bits)
#define DROP_KEEP(drop, keep)          \
  do {                                 \
    assert((keep) <= 1);               \
    if ((keep) == 1)                   \
      PICK((drop) + 1) = TOP();        \
    thread->value_stack_top -= (drop); \
  } while (0)

#define GOTO(offset) pc = &istream[offset]

#define PUSH_CALL()                                           \
  do {                                                        \
    TRAP_IF(thread->call_stack_top >= thread->call_stack_end, \
            CallStackExhausted);                              \
    (*thread->call_stack_top++) = (pc - istream);             \
  } while (0)

#define POP_CALL() (*--thread->call_stack_top)

#define GET_MEMORY(var)                      \
  uint32_t memory_index = read_u32(&pc);     \
  assert(memory_index < env->memories.size); \
  InterpreterMemory* var = &env->memories.data[memory_index]

#define LOAD(type, mem_type)                                               \
  do {                                                                     \
    GET_MEMORY(memory);                                                    \
    uint64_t offset = static_cast<uint64_t>(POP_I32()) + read_u32(&pc);    \
    MEM_TYPE_##mem_type value;                                             \
    TRAP_IF(offset + sizeof(value) > memory->byte_size,                    \
            MemoryAccessOutOfBounds);                                      \
    void* src =                                                            \
        reinterpret_cast<void*>(reinterpret_cast<intptr_t>(memory->data) + \
                                static_cast<uint32_t>(offset));            \
    memcpy(&value, src, sizeof(MEM_TYPE_##mem_type));                      \
    PUSH_##type(static_cast<MEM_TYPE_EXTEND_##type##_##mem_type>(value));  \
  } while (0)

#define STORE(type, mem_type)                                              \
  do {                                                                     \
    GET_MEMORY(memory);                                                    \
    VALUE_TYPE_##type value = POP_##type();                                \
    uint64_t offset = static_cast<uint64_t>(POP_I32()) + read_u32(&pc);    \
    MEM_TYPE_##mem_type src = static_cast<MEM_TYPE_##mem_type>(value);     \
    TRAP_IF(offset + sizeof(src) > memory->byte_size,                      \
            MemoryAccessOutOfBounds);                                      \
    void* dst =                                                            \
        reinterpret_cast<void*>(reinterpret_cast<intptr_t>(memory->data) + \
                                static_cast<uint32_t>(offset));            \
    memcpy(dst, &src, sizeof(MEM_TYPE_##mem_type));                        \
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

#define UNOP_FLOAT(type, func)                                 \
  do {                                                         \
    FLOAT_TYPE_##type value = BITCAST_TO_##type(POP_##type()); \
    PUSH_##type(BITCAST_FROM_##type(func(value)));             \
    break;                                                     \
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
                                            uint32_t* out_offset,
                                            uint32_t* out_drop,
                                            uint8_t* out_keep) {
  *out_offset = read_u32_at(pc + WABT_TABLE_ENTRY_OFFSET_OFFSET);
  *out_drop = read_u32_at(pc + WABT_TABLE_ENTRY_DROP_OFFSET);
  *out_keep = *(pc + WABT_TABLE_ENTRY_KEEP_OFFSET);
}

bool func_signatures_are_equal(InterpreterEnvironment* env,
                               uint32_t sig_index_0,
                               uint32_t sig_index_1) {
  if (sig_index_0 == sig_index_1)
    return true;
  InterpreterFuncSignature* sig_0 = &env->sigs.data[sig_index_0];
  InterpreterFuncSignature* sig_1 = &env->sigs.data[sig_index_1];
  return type_vectors_are_equal(&sig_0->param_types, &sig_1->param_types) &&
         type_vectors_are_equal(&sig_0->result_types, &sig_1->result_types);
}

InterpreterResult call_host(InterpreterThread* thread, InterpreterFunc* func) {
  assert(func->is_host);
  assert(func->sig_index < thread->env->sigs.size);
  InterpreterFuncSignature* sig = &thread->env->sigs.data[func->sig_index];

  uint32_t num_args = sig->param_types.size;
  if (thread->host_args.size < num_args) {
    resize_interpreter_typed_value_vector(&thread->host_args, num_args);
  }

  uint32_t i;
  for (i = num_args; i > 0; --i) {
    InterpreterValue value = POP();
    InterpreterTypedValue* arg = &thread->host_args.data[i - 1];
    arg->type = sig->param_types.data[i - 1];
    arg->value = value;
  }

  uint32_t num_results = sig->result_types.size;
  InterpreterTypedValue* call_result_values =
      static_cast<InterpreterTypedValue*>(
          alloca(sizeof(InterpreterTypedValue) * num_results));

  Result call_result = func->host.callback(
      func, sig, num_args, thread->host_args.data, num_results,
      call_result_values, func->host.user_data);
  TRAP_IF(call_result != Result::Ok, HostTrapped);

  for (i = 0; i < num_results; ++i) {
    TRAP_IF(call_result_values[i].type != sig->result_types.data[i],
            HostResultTypeMismatch);
    PUSH(call_result_values[i].value);
  }

  return InterpreterResult::Ok;
}

InterpreterResult run_interpreter(InterpreterThread* thread,
                                  uint32_t num_instructions,
                                  uint32_t* call_stack_return_top) {
  InterpreterResult result = InterpreterResult::Ok;
  assert(call_stack_return_top < thread->call_stack_end);

  InterpreterEnvironment* env = thread->env;

  const uint8_t* istream = reinterpret_cast<const uint8_t*>(env->istream.start);
  const uint8_t* pc = &istream[thread->pc];
  uint32_t i;
  for (i = 0; i < num_instructions; ++i) {
    InterpreterOpcode opcode = static_cast<InterpreterOpcode>(*pc++);
    switch (opcode) {
      case InterpreterOpcode::Select: {
        VALUE_TYPE_I32 cond = POP_I32();
        InterpreterValue false_ = POP();
        InterpreterValue true_ = POP();
        PUSH(cond ? true_ : false_);
        break;
      }

      case InterpreterOpcode::Br:
        GOTO(read_u32(&pc));
        break;

      case InterpreterOpcode::BrIf: {
        uint32_t new_pc = read_u32(&pc);
        if (POP_I32())
          GOTO(new_pc);
        break;
      }

      case InterpreterOpcode::BrTable: {
        uint32_t num_targets = read_u32(&pc);
        uint32_t table_offset = read_u32(&pc);
        VALUE_TYPE_I32 key = POP_I32();
        uint32_t key_offset =
            (key >= num_targets ? num_targets : key) * WABT_TABLE_ENTRY_SIZE;
        const uint8_t* entry = istream + table_offset + key_offset;
        uint32_t new_pc;
        uint32_t drop_count;
        uint8_t keep_count;
        read_table_entry_at(entry, &new_pc, &drop_count, &keep_count);
        DROP_KEEP(drop_count, keep_count);
        GOTO(new_pc);
        break;
      }

      case InterpreterOpcode::Return:
        if (thread->call_stack_top == call_stack_return_top) {
          result = InterpreterResult::Returned;
          goto exit_loop;
        }
        GOTO(POP_CALL());
        break;

      case InterpreterOpcode::Unreachable:
        TRAP(Unreachable);
        break;

      case InterpreterOpcode::I32Const:
        PUSH_I32(read_u32(&pc));
        break;

      case InterpreterOpcode::I64Const:
        PUSH_I64(read_u64(&pc));
        break;

      case InterpreterOpcode::F32Const:
        PUSH_F32(read_u32(&pc));
        break;

      case InterpreterOpcode::F64Const:
        PUSH_F64(read_u64(&pc));
        break;

      case InterpreterOpcode::GetGlobal: {
        uint32_t index = read_u32(&pc);
        assert(index < env->globals.size);
        PUSH(env->globals.data[index].typed_value.value);
        break;
      }

      case InterpreterOpcode::SetGlobal: {
        uint32_t index = read_u32(&pc);
        assert(index < env->globals.size);
        env->globals.data[index].typed_value.value = POP();
        break;
      }

      case InterpreterOpcode::GetLocal: {
        InterpreterValue value = PICK(read_u32(&pc));
        PUSH(value);
        break;
      }

      case InterpreterOpcode::SetLocal: {
        InterpreterValue value = POP();
        PICK(read_u32(&pc)) = value;
        break;
      }

      case InterpreterOpcode::TeeLocal:
        PICK(read_u32(&pc)) = TOP();
        break;

      case InterpreterOpcode::Call: {
        uint32_t offset = read_u32(&pc);
        PUSH_CALL();
        GOTO(offset);
        break;
      }

      case InterpreterOpcode::CallIndirect: {
        uint32_t table_index = read_u32(&pc);
        assert(table_index < env->tables.size);
        InterpreterTable* table = &env->tables.data[table_index];
        uint32_t sig_index = read_u32(&pc);
        assert(sig_index < env->sigs.size);
        VALUE_TYPE_I32 entry_index = POP_I32();
        TRAP_IF(entry_index >= table->func_indexes.size, UndefinedTableIndex);
        uint32_t func_index = table->func_indexes.data[entry_index];
        TRAP_IF(func_index == WABT_INVALID_INDEX, UninitializedTableElement);
        InterpreterFunc* func = &env->funcs.data[func_index];
        TRAP_UNLESS(func_signatures_are_equal(env, func->sig_index, sig_index),
                    IndirectCallSignatureMismatch);
        if (func->is_host) {
          call_host(thread, func);
        } else {
          PUSH_CALL();
          GOTO(func->defined.offset);
        }
        break;
      }

      case InterpreterOpcode::CallHost: {
        uint32_t func_index = read_u32(&pc);
        assert(func_index < env->funcs.size);
        InterpreterFunc* func = &env->funcs.data[func_index];
        call_host(thread, func);
        break;
      }

      case InterpreterOpcode::I32Load8S:
        LOAD(I32, I8);
        break;

      case InterpreterOpcode::I32Load8U:
        LOAD(I32, U8);
        break;

      case InterpreterOpcode::I32Load16S:
        LOAD(I32, I16);
        break;

      case InterpreterOpcode::I32Load16U:
        LOAD(I32, U16);
        break;

      case InterpreterOpcode::I64Load8S:
        LOAD(I64, I8);
        break;

      case InterpreterOpcode::I64Load8U:
        LOAD(I64, U8);
        break;

      case InterpreterOpcode::I64Load16S:
        LOAD(I64, I16);
        break;

      case InterpreterOpcode::I64Load16U:
        LOAD(I64, U16);
        break;

      case InterpreterOpcode::I64Load32S:
        LOAD(I64, I32);
        break;

      case InterpreterOpcode::I64Load32U:
        LOAD(I64, U32);
        break;

      case InterpreterOpcode::I32Load:
        LOAD(I32, U32);
        break;

      case InterpreterOpcode::I64Load:
        LOAD(I64, U64);
        break;

      case InterpreterOpcode::F32Load:
        LOAD(F32, F32);
        break;

      case InterpreterOpcode::F64Load:
        LOAD(F64, F64);
        break;

      case InterpreterOpcode::I32Store8:
        STORE(I32, U8);
        break;

      case InterpreterOpcode::I32Store16:
        STORE(I32, U16);
        break;

      case InterpreterOpcode::I64Store8:
        STORE(I64, U8);
        break;

      case InterpreterOpcode::I64Store16:
        STORE(I64, U16);
        break;

      case InterpreterOpcode::I64Store32:
        STORE(I64, U32);
        break;

      case InterpreterOpcode::I32Store:
        STORE(I32, U32);
        break;

      case InterpreterOpcode::I64Store:
        STORE(I64, U64);
        break;

      case InterpreterOpcode::F32Store:
        STORE(F32, F32);
        break;

      case InterpreterOpcode::F64Store:
        STORE(F64, F64);
        break;

      case InterpreterOpcode::CurrentMemory: {
        GET_MEMORY(memory);
        PUSH_I32(memory->page_limits.initial);
        break;
      }

      case InterpreterOpcode::GrowMemory: {
        GET_MEMORY(memory);
        uint32_t old_page_size = memory->page_limits.initial;
        uint32_t old_byte_size = memory->byte_size;
        VALUE_TYPE_I32 grow_pages = POP_I32();
        uint32_t new_page_size = old_page_size + grow_pages;
        uint32_t max_page_size = memory->page_limits.has_max
                                     ? memory->page_limits.max
                                     : WABT_MAX_PAGES;
        PUSH_NEG_1_AND_BREAK_IF(new_page_size > max_page_size);
        PUSH_NEG_1_AND_BREAK_IF(
            static_cast<uint64_t>(new_page_size) * WABT_PAGE_SIZE > UINT32_MAX);
        uint32_t new_byte_size = new_page_size * WABT_PAGE_SIZE;
        char* new_data = new char [new_byte_size];
        memcpy(new_data, memory->data, old_byte_size);
        memset(new_data + old_byte_size, 0, new_byte_size - old_byte_size);
        delete [] memory->data;
        memory->data = new_data;
        memory->page_limits.initial = new_page_size;
        memory->byte_size = new_byte_size;
        PUSH_I32(old_page_size);
        break;
      }

      case InterpreterOpcode::I32Add:
        BINOP(I32, I32, +);
        break;

      case InterpreterOpcode::I32Sub:
        BINOP(I32, I32, -);
        break;

      case InterpreterOpcode::I32Mul:
        BINOP(I32, I32, *);
        break;

      case InterpreterOpcode::I32DivS:
        BINOP_DIV_S(I32);
        break;

      case InterpreterOpcode::I32DivU:
        BINOP_DIV_REM_U(I32, /);
        break;

      case InterpreterOpcode::I32RemS:
        BINOP_REM_S(I32);
        break;

      case InterpreterOpcode::I32RemU:
        BINOP_DIV_REM_U(I32, %);
        break;

      case InterpreterOpcode::I32And:
        BINOP(I32, I32, &);
        break;

      case InterpreterOpcode::I32Or:
        BINOP(I32, I32, |);
        break;

      case InterpreterOpcode::I32Xor:
        BINOP(I32, I32, ^);
        break;

      case InterpreterOpcode::I32Shl:
        BINOP_SHIFT(I32, <<, UNSIGNED);
        break;

      case InterpreterOpcode::I32ShrU:
        BINOP_SHIFT(I32, >>, UNSIGNED);
        break;

      case InterpreterOpcode::I32ShrS:
        BINOP_SHIFT(I32, >>, SIGNED);
        break;

      case InterpreterOpcode::I32Eq:
        BINOP(I32, I32, ==);
        break;

      case InterpreterOpcode::I32Ne:
        BINOP(I32, I32, !=);
        break;

      case InterpreterOpcode::I32LtS:
        BINOP_SIGNED(I32, I32, <);
        break;

      case InterpreterOpcode::I32LeS:
        BINOP_SIGNED(I32, I32, <=);
        break;

      case InterpreterOpcode::I32LtU:
        BINOP(I32, I32, <);
        break;

      case InterpreterOpcode::I32LeU:
        BINOP(I32, I32, <=);
        break;

      case InterpreterOpcode::I32GtS:
        BINOP_SIGNED(I32, I32, >);
        break;

      case InterpreterOpcode::I32GeS:
        BINOP_SIGNED(I32, I32, >=);
        break;

      case InterpreterOpcode::I32GtU:
        BINOP(I32, I32, >);
        break;

      case InterpreterOpcode::I32GeU:
        BINOP(I32, I32, >=);
        break;

      case InterpreterOpcode::I32Clz: {
        VALUE_TYPE_I32 value = POP_I32();
        PUSH_I32(value != 0 ? wabt_clz_u32(value) : 32);
        break;
      }

      case InterpreterOpcode::I32Ctz: {
        VALUE_TYPE_I32 value = POP_I32();
        PUSH_I32(value != 0 ? wabt_ctz_u32(value) : 32);
        break;
      }

      case InterpreterOpcode::I32Popcnt: {
        VALUE_TYPE_I32 value = POP_I32();
        PUSH_I32(wabt_popcount_u32(value));
        break;
      }

      case InterpreterOpcode::I32Eqz: {
        VALUE_TYPE_I32 value = POP_I32();
        PUSH_I32(value == 0);
        break;
      }

      case InterpreterOpcode::I64Add:
        BINOP(I64, I64, +);
        break;

      case InterpreterOpcode::I64Sub:
        BINOP(I64, I64, -);
        break;

      case InterpreterOpcode::I64Mul:
        BINOP(I64, I64, *);
        break;

      case InterpreterOpcode::I64DivS:
        BINOP_DIV_S(I64);
        break;

      case InterpreterOpcode::I64DivU:
        BINOP_DIV_REM_U(I64, /);
        break;

      case InterpreterOpcode::I64RemS:
        BINOP_REM_S(I64);
        break;

      case InterpreterOpcode::I64RemU:
        BINOP_DIV_REM_U(I64, %);
        break;

      case InterpreterOpcode::I64And:
        BINOP(I64, I64, &);
        break;

      case InterpreterOpcode::I64Or:
        BINOP(I64, I64, |);
        break;

      case InterpreterOpcode::I64Xor:
        BINOP(I64, I64, ^);
        break;

      case InterpreterOpcode::I64Shl:
        BINOP_SHIFT(I64, <<, UNSIGNED);
        break;

      case InterpreterOpcode::I64ShrU:
        BINOP_SHIFT(I64, >>, UNSIGNED);
        break;

      case InterpreterOpcode::I64ShrS:
        BINOP_SHIFT(I64, >>, SIGNED);
        break;

      case InterpreterOpcode::I64Eq:
        BINOP(I32, I64, ==);
        break;

      case InterpreterOpcode::I64Ne:
        BINOP(I32, I64, !=);
        break;

      case InterpreterOpcode::I64LtS:
        BINOP_SIGNED(I32, I64, <);
        break;

      case InterpreterOpcode::I64LeS:
        BINOP_SIGNED(I32, I64, <=);
        break;

      case InterpreterOpcode::I64LtU:
        BINOP(I32, I64, <);
        break;

      case InterpreterOpcode::I64LeU:
        BINOP(I32, I64, <=);
        break;

      case InterpreterOpcode::I64GtS:
        BINOP_SIGNED(I32, I64, >);
        break;

      case InterpreterOpcode::I64GeS:
        BINOP_SIGNED(I32, I64, >=);
        break;

      case InterpreterOpcode::I64GtU:
        BINOP(I32, I64, >);
        break;

      case InterpreterOpcode::I64GeU:
        BINOP(I32, I64, >=);
        break;

      case InterpreterOpcode::I64Clz: {
        VALUE_TYPE_I64 value = POP_I64();
        PUSH_I64(value != 0 ? wabt_clz_u64(value) : 64);
        break;
      }

      case InterpreterOpcode::I64Ctz: {
        VALUE_TYPE_I64 value = POP_I64();
        PUSH_I64(value != 0 ? wabt_ctz_u64(value) : 64);
        break;
      }

      case InterpreterOpcode::I64Popcnt: {
        VALUE_TYPE_I64 value = POP_I64();
        PUSH_I64(wabt_popcount_u64(value));
        break;
      }

      case InterpreterOpcode::F32Add:
        BINOP_FLOAT(F32, +);
        break;

      case InterpreterOpcode::F32Sub:
        BINOP_FLOAT(F32, -);
        break;

      case InterpreterOpcode::F32Mul:
        BINOP_FLOAT(F32, *);
        break;

      case InterpreterOpcode::F32Div:
        BINOP_FLOAT_DIV(F32);
        break;

      case InterpreterOpcode::F32Min:
        MINMAX_FLOAT(F32, MIN);
        break;

      case InterpreterOpcode::F32Max:
        MINMAX_FLOAT(F32, MAX);
        break;

      case InterpreterOpcode::F32Abs:
        TOP().f32_bits &= ~F32_SIGN_MASK;
        break;

      case InterpreterOpcode::F32Neg:
        TOP().f32_bits ^= F32_SIGN_MASK;
        break;

      case InterpreterOpcode::F32Copysign: {
        VALUE_TYPE_F32 rhs = POP_F32();
        VALUE_TYPE_F32 lhs = POP_F32();
        PUSH_F32((lhs & ~F32_SIGN_MASK) | (rhs & F32_SIGN_MASK));
        break;
      }

      case InterpreterOpcode::F32Ceil:
        UNOP_FLOAT(F32, ceilf);
        break;

      case InterpreterOpcode::F32Floor:
        UNOP_FLOAT(F32, floorf);
        break;

      case InterpreterOpcode::F32Trunc:
        UNOP_FLOAT(F32, truncf);
        break;

      case InterpreterOpcode::F32Nearest:
        UNOP_FLOAT(F32, nearbyintf);
        break;

      case InterpreterOpcode::F32Sqrt:
        UNOP_FLOAT(F32, sqrtf);
        break;

      case InterpreterOpcode::F32Eq:
        BINOP_FLOAT_COMPARE(F32, ==);
        break;

      case InterpreterOpcode::F32Ne:
        BINOP_FLOAT_COMPARE(F32, !=);
        break;

      case InterpreterOpcode::F32Lt:
        BINOP_FLOAT_COMPARE(F32, <);
        break;

      case InterpreterOpcode::F32Le:
        BINOP_FLOAT_COMPARE(F32, <=);
        break;

      case InterpreterOpcode::F32Gt:
        BINOP_FLOAT_COMPARE(F32, >);
        break;

      case InterpreterOpcode::F32Ge:
        BINOP_FLOAT_COMPARE(F32, >=);
        break;

      case InterpreterOpcode::F64Add:
        BINOP_FLOAT(F64, +);
        break;

      case InterpreterOpcode::F64Sub:
        BINOP_FLOAT(F64, -);
        break;

      case InterpreterOpcode::F64Mul:
        BINOP_FLOAT(F64, *);
        break;

      case InterpreterOpcode::F64Div:
        BINOP_FLOAT_DIV(F64);
        break;

      case InterpreterOpcode::F64Min:
        MINMAX_FLOAT(F64, MIN);
        break;

      case InterpreterOpcode::F64Max:
        MINMAX_FLOAT(F64, MAX);
        break;

      case InterpreterOpcode::F64Abs:
        TOP().f64_bits &= ~F64_SIGN_MASK;
        break;

      case InterpreterOpcode::F64Neg:
        TOP().f64_bits ^= F64_SIGN_MASK;
        break;

      case InterpreterOpcode::F64Copysign: {
        VALUE_TYPE_F64 rhs = POP_F64();
        VALUE_TYPE_F64 lhs = POP_F64();
        PUSH_F64((lhs & ~F64_SIGN_MASK) | (rhs & F64_SIGN_MASK));
        break;
      }

      case InterpreterOpcode::F64Ceil:
        UNOP_FLOAT(F64, ceil);
        break;

      case InterpreterOpcode::F64Floor:
        UNOP_FLOAT(F64, floor);
        break;

      case InterpreterOpcode::F64Trunc:
        UNOP_FLOAT(F64, trunc);
        break;

      case InterpreterOpcode::F64Nearest:
        UNOP_FLOAT(F64, nearbyint);
        break;

      case InterpreterOpcode::F64Sqrt:
        UNOP_FLOAT(F64, sqrt);
        break;

      case InterpreterOpcode::F64Eq:
        BINOP_FLOAT_COMPARE(F64, ==);
        break;

      case InterpreterOpcode::F64Ne:
        BINOP_FLOAT_COMPARE(F64, !=);
        break;

      case InterpreterOpcode::F64Lt:
        BINOP_FLOAT_COMPARE(F64, <);
        break;

      case InterpreterOpcode::F64Le:
        BINOP_FLOAT_COMPARE(F64, <=);
        break;

      case InterpreterOpcode::F64Gt:
        BINOP_FLOAT_COMPARE(F64, >);
        break;

      case InterpreterOpcode::F64Ge:
        BINOP_FLOAT_COMPARE(F64, >=);
        break;

      case InterpreterOpcode::I32TruncSF32: {
        VALUE_TYPE_F32 value = POP_F32();
        TRAP_IF(is_nan_f32(value), InvalidConversionToInteger);
        TRAP_UNLESS(is_in_range_i32_trunc_s_f32(value), IntegerOverflow);
        PUSH_I32(static_cast<int32_t>(BITCAST_TO_F32(value)));
        break;
      }

      case InterpreterOpcode::I32TruncSF64: {
        VALUE_TYPE_F64 value = POP_F64();
        TRAP_IF(is_nan_f64(value), InvalidConversionToInteger);
        TRAP_UNLESS(is_in_range_i32_trunc_s_f64(value), IntegerOverflow);
        PUSH_I32(static_cast<int32_t>(BITCAST_TO_F64(value)));
        break;
      }

      case InterpreterOpcode::I32TruncUF32: {
        VALUE_TYPE_F32 value = POP_F32();
        TRAP_IF(is_nan_f32(value), InvalidConversionToInteger);
        TRAP_UNLESS(is_in_range_i32_trunc_u_f32(value), IntegerOverflow);
        PUSH_I32(static_cast<uint32_t>(BITCAST_TO_F32(value)));
        break;
      }

      case InterpreterOpcode::I32TruncUF64: {
        VALUE_TYPE_F64 value = POP_F64();
        TRAP_IF(is_nan_f64(value), InvalidConversionToInteger);
        TRAP_UNLESS(is_in_range_i32_trunc_u_f64(value), IntegerOverflow);
        PUSH_I32(static_cast<uint32_t>(BITCAST_TO_F64(value)));
        break;
      }

      case InterpreterOpcode::I32WrapI64: {
        VALUE_TYPE_I64 value = POP_I64();
        PUSH_I32(static_cast<uint32_t>(value));
        break;
      }

      case InterpreterOpcode::I64TruncSF32: {
        VALUE_TYPE_F32 value = POP_F32();
        TRAP_IF(is_nan_f32(value), InvalidConversionToInteger);
        TRAP_UNLESS(is_in_range_i64_trunc_s_f32(value), IntegerOverflow);
        PUSH_I64(static_cast<int64_t>(BITCAST_TO_F32(value)));
        break;
      }

      case InterpreterOpcode::I64TruncSF64: {
        VALUE_TYPE_F64 value = POP_F64();
        TRAP_IF(is_nan_f64(value), InvalidConversionToInteger);
        TRAP_UNLESS(is_in_range_i64_trunc_s_f64(value), IntegerOverflow);
        PUSH_I64(static_cast<int64_t>(BITCAST_TO_F64(value)));
        break;
      }

      case InterpreterOpcode::I64TruncUF32: {
        VALUE_TYPE_F32 value = POP_F32();
        TRAP_IF(is_nan_f32(value), InvalidConversionToInteger);
        TRAP_UNLESS(is_in_range_i64_trunc_u_f32(value), IntegerOverflow);
        PUSH_I64(static_cast<uint64_t>(BITCAST_TO_F32(value)));
        break;
      }

      case InterpreterOpcode::I64TruncUF64: {
        VALUE_TYPE_F64 value = POP_F64();
        TRAP_IF(is_nan_f64(value), InvalidConversionToInteger);
        TRAP_UNLESS(is_in_range_i64_trunc_u_f64(value), IntegerOverflow);
        PUSH_I64(static_cast<uint64_t>(BITCAST_TO_F64(value)));
        break;
      }

      case InterpreterOpcode::I64ExtendSI32: {
        VALUE_TYPE_I32 value = POP_I32();
        PUSH_I64(static_cast<int64_t>(BITCAST_I32_TO_SIGNED(value)));
        break;
      }

      case InterpreterOpcode::I64ExtendUI32: {
        VALUE_TYPE_I32 value = POP_I32();
        PUSH_I64(static_cast<uint64_t>(value));
        break;
      }

      case InterpreterOpcode::F32ConvertSI32: {
        VALUE_TYPE_I32 value = POP_I32();
        PUSH_F32(
            BITCAST_FROM_F32(static_cast<float>(BITCAST_I32_TO_SIGNED(value))));
        break;
      }

      case InterpreterOpcode::F32ConvertUI32: {
        VALUE_TYPE_I32 value = POP_I32();
        PUSH_F32(BITCAST_FROM_F32(static_cast<float>(value)));
        break;
      }

      case InterpreterOpcode::F32ConvertSI64: {
        VALUE_TYPE_I64 value = POP_I64();
        PUSH_F32(
            BITCAST_FROM_F32(static_cast<float>(BITCAST_I64_TO_SIGNED(value))));
        break;
      }

      case InterpreterOpcode::F32ConvertUI64: {
        VALUE_TYPE_I64 value = POP_I64();
        PUSH_F32(BITCAST_FROM_F32(static_cast<float>(value)));
        break;
      }

      case InterpreterOpcode::F32DemoteF64: {
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

      case InterpreterOpcode::F32ReinterpretI32: {
        VALUE_TYPE_I32 value = POP_I32();
        PUSH_F32(value);
        break;
      }

      case InterpreterOpcode::F64ConvertSI32: {
        VALUE_TYPE_I32 value = POP_I32();
        PUSH_F64(BITCAST_FROM_F64(
            static_cast<double>(BITCAST_I32_TO_SIGNED(value))));
        break;
      }

      case InterpreterOpcode::F64ConvertUI32: {
        VALUE_TYPE_I32 value = POP_I32();
        PUSH_F64(BITCAST_FROM_F64(static_cast<double>(value)));
        break;
      }

      case InterpreterOpcode::F64ConvertSI64: {
        VALUE_TYPE_I64 value = POP_I64();
        PUSH_F64(BITCAST_FROM_F64(
            static_cast<double>(BITCAST_I64_TO_SIGNED(value))));
        break;
      }

      case InterpreterOpcode::F64ConvertUI64: {
        VALUE_TYPE_I64 value = POP_I64();
        PUSH_F64(BITCAST_FROM_F64(static_cast<double>(value)));
        break;
      }

      case InterpreterOpcode::F64PromoteF32: {
        VALUE_TYPE_F32 value = POP_F32();
        PUSH_F64(BITCAST_FROM_F64(static_cast<double>(BITCAST_TO_F32(value))));
        break;
      }

      case InterpreterOpcode::F64ReinterpretI64: {
        VALUE_TYPE_I64 value = POP_I64();
        PUSH_F64(value);
        break;
      }

      case InterpreterOpcode::I32ReinterpretF32: {
        VALUE_TYPE_F32 value = POP_F32();
        PUSH_I32(value);
        break;
      }

      case InterpreterOpcode::I64ReinterpretF64: {
        VALUE_TYPE_F64 value = POP_F64();
        PUSH_I64(value);
        break;
      }

      case InterpreterOpcode::I32Rotr:
        BINOP_ROT(I32, RIGHT);
        break;

      case InterpreterOpcode::I32Rotl:
        BINOP_ROT(I32, LEFT);
        break;

      case InterpreterOpcode::I64Rotr:
        BINOP_ROT(I64, RIGHT);
        break;

      case InterpreterOpcode::I64Rotl:
        BINOP_ROT(I64, LEFT);
        break;

      case InterpreterOpcode::I64Eqz: {
        VALUE_TYPE_I64 value = POP_I64();
        PUSH_I64(value == 0);
        break;
      }

      case InterpreterOpcode::Alloca: {
        InterpreterValue* old_value_stack_top = thread->value_stack_top;
        thread->value_stack_top += read_u32(&pc);
        CHECK_STACK();
        memset(old_value_stack_top, 0,
               (thread->value_stack_top - old_value_stack_top) *
                   sizeof(InterpreterValue));
        break;
      }

      case InterpreterOpcode::BrUnless: {
        uint32_t new_pc = read_u32(&pc);
        if (!POP_I32())
          GOTO(new_pc);
        break;
      }

      case InterpreterOpcode::Drop:
        (void)POP();
        break;

      case InterpreterOpcode::DropKeep: {
        uint32_t drop_count = read_u32(&pc);
        uint8_t keep_count = *pc++;
        DROP_KEEP(drop_count, keep_count);
        break;
      }

      case InterpreterOpcode::Data:
        /* shouldn't ever execute this */
        assert(0);
        break;

      case InterpreterOpcode::Nop:
        break;

      default:
        assert(0);
        break;
    }
  }

exit_loop:
  thread->pc = pc - istream;
  return result;
}

void trace_pc(InterpreterThread* thread, Stream* stream) {
  const uint8_t* istream =
      reinterpret_cast<const uint8_t*>(thread->env->istream.start);
  const uint8_t* pc = &istream[thread->pc];
  size_t value_stack_depth = thread->value_stack_top - thread->value_stack.data;
  size_t call_stack_depth = thread->call_stack_top - thread->call_stack.data;

  writef(stream, "#%" PRIzd ". %4" PRIzd ": V:%-3" PRIzd "| ", call_stack_depth,
         pc - reinterpret_cast<uint8_t*>(thread->env->istream.start),
         value_stack_depth);

  InterpreterOpcode opcode = static_cast<InterpreterOpcode>(*pc++);
  switch (opcode) {
    case InterpreterOpcode::Select:
      writef(stream, "%s %u, %" PRIu64 ", %" PRIu64 "\n",
             get_interpreter_opcode_name(opcode), PICK(3).i32, PICK(2).i64,
             PICK(1).i64);
      break;

    case InterpreterOpcode::Br:
      writef(stream, "%s @%u\n", get_interpreter_opcode_name(opcode),
             read_u32_at(pc));
      break;

    case InterpreterOpcode::BrIf:
      writef(stream, "%s @%u, %u\n", get_interpreter_opcode_name(opcode),
             read_u32_at(pc), TOP().i32);
      break;

    case InterpreterOpcode::BrTable: {
      uint32_t num_targets = read_u32_at(pc);
      uint32_t table_offset = read_u32_at(pc + 4);
      VALUE_TYPE_I32 key = TOP().i32;
      writef(stream, "%s %u, $#%u, table:$%u\n",
             get_interpreter_opcode_name(opcode), key, num_targets,
             table_offset);
      break;
    }

    case InterpreterOpcode::Nop:
    case InterpreterOpcode::Return:
    case InterpreterOpcode::Unreachable:
    case InterpreterOpcode::Drop:
      writef(stream, "%s\n", get_interpreter_opcode_name(opcode));
      break;

    case InterpreterOpcode::CurrentMemory: {
      uint32_t memory_index = read_u32(&pc);
      writef(stream, "%s $%u\n", get_interpreter_opcode_name(opcode),
             memory_index);
      break;
    }

    case InterpreterOpcode::I32Const:
      writef(stream, "%s $%u\n", get_interpreter_opcode_name(opcode),
             read_u32_at(pc));
      break;

    case InterpreterOpcode::I64Const:
      writef(stream, "%s $%" PRIu64 "\n", get_interpreter_opcode_name(opcode),
             read_u64_at(pc));
      break;

    case InterpreterOpcode::F32Const:
      writef(stream, "%s $%g\n", get_interpreter_opcode_name(opcode),
             bitcast_u32_to_f32(read_u32_at(pc)));
      break;

    case InterpreterOpcode::F64Const:
      writef(stream, "%s $%g\n", get_interpreter_opcode_name(opcode),
             bitcast_u64_to_f64(read_u64_at(pc)));
      break;

    case InterpreterOpcode::GetLocal:
    case InterpreterOpcode::GetGlobal:
      writef(stream, "%s $%u\n", get_interpreter_opcode_name(opcode),
             read_u32_at(pc));
      break;

    case InterpreterOpcode::SetLocal:
    case InterpreterOpcode::SetGlobal:
    case InterpreterOpcode::TeeLocal:
      writef(stream, "%s $%u, %u\n", get_interpreter_opcode_name(opcode),
             read_u32_at(pc), TOP().i32);
      break;

    case InterpreterOpcode::Call:
      writef(stream, "%s @%u\n", get_interpreter_opcode_name(opcode),
             read_u32_at(pc));
      break;

    case InterpreterOpcode::CallIndirect:
      writef(stream, "%s $%u, %u\n", get_interpreter_opcode_name(opcode),
             read_u32_at(pc), TOP().i32);
      break;

    case InterpreterOpcode::CallHost:
      writef(stream, "%s $%u\n", get_interpreter_opcode_name(opcode),
             read_u32_at(pc));
      break;

    case InterpreterOpcode::I32Load8S:
    case InterpreterOpcode::I32Load8U:
    case InterpreterOpcode::I32Load16S:
    case InterpreterOpcode::I32Load16U:
    case InterpreterOpcode::I64Load8S:
    case InterpreterOpcode::I64Load8U:
    case InterpreterOpcode::I64Load16S:
    case InterpreterOpcode::I64Load16U:
    case InterpreterOpcode::I64Load32S:
    case InterpreterOpcode::I64Load32U:
    case InterpreterOpcode::I32Load:
    case InterpreterOpcode::I64Load:
    case InterpreterOpcode::F32Load:
    case InterpreterOpcode::F64Load: {
      uint32_t memory_index = read_u32(&pc);
      writef(stream, "%s $%u:%u+$%u\n", get_interpreter_opcode_name(opcode),
             memory_index, TOP().i32, read_u32_at(pc));
      break;
    }

    case InterpreterOpcode::I32Store8:
    case InterpreterOpcode::I32Store16:
    case InterpreterOpcode::I32Store: {
      uint32_t memory_index = read_u32(&pc);
      writef(stream, "%s $%u:%u+$%u, %u\n", get_interpreter_opcode_name(opcode),
             memory_index, PICK(2).i32, read_u32_at(pc), PICK(1).i32);
      break;
    }

    case InterpreterOpcode::I64Store8:
    case InterpreterOpcode::I64Store16:
    case InterpreterOpcode::I64Store32:
    case InterpreterOpcode::I64Store: {
      uint32_t memory_index = read_u32(&pc);
      writef(stream, "%s $%u:%u+$%u, %" PRIu64 "\n",
             get_interpreter_opcode_name(opcode), memory_index, PICK(2).i32,
             read_u32_at(pc), PICK(1).i64);
      break;
    }

    case InterpreterOpcode::F32Store: {
      uint32_t memory_index = read_u32(&pc);
      writef(stream, "%s $%u:%u+$%u, %g\n", get_interpreter_opcode_name(opcode),
             memory_index, PICK(2).i32, read_u32_at(pc),
             bitcast_u32_to_f32(PICK(1).f32_bits));
      break;
    }

    case InterpreterOpcode::F64Store: {
      uint32_t memory_index = read_u32(&pc);
      writef(stream, "%s $%u:%u+$%u, %g\n", get_interpreter_opcode_name(opcode),
             memory_index, PICK(2).i32, read_u32_at(pc),
             bitcast_u64_to_f64(PICK(1).f64_bits));
      break;
    }

    case InterpreterOpcode::GrowMemory: {
      uint32_t memory_index = read_u32(&pc);
      writef(stream, "%s $%u:%u\n", get_interpreter_opcode_name(opcode),
             memory_index, TOP().i32);
      break;
    }

    case InterpreterOpcode::I32Add:
    case InterpreterOpcode::I32Sub:
    case InterpreterOpcode::I32Mul:
    case InterpreterOpcode::I32DivS:
    case InterpreterOpcode::I32DivU:
    case InterpreterOpcode::I32RemS:
    case InterpreterOpcode::I32RemU:
    case InterpreterOpcode::I32And:
    case InterpreterOpcode::I32Or:
    case InterpreterOpcode::I32Xor:
    case InterpreterOpcode::I32Shl:
    case InterpreterOpcode::I32ShrU:
    case InterpreterOpcode::I32ShrS:
    case InterpreterOpcode::I32Eq:
    case InterpreterOpcode::I32Ne:
    case InterpreterOpcode::I32LtS:
    case InterpreterOpcode::I32LeS:
    case InterpreterOpcode::I32LtU:
    case InterpreterOpcode::I32LeU:
    case InterpreterOpcode::I32GtS:
    case InterpreterOpcode::I32GeS:
    case InterpreterOpcode::I32GtU:
    case InterpreterOpcode::I32GeU:
    case InterpreterOpcode::I32Rotr:
    case InterpreterOpcode::I32Rotl:
      writef(stream, "%s %u, %u\n", get_interpreter_opcode_name(opcode),
             PICK(2).i32, PICK(1).i32);
      break;

    case InterpreterOpcode::I32Clz:
    case InterpreterOpcode::I32Ctz:
    case InterpreterOpcode::I32Popcnt:
    case InterpreterOpcode::I32Eqz:
      writef(stream, "%s %u\n", get_interpreter_opcode_name(opcode), TOP().i32);
      break;

    case InterpreterOpcode::I64Add:
    case InterpreterOpcode::I64Sub:
    case InterpreterOpcode::I64Mul:
    case InterpreterOpcode::I64DivS:
    case InterpreterOpcode::I64DivU:
    case InterpreterOpcode::I64RemS:
    case InterpreterOpcode::I64RemU:
    case InterpreterOpcode::I64And:
    case InterpreterOpcode::I64Or:
    case InterpreterOpcode::I64Xor:
    case InterpreterOpcode::I64Shl:
    case InterpreterOpcode::I64ShrU:
    case InterpreterOpcode::I64ShrS:
    case InterpreterOpcode::I64Eq:
    case InterpreterOpcode::I64Ne:
    case InterpreterOpcode::I64LtS:
    case InterpreterOpcode::I64LeS:
    case InterpreterOpcode::I64LtU:
    case InterpreterOpcode::I64LeU:
    case InterpreterOpcode::I64GtS:
    case InterpreterOpcode::I64GeS:
    case InterpreterOpcode::I64GtU:
    case InterpreterOpcode::I64GeU:
    case InterpreterOpcode::I64Rotr:
    case InterpreterOpcode::I64Rotl:
      writef(stream, "%s %" PRIu64 ", %" PRIu64 "\n",
             get_interpreter_opcode_name(opcode), PICK(2).i64, PICK(1).i64);
      break;

    case InterpreterOpcode::I64Clz:
    case InterpreterOpcode::I64Ctz:
    case InterpreterOpcode::I64Popcnt:
    case InterpreterOpcode::I64Eqz:
      writef(stream, "%s %" PRIu64 "\n", get_interpreter_opcode_name(opcode),
             TOP().i64);
      break;

    case InterpreterOpcode::F32Add:
    case InterpreterOpcode::F32Sub:
    case InterpreterOpcode::F32Mul:
    case InterpreterOpcode::F32Div:
    case InterpreterOpcode::F32Min:
    case InterpreterOpcode::F32Max:
    case InterpreterOpcode::F32Copysign:
    case InterpreterOpcode::F32Eq:
    case InterpreterOpcode::F32Ne:
    case InterpreterOpcode::F32Lt:
    case InterpreterOpcode::F32Le:
    case InterpreterOpcode::F32Gt:
    case InterpreterOpcode::F32Ge:
      writef(stream, "%s %g, %g\n", get_interpreter_opcode_name(opcode),
             bitcast_u32_to_f32(PICK(2).i32), bitcast_u32_to_f32(PICK(1).i32));
      break;

    case InterpreterOpcode::F32Abs:
    case InterpreterOpcode::F32Neg:
    case InterpreterOpcode::F32Ceil:
    case InterpreterOpcode::F32Floor:
    case InterpreterOpcode::F32Trunc:
    case InterpreterOpcode::F32Nearest:
    case InterpreterOpcode::F32Sqrt:
      writef(stream, "%s %g\n", get_interpreter_opcode_name(opcode),
             bitcast_u32_to_f32(TOP().i32));
      break;

    case InterpreterOpcode::F64Add:
    case InterpreterOpcode::F64Sub:
    case InterpreterOpcode::F64Mul:
    case InterpreterOpcode::F64Div:
    case InterpreterOpcode::F64Min:
    case InterpreterOpcode::F64Max:
    case InterpreterOpcode::F64Copysign:
    case InterpreterOpcode::F64Eq:
    case InterpreterOpcode::F64Ne:
    case InterpreterOpcode::F64Lt:
    case InterpreterOpcode::F64Le:
    case InterpreterOpcode::F64Gt:
    case InterpreterOpcode::F64Ge:
      writef(stream, "%s %g, %g\n", get_interpreter_opcode_name(opcode),
             bitcast_u64_to_f64(PICK(2).i64), bitcast_u64_to_f64(PICK(1).i64));
      break;

    case InterpreterOpcode::F64Abs:
    case InterpreterOpcode::F64Neg:
    case InterpreterOpcode::F64Ceil:
    case InterpreterOpcode::F64Floor:
    case InterpreterOpcode::F64Trunc:
    case InterpreterOpcode::F64Nearest:
    case InterpreterOpcode::F64Sqrt:
      writef(stream, "%s %g\n", get_interpreter_opcode_name(opcode),
             bitcast_u64_to_f64(TOP().i64));
      break;

    case InterpreterOpcode::I32TruncSF32:
    case InterpreterOpcode::I32TruncUF32:
    case InterpreterOpcode::I64TruncSF32:
    case InterpreterOpcode::I64TruncUF32:
    case InterpreterOpcode::F64PromoteF32:
    case InterpreterOpcode::I32ReinterpretF32:
      writef(stream, "%s %g\n", get_interpreter_opcode_name(opcode),
             bitcast_u32_to_f32(TOP().i32));
      break;

    case InterpreterOpcode::I32TruncSF64:
    case InterpreterOpcode::I32TruncUF64:
    case InterpreterOpcode::I64TruncSF64:
    case InterpreterOpcode::I64TruncUF64:
    case InterpreterOpcode::F32DemoteF64:
    case InterpreterOpcode::I64ReinterpretF64:
      writef(stream, "%s %g\n", get_interpreter_opcode_name(opcode),
             bitcast_u64_to_f64(TOP().i64));
      break;

    case InterpreterOpcode::I32WrapI64:
    case InterpreterOpcode::F32ConvertSI64:
    case InterpreterOpcode::F32ConvertUI64:
    case InterpreterOpcode::F64ConvertSI64:
    case InterpreterOpcode::F64ConvertUI64:
    case InterpreterOpcode::F64ReinterpretI64:
      writef(stream, "%s %" PRIu64 "\n", get_interpreter_opcode_name(opcode),
             TOP().i64);
      break;

    case InterpreterOpcode::I64ExtendSI32:
    case InterpreterOpcode::I64ExtendUI32:
    case InterpreterOpcode::F32ConvertSI32:
    case InterpreterOpcode::F32ConvertUI32:
    case InterpreterOpcode::F32ReinterpretI32:
    case InterpreterOpcode::F64ConvertSI32:
    case InterpreterOpcode::F64ConvertUI32:
      writef(stream, "%s %u\n", get_interpreter_opcode_name(opcode), TOP().i32);
      break;

    case InterpreterOpcode::Alloca:
      writef(stream, "%s $%u\n", get_interpreter_opcode_name(opcode),
             read_u32_at(pc));
      break;

    case InterpreterOpcode::BrUnless:
      writef(stream, "%s @%u, %u\n", get_interpreter_opcode_name(opcode),
             read_u32_at(pc), TOP().i32);
      break;

    case InterpreterOpcode::DropKeep:
      writef(stream, "%s $%u $%u\n", get_interpreter_opcode_name(opcode),
             read_u32_at(pc), *(pc + 4));
      break;

    case InterpreterOpcode::Data:
      /* shouldn't ever execute this */
      assert(0);
      break;

    default:
      assert(0);
      break;
  }
}

void disassemble(InterpreterEnvironment* env,
                 Stream* stream,
                 uint32_t from,
                 uint32_t to) {
  /* TODO(binji): mark function entries */
  /* TODO(binji): track value stack size */
  if (from >= env->istream.size)
    return;
  if (to > env->istream.size)
    to = env->istream.size;
  const uint8_t* istream = reinterpret_cast<const uint8_t*>(env->istream.start);
  const uint8_t* pc = &istream[from];

  while (static_cast<uint32_t>(pc - istream) < to) {
    writef(stream, "%4" PRIzd "| ", pc - istream);

    InterpreterOpcode opcode = static_cast<InterpreterOpcode>(*pc++);
    switch (opcode) {
      case InterpreterOpcode::Select:
        writef(stream, "%s %%[-3], %%[-2], %%[-1]\n",
               get_interpreter_opcode_name(opcode));
        break;

      case InterpreterOpcode::Br:
        writef(stream, "%s @%u\n", get_interpreter_opcode_name(opcode),
               read_u32(&pc));
        break;

      case InterpreterOpcode::BrIf:
        writef(stream, "%s @%u, %%[-1]\n", get_interpreter_opcode_name(opcode),
               read_u32(&pc));
        break;

      case InterpreterOpcode::BrTable: {
        uint32_t num_targets = read_u32(&pc);
        uint32_t table_offset = read_u32(&pc);
        writef(stream, "%s %%[-1], $#%u, table:$%u\n",
               get_interpreter_opcode_name(opcode), num_targets, table_offset);
        break;
      }

      case InterpreterOpcode::Nop:
      case InterpreterOpcode::Return:
      case InterpreterOpcode::Unreachable:
      case InterpreterOpcode::Drop:
        writef(stream, "%s\n", get_interpreter_opcode_name(opcode));
        break;

      case InterpreterOpcode::CurrentMemory: {
        uint32_t memory_index = read_u32(&pc);
        writef(stream, "%s $%u\n", get_interpreter_opcode_name(opcode),
               memory_index);
        break;
      }

      case InterpreterOpcode::I32Const:
        writef(stream, "%s $%u\n", get_interpreter_opcode_name(opcode),
               read_u32(&pc));
        break;

      case InterpreterOpcode::I64Const:
        writef(stream, "%s $%" PRIu64 "\n", get_interpreter_opcode_name(opcode),
               read_u64(&pc));
        break;

      case InterpreterOpcode::F32Const:
        writef(stream, "%s $%g\n", get_interpreter_opcode_name(opcode),
               bitcast_u32_to_f32(read_u32(&pc)));
        break;

      case InterpreterOpcode::F64Const:
        writef(stream, "%s $%g\n", get_interpreter_opcode_name(opcode),
               bitcast_u64_to_f64(read_u64(&pc)));
        break;

      case InterpreterOpcode::GetLocal:
      case InterpreterOpcode::GetGlobal:
        writef(stream, "%s $%u\n", get_interpreter_opcode_name(opcode),
               read_u32(&pc));
        break;

      case InterpreterOpcode::SetLocal:
      case InterpreterOpcode::SetGlobal:
      case InterpreterOpcode::TeeLocal:
        writef(stream, "%s $%u, %%[-1]\n", get_interpreter_opcode_name(opcode),
               read_u32(&pc));
        break;

      case InterpreterOpcode::Call:
        writef(stream, "%s @%u\n", get_interpreter_opcode_name(opcode),
               read_u32(&pc));
        break;

      case InterpreterOpcode::CallIndirect: {
        uint32_t table_index = read_u32(&pc);
        writef(stream, "%s $%u:%u, %%[-1]\n",
               get_interpreter_opcode_name(opcode), table_index, read_u32(&pc));
        break;
      }

      case InterpreterOpcode::CallHost:
        writef(stream, "%s $%u\n", get_interpreter_opcode_name(opcode),
               read_u32(&pc));
        break;

      case InterpreterOpcode::I32Load8S:
      case InterpreterOpcode::I32Load8U:
      case InterpreterOpcode::I32Load16S:
      case InterpreterOpcode::I32Load16U:
      case InterpreterOpcode::I64Load8S:
      case InterpreterOpcode::I64Load8U:
      case InterpreterOpcode::I64Load16S:
      case InterpreterOpcode::I64Load16U:
      case InterpreterOpcode::I64Load32S:
      case InterpreterOpcode::I64Load32U:
      case InterpreterOpcode::I32Load:
      case InterpreterOpcode::I64Load:
      case InterpreterOpcode::F32Load:
      case InterpreterOpcode::F64Load: {
        uint32_t memory_index = read_u32(&pc);
        writef(stream, "%s $%u:%%[-1]+$%u\n",
               get_interpreter_opcode_name(opcode), memory_index,
               read_u32(&pc));
        break;
      }

      case InterpreterOpcode::I32Store8:
      case InterpreterOpcode::I32Store16:
      case InterpreterOpcode::I32Store:
      case InterpreterOpcode::I64Store8:
      case InterpreterOpcode::I64Store16:
      case InterpreterOpcode::I64Store32:
      case InterpreterOpcode::I64Store:
      case InterpreterOpcode::F32Store:
      case InterpreterOpcode::F64Store: {
        uint32_t memory_index = read_u32(&pc);
        writef(stream, "%s %%[-2]+$%u, $%u:%%[-1]\n",
               get_interpreter_opcode_name(opcode), memory_index,
               read_u32(&pc));
        break;
      }

      case InterpreterOpcode::I32Add:
      case InterpreterOpcode::I32Sub:
      case InterpreterOpcode::I32Mul:
      case InterpreterOpcode::I32DivS:
      case InterpreterOpcode::I32DivU:
      case InterpreterOpcode::I32RemS:
      case InterpreterOpcode::I32RemU:
      case InterpreterOpcode::I32And:
      case InterpreterOpcode::I32Or:
      case InterpreterOpcode::I32Xor:
      case InterpreterOpcode::I32Shl:
      case InterpreterOpcode::I32ShrU:
      case InterpreterOpcode::I32ShrS:
      case InterpreterOpcode::I32Eq:
      case InterpreterOpcode::I32Ne:
      case InterpreterOpcode::I32LtS:
      case InterpreterOpcode::I32LeS:
      case InterpreterOpcode::I32LtU:
      case InterpreterOpcode::I32LeU:
      case InterpreterOpcode::I32GtS:
      case InterpreterOpcode::I32GeS:
      case InterpreterOpcode::I32GtU:
      case InterpreterOpcode::I32GeU:
      case InterpreterOpcode::I32Rotr:
      case InterpreterOpcode::I32Rotl:
      case InterpreterOpcode::F32Add:
      case InterpreterOpcode::F32Sub:
      case InterpreterOpcode::F32Mul:
      case InterpreterOpcode::F32Div:
      case InterpreterOpcode::F32Min:
      case InterpreterOpcode::F32Max:
      case InterpreterOpcode::F32Copysign:
      case InterpreterOpcode::F32Eq:
      case InterpreterOpcode::F32Ne:
      case InterpreterOpcode::F32Lt:
      case InterpreterOpcode::F32Le:
      case InterpreterOpcode::F32Gt:
      case InterpreterOpcode::F32Ge:
      case InterpreterOpcode::I64Add:
      case InterpreterOpcode::I64Sub:
      case InterpreterOpcode::I64Mul:
      case InterpreterOpcode::I64DivS:
      case InterpreterOpcode::I64DivU:
      case InterpreterOpcode::I64RemS:
      case InterpreterOpcode::I64RemU:
      case InterpreterOpcode::I64And:
      case InterpreterOpcode::I64Or:
      case InterpreterOpcode::I64Xor:
      case InterpreterOpcode::I64Shl:
      case InterpreterOpcode::I64ShrU:
      case InterpreterOpcode::I64ShrS:
      case InterpreterOpcode::I64Eq:
      case InterpreterOpcode::I64Ne:
      case InterpreterOpcode::I64LtS:
      case InterpreterOpcode::I64LeS:
      case InterpreterOpcode::I64LtU:
      case InterpreterOpcode::I64LeU:
      case InterpreterOpcode::I64GtS:
      case InterpreterOpcode::I64GeS:
      case InterpreterOpcode::I64GtU:
      case InterpreterOpcode::I64GeU:
      case InterpreterOpcode::I64Rotr:
      case InterpreterOpcode::I64Rotl:
      case InterpreterOpcode::F64Add:
      case InterpreterOpcode::F64Sub:
      case InterpreterOpcode::F64Mul:
      case InterpreterOpcode::F64Div:
      case InterpreterOpcode::F64Min:
      case InterpreterOpcode::F64Max:
      case InterpreterOpcode::F64Copysign:
      case InterpreterOpcode::F64Eq:
      case InterpreterOpcode::F64Ne:
      case InterpreterOpcode::F64Lt:
      case InterpreterOpcode::F64Le:
      case InterpreterOpcode::F64Gt:
      case InterpreterOpcode::F64Ge:
        writef(stream, "%s %%[-2], %%[-1]\n",
               get_interpreter_opcode_name(opcode));
        break;

      case InterpreterOpcode::I32Clz:
      case InterpreterOpcode::I32Ctz:
      case InterpreterOpcode::I32Popcnt:
      case InterpreterOpcode::I32Eqz:
      case InterpreterOpcode::I64Clz:
      case InterpreterOpcode::I64Ctz:
      case InterpreterOpcode::I64Popcnt:
      case InterpreterOpcode::I64Eqz:
      case InterpreterOpcode::F32Abs:
      case InterpreterOpcode::F32Neg:
      case InterpreterOpcode::F32Ceil:
      case InterpreterOpcode::F32Floor:
      case InterpreterOpcode::F32Trunc:
      case InterpreterOpcode::F32Nearest:
      case InterpreterOpcode::F32Sqrt:
      case InterpreterOpcode::F64Abs:
      case InterpreterOpcode::F64Neg:
      case InterpreterOpcode::F64Ceil:
      case InterpreterOpcode::F64Floor:
      case InterpreterOpcode::F64Trunc:
      case InterpreterOpcode::F64Nearest:
      case InterpreterOpcode::F64Sqrt:
      case InterpreterOpcode::I32TruncSF32:
      case InterpreterOpcode::I32TruncUF32:
      case InterpreterOpcode::I64TruncSF32:
      case InterpreterOpcode::I64TruncUF32:
      case InterpreterOpcode::F64PromoteF32:
      case InterpreterOpcode::I32ReinterpretF32:
      case InterpreterOpcode::I32TruncSF64:
      case InterpreterOpcode::I32TruncUF64:
      case InterpreterOpcode::I64TruncSF64:
      case InterpreterOpcode::I64TruncUF64:
      case InterpreterOpcode::F32DemoteF64:
      case InterpreterOpcode::I64ReinterpretF64:
      case InterpreterOpcode::I32WrapI64:
      case InterpreterOpcode::F32ConvertSI64:
      case InterpreterOpcode::F32ConvertUI64:
      case InterpreterOpcode::F64ConvertSI64:
      case InterpreterOpcode::F64ConvertUI64:
      case InterpreterOpcode::F64ReinterpretI64:
      case InterpreterOpcode::I64ExtendSI32:
      case InterpreterOpcode::I64ExtendUI32:
      case InterpreterOpcode::F32ConvertSI32:
      case InterpreterOpcode::F32ConvertUI32:
      case InterpreterOpcode::F32ReinterpretI32:
      case InterpreterOpcode::F64ConvertSI32:
      case InterpreterOpcode::F64ConvertUI32:
        writef(stream, "%s %%[-1]\n", get_interpreter_opcode_name(opcode));
        break;

      case InterpreterOpcode::GrowMemory: {
        uint32_t memory_index = read_u32(&pc);
        writef(stream, "%s $%u:%%[-1]\n", get_interpreter_opcode_name(opcode),
               memory_index);
        break;
      }

      case InterpreterOpcode::Alloca:
        writef(stream, "%s $%u\n", get_interpreter_opcode_name(opcode),
               read_u32(&pc));
        break;

      case InterpreterOpcode::BrUnless:
        writef(stream, "%s @%u, %%[-1]\n", get_interpreter_opcode_name(opcode),
               read_u32(&pc));
        break;

      case InterpreterOpcode::DropKeep: {
        uint32_t drop = read_u32(&pc);
        uint32_t keep = *pc++;
        writef(stream, "%s $%u $%u\n", get_interpreter_opcode_name(opcode),
               drop, keep);
        break;
      }

      case InterpreterOpcode::Data: {
        uint32_t num_bytes = read_u32(&pc);
        writef(stream, "%s $%u\n", get_interpreter_opcode_name(opcode),
               num_bytes);
        /* for now, the only reason this is emitted is for br_table, so display
         * it as a list of table entries */
        if (num_bytes % WABT_TABLE_ENTRY_SIZE == 0) {
          uint32_t num_entries = num_bytes / WABT_TABLE_ENTRY_SIZE;
          uint32_t i;
          for (i = 0; i < num_entries; ++i) {
            writef(stream, "%4" PRIzd "| ", pc - istream);
            uint32_t offset;
            uint32_t drop;
            uint8_t keep;
            read_table_entry_at(pc, &offset, &drop, &keep);
            writef(stream, "  entry %d: offset: %u drop: %u keep: %u\n", i,
                   offset, drop, keep);
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

void disassemble_module(InterpreterEnvironment* env,
                        Stream* stream,
                        InterpreterModule* module) {
  assert(!module->is_host);
  disassemble(env, stream, module->defined.istream_start,
              module->defined.istream_end);
}

}  // namespace wabt
