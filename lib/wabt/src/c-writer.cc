/*
 * Copyright 2017 WebAssembly Community Group participants
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

#include "src/c-writer.h"

#include <cctype>
#include <cinttypes>
#include <map>
#include <set>

#include "src/cast.h"
#include "src/common.h"
#include "src/ir.h"
#include "src/literal.h"
#include "src/stream.h"
#include "src/string-view.h"

#define INDENT_SIZE 2

#define UNIMPLEMENTED(x) printf("unimplemented: %s\n", (x)), abort()

namespace wabt {

namespace {

struct Label {
  Label(LabelType label_type,
        const std::string& name,
        const TypeVector& sig,
        size_t type_stack_size,
        bool used = false)
      : label_type(label_type),
        name(name),
        sig(sig),
        type_stack_size(type_stack_size),
        used(used) {}

  bool HasValue() const {
    return label_type != LabelType::Loop && !sig.empty();
  }

  LabelType label_type;
  const std::string& name;
  const TypeVector& sig;
  size_t type_stack_size;
  bool used = false;
};

template <int>
struct Name {
  explicit Name(const std::string& name) : name(name) {}
  const std::string& name;
};

typedef Name<0> LocalName;
typedef Name<1> GlobalName;
typedef Name<2> ExternalPtr;
typedef Name<3> ExternalRef;

struct GotoLabel {
  explicit GotoLabel(const Var& var) : var(var) {}
  const Var& var;
};

struct LabelDecl {
  explicit LabelDecl(const std::string& name) : name(name) {}
  std::string name;
};

struct GlobalVar {
  explicit GlobalVar(const Var& var) : var(var) {}
  const Var& var;
};

struct StackVar {
  explicit StackVar(Index index, Type type = Type::Any)
      : index(index), type(type) {}
  Index index;
  Type type;
};

struct TypeEnum {
  explicit TypeEnum(Type type) : type(type) {}
  Type type;
};

struct SignedType {
  explicit SignedType(Type type) : type(type) {}
  Type type;
};

struct ResultType {
  explicit ResultType(const TypeVector& types) : types(types) {}
  const TypeVector& types;
};

struct Newline {};
struct OpenBrace {};
struct CloseBrace {};

int GetShiftMask(Type type) {
  switch (type) {
    case Type::I32: return 31;
    case Type::I64: return 63;
    default: WABT_UNREACHABLE; return 0;
  }
}

class CWriter {
 public:
  CWriter(Stream* c_stream,
          Stream* h_stream,
          const char* header_name,
          const WriteCOptions* options)
      : options_(options),
        c_stream_(c_stream),
        h_stream_(h_stream),
        header_name_(header_name) {}

  Result WriteModule(const Module&);

 private:
  typedef std::set<std::string> SymbolSet;
  typedef std::map<std::string, std::string> SymbolMap;
  typedef std::pair<Index, Type> StackTypePair;
  typedef std::map<StackTypePair, std::string> StackVarSymbolMap;

  void UseStream(Stream*);

  void WriteCHeader();
  void WriteCSource();

  size_t MarkTypeStack() const;
  void ResetTypeStack(size_t mark);
  Type StackType(Index) const;
  void PushType(Type);
  void PushTypes(const TypeVector&);
  void DropTypes(size_t count);

  void PushLabel(LabelType,
                 const std::string& name,
                 const FuncSignature&,
                 bool used = false);
  const Label* FindLabel(const Var& var);
  bool IsTopLabelUsed() const;
  void PopLabel();

  static std::string AddressOf(const std::string&);
  static std::string Deref(const std::string&);

  static char MangleType(Type);
  static std::string MangleTypes(const TypeVector&);
  static std::string MangleName(string_view);
  static std::string MangleFuncName(string_view,
                                    const TypeVector& param_types,
                                    const TypeVector& result_types);
  static std::string MangleGlobalName(string_view, Type);
  static std::string LegalizeName(string_view);
  static std::string ExportName(string_view mangled_name);
  std::string DefineName(SymbolSet*, string_view);
  std::string DefineImportName(const std::string& name,
                               string_view module_name,
                               string_view mangled_field_name);
  std::string DefineGlobalScopeName(const std::string&);
  std::string DefineLocalScopeName(const std::string&);
  std::string DefineStackVarName(Index, Type, string_view);

  void Indent(int size = INDENT_SIZE);
  void Dedent(int size = INDENT_SIZE);
  void WriteIndent();
  void WriteData(const void* src, size_t size);
  void Writef(const char* format, ...);

  template <typename T, typename U, typename... Args>
  void Write(T&& t, U&& u, Args&&... args) {
    Write(std::forward<T>(t));
    Write(std::forward<U>(u));
    Write(std::forward<Args>(args)...);
  }

  std::string GetGlobalName(const std::string&) const;

  enum class WriteExportsKind {
    Declarations,
    Definitions,
    Initializers,
  };

  void Write() {}
  void Write(Newline);
  void Write(OpenBrace);
  void Write(CloseBrace);
  void Write(Index);
  void Write(string_view);
  void Write(const LocalName&);
  void Write(const GlobalName&);
  void Write(const ExternalPtr&);
  void Write(const ExternalRef&);
  void Write(Type);
  void Write(SignedType);
  void Write(TypeEnum);
  void Write(const Var&);
  void Write(const GotoLabel&);
  void Write(const LabelDecl&);
  void Write(const GlobalVar&);
  void Write(const StackVar&);
  void Write(const ResultType&);
  void Write(const Const&);
  void WriteInitExpr(const ExprList&);
  void InitGlobalSymbols();
  std::string GenerateHeaderGuard() const;
  void WriteSourceTop();
  void WriteFuncTypes();
  void WriteImports();
  void WriteFuncDeclarations();
  void WriteFuncDeclaration(const FuncDeclaration&, const std::string&);
  void WriteGlobals();
  void WriteGlobal(const Global&, const std::string&);
  void WriteMemories();
  void WriteMemory(const std::string&);
  void WriteTables();
  void WriteTable(const std::string&);
  void WriteDataInitializers();
  void WriteElemInitializers();
  void WriteInitExports();
  void WriteExports(WriteExportsKind);
  void WriteInit();
  void WriteFuncs();
  void Write(const Func&);
  void WriteParams();
  void WriteLocals();
  void WriteStackVarDeclarations();
  void Write(const ExprList&);

  enum class AssignOp {
    Disallowed,
    Allowed,
  };

  void WriteSimpleUnaryExpr(Opcode, const char* op);
  void WriteInfixBinaryExpr(Opcode,
                            const char* op,
                            AssignOp = AssignOp::Allowed);
  void WritePrefixBinaryExpr(Opcode, const char* op);
  void WriteSignedBinaryExpr(Opcode, const char* op);
  void Write(const BinaryExpr&);
  void Write(const CompareExpr&);
  void Write(const ConvertExpr&);
  void Write(const LoadExpr&);
  void Write(const StoreExpr&);
  void Write(const UnaryExpr&);
  void Write(const TernaryExpr&);
  void Write(const SimdLaneOpExpr&);
  void Write(const SimdShuffleOpExpr&);

  const WriteCOptions* options_ = nullptr;
  const Module* module_ = nullptr;
  const Func* func_ = nullptr;
  Stream* stream_ = nullptr;
  MemoryStream func_stream_;
  Stream* c_stream_ = nullptr;
  Stream* h_stream_ = nullptr;
  std::string header_name_;
  Result result_ = Result::Ok;
  int indent_ = 0;
  bool should_write_indent_next_ = false;

  SymbolMap global_sym_map_;
  SymbolMap local_sym_map_;
  StackVarSymbolMap stack_var_sym_map_;
  SymbolSet global_syms_;
  SymbolSet local_syms_;
  SymbolSet import_syms_;
  TypeVector type_stack_;
  std::vector<Label> label_stack_;
};

static const char kImplicitFuncLabel[] = "$Bfunc";

static const char* s_global_symbols[] = {
    // keywords
    "_Alignas", "_Alignof", "asm", "_Atomic", "auto", "_Bool", "break", "case",
    "char", "_Complex", "const", "continue", "default", "do", "double", "else",
    "enum", "extern", "float", "for", "_Generic", "goto", "if", "_Imaginary",
    "inline", "int", "long", "_Noreturn", "_Pragma", "register", "restrict",
    "return", "short", "signed", "sizeof", "static", "_Static_assert", "struct",
    "switch", "_Thread_local", "typedef", "union", "unsigned", "void",
    "volatile", "while",

    // assert.h
    "assert", "static_assert",

    // math.h
    "abs", "acos", "acosh", "asin", "asinh", "atan", "atan2", "atanh", "cbrt",
    "ceil", "copysign", "cos", "cosh", "double_t", "erf", "erfc", "exp", "exp2",
    "expm1", "fabs", "fdim", "float_t", "floor", "fma", "fmax", "fmin", "fmod",
    "fpclassify", "FP_FAST_FMA", "FP_FAST_FMAF", "FP_FAST_FMAL", "FP_ILOGB0",
    "FP_ILOGBNAN", "FP_INFINITE", "FP_NAN", "FP_NORMAL", "FP_SUBNORMAL",
    "FP_ZERO", "frexp", "HUGE_VAL", "HUGE_VALF", "HUGE_VALL", "hypot", "ilogb",
    "INFINITY", "isfinite", "isgreater", "isgreaterequal", "isinf", "isless",
    "islessequal", "islessgreater", "isnan", "isnormal", "isunordered", "ldexp",
    "lgamma", "llrint", "llround", "log", "log10", "log1p", "log2", "logb",
    "lrint", "lround", "MATH_ERREXCEPT", "math_errhandling", "MATH_ERRNO",
    "modf", "nan", "NAN", "nanf", "nanl", "nearbyint", "nextafter",
    "nexttoward", "pow", "remainder", "remquo", "rint", "round", "scalbln",
    "scalbn", "signbit", "sin", "sinh", "sqrt", "tan", "tanh", "tgamma",
    "trunc",

    // stdint.h
    "INT16_C", "INT16_MAX", "INT16_MIN", "int16_t", "INT32_MAX", "INT32_MIN",
    "int32_t", "INT64_C", "INT64_MAX", "INT64_MIN", "int64_t", "INT8_C",
    "INT8_MAX", "INT8_MIN", "int8_t", "INT_FAST16_MAX", "INT_FAST16_MIN",
    "int_fast16_t", "INT_FAST32_MAX", "INT_FAST32_MIN", "int_fast32_t",
    "INT_FAST64_MAX", "INT_FAST64_MIN", "int_fast64_t", "INT_FAST8_MAX",
    "INT_FAST8_MIN", "int_fast8_t", "INT_LEAST16_MAX", "INT_LEAST16_MIN",
    "int_least16_t", "INT_LEAST32_MAX", "INT_LEAST32_MIN", "int_least32_t",
    "INT_LEAST64_MAX", "INT_LEAST64_MIN", "int_least64_t", "INT_LEAST8_MAX",
    "INT_LEAST8_MIN", "int_least8_t", "INTMAX_C", "INTMAX_MAX", "INTMAX_MIN",
    "intmax_t", "INTPTR_MAX", "INTPTR_MIN", "intptr_t", "PTRDIFF_MAX",
    "PTRDIFF_MIN", "SIG_ATOMIC_MAX", "SIG_ATOMIC_MIN", "SIZE_MAX", "UINT16_C",
    "UINT16_MAX", "uint16_t", "UINT32_C", "UINT32_MAX", "uint32_t", "UINT64_C",
    "UINT64_MAX", "uint64_t", "UINT8_MAX", "uint8_t", "UINT_FAST16_MAX",
    "uint_fast16_t", "UINT_FAST32_MAX", "uint_fast32_t", "UINT_FAST64_MAX",
    "uint_fast64_t", "UINT_FAST8_MAX", "uint_fast8_t", "UINT_LEAST16_MAX",
    "uint_least16_t", "UINT_LEAST32_MAX", "uint_least32_t", "UINT_LEAST64_MAX",
    "uint_least64_t", "UINT_LEAST8_MAX", "uint_least8_t", "UINTMAX_C",
    "UINTMAX_MAX", "uintmax_t", "UINTPTR_MAX", "uintptr_t", "UNT8_C",
    "WCHAR_MAX", "WCHAR_MIN", "WINT_MAX", "WINT_MIN",

    // stdlib.h
    "abort", "abs", "atexit", "atof", "atoi", "atol", "atoll", "at_quick_exit",
    "bsearch", "calloc", "div", "div_t", "exit", "_Exit", "EXIT_FAILURE",
    "EXIT_SUCCESS", "free", "getenv", "labs", "ldiv", "ldiv_t", "llabs",
    "lldiv", "lldiv_t", "malloc", "MB_CUR_MAX", "mblen", "mbstowcs", "mbtowc",
    "qsort", "quick_exit", "rand", "RAND_MAX", "realloc", "size_t", "srand",
    "strtod", "strtof", "strtol", "strtold", "strtoll", "strtoul", "strtoull",
    "system", "wcstombs", "wctomb",

    // string.h
    "memchr", "memcmp", "memcpy", "memmove", "memset", "NULL", "size_t",
    "strcat", "strchr", "strcmp", "strcoll", "strcpy", "strcspn", "strerror",
    "strlen", "strncat", "strncmp", "strncpy", "strpbrk", "strrchr", "strspn",
    "strstr", "strtok", "strxfrm",

    // defined
    "CALL_INDIRECT", "DEFINE_LOAD", "DEFINE_REINTERPRET", "DEFINE_STORE",
    "DIVREM_U", "DIV_S", "DIV_U", "f32", "f32_load", "f32_reinterpret_i32",
    "f32_store", "f64", "f64_load", "f64_reinterpret_i64", "f64_store", "FMAX",
    "FMIN", "FUNC_EPILOGUE", "FUNC_PROLOGUE", "func_types", "I32_CLZ",
    "I32_CLZ", "I32_DIV_S", "i32_load", "i32_load16_s", "i32_load16_u",
    "i32_load8_s", "i32_load8_u", "I32_POPCNT", "i32_reinterpret_f32",
    "I32_REM_S", "I32_ROTL", "I32_ROTR", "i32_store", "i32_store16",
    "i32_store8", "I32_TRUNC_S_F32", "I32_TRUNC_S_F64", "I32_TRUNC_U_F32",
    "I32_TRUNC_U_F64", "I64_CTZ", "I64_CTZ", "I64_DIV_S", "i64_load",
    "i64_load16_s", "i64_load16_u", "i64_load32_s", "i64_load32_u",
    "i64_load8_s", "i64_load8_u", "I64_POPCNT", "i64_reinterpret_f64",
    "I64_REM_S", "I64_ROTL", "I64_ROTR", "i64_store", "i64_store16",
    "i64_store32", "i64_store8", "I64_TRUNC_S_F32", "I64_TRUNC_S_F64",
    "I64_TRUNC_U_F32", "I64_TRUNC_U_F64", "init", "init_elem_segment",
    "init_func_types", "init_globals", "init_memory", "init_table", "LIKELY",
    "MEMCHECK", "REM_S", "REM_U", "ROTL", "ROTR", "s16", "s32", "s64", "s8",
    "TRAP", "TRUNC_S", "TRUNC_U", "Type", "u16", "u32", "u64", "u8", "UNLIKELY",
    "UNREACHABLE", "WASM_RT_ADD_PREFIX", "wasm_rt_allocate_memory",
    "wasm_rt_allocate_table", "wasm_rt_anyfunc_t", "wasm_rt_call_stack_depth",
    "wasm_rt_elem_t", "WASM_RT_F32", "WASM_RT_F64", "wasm_rt_grow_memory",
    "WASM_RT_I32", "WASM_RT_I64", "WASM_RT_INCLUDED_",
    "WASM_RT_MAX_CALL_STACK_DEPTH", "wasm_rt_memory_t", "WASM_RT_MODULE_PREFIX",
    "WASM_RT_PASTE_", "WASM_RT_PASTE", "wasm_rt_register_func_type",
    "wasm_rt_table_t", "wasm_rt_trap", "WASM_RT_TRAP_CALL_INDIRECT",
    "WASM_RT_TRAP_DIV_BY_ZERO", "WASM_RT_TRAP_EXHAUSTION",
    "WASM_RT_TRAP_INT_OVERFLOW", "WASM_RT_TRAP_INVALID_CONVERSION",
    "WASM_RT_TRAP_NONE", "WASM_RT_TRAP_OOB", "wasm_rt_trap_t",
    "WASM_RT_TRAP_UNREACHABLE",

};

#define SECTION_NAME(x) s_header_##x
#include "src/prebuilt/wasm2c.include.h"
#undef SECTION_NAME

#define SECTION_NAME(x) s_source_##x
#include "src/prebuilt/wasm2c.include.c"
#undef SECTION_NAME

size_t CWriter::MarkTypeStack() const {
  return type_stack_.size();
}

void CWriter::ResetTypeStack(size_t mark) {
  assert(mark <= type_stack_.size());
  type_stack_.erase(type_stack_.begin() + mark, type_stack_.end());
}

Type CWriter::StackType(Index index) const {
  assert(index < type_stack_.size());
  return *(type_stack_.rbegin() + index);
}

void CWriter::PushType(Type type) {
  type_stack_.push_back(type);
}

void CWriter::PushTypes(const TypeVector& types) {
  type_stack_.insert(type_stack_.end(), types.begin(), types.end());
}

void CWriter::DropTypes(size_t count) {
  assert(count <= type_stack_.size());
  type_stack_.erase(type_stack_.end() - count, type_stack_.end());
}

void CWriter::PushLabel(LabelType label_type,
                        const std::string& name,
                        const FuncSignature& sig,
                        bool used) {
  // TODO(binji): Add multi-value support.
  if ((label_type != LabelType::Func && sig.GetNumParams() != 0) ||
      sig.GetNumResults() > 1) {
    UNIMPLEMENTED("multi value support");
  }

  label_stack_.emplace_back(label_type, name, sig.result_types,
                            type_stack_.size(), used);
}

const Label* CWriter::FindLabel(const Var& var) {
  Label* label = nullptr;

  if (var.is_index()) {
    // We've generated names for all labels, so we should only be using an
    // index when branching to the implicit function label, which can't be
    // named.
    assert(var.index() + 1 == label_stack_.size());
    label = &label_stack_[0];
  } else {
    assert(var.is_name());
    for (Index i = label_stack_.size(); i > 0; --i) {
      label = &label_stack_[i - 1];
      if (label->name == var.name())
        break;
    }
  }

  assert(label);
  label->used = true;
  return label;
}

bool CWriter::IsTopLabelUsed() const {
  assert(!label_stack_.empty());
  return label_stack_.back().used;
}

void CWriter::PopLabel() {
  label_stack_.pop_back();
}

// static
std::string CWriter::AddressOf(const std::string& s) {
  return "(&" + s + ")";
}

// static
std::string CWriter::Deref(const std::string& s) {
  return "(*" + s + ")";
}

// static
char CWriter::MangleType(Type type) {
  switch (type) {
    case Type::I32: return 'i';
    case Type::I64: return 'j';
    case Type::F32: return 'f';
    case Type::F64: return 'd';
    default: WABT_UNREACHABLE;
  }
}

// static
std::string CWriter::MangleTypes(const TypeVector& types) {
  if (types.empty())
    return std::string("v");

  std::string result;
  for (auto type : types) {
    result += MangleType(type);
  }
  return result;
}

// static
std::string CWriter::MangleName(string_view name) {
  const char kPrefix = 'Z';
  std::string result = "Z_";

  if (!name.empty()) {
    for (char c : name) {
      if ((isalnum(c) && c != kPrefix) || c == '_') {
        result += c;
      } else {
        result += kPrefix;
        result += StringPrintf("%02X", static_cast<uint8_t>(c));
      }
    }
  }

  return result;
}

// static
std::string CWriter::MangleFuncName(string_view name,
                                    const TypeVector& param_types,
                                    const TypeVector& result_types) {
  std::string sig = MangleTypes(result_types) + MangleTypes(param_types);
  return MangleName(name) + MangleName(sig);
}

// static
std::string CWriter::MangleGlobalName(string_view name, Type type) {
  std::string sig(1, MangleType(type));
  return MangleName(name) + MangleName(sig);
}

// static
std::string CWriter::ExportName(string_view mangled_name) {
  return "WASM_RT_ADD_PREFIX(" + mangled_name.to_string() + ")";
}

// static
std::string CWriter::LegalizeName(string_view name) {
  if (name.empty())
    return "_";

  std::string result;
  result = isalpha(name[0]) ? name[0] : '_';
  for (size_t i = 1; i < name.size(); ++i)
    result += isalnum(name[i]) ? name[i] : '_';

  return result;
}

std::string CWriter::DefineName(SymbolSet* set, string_view name) {
  std::string legal = LegalizeName(name);
  if (set->find(legal) != set->end()) {
    std::string base = legal + "_";
    size_t count = 0;
    do {
      legal = base + std::to_string(count++);
    } while (set->find(legal) != set->end());
  }
  set->insert(legal);
  return legal;
}

string_view StripLeadingDollar(string_view name) {
  if (!name.empty() && name[0] == '$') {
    name.remove_prefix(1);
  }
  return name;
}

std::string CWriter::DefineImportName(const std::string& name,
                                      string_view module,
                                      string_view mangled_field_name) {
  std::string mangled = MangleName(module) + mangled_field_name.to_string();
  import_syms_.insert(name);
  global_syms_.insert(mangled);
  global_sym_map_.insert(SymbolMap::value_type(name, mangled));
  return "(*" + mangled + ")";
}

std::string CWriter::DefineGlobalScopeName(const std::string& name) {
  std::string unique = DefineName(&global_syms_, StripLeadingDollar(name));
  global_sym_map_.insert(SymbolMap::value_type(name, unique));
  return unique;
}

std::string CWriter::DefineLocalScopeName(const std::string& name) {
  std::string unique = DefineName(&local_syms_, StripLeadingDollar(name));
  local_sym_map_.insert(SymbolMap::value_type(name, unique));
  return unique;
}

std::string CWriter::DefineStackVarName(Index index,
                                        Type type,
                                        string_view name) {
  std::string unique = DefineName(&local_syms_, name);
  StackTypePair stp = {index, type};
  stack_var_sym_map_.insert(StackVarSymbolMap::value_type(stp, unique));
  return unique;
}

void CWriter::Indent(int size) {
  indent_ += size;
}

void CWriter::Dedent(int size) {
  indent_ -= size;
  assert(indent_ >= 0);
}

void CWriter::WriteIndent() {
  static char s_indent[] =
      "                                                                       "
      "                                                                       ";
  static size_t s_indent_len = sizeof(s_indent) - 1;
  size_t to_write = indent_;
  while (to_write >= s_indent_len) {
    stream_->WriteData(s_indent, s_indent_len);
    to_write -= s_indent_len;
  }
  if (to_write > 0) {
    stream_->WriteData(s_indent, to_write);
  }
}

void CWriter::WriteData(const void* src, size_t size) {
  if (should_write_indent_next_) {
    WriteIndent();
    should_write_indent_next_ = false;
  }
  stream_->WriteData(src, size);
}

void WABT_PRINTF_FORMAT(2, 3) CWriter::Writef(const char* format, ...) {
  WABT_SNPRINTF_ALLOCA(buffer, length, format);
  WriteData(buffer, length);
}

void CWriter::Write(Newline) {
  Write("\n");
  should_write_indent_next_ = true;
}

void CWriter::Write(OpenBrace) {
  Write("{");
  Indent();
  Write(Newline());
}

void CWriter::Write(CloseBrace) {
  Dedent();
  Write("}");
}

void CWriter::Write(Index index) {
  Writef("%" PRIindex, index);
}

void CWriter::Write(string_view s) {
  WriteData(s.data(), s.size());
}

void CWriter::Write(const LocalName& name) {
  assert(local_sym_map_.count(name.name) == 1);
  Write(local_sym_map_[name.name]);
}

std::string CWriter::GetGlobalName(const std::string& name) const {
  assert(global_sym_map_.count(name) == 1);
  auto iter = global_sym_map_.find(name);
  assert(iter != global_sym_map_.end());
  return iter->second;
}

void CWriter::Write(const GlobalName& name) {
  Write(GetGlobalName(name.name));
}

void CWriter::Write(const ExternalPtr& name) {
  bool is_import = import_syms_.count(name.name) != 0;
  if (is_import) {
    Write(GetGlobalName(name.name));
  } else {
    Write(AddressOf(GetGlobalName(name.name)));
  }
}

void CWriter::Write(const ExternalRef& name) {
  bool is_import = import_syms_.count(name.name) != 0;
  if (is_import) {
    Write(Deref(GetGlobalName(name.name)));
  } else {
    Write(GetGlobalName(name.name));
  }
}

void CWriter::Write(const Var& var) {
  assert(var.is_name());
  Write(LocalName(var.name()));
}

void CWriter::Write(const GotoLabel& goto_label) {
  const Label* label = FindLabel(goto_label.var);
  if (label->HasValue()) {
    assert(label->sig.size() == 1);
    assert(type_stack_.size() >= label->type_stack_size);
    Index dst = type_stack_.size() - label->type_stack_size - 1;
    if (dst != 0)
      Write(StackVar(dst, label->sig[0]), " = ", StackVar(0), "; ");
  }

  if (goto_label.var.is_name()) {
    Write("goto ", goto_label.var, ";");
  } else {
    // We've generated names for all labels, so we should only be using an
    // index when branching to the implicit function label, which can't be
    // named.
    Write("goto ", Var(kImplicitFuncLabel), ";");
  }
}

void CWriter::Write(const LabelDecl& label) {
  if (IsTopLabelUsed())
    Write(label.name, ":;", Newline());
}

void CWriter::Write(const GlobalVar& var) {
  assert(var.var.is_name());
  Write(ExternalRef(var.var.name()));
}

void CWriter::Write(const StackVar& sv) {
  Index index = type_stack_.size() - 1 - sv.index;
  Type type = sv.type;
  if (type == Type::Any) {
    assert(index < type_stack_.size());
    type = type_stack_[index];
  }

  StackTypePair stp = {index, type};
  auto iter = stack_var_sym_map_.find(stp);
  if (iter == stack_var_sym_map_.end()) {
    std::string name = MangleType(type) + std::to_string(index);
    Write(DefineStackVarName(index, type, name));
  } else {
    Write(iter->second);
  }
}

void CWriter::Write(Type type) {
  switch (type) {
    case Type::I32: Write("u32"); break;
    case Type::I64: Write("u64"); break;
    case Type::F32: Write("f32"); break;
    case Type::F64: Write("f64"); break;
    default:
      WABT_UNREACHABLE;
  }
}

void CWriter::Write(TypeEnum type) {
  switch (type.type) {
    case Type::I32: Write("WASM_RT_I32"); break;
    case Type::I64: Write("WASM_RT_I64"); break;
    case Type::F32: Write("WASM_RT_F32"); break;
    case Type::F64: Write("WASM_RT_F64"); break;
    default:
      WABT_UNREACHABLE;
  }
}

void CWriter::Write(SignedType type) {
  switch (type.type) {
    case Type::I32: Write("s32"); break;
    case Type::I64: Write("s64"); break;
    default:
      WABT_UNREACHABLE;
  }
}

void CWriter::Write(const ResultType& rt) {
  if (!rt.types.empty()) {
    Write(rt.types[0]);
  } else {
    Write("void");
  }
}

void CWriter::Write(const Const& const_) {
  switch (const_.type) {
    case Type::I32:
      Writef("%uu", static_cast<int32_t>(const_.u32));
      break;

    case Type::I64:
      Writef("%" PRIu64 "ull", static_cast<int64_t>(const_.u64));
      break;

    case Type::F32: {
      // TODO(binji): Share with similar float info in interp.cc and literal.cc
      if ((const_.f32_bits & 0x7f800000u) == 0x7f800000u) {
        const char* sign = (const_.f32_bits & 0x80000000) ? "-" : "";
        uint32_t significand = const_.f32_bits & 0x7fffffu;
        if (significand == 0) {
          // Infinity.
          Writef("%sINFINITY", sign);
        } else {
          // Nan.
          Writef("f32_reinterpret_i32(0x%08x) /* %snan:0x%06x */",
                 const_.f32_bits, sign, significand);
        }
      } else if (const_.f32_bits == 0x80000000) {
        // Negative zero. Special-cased so it isn't written as -0 below.
        Writef("-0.f");
      } else {
        Writef("%.9g", Bitcast<float>(const_.f32_bits));
      }
      break;
    }

    case Type::F64:
      // TODO(binji): Share with similar float info in interp.cc and literal.cc
      if ((const_.f64_bits & 0x7ff0000000000000ull) == 0x7ff0000000000000ull) {
        const char* sign = (const_.f64_bits & 0x8000000000000000ull) ? "-" : "";
        uint64_t significand = const_.f64_bits & 0xfffffffffffffull;
        if (significand == 0) {
          // Infinity.
          Writef("%sINFINITY", sign);
        } else {
          // Nan.
          Writef("f64_reinterpret_i64(0x%016" PRIx64 ") /* %snan:0x%013" PRIx64
                 " */",
                 const_.f64_bits, sign, significand);
        }
      } else if (const_.f64_bits == 0x8000000000000000ull) {
        // Negative zero. Special-cased so it isn't written as -0 below.
        Writef("-0.0");
      } else {
        Writef("%.17g", Bitcast<double>(const_.f64_bits));
      }
      break;

    default:
      WABT_UNREACHABLE;
  }
}

void CWriter::WriteInitExpr(const ExprList& expr_list) {
  if (expr_list.empty())
    return;

  assert(expr_list.size() == 1);
  const Expr* expr = &expr_list.front();
  switch (expr_list.front().type()) {
    case ExprType::Const:
      Write(cast<ConstExpr>(expr)->const_);
      break;

    case ExprType::GetGlobal:
      Write(GlobalVar(cast<GetGlobalExpr>(expr)->var));
      break;

    default:
      WABT_UNREACHABLE;
  }
}

void CWriter::InitGlobalSymbols() {
  for (const char* symbol : s_global_symbols) {
    global_syms_.insert(symbol);
  }
}

std::string CWriter::GenerateHeaderGuard() const {
  std::string result;
  for (char c : header_name_) {
    if (isalnum(c) || c == '_') {
      result += toupper(c);
    } else {
      result += '_';
    }
  }
  result += "_GENERATED_";
  return result;
}

void CWriter::WriteSourceTop() {
  Write(s_source_includes);
  Write(Newline(), "#include \"", header_name_, "\"", Newline());
  Write(s_source_declarations);
}

void CWriter::WriteFuncTypes() {
  Write(Newline());
  Writef("static u32 func_types[%" PRIzd "];", module_->func_types.size());
  Write(Newline(), Newline());
  Write("static void init_func_types(void) {", Newline());
  Index func_type_index = 0;
  for (FuncType* func_type : module_->func_types) {
    Index num_params = func_type->GetNumParams();
    Index num_results = func_type->GetNumResults();
    Write("  func_types[", func_type_index, "] = wasm_rt_register_func_type(",
          num_params, ", ", num_results);
    for (Index i = 0; i < num_params; ++i) {
      Write(", ", TypeEnum(func_type->GetParamType(i)));
    }

    for (Index i = 0; i < num_results; ++i) {
      Write(", ", TypeEnum(func_type->GetResultType(i)));
    }

    Write(");", Newline());
    ++func_type_index;
  }
  Write("}", Newline());
}

void CWriter::WriteImports() {
  if (module_->imports.empty())
    return;

  Write(Newline());

  // TODO(binji): Write imports ordered by type.
  for (const Import* import : module_->imports) {
    Write("/* import: '", import->module_name, "' '", import->field_name,
          "' */", Newline());
    Write("extern ");
    switch (import->kind()) {
      case ExternalKind::Func: {
        const Func& func = cast<FuncImport>(import)->func;
        WriteFuncDeclaration(
            func.decl,
            DefineImportName(
                func.name, import->module_name,
                MangleFuncName(import->field_name, func.decl.sig.param_types,
                               func.decl.sig.result_types)));
        Write(";");
        break;
      }

      case ExternalKind::Global: {
        const Global& global = cast<GlobalImport>(import)->global;
        WriteGlobal(global,
                    DefineImportName(
                        global.name, import->module_name,
                        MangleGlobalName(import->field_name, global.type)));
        Write(";");
        break;
      }

      case ExternalKind::Memory: {
        const Memory& memory = cast<MemoryImport>(import)->memory;
        WriteMemory(DefineImportName(memory.name, import->module_name,
                                     MangleName(import->field_name)));
        break;
      }

      case ExternalKind::Table: {
        const Table& table = cast<TableImport>(import)->table;
        WriteTable(DefineImportName(table.name, import->module_name,
                                    MangleName(import->field_name)));
        break;
      }

      default:
        WABT_UNREACHABLE;
    }

    Write(Newline());
  }
}

void CWriter::WriteFuncDeclarations() {
  if (module_->funcs.size() == module_->num_func_imports)
    return;

  Write(Newline());

  Index func_index = 0;
  for (const Func* func : module_->funcs) {
    bool is_import = func_index < module_->num_func_imports;
    if (!is_import) {
      Write("static ");
      WriteFuncDeclaration(func->decl, DefineGlobalScopeName(func->name));
      Write(";", Newline());
    }
    ++func_index;
  }
}

void CWriter::WriteFuncDeclaration(const FuncDeclaration& decl,
                                   const std::string& name) {
  Write(ResultType(decl.sig.result_types), " ", name, "(");
  if (decl.GetNumParams() == 0) {
    Write("void");
  } else {
    for (Index i = 0; i < decl.GetNumParams(); ++i) {
      if (i != 0)
        Write(", ");
      Write(decl.GetParamType(i));
    }
  }
  Write(")");
}

void CWriter::WriteGlobals() {
  Index global_index = 0;
  if (module_->globals.size() != module_->num_global_imports) {
    Write(Newline());

    for (const Global* global : module_->globals) {
      bool is_import = global_index < module_->num_global_imports;
      if (!is_import) {
        Write("static ");
        WriteGlobal(*global, DefineGlobalScopeName(global->name));
        Write(";", Newline());
      }
      ++global_index;
    }
  }

  Write(Newline(), "static void init_globals(void) ", OpenBrace());
  global_index = 0;
  for (const Global* global : module_->globals) {
    bool is_import = global_index < module_->num_global_imports;
    if (!is_import) {
      assert(!global->init_expr.empty());
      Write(GlobalName(global->name), " = ");
      WriteInitExpr(global->init_expr);
      Write(";", Newline());
    }
    ++global_index;
  }
  Write(CloseBrace(), Newline());
}

void CWriter::WriteGlobal(const Global& global, const std::string& name) {
  Write(global.type, " ", name);
}

void CWriter::WriteMemories() {
  if (module_->memories.size() == module_->num_memory_imports)
    return;

  Write(Newline());

  assert(module_->memories.size() <= 1);
  Index memory_index = 0;
  for (const Memory* memory : module_->memories) {
    bool is_import = memory_index < module_->num_memory_imports;
    if (!is_import) {
      Write("static ");
      WriteMemory(DefineGlobalScopeName(memory->name));
      Write(Newline());
    }
    ++memory_index;
  }
}

void CWriter::WriteMemory(const std::string& name) {
  Write("wasm_rt_memory_t ", name, ";");
}

void CWriter::WriteTables() {
  if (module_->tables.size() == module_->num_table_imports)
    return;

  Write(Newline());

  assert(module_->tables.size() <= 1);
  Index table_index = 0;
  for (const Table* table : module_->tables) {
    bool is_import = table_index < module_->num_table_imports;
    if (!is_import) {
      Write("static ");
      WriteTable(DefineGlobalScopeName(table->name));
      Write(Newline());
    }
    ++table_index;
  }
}

void CWriter::WriteTable(const std::string& name) {
  Write("wasm_rt_table_t ", name, ";");
}

void CWriter::WriteDataInitializers() {
  const Memory* memory = nullptr;
  Index data_segment_index = 0;

  if (!module_->memories.empty()) {
    if (module_->data_segments.empty()) {
      Write(Newline());
    } else {
      for (const DataSegment* data_segment : module_->data_segments) {
        Write(Newline(), "static const u8 data_segment_data_",
              data_segment_index, "[] = ", OpenBrace());
        size_t i = 0;
        for (uint8_t x : data_segment->data) {
          Writef("0x%02x, ", x);
          if ((++i % 12) == 0)
            Write(Newline());
        }
        if (i > 0)
          Write(Newline());
        Write(CloseBrace(), ";", Newline());
        ++data_segment_index;
      }
    }

    memory = module_->memories[0];
  }

  Write(Newline(), "static void init_memory(void) ", OpenBrace());
  if (memory && module_->num_memory_imports == 0) {
    uint32_t max =
        memory->page_limits.has_max ? memory->page_limits.max : 65536;
    Write("wasm_rt_allocate_memory(", ExternalPtr(memory->name), ", ",
          memory->page_limits.initial, ", ", max, ");", Newline());
  }
  data_segment_index = 0;
  for (const DataSegment* data_segment : module_->data_segments) {
    Write("memcpy(&(", ExternalRef(memory->name), ".data[");
    WriteInitExpr(data_segment->offset);
    Write("]), data_segment_data_", data_segment_index, ", ",
          data_segment->data.size(), ");", Newline());
    ++data_segment_index;
  }

  Write(CloseBrace(), Newline());
}

void CWriter::WriteElemInitializers() {
  const Table* table = module_->tables.empty() ? nullptr : module_->tables[0];

  Write(Newline(), "static void init_table(void) ", OpenBrace());
  Write("uint32_t offset;", Newline());
  if (table && module_->num_table_imports == 0) {
    uint32_t max =
        table->elem_limits.has_max ? table->elem_limits.max : UINT32_MAX;
    Write("wasm_rt_allocate_table(", ExternalPtr(table->name), ", ",
          table->elem_limits.initial, ", ", max, ");", Newline());
  }
  Index elem_segment_index = 0;
  for (const ElemSegment* elem_segment : module_->elem_segments) {
    Write("offset = ");
    WriteInitExpr(elem_segment->offset);
    Write(";", Newline());

    size_t i = 0;
    for (const Var& var : elem_segment->vars) {
      const Func* func = module_->GetFunc(var);
      Index func_type_index = module_->GetFuncTypeIndex(func->decl.type_var);

      Write(ExternalRef(table->name), ".data[offset + ", i,
            "] = (wasm_rt_elem_t){func_types[", func_type_index,
            "], (wasm_rt_anyfunc_t)", ExternalPtr(func->name), "};", Newline());
      ++i;
    }
    ++elem_segment_index;
  }

  Write(CloseBrace(), Newline());
}

void CWriter::WriteInitExports() {
  Write(Newline(), "static void init_exports(void) ", OpenBrace());
  WriteExports(WriteExportsKind::Initializers);
  Write(CloseBrace(), Newline());
}

void CWriter::WriteExports(WriteExportsKind kind) {
  if (module_->exports.empty())
    return;

  if (kind != WriteExportsKind::Initializers) {
    Write(Newline());
  }

  for (const Export* export_ : module_->exports) {
    Write("/* export: '", export_->name, "' */", Newline());
    if (kind == WriteExportsKind::Declarations) {
      Write("extern ");
    }

    std::string mangled_name;
    std::string internal_name;

    switch (export_->kind) {
      case ExternalKind::Func: {
        const Func* func = module_->GetFunc(export_->var);
        mangled_name =
            ExportName(MangleFuncName(export_->name, func->decl.sig.param_types,
                                      func->decl.sig.result_types));
        internal_name = func->name;
        if (kind != WriteExportsKind::Initializers) {
          WriteFuncDeclaration(func->decl, Deref(mangled_name));
          Write(";");
        }
        break;
      }

      case ExternalKind::Global: {
        const Global* global = module_->GetGlobal(export_->var);
        mangled_name =
            ExportName(MangleGlobalName(export_->name, global->type));
        internal_name = global->name;
        if (kind != WriteExportsKind::Initializers) {
          WriteGlobal(*global, Deref(mangled_name));
          Write(";");
        }
        break;
      }

      case ExternalKind::Memory: {
        const Memory* memory = module_->GetMemory(export_->var);
        mangled_name = ExportName(MangleName(export_->name));
        internal_name = memory->name;
        if (kind != WriteExportsKind::Initializers) {
          WriteMemory(Deref(mangled_name));
        }
        break;
      }

      case ExternalKind::Table: {
        const Table* table = module_->GetTable(export_->var);
        mangled_name = ExportName(MangleName(export_->name));
        internal_name = table->name;
        if (kind != WriteExportsKind::Initializers) {
          WriteTable(Deref(mangled_name));
        }
        break;
      }

      default:
        WABT_UNREACHABLE;
    }

    if (kind == WriteExportsKind::Initializers) {
      Write(mangled_name, " = ", ExternalPtr(internal_name), ";");
    }

    Write(Newline());
  }
}

void CWriter::WriteInit() {
  Write(Newline(), "void WASM_RT_ADD_PREFIX(init)(void) ", OpenBrace());
  Write("init_func_types();", Newline());
  Write("init_globals();", Newline());
  Write("init_memory();", Newline());
  Write("init_table();", Newline());
  Write("init_exports();", Newline());
  for (Var* var : module_->starts) {
    Write(ExternalRef(module_->GetFunc(*var)->name), "();", Newline());
  }
  Write(CloseBrace(), Newline());
}

void CWriter::WriteFuncs() {
  Index func_index = 0;
  for (const Func* func : module_->funcs) {
    bool is_import = func_index < module_->num_func_imports;
    if (!is_import)
      Write(Newline(), *func, Newline());
    ++func_index;
  }
}

void CWriter::Write(const Func& func) {
  func_ = &func;
  // Copy symbols from global symbol table so we don't shadow them.
  local_syms_ = global_syms_;
  local_sym_map_.clear();
  stack_var_sym_map_.clear();

  Write("static ", ResultType(func.decl.sig.result_types), " ",
        GlobalName(func.name), "(");
  WriteParams();
  WriteLocals();
  Write("FUNC_PROLOGUE;", Newline());

  stream_ = &func_stream_;
  stream_->ClearOffset();

  std::string label = DefineLocalScopeName(kImplicitFuncLabel);
  ResetTypeStack(0);
  std::string empty;  // Must not be temporary, since address is taken by Label.
  PushLabel(LabelType::Func, empty, func.decl.sig);
  Write(func.exprs, LabelDecl(label));
  PopLabel();
  ResetTypeStack(0);
  PushTypes(func.decl.sig.result_types);
  Write("FUNC_EPILOGUE;", Newline());

  if (!func.decl.sig.result_types.empty()) {
    // Return the top of the stack implicitly.
    Write("return ", StackVar(0), ";", Newline());
  }

  stream_ = c_stream_;

  WriteStackVarDeclarations();

  std::unique_ptr<OutputBuffer> buf = func_stream_.ReleaseOutputBuffer();
  stream_->WriteData(buf->data.data(), buf->data.size());

  Write(CloseBrace());

  func_stream_.Clear();
  func_ = nullptr;
}

void CWriter::WriteParams() {
  if (func_->decl.sig.param_types.empty()) {
    Write("void");
  } else {
    std::vector<std::string> index_to_name;
    MakeTypeBindingReverseMapping(func_->decl.sig.param_types.size(),
                                  func_->param_bindings, &index_to_name);
    Indent(4);
    for (Index i = 0; i < func_->GetNumParams(); ++i) {
      if (i != 0) {
        Write(", ");
        if ((i % 8) == 0)
          Write(Newline());
      }
      Write(func_->GetParamType(i), " ",
            DefineLocalScopeName(index_to_name[i]));
    }
    Dedent(4);
  }
  Write(") ", OpenBrace());
}

void CWriter::WriteLocals() {
  std::vector<std::string> index_to_name;
  MakeTypeBindingReverseMapping(func_->local_types.size(),
                                func_->local_bindings, &index_to_name);
  for (Type type : {Type::I32, Type::I64, Type::F32, Type::F64}) {
    Index local_index = 0;
    size_t count = 0;
    for (Type local_type : func_->local_types) {
      if (local_type == type) {
        if (count == 0) {
          Write(type, " ");
          Indent(4);
        } else {
          Write(", ");
          if ((count % 8) == 0)
            Write(Newline());
        }

        Write(DefineLocalScopeName(index_to_name[local_index]), " = 0");
        ++count;
      }
      ++local_index;
    }
    if (count != 0) {
      Dedent(4);
      Write(";", Newline());
    }
  }
}

void CWriter::WriteStackVarDeclarations() {
  for (Type type : {Type::I32, Type::I64, Type::F32, Type::F64}) {
    size_t count = 0;
    for (const auto& pair : stack_var_sym_map_) {
      Type stp_type = pair.first.second;
      const std::string& name = pair.second;

      if (stp_type == type) {
        if (count == 0) {
          Write(type, " ");
          Indent(4);
        } else {
          Write(", ");
          if ((count % 8) == 0)
            Write(Newline());
        }

        Write(name);
        ++count;
      }
    }
    if (count != 0) {
      Dedent(4);
      Write(";", Newline());
    }
  }
}

void CWriter::Write(const ExprList& exprs) {
  for (const Expr& expr : exprs) {
    switch (expr.type()) {
      case ExprType::Binary:
        Write(*cast<BinaryExpr>(&expr));
        break;

      case ExprType::Block: {
        const Block& block = cast<BlockExpr>(&expr)->block;
        std::string label = DefineLocalScopeName(block.label);
        size_t mark = MarkTypeStack();
        PushLabel(LabelType::Block, block.label, block.decl.sig);
        Write(block.exprs, LabelDecl(label));
        ResetTypeStack(mark);
        PopLabel();
        PushTypes(block.decl.sig.result_types);
        break;
      }

      case ExprType::Br:
        Write(GotoLabel(cast<BrExpr>(&expr)->var), Newline());
        // Stop processing this ExprList, since the following are unreachable.
        return;

      case ExprType::BrIf:
        Write("if (", StackVar(0), ") {");
        DropTypes(1);
        Write(GotoLabel(cast<BrIfExpr>(&expr)->var), "}", Newline());
        break;

      case ExprType::BrTable: {
        const auto* bt_expr = cast<BrTableExpr>(&expr);
        Write("switch (", StackVar(0), ") ", OpenBrace());
        DropTypes(1);
        Index i = 0;
        for (const Var& var : bt_expr->targets) {
          Write("case ", i++, ": ", GotoLabel(var), Newline());
        }
        Write("default: ");
        Write(GotoLabel(bt_expr->default_target), Newline(), CloseBrace(),
              Newline());
        // Stop processing this ExprList, since the following are unreachable.
        return;
      }

      case ExprType::Call: {
        const Var& var = cast<CallExpr>(&expr)->var;
        const Func& func = *module_->GetFunc(var);
        Index num_params = func.GetNumParams();
        Index num_results = func.GetNumResults();
        assert(type_stack_.size() >= num_params);
        if (num_results > 0) {
          assert(num_results == 1);
          Write(StackVar(num_params - 1, func.GetResultType(0)), " = ");
        }

        Write(GlobalVar(var), "(");
        for (Index i = 0; i < num_params; ++i) {
          if (i != 0) {
            Write(", ");
          }
          Write(StackVar(num_params - i - 1));
        }
        Write(");", Newline());
        DropTypes(num_params);
        PushTypes(func.decl.sig.result_types);
        break;
      }

      case ExprType::CallIndirect: {
        const FuncDeclaration& decl = cast<CallIndirectExpr>(&expr)->decl;
        Index num_params = decl.GetNumParams();
        Index num_results = decl.GetNumResults();
        assert(type_stack_.size() > num_params);
        if (num_results > 0) {
          assert(num_results == 1);
          Write(StackVar(num_params, decl.GetResultType(0)), " = ");
        }

        assert(module_->tables.size() == 1);
        const Table* table = module_->tables[0];

        assert(decl.has_func_type);
        Index func_type_index = module_->GetFuncTypeIndex(decl.type_var);

        Write("CALL_INDIRECT(", ExternalRef(table->name), ", ");
        WriteFuncDeclaration(decl, "(*)");
        Write(", ", func_type_index, ", ", StackVar(0));
        for (Index i = 0; i < num_params; ++i) {
          Write(", ", StackVar(num_params - i));
        }
        Write(");", Newline());
        DropTypes(num_params + 1);
        PushTypes(decl.sig.result_types);
        break;
      }

      case ExprType::Compare:
        Write(*cast<CompareExpr>(&expr));
        break;

      case ExprType::Const: {
        const Const& const_ = cast<ConstExpr>(&expr)->const_;
        PushType(const_.type);
        Write(StackVar(0), " = ", const_, ";", Newline());
        break;
      }

      case ExprType::Convert:
        Write(*cast<ConvertExpr>(&expr));
        break;

      case ExprType::Drop:
        DropTypes(1);
        break;

      case ExprType::GetGlobal: {
        const Var& var = cast<GetGlobalExpr>(&expr)->var;
        PushType(module_->GetGlobal(var)->type);
        Write(StackVar(0), " = ", GlobalVar(var), ";", Newline());
        break;
      }

      case ExprType::GetLocal: {
        const Var& var = cast<GetLocalExpr>(&expr)->var;
        PushType(func_->GetLocalType(var));
        Write(StackVar(0), " = ", var, ";", Newline());
        break;
      }

      case ExprType::If: {
        const IfExpr& if_ = *cast<IfExpr>(&expr);
        Write("if (", StackVar(0), ") ", OpenBrace());
        DropTypes(1);
        std::string label = DefineLocalScopeName(if_.true_.label);
        size_t mark = MarkTypeStack();
        PushLabel(LabelType::If, if_.true_.label, if_.true_.decl.sig);
        Write(if_.true_.exprs, CloseBrace());
        if (!if_.false_.empty()) {
          ResetTypeStack(mark);
          Write(" else ", OpenBrace(), if_.false_, CloseBrace());
        }
        ResetTypeStack(mark);
        Write(Newline(), LabelDecl(label));
        PopLabel();
        PushTypes(if_.true_.decl.sig.result_types);
        break;
      }

      case ExprType::Load:
        Write(*cast<LoadExpr>(&expr));
        break;

      case ExprType::Loop: {
        const Block& block = cast<LoopExpr>(&expr)->block;
        if (!block.exprs.empty()) {
          Write(DefineLocalScopeName(block.label), ": ");
          Indent();
          size_t mark = MarkTypeStack();
          PushLabel(LabelType::Loop, block.label, block.decl.sig);
          Write(Newline(), block.exprs);
          ResetTypeStack(mark);
          PopLabel();
          PushTypes(block.decl.sig.result_types);
          Dedent();
        }
        break;
      }

      case ExprType::MemoryGrow: {
        assert(module_->memories.size() == 1);
        Memory* memory = module_->memories[0];

        Write(StackVar(0), " = wasm_rt_grow_memory(", ExternalPtr(memory->name),
              ", ", StackVar(0), ");", Newline());
        break;
      }

      case ExprType::MemorySize: {
        assert(module_->memories.size() == 1);
        Memory* memory = module_->memories[0];

        PushType(Type::I32);
        Write(StackVar(0), " = ", ExternalRef(memory->name), ".pages;",
              Newline());
        break;
      }

      case ExprType::Nop:
        break;

      case ExprType::Return:
        // Goto the function label instead; this way we can do shared function
        // cleanup code in one place.
        Write(GotoLabel(Var(label_stack_.size() - 1)), Newline());
        // Stop processing this ExprList, since the following are unreachable.
        return;

      case ExprType::Select: {
        Type type = StackType(1);
        Write(StackVar(2), " = ", StackVar(0), " ? ", StackVar(2), " : ",
              StackVar(1), ";", Newline());
        DropTypes(3);
        PushType(type);
        break;
      }

      case ExprType::SetGlobal: {
        const Var& var = cast<SetGlobalExpr>(&expr)->var;
        Write(GlobalVar(var), " = ", StackVar(0), ";", Newline());
        DropTypes(1);
        break;
      }

      case ExprType::SetLocal: {
        const Var& var = cast<SetLocalExpr>(&expr)->var;
        Write(var, " = ", StackVar(0), ";", Newline());
        DropTypes(1);
        break;
      }

      case ExprType::Store:
        Write(*cast<StoreExpr>(&expr));
        break;

      case ExprType::TeeLocal: {
        const Var& var = cast<TeeLocalExpr>(&expr)->var;
        Write(var, " = ", StackVar(0), ";", Newline());
        break;
      }

      case ExprType::Unary:
        Write(*cast<UnaryExpr>(&expr));
        break;

      case ExprType::Ternary:
        Write(*cast<TernaryExpr>(&expr));
        break;

      case ExprType::SimdLaneOp: {
        Write(*cast<SimdLaneOpExpr>(&expr));
        break;
      }

      case ExprType::SimdShuffleOp: {
        Write(*cast<SimdShuffleOpExpr>(&expr));
        break;
      }

      case ExprType::Unreachable:
        Write("UNREACHABLE;", Newline());
        return;

      case ExprType::AtomicWait:
      case ExprType::AtomicWake:
      case ExprType::AtomicLoad:
      case ExprType::AtomicRmw:
      case ExprType::AtomicRmwCmpxchg:
      case ExprType::AtomicStore:
      case ExprType::IfExcept:
      case ExprType::Rethrow:
      case ExprType::Throw:
      case ExprType::Try:
        UNIMPLEMENTED("...");
        break;
    }
  }
}

void CWriter::WriteSimpleUnaryExpr(Opcode opcode, const char* op) {
  Type result_type = opcode.GetResultType();
  Write(StackVar(0, result_type), " = ", op, "(", StackVar(0), ");", Newline());
  DropTypes(1);
  PushType(opcode.GetResultType());
}

void CWriter::WriteInfixBinaryExpr(Opcode opcode,
                                   const char* op,
                                   AssignOp assign_op) {
  Type result_type = opcode.GetResultType();
  Write(StackVar(1, result_type));
  if (assign_op == AssignOp::Allowed) {
    Write(" ", op, "= ", StackVar(0));
  } else {
    Write(" = ", StackVar(1), " ", op, " ", StackVar(0));
  }
  Write(";", Newline());
  DropTypes(2);
  PushType(result_type);
}

void CWriter::WritePrefixBinaryExpr(Opcode opcode, const char* op) {
  Type result_type = opcode.GetResultType();
  Write(StackVar(1, result_type), " = ", op, "(", StackVar(1), ", ",
        StackVar(0), ");", Newline());
  DropTypes(2);
  PushType(result_type);
}

void CWriter::WriteSignedBinaryExpr(Opcode opcode, const char* op) {
  Type result_type = opcode.GetResultType();
  Type type = opcode.GetParamType1();
  assert(opcode.GetParamType2() == type);
  Write(StackVar(1, result_type), " = (", type, ")((", SignedType(type), ")",
        StackVar(1), " ", op, " (", SignedType(type), ")", StackVar(0), ");",
        Newline());
  DropTypes(2);
  PushType(result_type);
}

void CWriter::Write(const BinaryExpr& expr) {
  switch (expr.opcode) {
    case Opcode::I32Add:
    case Opcode::I64Add:
    case Opcode::F32Add:
    case Opcode::F64Add:
      WriteInfixBinaryExpr(expr.opcode, "+");
      break;

    case Opcode::I32Sub:
    case Opcode::I64Sub:
    case Opcode::F32Sub:
    case Opcode::F64Sub:
      WriteInfixBinaryExpr(expr.opcode, "-");
      break;

    case Opcode::I32Mul:
    case Opcode::I64Mul:
    case Opcode::F32Mul:
    case Opcode::F64Mul:
      WriteInfixBinaryExpr(expr.opcode, "*");
      break;

    case Opcode::I32DivS:
      WritePrefixBinaryExpr(expr.opcode, "I32_DIV_S");
      break;

    case Opcode::I64DivS:
      WritePrefixBinaryExpr(expr.opcode, "I64_DIV_S");
      break;

    case Opcode::I32DivU:
    case Opcode::I64DivU:
      WritePrefixBinaryExpr(expr.opcode, "DIV_U");
      break;

    case Opcode::F32Div:
    case Opcode::F64Div:
      WriteInfixBinaryExpr(expr.opcode, "/");
      break;

    case Opcode::I32RemS:
      WritePrefixBinaryExpr(expr.opcode, "I32_REM_S");
      break;

    case Opcode::I64RemS:
      WritePrefixBinaryExpr(expr.opcode, "I64_REM_S");
      break;

    case Opcode::I32RemU:
    case Opcode::I64RemU:
      WritePrefixBinaryExpr(expr.opcode, "REM_U");
      break;

    case Opcode::I32And:
    case Opcode::I64And:
      WriteInfixBinaryExpr(expr.opcode, "&");
      break;

    case Opcode::I32Or:
    case Opcode::I64Or:
      WriteInfixBinaryExpr(expr.opcode, "|");
      break;

    case Opcode::I32Xor:
    case Opcode::I64Xor:
      WriteInfixBinaryExpr(expr.opcode, "^");
      break;

    case Opcode::I32Shl:
    case Opcode::I64Shl:
      Write(StackVar(1), " <<= (", StackVar(0), " & ",
            GetShiftMask(expr.opcode.GetResultType()), ");", Newline());
      DropTypes(1);
      break;

    case Opcode::I32ShrS:
    case Opcode::I64ShrS: {
      Type type = expr.opcode.GetResultType();
      Write(StackVar(1), " = (", type, ")((", SignedType(type), ")",
            StackVar(1), " >> (", StackVar(0), " & ", GetShiftMask(type), "));",
            Newline());
      DropTypes(1);
      break;
    }

    case Opcode::I32ShrU:
    case Opcode::I64ShrU:
      Write(StackVar(1), " >>= (", StackVar(0), " & ",
            GetShiftMask(expr.opcode.GetResultType()), ");", Newline());
      DropTypes(1);
      break;

    case Opcode::I32Rotl:
      WritePrefixBinaryExpr(expr.opcode, "I32_ROTL");
      break;

    case Opcode::I64Rotl:
      WritePrefixBinaryExpr(expr.opcode, "I64_ROTL");
      break;

    case Opcode::I32Rotr:
      WritePrefixBinaryExpr(expr.opcode, "I32_ROTR");
      break;

    case Opcode::I64Rotr:
      WritePrefixBinaryExpr(expr.opcode, "I64_ROTR");
      break;

    case Opcode::F32Min:
    case Opcode::F64Min:
      WritePrefixBinaryExpr(expr.opcode, "FMIN");
      break;

    case Opcode::F32Max:
    case Opcode::F64Max:
      WritePrefixBinaryExpr(expr.opcode, "FMAX");
      break;

    case Opcode::F32Copysign:
      WritePrefixBinaryExpr(expr.opcode, "copysignf");
      break;

    case Opcode::F64Copysign:
      WritePrefixBinaryExpr(expr.opcode, "copysign");
      break;

    default:
      WABT_UNREACHABLE;
  }
}

void CWriter::Write(const CompareExpr& expr) {
  switch (expr.opcode) {
    case Opcode::I32Eq:
    case Opcode::I64Eq:
    case Opcode::F32Eq:
    case Opcode::F64Eq:
      WriteInfixBinaryExpr(expr.opcode, "==", AssignOp::Disallowed);
      break;

    case Opcode::I32Ne:
    case Opcode::I64Ne:
    case Opcode::F32Ne:
    case Opcode::F64Ne:
      WriteInfixBinaryExpr(expr.opcode, "!=", AssignOp::Disallowed);
      break;

    case Opcode::I32LtS:
    case Opcode::I64LtS:
      WriteSignedBinaryExpr(expr.opcode, "<");
      break;

    case Opcode::I32LtU:
    case Opcode::I64LtU:
    case Opcode::F32Lt:
    case Opcode::F64Lt:
      WriteInfixBinaryExpr(expr.opcode, "<", AssignOp::Disallowed);
      break;

    case Opcode::I32LeS:
    case Opcode::I64LeS:
      WriteSignedBinaryExpr(expr.opcode, "<=");
      break;

    case Opcode::I32LeU:
    case Opcode::I64LeU:
    case Opcode::F32Le:
    case Opcode::F64Le:
      WriteInfixBinaryExpr(expr.opcode, "<=", AssignOp::Disallowed);
      break;

    case Opcode::I32GtS:
    case Opcode::I64GtS:
      WriteSignedBinaryExpr(expr.opcode, ">");
      break;

    case Opcode::I32GtU:
    case Opcode::I64GtU:
    case Opcode::F32Gt:
    case Opcode::F64Gt:
      WriteInfixBinaryExpr(expr.opcode, ">", AssignOp::Disallowed);
      break;

    case Opcode::I32GeS:
    case Opcode::I64GeS:
      WriteSignedBinaryExpr(expr.opcode, ">=");
      break;

    case Opcode::I32GeU:
    case Opcode::I64GeU:
    case Opcode::F32Ge:
    case Opcode::F64Ge:
      WriteInfixBinaryExpr(expr.opcode, ">=", AssignOp::Disallowed);
      break;

    default:
      WABT_UNREACHABLE;
  }
}

void CWriter::Write(const ConvertExpr& expr) {
  switch (expr.opcode) {
    case Opcode::I32Eqz:
    case Opcode::I64Eqz:
      WriteSimpleUnaryExpr(expr.opcode, "!");
      break;

    case Opcode::I64ExtendSI32:
      WriteSimpleUnaryExpr(expr.opcode, "(u64)(s64)(s32)");
      break;

    case Opcode::I64ExtendUI32:
      WriteSimpleUnaryExpr(expr.opcode, "(u64)");
      break;

    case Opcode::I32WrapI64:
      WriteSimpleUnaryExpr(expr.opcode, "(u32)");
      break;

    case Opcode::I32TruncSF32:
      WriteSimpleUnaryExpr(expr.opcode, "I32_TRUNC_S_F32");
      break;

    case Opcode::I64TruncSF32:
      WriteSimpleUnaryExpr(expr.opcode, "I64_TRUNC_S_F32");
      break;

    case Opcode::I32TruncSF64:
      WriteSimpleUnaryExpr(expr.opcode, "I32_TRUNC_S_F64");
      break;

    case Opcode::I64TruncSF64:
      WriteSimpleUnaryExpr(expr.opcode, "I64_TRUNC_S_F64");
      break;

    case Opcode::I32TruncUF32:
      WriteSimpleUnaryExpr(expr.opcode, "I32_TRUNC_U_F32");
      break;

    case Opcode::I64TruncUF32:
      WriteSimpleUnaryExpr(expr.opcode, "I64_TRUNC_U_F32");
      break;

    case Opcode::I32TruncUF64:
      WriteSimpleUnaryExpr(expr.opcode, "I32_TRUNC_U_F64");
      break;

    case Opcode::I64TruncUF64:
      WriteSimpleUnaryExpr(expr.opcode, "I64_TRUNC_U_F64");
      break;

    case Opcode::I32TruncSSatF32:
    case Opcode::I64TruncSSatF32:
    case Opcode::I32TruncSSatF64:
    case Opcode::I64TruncSSatF64:
    case Opcode::I32TruncUSatF32:
    case Opcode::I64TruncUSatF32:
    case Opcode::I32TruncUSatF64:
    case Opcode::I64TruncUSatF64:
      UNIMPLEMENTED(expr.opcode.GetName());
      break;

    case Opcode::F32ConvertSI32:
      WriteSimpleUnaryExpr(expr.opcode, "(f32)(s32)");
      break;

    case Opcode::F32ConvertSI64:
      WriteSimpleUnaryExpr(expr.opcode, "(f32)(s64)");
      break;

    case Opcode::F32ConvertUI32:
    case Opcode::F32DemoteF64:
      WriteSimpleUnaryExpr(expr.opcode, "(f32)");
      break;

    case Opcode::F32ConvertUI64:
      // TODO(binji): This needs to be handled specially (see
      // wabt_convert_uint64_to_float).
      WriteSimpleUnaryExpr(expr.opcode, "(f32)");
      break;

    case Opcode::F64ConvertSI32:
      WriteSimpleUnaryExpr(expr.opcode, "(f64)(s32)");
      break;

    case Opcode::F64ConvertSI64:
      WriteSimpleUnaryExpr(expr.opcode, "(f64)(s64)");
      break;

    case Opcode::F64ConvertUI32:
    case Opcode::F64PromoteF32:
      WriteSimpleUnaryExpr(expr.opcode, "(f64)");
      break;

    case Opcode::F64ConvertUI64:
      // TODO(binji): This needs to be handled specially (see
      // wabt_convert_uint64_to_double).
      WriteSimpleUnaryExpr(expr.opcode, "(f64)");
      break;

    case Opcode::F32ReinterpretI32:
      WriteSimpleUnaryExpr(expr.opcode, "f32_reinterpret_i32");
      break;

    case Opcode::I32ReinterpretF32:
      WriteSimpleUnaryExpr(expr.opcode, "i32_reinterpret_f32");
      break;

    case Opcode::F64ReinterpretI64:
      WriteSimpleUnaryExpr(expr.opcode, "f64_reinterpret_i64");
      break;

    case Opcode::I64ReinterpretF64:
      WriteSimpleUnaryExpr(expr.opcode, "i64_reinterpret_f64");
      break;

    default:
      WABT_UNREACHABLE;
  }
}

void CWriter::Write(const LoadExpr& expr) {
  const char* func = nullptr;
  switch (expr.opcode) {
    case Opcode::I32Load: func = "i32_load"; break;
    case Opcode::I64Load: func = "i64_load"; break;
    case Opcode::F32Load: func = "f32_load"; break;
    case Opcode::F64Load: func = "f64_load"; break;
    case Opcode::I32Load8S: func = "i32_load8_s"; break;
    case Opcode::I64Load8S: func = "i64_load8_s"; break;
    case Opcode::I32Load8U: func = "i32_load8_u"; break;
    case Opcode::I64Load8U: func = "i64_load8_u"; break;
    case Opcode::I32Load16S: func = "i32_load16_s"; break;
    case Opcode::I64Load16S: func = "i64_load16_s"; break;
    case Opcode::I32Load16U: func = "i32_load16_u"; break;
    case Opcode::I64Load16U: func = "i32_load16_u"; break;
    case Opcode::I64Load32S: func = "i64_load32_s"; break;
    case Opcode::I64Load32U: func = "i64_load32_u"; break;

    default:
      WABT_UNREACHABLE;
  }

  assert(module_->memories.size() == 1);
  Memory* memory = module_->memories[0];

  Type result_type = expr.opcode.GetResultType();
  Write(StackVar(0, result_type), " = ", func, "(", ExternalPtr(memory->name),
        ", (u64)(", StackVar(0));
  if (expr.offset != 0)
    Write(" + ", expr.offset);
  Write("));", Newline());
  DropTypes(1);
  PushType(result_type);
}

void CWriter::Write(const StoreExpr& expr) {
  const char* func = nullptr;
  switch (expr.opcode) {
    case Opcode::I32Store: func = "i32_store"; break;
    case Opcode::I64Store: func = "i64_store"; break;
    case Opcode::F32Store: func = "f32_store"; break;
    case Opcode::F64Store: func = "f64_store"; break;
    case Opcode::I32Store8: func = "i32_store8"; break;
    case Opcode::I64Store8: func = "i64_store8"; break;
    case Opcode::I32Store16: func = "i32_store16"; break;
    case Opcode::I64Store16: func = "i64_store16"; break;
    case Opcode::I64Store32: func = "i64_store32"; break;

    default:
      WABT_UNREACHABLE;
  }

  assert(module_->memories.size() == 1);
  Memory* memory = module_->memories[0];

  Write(func, "(", ExternalPtr(memory->name), ", (u64)(", StackVar(1));
  if (expr.offset != 0)
    Write(" + ", expr.offset);
  Write("), ", StackVar(0), ");", Newline());
  DropTypes(2);
}

void CWriter::Write(const UnaryExpr& expr) {
  switch (expr.opcode) {
    case Opcode::I32Clz:
      WriteSimpleUnaryExpr(expr.opcode, "I32_CLZ");
      break;

    case Opcode::I64Clz:
      WriteSimpleUnaryExpr(expr.opcode, "I64_CLZ");
      break;

    case Opcode::I32Ctz:
      WriteSimpleUnaryExpr(expr.opcode, "I32_CTZ");
      break;

    case Opcode::I64Ctz:
      WriteSimpleUnaryExpr(expr.opcode, "I64_CTZ");
      break;

    case Opcode::I32Popcnt:
      WriteSimpleUnaryExpr(expr.opcode, "I32_POPCNT");
      break;

    case Opcode::I64Popcnt:
      WriteSimpleUnaryExpr(expr.opcode, "I64_POPCNT");
      break;

    case Opcode::F32Neg:
    case Opcode::F64Neg:
      WriteSimpleUnaryExpr(expr.opcode, "-");
      break;

    case Opcode::F32Abs:
      WriteSimpleUnaryExpr(expr.opcode, "fabsf");
      break;

    case Opcode::F64Abs:
      WriteSimpleUnaryExpr(expr.opcode, "fabs");
      break;

    case Opcode::F32Sqrt:
      WriteSimpleUnaryExpr(expr.opcode, "sqrtf");
      break;

    case Opcode::F64Sqrt:
      WriteSimpleUnaryExpr(expr.opcode, "sqrt");
      break;

    case Opcode::F32Ceil:
      WriteSimpleUnaryExpr(expr.opcode, "ceilf");
      break;

    case Opcode::F64Ceil:
      WriteSimpleUnaryExpr(expr.opcode, "ceil");
      break;

    case Opcode::F32Floor:
      WriteSimpleUnaryExpr(expr.opcode, "floorf");
      break;

    case Opcode::F64Floor:
      WriteSimpleUnaryExpr(expr.opcode, "floor");
      break;

    case Opcode::F32Trunc:
      WriteSimpleUnaryExpr(expr.opcode, "truncf");
      break;

    case Opcode::F64Trunc:
      WriteSimpleUnaryExpr(expr.opcode, "trunc");
      break;

    case Opcode::F32Nearest:
      WriteSimpleUnaryExpr(expr.opcode, "nearbyintf");
      break;

    case Opcode::F64Nearest:
      WriteSimpleUnaryExpr(expr.opcode, "nearbyint");
      break;

    case Opcode::I32Extend8S:
    case Opcode::I32Extend16S:
    case Opcode::I64Extend8S:
    case Opcode::I64Extend16S:
    case Opcode::I64Extend32S:
      UNIMPLEMENTED(expr.opcode.GetName());
      break;

    default:
      WABT_UNREACHABLE;
  }
}

void CWriter::Write(const TernaryExpr& expr) {
  switch (expr.opcode) {
    case Opcode::V128BitSelect: {
      Type result_type = expr.opcode.GetResultType();
      Write(StackVar(2, result_type), " = ", "v128.bitselect", "(", StackVar(0),
            ", ", StackVar(1), ", ", StackVar(2), ");", Newline());
      DropTypes(3);
      PushType(result_type);
      break;
    }
    default:
      WABT_UNREACHABLE;
  }
}

void CWriter::Write(const SimdLaneOpExpr& expr) {
  Type result_type = expr.opcode.GetResultType();

  switch (expr.opcode) {
    case Opcode::I8X16ExtractLaneS:
    case Opcode::I8X16ExtractLaneU:
    case Opcode::I16X8ExtractLaneS:
    case Opcode::I16X8ExtractLaneU:
    case Opcode::I32X4ExtractLane:
    case Opcode::I64X2ExtractLane:
    case Opcode::F32X4ExtractLane:
    case Opcode::F64X2ExtractLane: {
      Write(StackVar(0, result_type), " = ", expr.opcode.GetName(), "(",
            StackVar(0), ", lane Imm: ", expr.val, ");", Newline());
      DropTypes(1);
      break;
    }
    case Opcode::I8X16ReplaceLane:
    case Opcode::I16X8ReplaceLane:
    case Opcode::I32X4ReplaceLane:
    case Opcode::I64X2ReplaceLane:
    case Opcode::F32X4ReplaceLane:
    case Opcode::F64X2ReplaceLane: {
      Write(StackVar(1, result_type), " = ", expr.opcode.GetName(), "(",
            StackVar(0), ", ", StackVar(1), ", lane Imm: ", expr.val, ");",
            Newline());
      DropTypes(2);
      break;
    }
    default:
      WABT_UNREACHABLE;
  }

  PushType(result_type);
}

void CWriter::Write(const SimdShuffleOpExpr& expr) {
  Type result_type = expr.opcode.GetResultType();
  Write(StackVar(1, result_type), " = ", expr.opcode.GetName(), "(",
        StackVar(1), " ", StackVar(0), ", lane Imm: $0x%08x %08x %08x %08x",
        expr.val.v[0], expr.val.v[1], expr.val.v[2], expr.val.v[3], ")",
        Newline());
  DropTypes(2);
  PushType(result_type);
}

void CWriter::WriteCHeader() {
  stream_ = h_stream_;
  std::string guard = GenerateHeaderGuard();
  Write("#ifndef ", guard, Newline());
  Write("#define ", guard, Newline());
  Write(s_header_top);
  WriteImports();
  WriteExports(WriteExportsKind::Declarations);
  Write(s_header_bottom);
  Write(Newline(), "#endif  /* ", guard, " */", Newline());
}

void CWriter::WriteCSource() {
  stream_ = c_stream_;
  WriteSourceTop();
  WriteFuncTypes();
  WriteFuncDeclarations();
  WriteGlobals();
  WriteMemories();
  WriteTables();
  WriteFuncs();
  WriteDataInitializers();
  WriteElemInitializers();
  WriteExports(WriteExportsKind::Definitions);
  WriteInitExports();
  WriteInit();
}

Result CWriter::WriteModule(const Module& module) {
  WABT_USE(options_);
  module_ = &module;
  InitGlobalSymbols();
  WriteCHeader();
  WriteCSource();
  return result_;
}

}  // end anonymous namespace

Result WriteC(Stream* c_stream,
              Stream* h_stream,
              const char* header_name,
              const Module* module,
              const WriteCOptions* options) {
  CWriter c_writer(c_stream, h_stream, header_name, options);
  return c_writer.WriteModule(*module);
}

}  // namespace wabt
