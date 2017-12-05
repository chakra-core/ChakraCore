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

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "src/binary-reader-interpreter.h"
#include "src/binary-reader.h"
#include "src/cast.h"
#include "src/error-handler.h"
#include "src/feature.h"
#include "src/interpreter.h"
#include "src/literal.h"
#include "src/option-parser.h"
#include "src/resolve-names.h"
#include "src/stream.h"
#include "src/validator.h"
#include "src/wast-lexer.h"
#include "src/wast-parser.h"

using namespace wabt;
using namespace wabt::interpreter;

static int s_verbose;
static const char* s_infile;
static Thread::Options s_thread_options;
static Features s_features;

static std::unique_ptr<FileStream> s_log_stream;
static std::unique_ptr<FileStream> s_stdout_stream;

enum class RunVerbosity {
  Quiet = 0,
  Verbose = 1,
};

static const char s_description[] =
R"(  read a Spectest JSON file, and run its tests in the interpreter.

examples:
  # parse test.json and run the spec tests
  $ spectest-interp test.json
)";

static void ParseOptions(int argc, char** argv) {
  OptionParser parser("spectest-interp", s_description);

  parser.AddOption('v', "verbose", "Use multiple times for more info", []() {
    s_verbose++;
    s_log_stream = FileStream::CreateStdout();
  });
  parser.AddHelpOption();
  s_features.AddOptions(&parser);
  parser.AddOption('V', "value-stack-size", "SIZE",
                   "Size in elements of the value stack",
                   [](const std::string& argument) {
                     // TODO(binji): validate.
                     s_thread_options.value_stack_size = atoi(argument.c_str());
                   });
  parser.AddOption('C', "call-stack-size", "SIZE",
                   "Size in elements of the call stack",
                   [](const std::string& argument) {
                     // TODO(binji): validate.
                     s_thread_options.call_stack_size = atoi(argument.c_str());
                   });
  parser.AddOption('t', "trace", "Trace execution", []() {
    s_thread_options.trace_stream = s_stdout_stream.get();
  });

  parser.AddArgument("filename", OptionParser::ArgumentCount::One,
                     [](const char* argument) { s_infile = argument; });
  parser.Parse(argc, argv);
}

enum class ModuleType {
  Text,
  Binary,
};

static string_view GetDirname(string_view path) {
  // Strip everything after and including the last slash (or backslash), e.g.:
  //
  // s = "foo/bar/baz", => "foo/bar"
  // s = "/usr/local/include/stdio.h", => "/usr/local/include"
  // s = "foo.bar", => ""
  // s = "some\windows\directory", => "some\windows"
  size_t last_slash = path.find_last_of('/');
  size_t last_backslash = path.find_last_of('\\');
  if (last_slash == string_view::npos)
    last_slash = 0;
  if (last_backslash == string_view::npos)
    last_backslash = 0;

  return path.substr(0, std::max(last_slash, last_backslash));
}

static interpreter::Result GetGlobalExportByName(Thread* thread,
                                                 interpreter::Module* module,
                                                 string_view name,
                                                 TypedValues* out_results) {
  interpreter::Export* export_ = module->GetExport(name);
  if (!export_)
    return interpreter::Result::UnknownExport;
  if (export_->kind != ExternalKind::Global)
    return interpreter::Result::ExportKindMismatch;

  interpreter::Global* global = thread->env()->GetGlobal(export_->index);
  out_results->clear();
  out_results->push_back(global->typed_value);
  return interpreter::Result::Ok;
}

static wabt::Result ReadModule(const char* module_filename,
                               Environment* env,
                               ErrorHandler* error_handler,
                               DefinedModule** out_module) {
  wabt::Result result;
  std::vector<uint8_t> file_data;

  *out_module = nullptr;

  result = ReadFile(module_filename, &file_data);
  if (Succeeded(result)) {
    const bool kReadDebugNames = true;
    const bool kStopOnFirstError = true;
    ReadBinaryOptions options(s_features, s_log_stream.get(), kReadDebugNames,
                              kStopOnFirstError);
    result = ReadBinaryInterpreter(env, DataOrNull(file_data), file_data.size(),
                                   &options, error_handler, out_module);

    if (Succeeded(result)) {
      if (s_verbose)
        env->DisassembleModule(s_stdout_stream.get(), *out_module);
    }
  }
  return result;
}

static interpreter::Result DefaultHostCallback(
    const HostFunc* func,
    const interpreter::FuncSignature* sig,
    Index num_args,
    TypedValue* args,
    Index num_results,
    TypedValue* out_results,
    void* user_data) {
  memset(out_results, 0, sizeof(TypedValue) * num_results);
  for (Index i = 0; i < num_results; ++i)
    out_results[i].type = sig->result_types[i];

  TypedValues vec_args(args, args + num_args);
  TypedValues vec_results(out_results, out_results + num_results);

  printf("called host ");
  WriteCall(s_stdout_stream.get(), func->module_name, func->field_name,
            vec_args, vec_results, interpreter::Result::Ok);
  return interpreter::Result::Ok;
}

#define PRIimport "\"" PRIstringview "." PRIstringview "\""
#define PRINTF_IMPORT_ARG(x)                    \
  WABT_PRINTF_STRING_VIEW_ARG((x).module_name) \
  , WABT_PRINTF_STRING_VIEW_ARG((x).field_name)

class SpectestHostImportDelegate : public HostImportDelegate {
 public:
  wabt::Result ImportFunc(interpreter::FuncImport* import,
                          interpreter::Func* func,
                          interpreter::FuncSignature* func_sig,
                          const ErrorCallback& callback) override {
    if (import->field_name == "print") {
      cast<HostFunc>(func)->callback = DefaultHostCallback;
      return wabt::Result::Ok;
    } else {
      PrintError(callback, "unknown host function import " PRIimport,
                 PRINTF_IMPORT_ARG(*import));
      return wabt::Result::Error;
    }
  }

  wabt::Result ImportTable(interpreter::TableImport* import,
                           interpreter::Table* table,
                           const ErrorCallback& callback) override {
    if (import->field_name == "table") {
      table->limits.has_max = true;
      table->limits.initial = 10;
      table->limits.max = 20;
      return wabt::Result::Ok;
    } else {
      PrintError(callback, "unknown host table import " PRIimport,
                 PRINTF_IMPORT_ARG(*import));
      return wabt::Result::Error;
    }
  }

  wabt::Result ImportMemory(interpreter::MemoryImport* import,
                            interpreter::Memory* memory,
                            const ErrorCallback& callback) override {
    if (import->field_name == "memory") {
      memory->page_limits.has_max = true;
      memory->page_limits.initial = 1;
      memory->page_limits.max = 2;
      memory->data.resize(memory->page_limits.initial * WABT_MAX_PAGES);
      return wabt::Result::Ok;
    } else {
      PrintError(callback, "unknown host memory import " PRIimport,
                 PRINTF_IMPORT_ARG(*import));
      return wabt::Result::Error;
    }
  }

  wabt::Result ImportGlobal(interpreter::GlobalImport* import,
                            interpreter::Global* global,
                            const ErrorCallback& callback) override {
    if (import->field_name == "global") {
      switch (global->typed_value.type) {
        case Type::I32:
          global->typed_value.value.i32 = 666;
          break;

        case Type::F32: {
          float value = 666.6f;
          memcpy(&global->typed_value.value.f32_bits, &value, sizeof(value));
          break;
        }

        case Type::I64:
          global->typed_value.value.i64 = 666;
          break;

        case Type::F64: {
          double value = 666.6;
          memcpy(&global->typed_value.value.f64_bits, &value, sizeof(value));
          break;
        }

        default:
          PrintError(callback, "bad type for host global import " PRIimport,
                     PRINTF_IMPORT_ARG(*import));
          return wabt::Result::Error;
      }

      return wabt::Result::Ok;
    } else {
      PrintError(callback, "unknown host global import " PRIimport,
                 PRINTF_IMPORT_ARG(*import));
      return wabt::Result::Error;
    }
  }

 private:
  void PrintError(const ErrorCallback& callback, const char* format, ...) {
    WABT_SNPRINTF_ALLOCA(buffer, length, format);
    callback(buffer);
  }
};

static void InitEnvironment(Environment* env) {
  HostModule* host_module = env->AppendHostModule("spectest");
  host_module->import_delegate.reset(new SpectestHostImportDelegate());
}

enum class ActionType {
  Invoke,
  Get,
};

struct Action {
  ::ActionType type = ::ActionType::Invoke;
  std::string module_name;
  std::string field_name;
  TypedValues args;
};

// An extremely simple JSON parser that only knows how to parse the expected
// format from wat2wasm.
class SpecJSONParser {
 public:
  SpecJSONParser() : thread_(&env_, s_thread_options) {}

  wabt::Result ReadFile(const char* spec_json_filename);
  wabt::Result ParseCommands();

  int passed() const { return passed_; }
  int total() const { return total_; }

 private:
  void WABT_PRINTF_FORMAT(2, 3) PrintParseError(const char* format, ...);
  void WABT_PRINTF_FORMAT(2, 3) PrintCommandError(const char* format, ...);

  void PutbackChar();
  int ReadChar();
  void SkipWhitespace();
  bool Match(const char* s);
  wabt::Result Expect(const char* s);
  wabt::Result ExpectKey(const char* key);
  wabt::Result ParseUint32(uint32_t* out_int);
  wabt::Result ParseString(std::string* out_string);
  wabt::Result ParseKeyStringValue(const char* key, std::string* out_string);
  wabt::Result ParseOptNameStringValue(std::string* out_string);
  wabt::Result ParseLine();
  wabt::Result ParseTypeObject(Type* out_type);
  wabt::Result ParseTypeVector(TypeVector* out_types);
  wabt::Result ParseConst(TypedValue* out_value);
  wabt::Result ParseConstVector(TypedValues* out_values);
  wabt::Result ParseAction(::Action* out_action);
  wabt::Result ParseModuleType(ModuleType* out_type);

  std::string CreateModulePath(string_view filename);

  wabt::Result OnModuleCommand(string_view filename, string_view name);
  wabt::Result RunAction(::Action* action,
                         interpreter::Result* out_iresult,
                         TypedValues* out_results,
                         RunVerbosity verbose);
  wabt::Result OnActionCommand(::Action* action);
  wabt::Result ReadInvalidTextModule(const char* module_filename,
                                     Environment* env,
                                     ErrorHandler* error_handler);
  wabt::Result ReadInvalidModule(const char* module_filename,
                                 Environment* env,
                                 ModuleType module_type,
                                 const char* desc);
  wabt::Result OnAssertMalformedCommand(string_view filename,
                                        string_view text,
                                        ModuleType module_type);
  wabt::Result OnRegisterCommand(string_view name, string_view as);
  wabt::Result OnAssertUnlinkableCommand(string_view filename,
                                         string_view text,
                                         ModuleType module_type);
  wabt::Result OnAssertInvalidCommand(string_view filename,
                                      string_view text,
                                      ModuleType module_type);
  wabt::Result OnAssertUninstantiableCommand(string_view filename,
                                             string_view text,
                                             ModuleType module_type);
  wabt::Result OnAssertReturnCommand(::Action* action,
                                     const TypedValues& expected);
  wabt::Result OnAssertReturnNanCommand(::Action* action, bool canonical);
  wabt::Result OnAssertTrapCommand(::Action* action, string_view text);
  wabt::Result OnAssertExhaustionCommand(::Action* action);
  wabt::Result ParseCommand();

  Environment env_;
  Thread thread_;
  DefinedModule* last_module_ = nullptr;

  // Parsing info.
  std::vector<uint8_t> json_data_;
  std::string source_filename_;
  size_t json_offset_ = 0;
  Location loc_;
  Location prev_loc_;
  bool has_prev_loc_ = false;
  uint32_t command_line_number_ = 0;

  // Test info.
  int passed_ = 0;
  int total_ = 0;
};

#define EXPECT(x) CHECK_RESULT(Expect(x))
#define EXPECT_KEY(x) CHECK_RESULT(ExpectKey(x))
#define PARSE_KEY_STRING_VALUE(key, value) \
  CHECK_RESULT(ParseKeyStringValue(key, value))

wabt::Result SpecJSONParser::ReadFile(const char* spec_json_filename) {
  loc_.filename = spec_json_filename;
  loc_.line = 1;
  loc_.first_column = 1;
  InitEnvironment(&env_);

  return wabt::ReadFile(spec_json_filename, &json_data_);
}

void SpecJSONParser::PrintParseError(const char* format, ...) {
  WABT_SNPRINTF_ALLOCA(buffer, length, format);
  fprintf(stderr, "%s:%d:%d: %s\n", loc_.filename, loc_.line, loc_.first_column,
          buffer);
}

void SpecJSONParser::PrintCommandError(const char* format, ...) {
  WABT_SNPRINTF_ALLOCA(buffer, length, format);
  printf(PRIstringview ":%u: %s\n",
         WABT_PRINTF_STRING_VIEW_ARG(source_filename_), command_line_number_,
         buffer);
}

void SpecJSONParser::PutbackChar() {
  assert(has_prev_loc_);
  json_offset_--;
  loc_ = prev_loc_;
  has_prev_loc_ = false;
}

int SpecJSONParser::ReadChar() {
  if (json_offset_ >= json_data_.size())
    return -1;
  prev_loc_ = loc_;
  char c = json_data_[json_offset_++];
  if (c == '\n') {
    loc_.line++;
    loc_.first_column = 1;
  } else {
    loc_.first_column++;
  }
  has_prev_loc_ = true;
  return c;
}

void SpecJSONParser::SkipWhitespace() {
  while (1) {
    switch (ReadChar()) {
      case -1:
        return;

      case ' ':
      case '\t':
      case '\n':
      case '\r':
        break;

      default:
        PutbackChar();
        return;
    }
  }
}

bool SpecJSONParser::Match(const char* s) {
  SkipWhitespace();
  Location start_loc = loc_;
  size_t start_offset = json_offset_;
  while (*s && *s == ReadChar())
    s++;

  if (*s == 0) {
    return true;
  } else {
    json_offset_ = start_offset;
    loc_ = start_loc;
    return false;
  }
}

wabt::Result SpecJSONParser::Expect(const char* s) {
  if (Match(s)) {
    return wabt::Result::Ok;
  } else {
    PrintParseError("expected %s", s);
    return wabt::Result::Error;
  }
}

wabt::Result SpecJSONParser::ExpectKey(const char* key) {
  size_t keylen = strlen(key);
  size_t quoted_len = keylen + 2 + 1;
  char* quoted = static_cast<char*>(alloca(quoted_len));
  snprintf(quoted, quoted_len, "\"%s\"", key);
  EXPECT(quoted);
  EXPECT(":");
  return wabt::Result::Ok;
}

wabt::Result SpecJSONParser::ParseUint32(uint32_t* out_int) {
  uint32_t result = 0;
  SkipWhitespace();
  while (1) {
    int c = ReadChar();
    if (c >= '0' && c <= '9') {
      uint32_t last_result = result;
      result = result * 10 + static_cast<uint32_t>(c - '0');
      if (result < last_result) {
        PrintParseError("uint32 overflow");
        return wabt::Result::Error;
      }
    } else {
      PutbackChar();
      break;
    }
  }
  *out_int = result;
  return wabt::Result::Ok;
}

wabt::Result SpecJSONParser::ParseString(std::string* out_string) {
  out_string->clear();

  SkipWhitespace();
  if (ReadChar() != '"') {
    PrintParseError("expected string");
    return wabt::Result::Error;
  }

  while (1) {
    int c = ReadChar();
    if (c == '"') {
      break;
    } else if (c == '\\') {
      /* The only escape supported is \uxxxx. */
      c = ReadChar();
      if (c != 'u') {
        PrintParseError("expected escape: \\uxxxx");
        return wabt::Result::Error;
      }
      uint16_t code = 0;
      for (int i = 0; i < 4; ++i) {
        c = ReadChar();
        int cval;
        if (c >= '0' && c <= '9') {
          cval = c - '0';
        } else if (c >= 'a' && c <= 'f') {
          cval = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
          cval = c - 'A' + 10;
        } else {
          PrintParseError("expected hex char");
          return wabt::Result::Error;
        }
        code = (code << 4) + cval;
      }

      if (code < 256) {
        *out_string += code;
      } else {
        PrintParseError("only escape codes < 256 allowed, got %u\n", code);
      }
    } else {
      *out_string += c;
    }
  }
  return wabt::Result::Ok;
}

wabt::Result SpecJSONParser::ParseKeyStringValue(const char* key,
                                                 std::string* out_string) {
  out_string->clear();
  EXPECT_KEY(key);
  return ParseString(out_string);
}

wabt::Result SpecJSONParser::ParseOptNameStringValue(std::string* out_string) {
  out_string->clear();
  if (Match("\"name\"")) {
    EXPECT(":");
    CHECK_RESULT(ParseString(out_string));
    EXPECT(",");
  }
  return wabt::Result::Ok;
}

wabt::Result SpecJSONParser::ParseLine() {
  EXPECT_KEY("line");
  CHECK_RESULT(ParseUint32(&command_line_number_));
  return wabt::Result::Ok;
}

wabt::Result SpecJSONParser::ParseTypeObject(Type* out_type) {
  std::string type_str;
  EXPECT("{");
  PARSE_KEY_STRING_VALUE("type", &type_str);
  EXPECT("}");

  if (type_str == "i32") {
    *out_type = Type::I32;
    return wabt::Result::Ok;
  } else if (type_str == "f32") {
    *out_type = Type::F32;
    return wabt::Result::Ok;
  } else if (type_str == "i64") {
    *out_type = Type::I64;
    return wabt::Result::Ok;
  } else if (type_str == "f64") {
    *out_type = Type::F64;
    return wabt::Result::Ok;
  } else {
    PrintParseError("unknown type: \"" PRIstringview "\"",
                    WABT_PRINTF_STRING_VIEW_ARG(type_str));
    return wabt::Result::Error;
  }
}

wabt::Result SpecJSONParser::ParseTypeVector(TypeVector* out_types) {
  out_types->clear();
  EXPECT("[");
  bool first = true;
  while (!Match("]")) {
    if (!first)
      EXPECT(",");
    Type type;
    CHECK_RESULT(ParseTypeObject(&type));
    first = false;
    out_types->push_back(type);
  }
  return wabt::Result::Ok;
}

wabt::Result SpecJSONParser::ParseConst(TypedValue* out_value) {
  std::string type_str;
  std::string value_str;
  EXPECT("{");
  PARSE_KEY_STRING_VALUE("type", &type_str);
  EXPECT(",");
  PARSE_KEY_STRING_VALUE("value", &value_str);
  EXPECT("}");

  const char* value_start = value_str.data();
  const char* value_end = value_str.data() + value_str.size();

  if (type_str == "i32") {
    uint32_t value;
    CHECK_RESULT(
        ParseInt32(value_start, value_end, &value, ParseIntType::UnsignedOnly));
    out_value->type = Type::I32;
    out_value->value.i32 = value;
    return wabt::Result::Ok;
  } else if (type_str == "f32") {
    uint32_t value_bits;
    CHECK_RESULT(ParseInt32(value_start, value_end, &value_bits,
                            ParseIntType::UnsignedOnly));
    out_value->type = Type::F32;
    out_value->value.f32_bits = value_bits;
    return wabt::Result::Ok;
  } else if (type_str == "i64") {
    uint64_t value;
    CHECK_RESULT(
        ParseInt64(value_start, value_end, &value, ParseIntType::UnsignedOnly));
    out_value->type = Type::I64;
    out_value->value.i64 = value;
    return wabt::Result::Ok;
  } else if (type_str == "f64") {
    uint64_t value_bits;
    CHECK_RESULT(ParseInt64(value_start, value_end, &value_bits,
                            ParseIntType::UnsignedOnly));
    out_value->type = Type::F64;
    out_value->value.f64_bits = value_bits;
    return wabt::Result::Ok;
  } else {
    PrintParseError("unknown type: \"" PRIstringview "\"",
                    WABT_PRINTF_STRING_VIEW_ARG(type_str));
    return wabt::Result::Error;
  }
}

wabt::Result SpecJSONParser::ParseConstVector(TypedValues* out_values) {
  out_values->clear();
  EXPECT("[");
  bool first = true;
  while (!Match("]")) {
    if (!first)
      EXPECT(",");
    TypedValue value;
    CHECK_RESULT(ParseConst(&value));
    out_values->push_back(value);
    first = false;
  }
  return wabt::Result::Ok;
}

wabt::Result SpecJSONParser::ParseAction(::Action* out_action) {
  EXPECT_KEY("action");
  EXPECT("{");
  EXPECT_KEY("type");
  if (Match("\"invoke\"")) {
    out_action->type = ::ActionType::Invoke;
  } else {
    EXPECT("\"get\"");
    out_action->type = ::ActionType::Get;
  }
  EXPECT(",");
  if (Match("\"module\"")) {
    EXPECT(":");
    CHECK_RESULT(ParseString(&out_action->module_name));
    EXPECT(",");
  }
  PARSE_KEY_STRING_VALUE("field", &out_action->field_name);
  if (out_action->type == ::ActionType::Invoke) {
    EXPECT(",");
    EXPECT_KEY("args");
    CHECK_RESULT(ParseConstVector(&out_action->args));
  }
  EXPECT("}");
  return wabt::Result::Ok;
}

wabt::Result SpecJSONParser::ParseModuleType(ModuleType* out_type) {
  std::string module_type_str;

  PARSE_KEY_STRING_VALUE("module_type", &module_type_str);
  if (module_type_str == "text") {
    *out_type = ModuleType::Text;
    return wabt::Result::Ok;
  } else if (module_type_str == "binary") {
    *out_type = ModuleType::Binary;
    return wabt::Result::Ok;
  } else {
    PrintParseError("unknown module type: \"" PRIstringview "\"",
                    WABT_PRINTF_STRING_VIEW_ARG(module_type_str));
    return wabt::Result::Error;
  }
}

std::string SpecJSONParser::CreateModulePath(string_view filename) {
  const char* spec_json_filename = loc_.filename;
  string_view dirname = GetDirname(spec_json_filename);
  std::string path;

  if (dirname.size() == 0) {
    path = filename.to_string();
  } else {
    path = dirname.to_string();
    path += '/';
    path += filename.to_string();
  }

  ConvertBackslashToSlash(&path);
  return path;
}

wabt::Result SpecJSONParser::OnModuleCommand(string_view filename,
                                             string_view name) {
  std::string path = CreateModulePath(filename);
  Environment::MarkPoint mark = env_.Mark();
  ErrorHandlerFile error_handler(Location::Type::Binary);
  wabt::Result result =
      ReadModule(path.c_str(), &env_, &error_handler, &last_module_);

  if (Failed(result)) {
    env_.ResetToMarkPoint(mark);
    PrintCommandError("error reading module: \"%s\"", path.c_str());
    return wabt::Result::Error;
  }

  interpreter::Result iresult = thread_.RunStartFunction(last_module_);
  if (iresult != interpreter::Result::Ok) {
    env_.ResetToMarkPoint(mark);
    WriteResult(s_stdout_stream.get(), "error running start function", iresult);
    return wabt::Result::Error;
  }

  if (!name.empty()) {
    last_module_->name = name.to_string();
    env_.EmplaceModuleBinding(name.to_string(),
                             Binding(env_.GetModuleCount() - 1));
  }
  return wabt::Result::Ok;
}

wabt::Result SpecJSONParser::RunAction(::Action* action,
                                       interpreter::Result* out_iresult,
                                       TypedValues* out_results,
                                       RunVerbosity verbose) {
  out_results->clear();

  interpreter::Module* module;
  if (!action->module_name.empty()) {
    module = env_.FindModule(action->module_name);
  } else {
    module = env_.GetLastModule();
  }
  assert(module);

  switch (action->type) {
    case ::ActionType::Invoke:
      *out_iresult = thread_.RunExportByName(module, action->field_name,
                                             action->args, out_results);
      if (verbose == RunVerbosity::Verbose) {
        WriteCall(s_stdout_stream.get(), string_view(), action->field_name,
                  action->args, *out_results, *out_iresult);
      }
      return wabt::Result::Ok;

    case ::ActionType::Get: {
      *out_iresult = GetGlobalExportByName(&thread_, module, action->field_name,
                                           out_results);
      return wabt::Result::Ok;
    }

    default:
      PrintCommandError("invalid action type %d",
                        static_cast<int>(action->type));
      return wabt::Result::Error;
  }
}

wabt::Result SpecJSONParser::OnActionCommand(::Action* action) {
  TypedValues results;
  interpreter::Result iresult;

  total_++;
  wabt::Result result =
      RunAction(action, &iresult, &results, RunVerbosity::Verbose);
  if (Succeeded(result)) {
    if (iresult == interpreter::Result::Ok) {
      passed_++;
    } else {
      PrintCommandError("unexpected trap: %s", ResultToString(iresult));
      result = wabt::Result::Error;
    }
  }

  return result;
}

wabt::Result SpecJSONParser::ReadInvalidTextModule(
    const char* module_filename,
    Environment* env,
    ErrorHandler* error_handler) {
  std::unique_ptr<WastLexer> lexer =
      WastLexer::CreateFileLexer(module_filename);
  std::unique_ptr<Script> script;
  wabt::Result result = ParseWastScript(lexer.get(), &script, error_handler);
  if (Succeeded(result)) {
    wabt::Module* module = script->GetFirstModule();
    result = ResolveNamesModule(lexer.get(), module, error_handler);
    if (Succeeded(result)) {
      // Don't do a full validation, just validate the function signatures.
      result = ValidateFuncSignatures(lexer.get(), module, error_handler);
    }
  }
  return result;
}

wabt::Result SpecJSONParser::ReadInvalidModule(const char* module_filename,
                                               Environment* env,
                                               ModuleType module_type,
                                               const char* desc) {
  std::string header =
      StringPrintf(PRIstringview ":%d: %s passed",
                   WABT_PRINTF_STRING_VIEW_ARG(source_filename_),
                   command_line_number_, desc);

  switch (module_type) {
    case ModuleType::Text: {
      ErrorHandlerFile error_handler(Location::Type::Text, stdout, header,
                                     ErrorHandlerFile::PrintHeader::Once);
      return ReadInvalidTextModule(module_filename, env, &error_handler);
    }

    case ModuleType::Binary: {
      DefinedModule* module;
      ErrorHandlerFile error_handler(Location::Type::Binary, stdout, header,
                                     ErrorHandlerFile::PrintHeader::Once);
      return ReadModule(module_filename, env, &error_handler, &module);
    }
  }

  WABT_UNREACHABLE;
}

wabt::Result SpecJSONParser::OnAssertMalformedCommand(string_view filename,
                                                      string_view text,
                                                      ModuleType module_type) {
  Environment env;
  InitEnvironment(&env);

  total_++;
  std::string path = CreateModulePath(filename);
  wabt::Result result =
      ReadInvalidModule(path.c_str(), &env, module_type, "assert_malformed");
  if (Failed(result)) {
    passed_++;
    result = wabt::Result::Ok;
  } else {
    PrintCommandError("expected module to be malformed: \"%s\"", path.c_str());
    result = wabt::Result::Error;
  }

  return result;
}

wabt::Result SpecJSONParser::OnRegisterCommand(string_view name,
                                               string_view as) {
  Index module_index;
  if (!name.empty()) {
    module_index = env_.FindModuleIndex(name);
  } else {
    module_index = env_.GetLastModuleIndex();
  }

  if (module_index == kInvalidIndex) {
    PrintCommandError("unknown module in register");
    return wabt::Result::Error;
  }

  env_.EmplaceRegisteredModuleBinding(as.to_string(), Binding(module_index));
  return wabt::Result::Ok;
}

wabt::Result SpecJSONParser::OnAssertUnlinkableCommand(string_view filename,
                                                       string_view text,
                                                       ModuleType module_type) {
  total_++;
  std::string path = CreateModulePath(filename);
  Environment::MarkPoint mark = env_.Mark();
  wabt::Result result =
      ReadInvalidModule(path.c_str(), &env_, module_type, "assert_unlinkable");
  env_.ResetToMarkPoint(mark);

  if (Failed(result)) {
    passed_++;
    result = wabt::Result::Ok;
  } else {
    PrintCommandError("expected module to be unlinkable: \"%s\"", path.c_str());
    result = wabt::Result::Error;
  }

  return result;
}

wabt::Result SpecJSONParser::OnAssertInvalidCommand(string_view filename,
                                                    string_view text,
                                                    ModuleType module_type) {
  Environment env;
  InitEnvironment(&env);

  total_++;
  std::string path = CreateModulePath(filename);
  wabt::Result result =
      ReadInvalidModule(path.c_str(), &env, module_type, "assert_invalid");
  if (Failed(result)) {
    passed_++;
    result = wabt::Result::Ok;
  } else {
    PrintCommandError("expected module to be invalid: \"%s\"", path.c_str());
    result = wabt::Result::Error;
  }

  return result;
}

wabt::Result SpecJSONParser::OnAssertUninstantiableCommand(
    string_view filename,
    string_view text,
    ModuleType module_type) {
  ErrorHandlerFile error_handler(Location::Type::Binary);
  total_++;
  std::string path = CreateModulePath(filename);
  DefinedModule* module;
  Environment::MarkPoint mark = env_.Mark();
  wabt::Result result =
      ReadModule(path.c_str(), &env_, &error_handler, &module);

  if (Succeeded(result)) {
    interpreter::Result iresult = thread_.RunStartFunction(module);
    if (iresult == interpreter::Result::Ok) {
      PrintCommandError("expected error running start function: \"%s\"",
                        path.c_str());
      result = wabt::Result::Error;
    } else {
      passed_++;
      result = wabt::Result::Ok;
    }
  } else {
    PrintCommandError("error reading module: \"%s\"", path.c_str());
    result = wabt::Result::Error;
  }

  env_.ResetToMarkPoint(mark);
  return result;
}

static bool TypedValuesAreEqual(const TypedValue* tv1, const TypedValue* tv2) {
  if (tv1->type != tv2->type)
    return false;

  switch (tv1->type) {
    case Type::I32:
      return tv1->value.i32 == tv2->value.i32;
    case Type::F32:
      return tv1->value.f32_bits == tv2->value.f32_bits;
    case Type::I64:
      return tv1->value.i64 == tv2->value.i64;
    case Type::F64:
      return tv1->value.f64_bits == tv2->value.f64_bits;
    default:
      WABT_UNREACHABLE;
  }
}

wabt::Result SpecJSONParser::OnAssertReturnCommand(
    ::Action* action,
    const TypedValues& expected) {
  TypedValues results;
  interpreter::Result iresult;

  total_++;
  wabt::Result result =
      RunAction(action, &iresult, &results, RunVerbosity::Quiet);

  if (Succeeded(result)) {
    if (iresult == interpreter::Result::Ok) {
      if (results.size() == expected.size()) {
        for (size_t i = 0; i < results.size(); ++i) {
          const TypedValue* expected_tv = &expected[i];
          const TypedValue* actual_tv = &results[i];
          if (!TypedValuesAreEqual(expected_tv, actual_tv)) {
            PrintCommandError("mismatch in result %" PRIzd
                              " of assert_return: expected %s, got %s",
                              i, TypedValueToString(*expected_tv).c_str(),
                              TypedValueToString(*actual_tv).c_str());
            result = wabt::Result::Error;
          }
        }
      } else {
        PrintCommandError(
            "result length mismatch in assert_return: expected %" PRIzd
            ", got %" PRIzd,
            expected.size(), results.size());
        result = wabt::Result::Error;
      }
    } else {
      PrintCommandError("unexpected trap: %s", ResultToString(iresult));
      result = wabt::Result::Error;
    }
  }

  if (Succeeded(result))
    passed_++;

  return result;
}

wabt::Result SpecJSONParser::OnAssertReturnNanCommand(::Action* action,
                                                      bool canonical) {
  TypedValues results;
  interpreter::Result iresult;

  total_++;
  wabt::Result result =
      RunAction(action, &iresult, &results, RunVerbosity::Quiet);
  if (Succeeded(result)) {
    if (iresult == interpreter::Result::Ok) {
      if (results.size() != 1) {
        PrintCommandError("expected one result, got %" PRIzd, results.size());
        result = wabt::Result::Error;
      }

      const TypedValue& actual = results[0];
      switch (actual.type) {
        case Type::F32: {
          bool is_nan = canonical ? IsCanonicalNan(actual.value.f32_bits)
                                  : IsArithmeticNan(actual.value.f32_bits);
          if (!is_nan) {
            PrintCommandError("expected result to be nan, got %s",
                              TypedValueToString(actual).c_str());
            result = wabt::Result::Error;
          }
          break;
        }

        case Type::F64: {
          bool is_nan = canonical ? IsCanonicalNan(actual.value.f64_bits)
                                  : IsArithmeticNan(actual.value.f64_bits);
          if (!is_nan) {
            PrintCommandError("expected result to be nan, got %s",
                              TypedValueToString(actual).c_str());
            result = wabt::Result::Error;
          }
          break;
        }

        default:
          PrintCommandError("expected result type to be f32 or f64, got %s",
                            GetTypeName(actual.type));
          result = wabt::Result::Error;
          break;
      }
    } else {
      PrintCommandError("unexpected trap: %s", ResultToString(iresult));
      result = wabt::Result::Error;
    }
  }

  if (Succeeded(result))
    passed_++;

  return wabt::Result::Ok;
}

wabt::Result SpecJSONParser::OnAssertTrapCommand(::Action* action,
                                                 string_view text) {
  TypedValues results;
  interpreter::Result iresult;

  total_++;
  wabt::Result result =
      RunAction(action, &iresult, &results, RunVerbosity::Quiet);
  if (Succeeded(result)) {
    if (iresult != interpreter::Result::Ok) {
      passed_++;
    } else {
      PrintCommandError("expected trap: \"" PRIstringview "\"",
                        WABT_PRINTF_STRING_VIEW_ARG(text));
      result = wabt::Result::Error;
    }
  }

  return result;
}

wabt::Result SpecJSONParser::OnAssertExhaustionCommand(::Action* action) {
  TypedValues results;
  interpreter::Result iresult;

  total_++;
  wabt::Result result =
      RunAction(action, &iresult, &results, RunVerbosity::Quiet);
  if (Succeeded(result)) {
    if (iresult == interpreter::Result::TrapCallStackExhausted ||
        iresult == interpreter::Result::TrapValueStackExhausted) {
      passed_++;
    } else {
      PrintCommandError("expected call stack exhaustion");
      result = wabt::Result::Error;
    }
  }

  return result;
}

wabt::Result SpecJSONParser::ParseCommand() {
  EXPECT("{");
  EXPECT_KEY("type");
  if (Match("\"module\"")) {
    std::string name;
    std::string filename;

    EXPECT(",");
    CHECK_RESULT(ParseLine());
    EXPECT(",");
    CHECK_RESULT(ParseOptNameStringValue(&name));
    PARSE_KEY_STRING_VALUE("filename", &filename);
    OnModuleCommand(filename, name);
  } else if (Match("\"action\"")) {
    ::Action action;

    EXPECT(",");
    CHECK_RESULT(ParseLine());
    EXPECT(",");
    CHECK_RESULT(ParseAction(&action));
    OnActionCommand(&action);
  } else if (Match("\"register\"")) {
    std::string as;
    std::string name;

    EXPECT(",");
    CHECK_RESULT(ParseLine());
    EXPECT(",");
    CHECK_RESULT(ParseOptNameStringValue(&name));
    PARSE_KEY_STRING_VALUE("as", &as);
    OnRegisterCommand(name, as);
  } else if (Match("\"assert_malformed\"")) {
    std::string filename;
    std::string text;
    ModuleType module_type;

    EXPECT(",");
    CHECK_RESULT(ParseLine());
    EXPECT(",");
    PARSE_KEY_STRING_VALUE("filename", &filename);
    EXPECT(",");
    PARSE_KEY_STRING_VALUE("text", &text);
    EXPECT(",");
    CHECK_RESULT(ParseModuleType(&module_type));
    OnAssertMalformedCommand(filename, text, module_type);
  } else if (Match("\"assert_invalid\"")) {
    std::string filename;
    std::string text;
    ModuleType module_type;

    EXPECT(",");
    CHECK_RESULT(ParseLine());
    EXPECT(",");
    PARSE_KEY_STRING_VALUE("filename", &filename);
    EXPECT(",");
    PARSE_KEY_STRING_VALUE("text", &text);
    EXPECT(",");
    CHECK_RESULT(ParseModuleType(&module_type));
    OnAssertInvalidCommand(filename, text, module_type);
  } else if (Match("\"assert_unlinkable\"")) {
    std::string filename;
    std::string text;
    ModuleType module_type;

    EXPECT(",");
    CHECK_RESULT(ParseLine());
    EXPECT(",");
    PARSE_KEY_STRING_VALUE("filename", &filename);
    EXPECT(",");
    PARSE_KEY_STRING_VALUE("text", &text);
    EXPECT(",");
    CHECK_RESULT(ParseModuleType(&module_type));
    OnAssertUnlinkableCommand(filename, text, module_type);
  } else if (Match("\"assert_uninstantiable\"")) {
    std::string filename;
    std::string text;
    ModuleType module_type;

    EXPECT(",");
    CHECK_RESULT(ParseLine());
    EXPECT(",");
    PARSE_KEY_STRING_VALUE("filename", &filename);
    EXPECT(",");
    PARSE_KEY_STRING_VALUE("text", &text);
    EXPECT(",");
    CHECK_RESULT(ParseModuleType(&module_type));
    OnAssertUninstantiableCommand(filename, text, module_type);
  } else if (Match("\"assert_return\"")) {
    ::Action action;
    TypedValues expected;

    EXPECT(",");
    CHECK_RESULT(ParseLine());
    EXPECT(",");
    CHECK_RESULT(ParseAction(&action));
    EXPECT(",");
    EXPECT_KEY("expected");
    CHECK_RESULT(ParseConstVector(&expected));
    OnAssertReturnCommand(&action, expected);
  } else if (Match("\"assert_return_canonical_nan\"")) {
    ::Action action;
    TypeVector expected;

    EXPECT(",");
    CHECK_RESULT(ParseLine());
    EXPECT(",");
    CHECK_RESULT(ParseAction(&action));
    EXPECT(",");
    /* Not needed for wabt-interp, but useful for other parsers. */
    EXPECT_KEY("expected");
    CHECK_RESULT(ParseTypeVector(&expected));
    OnAssertReturnNanCommand(&action, true);
  } else if (Match("\"assert_return_arithmetic_nan\"")) {
    ::Action action;
    TypeVector expected;

    EXPECT(",");
    CHECK_RESULT(ParseLine());
    EXPECT(",");
    CHECK_RESULT(ParseAction(&action));
    EXPECT(",");
    /* Not needed for wabt-interp, but useful for other parsers. */
    EXPECT_KEY("expected");
    CHECK_RESULT(ParseTypeVector(&expected));
    OnAssertReturnNanCommand(&action, false);
  } else if (Match("\"assert_trap\"")) {
    ::Action action;
    std::string text;

    EXPECT(",");
    CHECK_RESULT(ParseLine());
    EXPECT(",");
    CHECK_RESULT(ParseAction(&action));
    EXPECT(",");
    PARSE_KEY_STRING_VALUE("text", &text);
    OnAssertTrapCommand(&action, text);
  } else if (Match("\"assert_exhaustion\"")) {
    ::Action action;
    std::string text;

    EXPECT(",");
    CHECK_RESULT(ParseLine());
    EXPECT(",");
    CHECK_RESULT(ParseAction(&action));
    OnAssertExhaustionCommand(&action);
  } else {
    PrintCommandError("unknown command type");
    return wabt::Result::Error;
  }
  EXPECT("}");
  return wabt::Result::Ok;
}

wabt::Result SpecJSONParser::ParseCommands() {
  EXPECT("{");
  PARSE_KEY_STRING_VALUE("source_filename", &source_filename_);
  EXPECT(",");
  EXPECT_KEY("commands");
  EXPECT("[");
  bool first = true;
  while (!Match("]")) {
    if (!first)
      EXPECT(",");
    CHECK_RESULT(ParseCommand());
    first = false;
  }
  EXPECT("}");
  return wabt::Result::Ok;
}

static wabt::Result ReadAndRunSpecJSON(const char* spec_json_filename) {
  SpecJSONParser parser;
  CHECK_RESULT(parser.ReadFile(spec_json_filename));
  wabt::Result result = parser.ParseCommands();
  printf("%d/%d tests passed.\n", parser.passed(), parser.total());
  return result;
}

int ProgramMain(int argc, char** argv) {
  InitStdio();
  s_stdout_stream = FileStream::CreateStdout();

  ParseOptions(argc, argv);

  wabt::Result result;
  result = ReadAndRunSpecJSON(s_infile);
  return result != wabt::Result::Ok;
}

int main(int argc, char** argv) {
  WABT_TRY
  return ProgramMain(argc, argv);
  WABT_CATCH_BAD_ALLOC_AND_EXIT
}
