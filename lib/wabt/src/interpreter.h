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

#include "array.h"
#include "common.h"
#include "binding-hash.h"
#include "type-vector.h"
#include "vector.h"
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

typedef uint32_t Uint32;
WABT_DEFINE_ARRAY(uint32, Uint32);

/* TODO(binji): identical to FuncSignature. Share? */
struct InterpreterFuncSignature {
  TypeVector param_types;
  TypeVector result_types;
};
WABT_DEFINE_VECTOR(interpreter_func_signature, InterpreterFuncSignature);

struct InterpreterTable {
  Limits limits;
  Uint32Array func_indexes;
};
WABT_DEFINE_VECTOR(interpreter_table, InterpreterTable);

struct InterpreterMemory {
  char* data;
  Limits page_limits;
  uint32_t byte_size; /* Cached from page_limits. */
};
WABT_DEFINE_VECTOR(interpreter_memory, InterpreterMemory);

union InterpreterValue {
  uint32_t i32;
  uint64_t i64;
  uint32_t f32_bits;
  uint64_t f64_bits;
};
WABT_DEFINE_ARRAY(interpreter_value, InterpreterValue);

struct InterpreterTypedValue {
  Type type;
  InterpreterValue value;
};
WABT_DEFINE_VECTOR(interpreter_typed_value, InterpreterTypedValue);

struct InterpreterGlobal {
  InterpreterTypedValue typed_value;
  bool mutable_;
  uint32_t import_index; /* or INVALID_INDEX if not imported */
};
WABT_DEFINE_VECTOR(interpreter_global, InterpreterGlobal);

struct InterpreterImport {
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
WABT_DEFINE_ARRAY(interpreter_import, InterpreterImport);

struct InterpreterFunc;

typedef Result (*InterpreterHostFuncCallback)(
    const struct InterpreterFunc* func,
    const InterpreterFuncSignature* sig,
    uint32_t num_args,
    InterpreterTypedValue* args,
    uint32_t num_results,
    InterpreterTypedValue* out_results,
    void* user_data);

struct InterpreterFunc {
  uint32_t sig_index;
  bool is_host;
  union {
    struct {
      uint32_t offset;
      uint32_t local_decl_count;
      uint32_t local_count;
      TypeVector param_and_local_types;
    } defined;
    struct {
      StringSlice module_name;
      StringSlice field_name;
      InterpreterHostFuncCallback callback;
      void* user_data;
    } host;
  };
};
WABT_DEFINE_VECTOR(interpreter_func, InterpreterFunc);

struct InterpreterExport {
  StringSlice name; /* Owned by the export_bindings hash */
  ExternalKind kind;
  uint32_t index;
};
WABT_DEFINE_VECTOR(interpreter_export, InterpreterExport);

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
  StringSlice name;
  InterpreterExportVector exports;
  BindingHash export_bindings;
  uint32_t memory_index; /* INVALID_INDEX if not defined */
  uint32_t table_index;  /* INVALID_INDEX if not defined */
  bool is_host;
  union {
    struct {
      InterpreterImportArray imports;
      uint32_t start_func_index; /* INVALID_INDEX if not defined */
      size_t istream_start;
      size_t istream_end;
    } defined;
    struct {
      InterpreterHostImportDelegate import_delegate;
    } host;
  };
};
WABT_DEFINE_VECTOR(interpreter_module, InterpreterModule);

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
  InterpreterModuleVector modules;
  InterpreterFuncSignatureVector sigs;
  InterpreterFuncVector funcs;
  InterpreterMemoryVector memories;
  InterpreterTableVector tables;
  InterpreterGlobalVector globals;
  OutputBuffer istream;
  BindingHash module_bindings;
  BindingHash registered_module_bindings;
};

struct InterpreterThread {
  InterpreterEnvironment* env;
  InterpreterValueArray value_stack;
  Uint32Array call_stack;
  InterpreterValue* value_stack_top;
  InterpreterValue* value_stack_end;
  uint32_t* call_stack_top;
  uint32_t* call_stack_end;
  uint32_t pc;

  /* a temporary buffer that is for passing args to host functions */
  InterpreterTypedValueVector host_args;
};

#define WABT_INTERPRETER_THREAD_OPTIONS_DEFAULT \
  { 512 * 1024 / sizeof(InterpreterValue), 64 * 1024, WABT_INVALID_OFFSET }

struct InterpreterThreadOptions {
  uint32_t value_stack_size;
  uint32_t call_stack_size;
  uint32_t pc;
};

bool is_nan_f32(uint32_t f32_bits);
bool is_nan_f64(uint64_t f64_bits);
bool func_signatures_are_equal(InterpreterEnvironment* env,
                               uint32_t sig_index_0,
                               uint32_t sig_index_1);

void init_interpreter_environment(InterpreterEnvironment* env);
void destroy_interpreter_environment(InterpreterEnvironment* env);
InterpreterEnvironmentMark mark_interpreter_environment(
    InterpreterEnvironment* env);
void reset_interpreter_environment_to_mark(InterpreterEnvironment* env,
                                           InterpreterEnvironmentMark mark);
InterpreterModule* append_host_module(InterpreterEnvironment* env,
                                      StringSlice name);
void init_interpreter_thread(InterpreterEnvironment* env,
                             InterpreterThread* thread,
                             InterpreterThreadOptions* options);
InterpreterResult push_thread_value(InterpreterThread* thread,
                                    InterpreterValue value);
void destroy_interpreter_thread(InterpreterThread* thread);
InterpreterResult call_host(InterpreterThread* thread, InterpreterFunc* func);
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
