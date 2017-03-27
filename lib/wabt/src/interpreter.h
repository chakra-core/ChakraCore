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

#ifndef WABT_INTERPRETER_H_
#define WABT_INTERPRETER_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "common.h"
#include "binding-hash.h"
#include "writer.h"

namespace wabt {

struct Stream;

#define FOREACH_INTERPRETER_RESULT(V)                                       \
  V(Ok, "ok")                                                               \
  /* returned from the top-most function */                                 \
  V(Returned, "returned")                                                   \
  /* memory access is out of bounds */                                      \
  V(TrapMemoryAccessOutOfBounds, "out of bounds memory access")             \
  /* converting from float -> int would overflow int */                     \
  V(TrapIntegerOverflow, "integer overflow")                                \
  /* dividend is zero in integer divide */                                  \
  V(TrapIntegerDivideByZero, "integer divide by zero")                      \
  /* converting from float -> int where float is nan */                     \
  V(TrapInvalidConversionToInteger, "invalid conversion to integer")        \
  /* function table index is out of bounds */                               \
  V(TrapUndefinedTableIndex, "undefined table index")                       \
  /* function table element is uninitialized */                             \
  V(TrapUninitializedTableElement, "uninitialized table element")           \
  /* unreachable instruction executed */                                    \
  V(TrapUnreachable, "unreachable executed")                                \
  /* call indirect signature doesn't match function table signature */      \
  V(TrapIndirectCallSignatureMismatch, "indirect call signature mismatch")  \
  /* ran out of call stack frames (probably infinite recursion) */          \
  V(TrapCallStackExhausted, "call stack exhausted")                         \
  /* ran out of value stack space */                                        \
  V(TrapValueStackExhausted, "value stack exhausted")                       \
  /* we called a host function, but the return value didn't match the */    \
  /* expected type */                                                       \
  V(TrapHostResultTypeMismatch, "host result type mismatch")                \
  /* we called an import function, but it didn't complete succesfully */    \
  V(TrapHostTrapped, "host function trapped")                               \
  /* we attempted to call a function with the an argument list that doesn't \
   * match the function signature */                                        \
  V(ArgumentTypeMismatch, "argument type mismatch")                         \
  /* we tried to get an export by name that doesn't exist */                \
  V(UnknownExport, "unknown export")                                        \
  /* the expected export kind doesn't match. */                             \
  V(ExportKindMismatch, "export kind mismatch")

enum class InterpreterResult {
#define V(Name, str) Name,
  FOREACH_INTERPRETER_RESULT(V)
#undef V
};

#define WABT_INVALID_INDEX static_cast<uint32_t>(~0)
#define WABT_INVALID_OFFSET static_cast<uint32_t>(~0)
#define WABT_TABLE_ENTRY_SIZE (sizeof(uint32_t) * 2 + sizeof(uint8_t))
#define WABT_TABLE_ENTRY_OFFSET_OFFSET 0
#define WABT_TABLE_ENTRY_DROP_OFFSET sizeof(uint32_t)
#define WABT_TABLE_ENTRY_KEEP_OFFSET (sizeof(uint32_t) * 2)

#define WABT_FOREACH_INTERPRETER_OPCODE(V)         \
  WABT_FOREACH_OPCODE(V)                           \
  V(___, ___, ___, 0, 0xfb, Alloca, "alloca")      \
  V(___, ___, ___, 0, 0xfc, BrUnless, "br_unless") \
  V(___, ___, ___, 0, 0xfd, CallHost, "call_host") \
  V(___, ___, ___, 0, 0xfe, Data, "data")          \
  V(___, ___, ___, 0, 0xff, DropKeep, "drop_keep")

enum class InterpreterOpcode {
/* push space on the value stack for N entries */
#define V(rtype, type1, type2, mem_size, code, Name, text) Name = code,
  WABT_FOREACH_INTERPRETER_OPCODE(V)
#undef V

      First = static_cast<int>(Opcode::First),
  Last = DropKeep,
};
static const int kInterpreterOpcodeCount = WABT_ENUM_COUNT(InterpreterOpcode);

struct InterpreterFuncSignature {
  std::vector<Type> param_types;
  std::vector<Type> result_types;
};

struct InterpreterTable {
  explicit InterpreterTable(const Limits& limits)
      : limits(limits), func_indexes(limits.initial, WABT_INVALID_INDEX) {}

  Limits limits;
  std::vector<uint32_t> func_indexes;
};

struct InterpreterMemory {
  InterpreterMemory() {
    WABT_ZERO_MEMORY(page_limits);
  }
  explicit InterpreterMemory(const Limits& limits)
      : page_limits(limits), data(limits.initial * WABT_PAGE_SIZE) {}

  Limits page_limits;
  std::vector<char> data;
};

union InterpreterValue {
  uint32_t i32;
  uint64_t i64;
  uint32_t f32_bits;
  uint64_t f64_bits;
};

struct InterpreterTypedValue {
  InterpreterTypedValue() {}
  explicit InterpreterTypedValue(Type type): type(type) {}
  InterpreterTypedValue(Type type, const InterpreterValue& value)
      : type(type), value(value) {}

  Type type;
  InterpreterValue value;
};

struct InterpreterGlobal {
  InterpreterGlobal() : mutable_(false), import_index(WABT_INVALID_INDEX) {}
  InterpreterGlobal(const InterpreterTypedValue& typed_value, bool mutable_)
      : typed_value(typed_value), mutable_(mutable_) {}

  InterpreterTypedValue typed_value;
  bool mutable_;
  uint32_t import_index; /* or INVALID_INDEX if not imported */
};

struct InterpreterImport {
  InterpreterImport();
  InterpreterImport(InterpreterImport&&);
  InterpreterImport& operator=(InterpreterImport&&);
  ~InterpreterImport();

  StringSlice module_name;
  StringSlice field_name;
  ExternalKind kind;
  union {
    struct {
      uint32_t sig_index;
    } func;
    struct {
      Limits limits;
    } table, memory;
    struct {
      Type type;
      bool mutable_;
    } global;
  };
};

struct InterpreterFunc;

typedef Result (*InterpreterHostFuncCallback)(
    const struct HostInterpreterFunc* func,
    const InterpreterFuncSignature* sig,
    uint32_t num_args,
    InterpreterTypedValue* args,
    uint32_t num_results,
    InterpreterTypedValue* out_results,
    void* user_data);

struct InterpreterFunc {
  WABT_DISALLOW_COPY_AND_ASSIGN(InterpreterFunc);
  InterpreterFunc(uint32_t sig_index, bool is_host)
      : sig_index(sig_index), is_host(is_host) {}
  virtual ~InterpreterFunc() {}

  inline struct DefinedInterpreterFunc* as_defined();
  inline struct HostInterpreterFunc* as_host();

  uint32_t sig_index;
  bool is_host;
};

struct DefinedInterpreterFunc : InterpreterFunc {
  DefinedInterpreterFunc(uint32_t sig_index)
      : InterpreterFunc(sig_index, false),
        offset(WABT_INVALID_INDEX),
        local_decl_count(0),
        local_count(0) {}

  uint32_t offset;
  uint32_t local_decl_count;
  uint32_t local_count;
  std::vector<Type> param_and_local_types;
};

struct HostInterpreterFunc : InterpreterFunc {
  HostInterpreterFunc(const StringSlice& module_name,
                      const StringSlice& field_name,
                      uint32_t sig_index)
      : InterpreterFunc(sig_index, true),
        module_name(module_name),
        field_name(field_name) {}

  StringSlice module_name;
  StringSlice field_name;
  InterpreterHostFuncCallback callback;
  void* user_data;
};

DefinedInterpreterFunc* InterpreterFunc::as_defined() {
  assert(!is_host);
  return static_cast<DefinedInterpreterFunc*>(this);
}

HostInterpreterFunc* InterpreterFunc::as_host() {
  assert(is_host);
  return static_cast<HostInterpreterFunc*>(this);
}

struct InterpreterExport {
  InterpreterExport(const StringSlice& name, ExternalKind kind, uint32_t index)
      : name(name), kind(kind), index(index) {}
  InterpreterExport(InterpreterExport&&);
  InterpreterExport& operator=(InterpreterExport&&);
  ~InterpreterExport();

  StringSlice name;
  ExternalKind kind;
  uint32_t index;
};

struct PrintErrorCallback {
  void* user_data;
  void (*print_error)(const char* msg, void* user_data);
};

struct InterpreterHostImportDelegate {
  void* user_data;
  Result (*import_func)(InterpreterImport*,
                        InterpreterFunc*,
                        InterpreterFuncSignature*,
                        PrintErrorCallback,
                        void* user_data);
  Result (*import_table)(InterpreterImport*,
                         InterpreterTable*,
                         PrintErrorCallback,
                         void* user_data);
  Result (*import_memory)(InterpreterImport*,
                          InterpreterMemory*,
                          PrintErrorCallback,
                          void* user_data);
  Result (*import_global)(InterpreterImport*,
                          InterpreterGlobal*,
                          PrintErrorCallback,
                          void* user_data);
};

struct InterpreterModule {
  WABT_DISALLOW_COPY_AND_ASSIGN(InterpreterModule);
  explicit InterpreterModule(bool is_host);
  InterpreterModule(const StringSlice& name, bool is_host);
  virtual ~InterpreterModule();

  inline struct DefinedInterpreterModule* as_defined();
  inline struct HostInterpreterModule* as_host();

  StringSlice name;
  std::vector<InterpreterExport> exports;
  BindingHash export_bindings;
  uint32_t memory_index; /* INVALID_INDEX if not defined */
  uint32_t table_index;  /* INVALID_INDEX if not defined */
  bool is_host;
};

struct DefinedInterpreterModule : InterpreterModule {
  explicit DefinedInterpreterModule(size_t istream_start);

  std::vector<InterpreterImport> imports;
  uint32_t start_func_index; /* INVALID_INDEX if not defined */
  size_t istream_start;
  size_t istream_end;
};

struct HostInterpreterModule : InterpreterModule {
  HostInterpreterModule(const StringSlice& name);

  InterpreterHostImportDelegate import_delegate;
};

DefinedInterpreterModule* InterpreterModule::as_defined() {
  assert(!is_host);
  return static_cast<DefinedInterpreterModule*>(this);
}

HostInterpreterModule* InterpreterModule::as_host() {
  assert(is_host);
  return static_cast<HostInterpreterModule*>(this);
}

/* Used to track and reset the state of the environment. */
struct InterpreterEnvironmentMark {
  size_t modules_size;
  size_t sigs_size;
  size_t funcs_size;
  size_t memories_size;
  size_t tables_size;
  size_t globals_size;
  size_t istream_size;
};

struct InterpreterEnvironment {
  InterpreterEnvironment();

  std::vector<std::unique_ptr<InterpreterModule>> modules;
  std::vector<InterpreterFuncSignature> sigs;
  std::vector<std::unique_ptr<InterpreterFunc>> funcs;
  std::vector<InterpreterMemory> memories;
  std::vector<InterpreterTable> tables;
  std::vector<InterpreterGlobal> globals;
  OutputBuffer istream;
  BindingHash module_bindings;
  BindingHash registered_module_bindings;
};

struct InterpreterThread {
  InterpreterThread();

  InterpreterEnvironment* env;
  std::vector<InterpreterValue> value_stack;
  std::vector<uint32_t> call_stack;
  InterpreterValue* value_stack_top;
  InterpreterValue* value_stack_end;
  uint32_t* call_stack_top;
  uint32_t* call_stack_end;
  uint32_t pc;
};

#define WABT_INTERPRETER_THREAD_OPTIONS_DEFAULT \
  { 512 * 1024 / sizeof(InterpreterValue), 64 * 1024, WABT_INVALID_OFFSET }

struct InterpreterThreadOptions {
  uint32_t value_stack_size;
  uint32_t call_stack_size;
  uint32_t pc;
};

bool is_canonical_nan_f32(uint32_t f32_bits);
bool is_canonical_nan_f64(uint64_t f64_bits);
bool is_arithmetic_nan_f32(uint32_t f32_bits);
bool is_arithmetic_nan_f64(uint64_t f64_bits);
bool func_signatures_are_equal(InterpreterEnvironment* env,
                               uint32_t sig_index_0,
                               uint32_t sig_index_1);

void destroy_interpreter_environment(InterpreterEnvironment* env);
InterpreterEnvironmentMark mark_interpreter_environment(
    InterpreterEnvironment* env);
void reset_interpreter_environment_to_mark(InterpreterEnvironment* env,
                                           InterpreterEnvironmentMark mark);
HostInterpreterModule* append_host_module(InterpreterEnvironment* env,
                                          StringSlice name);
void init_interpreter_thread(InterpreterEnvironment* env,
                             InterpreterThread* thread,
                             InterpreterThreadOptions* options);
InterpreterResult push_thread_value(InterpreterThread* thread,
                                    InterpreterValue value);
void destroy_interpreter_thread(InterpreterThread* thread);
InterpreterResult call_host(InterpreterThread* thread,
                            HostInterpreterFunc* func);
InterpreterResult run_interpreter(InterpreterThread* thread,
                                  uint32_t num_instructions,
                                  uint32_t* call_stack_return_top);
void trace_pc(InterpreterThread* thread, struct Stream* stream);
void disassemble(InterpreterEnvironment* env,
                 struct Stream* stream,
                 uint32_t from,
                 uint32_t to);
void disassemble_module(InterpreterEnvironment* env,
                        struct Stream* stream,
                        InterpreterModule* module);

InterpreterExport* get_interpreter_export_by_name(InterpreterModule* module,
                                                  const StringSlice* name);

}  // namespace wabt

#endif /* WABT_INTERPRETER_H_ */
