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

#include <functional>
#include <memory>
#include <vector>

#include "src/binding-hash.h"
#include "src/common.h"
#include "src/opcode.h"
#include "src/stream.h"

namespace wabt {

namespace interpreter {

#define FOREACH_INTERPRETER_RESULT(V)                                       \
  V(Ok, "ok")                                                               \
  /* returned from the top-most function */                                 \
  V(Returned, "returned")                                                   \
  /* memory access is out of bounds */                                      \
  V(TrapMemoryAccessOutOfBounds, "out of bounds memory access")             \
  /* atomic memory access is unaligned  */                                  \
  V(TrapAtomicMemoryAccessUnaligned, "atomic memory access is unaligned")   \
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

enum class Result {
#define V(Name, str) Name,
  FOREACH_INTERPRETER_RESULT(V)
#undef V
};

typedef uint32_t IstreamOffset;
static const IstreamOffset kInvalidIstreamOffset = ~0;

// A table entry has the following packed layout:
//
//   struct {
//     IstreamOffset offset;
//     uint32_t drop_count;
//     uint8_t keep_count;
//   };
#define WABT_TABLE_ENTRY_SIZE \
  (sizeof(IstreamOffset) + sizeof(uint32_t) + sizeof(uint8_t))
#define WABT_TABLE_ENTRY_OFFSET_OFFSET 0
#define WABT_TABLE_ENTRY_DROP_OFFSET sizeof(uint32_t)
#define WABT_TABLE_ENTRY_KEEP_OFFSET (sizeof(IstreamOffset) + sizeof(uint32_t))

struct FuncSignature {
  FuncSignature() = default;
  FuncSignature(Index param_count,
                Type* param_types,
                Index result_count,
                Type* result_types);

  std::vector<Type> param_types;
  std::vector<Type> result_types;
};

struct Table {
  explicit Table(const Limits& limits)
      : limits(limits), func_indexes(limits.initial, kInvalidIndex) {}

  Limits limits;
  std::vector<Index> func_indexes;
};

struct Memory {
  Memory() = default;
  explicit Memory(const Limits& limits)
      : page_limits(limits), data(limits.initial * WABT_PAGE_SIZE) {}

  Limits page_limits;
  std::vector<char> data;
};

// ValueTypeRep converts from one type to its representation on the
// stack. For example, float -> uint32_t. See Value below.
template <typename T>
struct ValueTypeRepT;

template <> struct ValueTypeRepT<int32_t> { typedef uint32_t type; };
template <> struct ValueTypeRepT<uint32_t> { typedef uint32_t type; };
template <> struct ValueTypeRepT<int64_t> { typedef uint64_t type; };
template <> struct ValueTypeRepT<uint64_t> { typedef uint64_t type; };
template <> struct ValueTypeRepT<float> { typedef uint32_t type; };
template <> struct ValueTypeRepT<double> { typedef uint64_t type; };

template <typename T>
using ValueTypeRep = typename ValueTypeRepT<T>::type;

union Value {
  uint32_t i32;
  uint64_t i64;
  ValueTypeRep<float> f32_bits;
  ValueTypeRep<double> f64_bits;
};

struct TypedValue {
  TypedValue() {}
  explicit TypedValue(Type type) : type(type) {}
  TypedValue(Type type, const Value& value) : type(type), value(value) {}

  Type type;
  Value value;
};

typedef std::vector<TypedValue> TypedValues;

struct Global {
  Global() : mutable_(false), import_index(kInvalidIndex) {}
  Global(const TypedValue& typed_value, bool mutable_)
      : typed_value(typed_value), mutable_(mutable_) {}

  TypedValue typed_value;
  bool mutable_;
  Index import_index; /* or INVALID_INDEX if not imported */
};

struct Import {
  explicit Import(ExternalKind kind) : kind(kind) {}
  Import(ExternalKind kind, string_view module_name, string_view field_name)
      : kind(kind),
        module_name(module_name.to_string()),
        field_name(field_name.to_string()) {}

  ExternalKind kind;
  std::string module_name;
  std::string field_name;
};

struct FuncImport : Import {
  FuncImport() : Import(ExternalKind::Func) {}
  FuncImport(string_view module_name, string_view field_name)
      : Import(ExternalKind::Func, module_name, field_name) {}

  Index sig_index = kInvalidIndex;
};

struct TableImport : Import {
  TableImport() : Import(ExternalKind::Table) {}
  TableImport(string_view module_name, string_view field_name)
      : Import(ExternalKind::Table, module_name, field_name) {}

  Limits limits;
};

struct MemoryImport : Import {
  MemoryImport() : Import(ExternalKind::Memory) {}
  MemoryImport(string_view module_name, string_view field_name)
      : Import(ExternalKind::Memory, module_name, field_name) {}

  Limits limits;
};

struct GlobalImport : Import {
  GlobalImport() : Import(ExternalKind::Global) {}
  GlobalImport(string_view module_name, string_view field_name)
      : Import(ExternalKind::Global, module_name, field_name) {}

  Type type = Type::Void;
  bool mutable_ = false;
};

struct ExceptImport : Import {
  ExceptImport() : Import(ExternalKind::Except) {}
  ExceptImport(string_view module_name, string_view field_name)
      : Import(ExternalKind::Except, module_name, field_name) {}
};

struct Func;

typedef Result (*HostFuncCallback)(const struct HostFunc* func,
                                   const FuncSignature* sig,
                                   Index num_args,
                                   TypedValue* args,
                                   Index num_results,
                                   TypedValue* out_results,
                                   void* user_data);

struct Func {
  WABT_DISALLOW_COPY_AND_ASSIGN(Func);
  Func(Index sig_index, bool is_host)
      : sig_index(sig_index), is_host(is_host) {}
  virtual ~Func() {}

  Index sig_index;
  bool is_host;
};

struct DefinedFunc : Func {
  DefinedFunc(Index sig_index)
      : Func(sig_index, false),
        offset(kInvalidIstreamOffset),
        local_decl_count(0),
        local_count(0) {}

  static bool classof(const Func* func) { return !func->is_host; }

  IstreamOffset offset;
  Index local_decl_count;
  Index local_count;
  std::vector<Type> param_and_local_types;
};

struct HostFunc : Func {
  HostFunc(string_view module_name, string_view field_name, Index sig_index)
      : Func(sig_index, true),
        module_name(module_name.to_string()),
        field_name(field_name.to_string()) {}

  static bool classof(const Func* func) { return func->is_host; }

  std::string module_name;
  std::string field_name;
  HostFuncCallback callback;
  void* user_data;
};

struct Export {
  Export(string_view name, ExternalKind kind, Index index)
      : name(name.to_string()), kind(kind), index(index) {}

  std::string name;
  ExternalKind kind;
  Index index;
};

class HostImportDelegate {
 public:
  typedef std::function<void(const char* msg)> ErrorCallback;

  virtual ~HostImportDelegate() {}
  virtual wabt::Result ImportFunc(FuncImport*,
                                  Func*,
                                  FuncSignature*,
                                  const ErrorCallback&) = 0;
  virtual wabt::Result ImportTable(TableImport*,
                                   Table*,
                                   const ErrorCallback&) = 0;
  virtual wabt::Result ImportMemory(MemoryImport*,
                                    Memory*,
                                    const ErrorCallback&) = 0;
  virtual wabt::Result ImportGlobal(GlobalImport*,
                                    Global*,
                                    const ErrorCallback&) = 0;
};

struct Module {
  WABT_DISALLOW_COPY_AND_ASSIGN(Module);
  explicit Module(bool is_host);
  Module(string_view name, bool is_host);
  virtual ~Module() = default;

  Export* GetExport(string_view name);

  std::string name;
  std::vector<Export> exports;
  BindingHash export_bindings;
  Index memory_index; /* kInvalidIndex if not defined */
  Index table_index;  /* kInvalidIndex if not defined */
  bool is_host;
};

struct DefinedModule : Module {
  DefinedModule();
  static bool classof(const Module* module) { return !module->is_host; }

  std::vector<FuncImport> func_imports;
  std::vector<TableImport> table_imports;
  std::vector<MemoryImport> memory_imports;
  std::vector<GlobalImport> global_imports;
  std::vector<ExceptImport> except_imports;
  Index start_func_index; /* kInvalidIndex if not defined */
  IstreamOffset istream_start;
  IstreamOffset istream_end;
};

struct HostModule : Module {
  explicit HostModule(string_view name);
  static bool classof(const Module* module) { return module->is_host; }

  std::unique_ptr<HostImportDelegate> import_delegate;
};

class Environment {
 public:
  // Used to track and reset the state of the environment.
  struct MarkPoint {
    size_t modules_size = 0;
    size_t sigs_size = 0;
    size_t funcs_size = 0;
    size_t memories_size = 0;
    size_t tables_size = 0;
    size_t globals_size = 0;
    size_t istream_size = 0;
  };

  Environment();

  OutputBuffer& istream() { return *istream_; }
  void SetIstream(std::unique_ptr<OutputBuffer> istream) {
    istream_ = std::move(istream);
  }
  std::unique_ptr<OutputBuffer> ReleaseIstream() { return std::move(istream_); }

  Index GetFuncSignatureCount() const { return sigs_.size(); }
  Index GetFuncCount() const { return funcs_.size(); }
  Index GetGlobalCount() const { return globals_.size(); }
  Index GetMemoryCount() const { return memories_.size(); }
  Index GetTableCount() const { return tables_.size(); }
  Index GetModuleCount() const { return modules_.size(); }

  Index GetLastModuleIndex() const {
    return modules_.empty() ? kInvalidIndex : modules_.size() - 1;
  }
  Index FindModuleIndex(string_view name) const;

  FuncSignature* GetFuncSignature(Index index) { return &sigs_[index]; }
  Func* GetFunc(Index index) {
    assert(index < funcs_.size());
    return funcs_[index].get();
  }
  Global* GetGlobal(Index index) {
    assert(index < globals_.size());
    return &globals_[index];
  }
  Memory* GetMemory(Index index) {
    assert(index < memories_.size());
    return &memories_[index];
  }
  Table* GetTable(Index index) {
    assert(index < tables_.size());
    return &tables_[index];
  }
  Module* GetModule(Index index) {
    assert(index < modules_.size());
    return modules_[index].get();
  }

  Module* GetLastModule() {
    return modules_.empty() ? nullptr : modules_.back().get();
  }
  Module* FindModule(string_view name);
  Module* FindRegisteredModule(string_view name);

  template <typename... Args>
  FuncSignature* EmplaceBackFuncSignature(Args&&... args) {
    sigs_.emplace_back(std::forward<Args>(args)...);
    return &sigs_.back();
  }

  template <typename... Args>
  Func* EmplaceBackFunc(Args&&... args) {
    funcs_.emplace_back(std::forward<Args>(args)...);
    return funcs_.back().get();
  }

  template <typename... Args>
  Global* EmplaceBackGlobal(Args&&... args) {
    globals_.emplace_back(std::forward<Args>(args)...);
    return &globals_.back();
  }

  template <typename... Args>
  Table* EmplaceBackTable(Args&&... args) {
    tables_.emplace_back(std::forward<Args>(args)...);
    return &tables_.back();
  }

  template <typename... Args>
  Memory* EmplaceBackMemory(Args&&... args) {
    memories_.emplace_back(std::forward<Args>(args)...);
    return &memories_.back();
  }

  template <typename... Args>
  Module* EmplaceBackModule(Args&&... args) {
    modules_.emplace_back(std::forward<Args>(args)...);
    return modules_.back().get();
  }

  template <typename... Args>
  void EmplaceModuleBinding(Args&&... args) {
    module_bindings_.emplace(std::forward<Args>(args)...);
  }

  template <typename... Args>
  void EmplaceRegisteredModuleBinding(Args&&... args) {
    registered_module_bindings_.emplace(std::forward<Args>(args)...);
  }

  HostModule* AppendHostModule(string_view name);

  bool FuncSignaturesAreEqual(Index sig_index_0, Index sig_index_1) const;

  MarkPoint Mark();
  void ResetToMarkPoint(const MarkPoint&);

  void Disassemble(Stream* stream, IstreamOffset from, IstreamOffset to);
  void DisassembleModule(Stream* stream, Module*);

 private:
  friend class Thread;

  std::vector<std::unique_ptr<Module>> modules_;
  std::vector<FuncSignature> sigs_;
  std::vector<std::unique_ptr<Func>> funcs_;
  std::vector<Memory> memories_;
  std::vector<Table> tables_;
  std::vector<Global> globals_;
  std::unique_ptr<OutputBuffer> istream_;
  BindingHash module_bindings_;
  BindingHash registered_module_bindings_;
};

class Thread {
 public:
  struct Options {
    static const uint32_t kDefaultValueStackSize = 512 * 1024 / sizeof(Value);
    static const uint32_t kDefaultCallStackSize = 64 * 1024;

    explicit Options(uint32_t value_stack_size = kDefaultValueStackSize,
                     uint32_t call_stack_size = kDefaultCallStackSize,
                     IstreamOffset pc = kInvalidIstreamOffset,
                     Stream* trace_stream = nullptr);

    uint32_t value_stack_size;
    uint32_t call_stack_size;
    IstreamOffset pc;
    Stream* trace_stream;
  };

  explicit Thread(Environment*, const Options& = Options());

  Environment* env() { return env_; }

  Result RunFunction(Index func_index,
                     const TypedValues& args,
                     TypedValues* out_results);
  Result RunStartFunction(DefinedModule* module);
  Result RunExport(const Export*,
                   const TypedValues& args,
                   TypedValues* out_results);
  Result RunExportByName(Module* module,
                         string_view name,
                         const TypedValues& args,
                         TypedValues* out_results);

 private:
  const uint8_t* GetIstream() const { return env_->istream_->data.data(); }

  Result PushArgs(const FuncSignature*, const TypedValues& args);
  void CopyResults(const FuncSignature*, TypedValues* out_results);

  Result Run(int num_instructions, IstreamOffset* call_stack_return_top);
  void Trace(Stream*);

  Memory* ReadMemory(const uint8_t** pc);
  template <typename MemType>
  Result GetAccessAddress(const uint8_t** pc, void** out_address);
  template <typename MemType>
  Result GetAtomicAccessAddress(const uint8_t** pc, void** out_address);

  Value& Top();
  Value& Pick(Index depth);

  Result Push(Value) WABT_WARN_UNUSED;
  Value Pop();

  // Push/Pop values with conversions, e.g. Push<float> will convert to the
  // ValueTypeRep (uint32_t) and push that. Similarly, Pop<float> will pop the
  // value and convert to float.
  template <typename T>
  Result Push(T) WABT_WARN_UNUSED;
  template <typename T>
  T Pop();

  // Push/Pop values without conversions, e.g. Push<float> will take a uint32_t
  // argument which is the integer representation of that float value.
  // Similarly, PopRep<float> will not convert the value to a float.
  template <typename T>
  Result PushRep(ValueTypeRep<T>) WABT_WARN_UNUSED;
  template <typename T>
  ValueTypeRep<T> PopRep();

  void DropKeep(uint32_t drop_count, uint8_t keep_count);

  Result PushCall(const uint8_t* pc) WABT_WARN_UNUSED;
  IstreamOffset PopCall();

  template <typename R, typename T> using UnopFunc      = R(T);
  template <typename R, typename T> using UnopTrapFunc  = Result(T, R*);
  template <typename R, typename T> using BinopFunc     = R(T, T);
  template <typename R, typename T> using BinopTrapFunc = Result(T, T, R*);

  template <typename MemType, typename ResultType = MemType>
  Result Load(const uint8_t** pc) WABT_WARN_UNUSED;
  template <typename MemType, typename ResultType = MemType>
  Result Store(const uint8_t** pc) WABT_WARN_UNUSED;
  template <typename MemType, typename ResultType = MemType>
  Result AtomicLoad(const uint8_t** pc) WABT_WARN_UNUSED;
  template <typename MemType, typename ResultType = MemType>
  Result AtomicStore(const uint8_t** pc) WABT_WARN_UNUSED;
  template <typename MemType, typename ResultType = MemType>
  Result AtomicRmw(BinopFunc<ResultType, ResultType>,
                   const uint8_t** pc) WABT_WARN_UNUSED;
  template <typename MemType, typename ResultType = MemType>
  Result AtomicRmwCmpxchg(const uint8_t** pc) WABT_WARN_UNUSED;

  template <typename R, typename T = R>
  Result Unop(UnopFunc<R, T> func) WABT_WARN_UNUSED;
  template <typename R, typename T = R>
  Result UnopTrap(UnopTrapFunc<R, T> func) WABT_WARN_UNUSED;

  template <typename R, typename T = R>
  Result Binop(BinopFunc<R, T> func) WABT_WARN_UNUSED;
  template <typename R, typename T = R>
  Result BinopTrap(BinopTrapFunc<R, T> func) WABT_WARN_UNUSED;

  Result RunDefinedFunction(IstreamOffset);

  Result CallHost(HostFunc*);

  Environment* env_;
  std::vector<Value> value_stack_;
  std::vector<IstreamOffset> call_stack_;
  Value* value_stack_top_;
  Value* value_stack_end_;
  IstreamOffset* call_stack_top_;
  IstreamOffset* call_stack_end_;
  IstreamOffset pc_;
  Stream* trace_stream_;
};

bool IsCanonicalNan(uint32_t f32_bits);
bool IsCanonicalNan(uint64_t f64_bits);
bool IsArithmeticNan(uint32_t f32_bits);
bool IsArithmeticNan(uint64_t f64_bits);

std::string TypedValueToString(const TypedValue&);
const char* ResultToString(Result);

void WriteTypedValue(Stream* stream, const TypedValue&);
void WriteTypedValues(Stream* stream, const TypedValues&);
void WriteResult(Stream* stream, const char* desc, Result);
void WriteCall(Stream* stream,
               string_view module_name,
               string_view func_name,
               const TypedValues& args,
               const TypedValues& results,
               Result);

}  // namespace interpreter
}  // namespace wabt

#endif /* WABT_INTERPRETER_H_ */
