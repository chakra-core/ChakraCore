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

#include "src/binary-reader.h"
#include "src/cast.h"
#include "src/error-formatter.h"
#include "src/feature.h"
#include "src/interp/binary-reader-interp.h"
#include "src/interp/interp.h"
#include "src/literal.h"
#include "src/option-parser.h"
#include "src/resolve-names.h"
#include "src/stream.h"
#include "src/validator.h"
#include "src/wast-lexer.h"
#include "src/wast-parser.h"

using namespace wabt;
using namespace wabt::interp;

static int s_verbose;
static const char* s_infile;
static Thread::Options s_thread_options;
static Stream* s_trace_stream;
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
  parser.AddOption('t', "trace", "Trace execution",
                   []() { s_trace_stream = s_stdout_stream.get(); });

  parser.AddArgument("filename", OptionParser::ArgumentCount::One,
                     [](const char* argument) { s_infile = argument; });
  parser.Parse(argc, argv);
}

namespace spectest {

class Command;
typedef std::unique_ptr<Command> CommandPtr;
typedef std::vector<CommandPtr> CommandPtrVector;

class Script {
 public:
  std::string filename;
  CommandPtrVector commands;
};

class Command {
 public:
  WABT_DISALLOW_COPY_AND_ASSIGN(Command);
  Command() = delete;
  virtual ~Command() = default;

  CommandType type;
  uint32_t line = 0;

 protected:
  explicit Command(CommandType type) : type(type) {}
};

template <CommandType TypeEnum>
class CommandMixin : public Command {
 public:
  static bool classof(const Command* cmd) { return cmd->type == TypeEnum; }
  CommandMixin() : Command(TypeEnum) {}
};

enum class ModuleType {
  Text,
  Binary,
};

class ModuleCommand : public CommandMixin<CommandType::Module> {
 public:
  ModuleType module = ModuleType::Binary;
  std::string filename;
  std::string name;
};

class Action {
 public:
  ActionType type = ActionType::Invoke;
  std::string module_name;
  std::string field_name;
  TypedValues args;
};

template <CommandType TypeEnum>
class ActionCommandBase : public CommandMixin<TypeEnum> {
 public:
  Action action;
};

typedef ActionCommandBase<CommandType::Action> ActionCommand;
typedef ActionCommandBase<CommandType::AssertReturnCanonicalNan>
    AssertReturnCanonicalNanCommand;
typedef ActionCommandBase<CommandType::AssertReturnArithmeticNan>
    AssertReturnArithmeticNanCommand;

class RegisterCommand : public CommandMixin<CommandType::Register> {
 public:
  std::string as;
  std::string name;
};

class AssertReturnCommand : public CommandMixin<CommandType::AssertReturn> {
 public:
  Action action;
  TypedValues expected;
};

template <CommandType TypeEnum>
class AssertTrapCommandBase : public CommandMixin<TypeEnum> {
 public:
  Action action;
  std::string text;
};

typedef AssertTrapCommandBase<CommandType::AssertTrap> AssertTrapCommand;
typedef AssertTrapCommandBase<CommandType::AssertExhaustion>
    AssertExhaustionCommand;

template <CommandType TypeEnum>
class AssertModuleCommand : public CommandMixin<TypeEnum> {
 public:
  ModuleType type = ModuleType::Binary;
  std::string filename;
  std::string text;
};

typedef AssertModuleCommand<CommandType::AssertMalformed>
    AssertMalformedCommand;
typedef AssertModuleCommand<CommandType::AssertInvalid> AssertInvalidCommand;
typedef AssertModuleCommand<CommandType::AssertUnlinkable>
    AssertUnlinkableCommand;
typedef AssertModuleCommand<CommandType::AssertUninstantiable>
    AssertUninstantiableCommand;

// An extremely simple JSON parser that only knows how to parse the expected
// format from wat2wasm.
class JSONParser {
 public:
  JSONParser() {}

  wabt::Result ReadFile(string_view spec_json_filename);
  wabt::Result ParseScript(Script* out_script);

 private:
  void WABT_PRINTF_FORMAT(2, 3) PrintError(const char* format, ...);

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
  wabt::Result ParseLine(uint32_t* out_line_number);
  wabt::Result ParseTypeObject(Type* out_type);
  wabt::Result ParseTypeVector(TypeVector* out_types);
  wabt::Result ParseConst(TypedValue* out_value);
  wabt::Result ParseConstVector(TypedValues* out_values);
  wabt::Result ParseAction(Action* out_action);
  wabt::Result ParseActionResult();
  wabt::Result ParseModuleType(ModuleType* out_type);

  std::string CreateModulePath(string_view filename);
  wabt::Result ParseFilename(std::string* out_filename);
  wabt::Result ParseCommand(CommandPtr* out_command);

  // Parsing info.
  std::vector<uint8_t> json_data_;
  size_t json_offset_ = 0;
  Location loc_;
  Location prev_loc_;
  bool has_prev_loc_ = false;
};

#define EXPECT(x) CHECK_RESULT(Expect(x))
#define EXPECT_KEY(x) CHECK_RESULT(ExpectKey(x))
#define PARSE_KEY_STRING_VALUE(key, value) \
  CHECK_RESULT(ParseKeyStringValue(key, value))

wabt::Result JSONParser::ReadFile(string_view spec_json_filename) {
  loc_.filename = spec_json_filename;
  loc_.line = 1;
  loc_.first_column = 1;

  return wabt::ReadFile(spec_json_filename, &json_data_);
}

void JSONParser::PrintError(const char* format, ...) {
  WABT_SNPRINTF_ALLOCA(buffer, length, format);
  fprintf(stderr, "%s:%d:%d: %s\n", loc_.filename.to_string().c_str(),
          loc_.line, loc_.first_column, buffer);
}

void JSONParser::PutbackChar() {
  assert(has_prev_loc_);
  json_offset_--;
  loc_ = prev_loc_;
  has_prev_loc_ = false;
}

int JSONParser::ReadChar() {
  if (json_offset_ >= json_data_.size()) {
    return -1;
  }
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

void JSONParser::SkipWhitespace() {
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

bool JSONParser::Match(const char* s) {
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

wabt::Result JSONParser::Expect(const char* s) {
  if (Match(s)) {
    return wabt::Result::Ok;
  } else {
    PrintError("expected %s", s);
    return wabt::Result::Error;
  }
}

wabt::Result JSONParser::ExpectKey(const char* key) {
  size_t keylen = strlen(key);
  size_t quoted_len = keylen + 2 + 1;
  char* quoted = static_cast<char*>(alloca(quoted_len));
  snprintf(quoted, quoted_len, "\"%s\"", key);
  EXPECT(quoted);
  EXPECT(":");
  return wabt::Result::Ok;
}

wabt::Result JSONParser::ParseUint32(uint32_t* out_int) {
  uint32_t result = 0;
  SkipWhitespace();
  while (1) {
    int c = ReadChar();
    if (c >= '0' && c <= '9') {
      uint32_t last_result = result;
      result = result * 10 + static_cast<uint32_t>(c - '0');
      if (result < last_result) {
        PrintError("uint32 overflow");
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

wabt::Result JSONParser::ParseString(std::string* out_string) {
  out_string->clear();

  SkipWhitespace();
  if (ReadChar() != '"') {
    PrintError("expected string");
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
        PrintError("expected escape: \\uxxxx");
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
          PrintError("expected hex char");
          return wabt::Result::Error;
        }
        code = (code << 4) + cval;
      }

      if (code < 256) {
        *out_string += code;
      } else {
        PrintError("only escape codes < 256 allowed, got %u\n", code);
      }
    } else {
      *out_string += c;
    }
  }
  return wabt::Result::Ok;
}

wabt::Result JSONParser::ParseKeyStringValue(const char* key,
                                             std::string* out_string) {
  out_string->clear();
  EXPECT_KEY(key);
  return ParseString(out_string);
}

wabt::Result JSONParser::ParseOptNameStringValue(std::string* out_string) {
  out_string->clear();
  if (Match("\"name\"")) {
    EXPECT(":");
    CHECK_RESULT(ParseString(out_string));
    EXPECT(",");
  }
  return wabt::Result::Ok;
}

wabt::Result JSONParser::ParseLine(uint32_t* out_line_number) {
  EXPECT_KEY("line");
  CHECK_RESULT(ParseUint32(out_line_number));
  return wabt::Result::Ok;
}

wabt::Result JSONParser::ParseTypeObject(Type* out_type) {
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
    PrintError("unknown type: \"%s\"", type_str.c_str());
    return wabt::Result::Error;
  }
}

wabt::Result JSONParser::ParseTypeVector(TypeVector* out_types) {
  out_types->clear();
  EXPECT("[");
  bool first = true;
  while (!Match("]")) {
    if (!first) {
      EXPECT(",");
    }
    Type type;
    CHECK_RESULT(ParseTypeObject(&type));
    first = false;
    out_types->push_back(type);
  }
  return wabt::Result::Ok;
}

wabt::Result JSONParser::ParseConst(TypedValue* out_value) {
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
    PrintError("unknown type: \"%s\"", type_str.c_str());
    return wabt::Result::Error;
  }
}

wabt::Result JSONParser::ParseConstVector(TypedValues* out_values) {
  out_values->clear();
  EXPECT("[");
  bool first = true;
  while (!Match("]")) {
    if (!first) {
      EXPECT(",");
    }
    TypedValue value;
    CHECK_RESULT(ParseConst(&value));
    out_values->push_back(value);
    first = false;
  }
  return wabt::Result::Ok;
}

wabt::Result JSONParser::ParseAction(Action* out_action) {
  EXPECT_KEY("action");
  EXPECT("{");
  EXPECT_KEY("type");
  if (Match("\"invoke\"")) {
    out_action->type = ActionType::Invoke;
  } else {
    EXPECT("\"get\"");
    out_action->type = ActionType::Get;
  }
  EXPECT(",");
  if (Match("\"module\"")) {
    EXPECT(":");
    CHECK_RESULT(ParseString(&out_action->module_name));
    EXPECT(",");
  }
  PARSE_KEY_STRING_VALUE("field", &out_action->field_name);
  if (out_action->type == ActionType::Invoke) {
    EXPECT(",");
    EXPECT_KEY("args");
    CHECK_RESULT(ParseConstVector(&out_action->args));
  }
  EXPECT("}");
  return wabt::Result::Ok;
}

wabt::Result JSONParser::ParseActionResult() {
  // Not needed for wabt-interp, but useful for other parsers.
  EXPECT_KEY("expected");
  TypeVector expected;
  CHECK_RESULT(ParseTypeVector(&expected));
  return wabt::Result::Ok;
}

wabt::Result JSONParser::ParseModuleType(ModuleType* out_type) {
  std::string module_type_str;

  PARSE_KEY_STRING_VALUE("module_type", &module_type_str);
  if (module_type_str == "text") {
    *out_type = ModuleType::Text;
    return wabt::Result::Ok;
  } else if (module_type_str == "binary") {
    *out_type = ModuleType::Binary;
    return wabt::Result::Ok;
  } else {
    PrintError("unknown module type: \"%s\"", module_type_str.c_str());
    return wabt::Result::Error;
  }
}

static string_view GetDirname(string_view path) {
  // Strip everything after and including the last slash (or backslash), e.g.:
  //
  // s = "foo/bar/baz", => "foo/bar"
  // s = "/usr/local/include/stdio.h", => "/usr/local/include"
  // s = "foo.bar", => ""
  // s = "some\windows\directory", => "some\windows"
  size_t last_slash = path.find_last_of('/');
  size_t last_backslash = path.find_last_of('\\');
  if (last_slash == string_view::npos) {
    last_slash = 0;
  }
  if (last_backslash == string_view::npos) {
    last_backslash = 0;
  }

  return path.substr(0, std::max(last_slash, last_backslash));
}

std::string JSONParser::CreateModulePath(string_view filename) {
  string_view spec_json_filename = loc_.filename;
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

wabt::Result JSONParser::ParseFilename(std::string* out_filename) {
  PARSE_KEY_STRING_VALUE("filename", out_filename);
  *out_filename = CreateModulePath(*out_filename);
  return wabt::Result::Ok;
}

wabt::Result JSONParser::ParseCommand(CommandPtr* out_command) {
  EXPECT("{");
  EXPECT_KEY("type");
  if (Match("\"module\"")) {
    auto command = MakeUnique<ModuleCommand>();
    EXPECT(",");
    CHECK_RESULT(ParseLine(&command->line));
    EXPECT(",");
    CHECK_RESULT(ParseOptNameStringValue(&command->name));
    CHECK_RESULT(ParseFilename(&command->filename));
    *out_command = std::move(command);
  } else if (Match("\"action\"")) {
    auto command = MakeUnique<ActionCommand>();
    EXPECT(",");
    CHECK_RESULT(ParseLine(&command->line));
    EXPECT(",");
    CHECK_RESULT(ParseAction(&command->action));
    EXPECT(",");
    CHECK_RESULT(ParseActionResult());
    *out_command = std::move(command);
  } else if (Match("\"register\"")) {
    auto command = MakeUnique<RegisterCommand>();
    EXPECT(",");
    CHECK_RESULT(ParseLine(&command->line));
    EXPECT(",");
    CHECK_RESULT(ParseOptNameStringValue(&command->name));
    PARSE_KEY_STRING_VALUE("as", &command->as);
    *out_command = std::move(command);
  } else if (Match("\"assert_malformed\"")) {
    auto command = MakeUnique<AssertMalformedCommand>();
    EXPECT(",");
    CHECK_RESULT(ParseLine(&command->line));
    EXPECT(",");
    CHECK_RESULT(ParseFilename(&command->filename));
    EXPECT(",");
    PARSE_KEY_STRING_VALUE("text", &command->text);
    EXPECT(",");
    CHECK_RESULT(ParseModuleType(&command->type));
    *out_command = std::move(command);
  } else if (Match("\"assert_invalid\"")) {
    auto command = MakeUnique<AssertInvalidCommand>();
    EXPECT(",");
    CHECK_RESULT(ParseLine(&command->line));
    EXPECT(",");
    CHECK_RESULT(ParseFilename(&command->filename));
    EXPECT(",");
    PARSE_KEY_STRING_VALUE("text", &command->text);
    EXPECT(",");
    CHECK_RESULT(ParseModuleType(&command->type));
    *out_command = std::move(command);
  } else if (Match("\"assert_unlinkable\"")) {
    auto command = MakeUnique<AssertUnlinkableCommand>();
    EXPECT(",");
    CHECK_RESULT(ParseLine(&command->line));
    EXPECT(",");
    CHECK_RESULT(ParseFilename(&command->filename));
    EXPECT(",");
    PARSE_KEY_STRING_VALUE("text", &command->text);
    EXPECT(",");
    CHECK_RESULT(ParseModuleType(&command->type));
    *out_command = std::move(command);
  } else if (Match("\"assert_uninstantiable\"")) {
    auto command = MakeUnique<AssertUninstantiableCommand>();
    EXPECT(",");
    CHECK_RESULT(ParseLine(&command->line));
    EXPECT(",");
    CHECK_RESULT(ParseFilename(&command->filename));
    EXPECT(",");
    PARSE_KEY_STRING_VALUE("text", &command->text);
    EXPECT(",");
    CHECK_RESULT(ParseModuleType(&command->type));
    *out_command = std::move(command);
  } else if (Match("\"assert_return\"")) {
    auto command = MakeUnique<AssertReturnCommand>();
    EXPECT(",");
    CHECK_RESULT(ParseLine(&command->line));
    EXPECT(",");
    CHECK_RESULT(ParseAction(&command->action));
    EXPECT(",");
    EXPECT_KEY("expected");
    CHECK_RESULT(ParseConstVector(&command->expected));
    *out_command = std::move(command);
  } else if (Match("\"assert_return_canonical_nan\"")) {
    auto command = MakeUnique<AssertReturnCanonicalNanCommand>();
    EXPECT(",");
    CHECK_RESULT(ParseLine(&command->line));
    EXPECT(",");
    CHECK_RESULT(ParseAction(&command->action));
    EXPECT(",");
    CHECK_RESULT(ParseActionResult());
    *out_command = std::move(command);
  } else if (Match("\"assert_return_arithmetic_nan\"")) {
    auto command = MakeUnique<AssertReturnArithmeticNanCommand>();
    EXPECT(",");
    CHECK_RESULT(ParseLine(&command->line));
    EXPECT(",");
    CHECK_RESULT(ParseAction(&command->action));
    EXPECT(",");
    CHECK_RESULT(ParseActionResult());
    *out_command = std::move(command);
  } else if (Match("\"assert_trap\"")) {
    auto command = MakeUnique<AssertTrapCommand>();
    EXPECT(",");
    CHECK_RESULT(ParseLine(&command->line));
    EXPECT(",");
    CHECK_RESULT(ParseAction(&command->action));
    EXPECT(",");
    PARSE_KEY_STRING_VALUE("text", &command->text);
    EXPECT(",");
    CHECK_RESULT(ParseActionResult());
    *out_command = std::move(command);
  } else if (Match("\"assert_exhaustion\"")) {
    auto command = MakeUnique<AssertExhaustionCommand>();
    EXPECT(",");
    CHECK_RESULT(ParseLine(&command->line));
    EXPECT(",");
    CHECK_RESULT(ParseAction(&command->action));
    EXPECT(",");
    CHECK_RESULT(ParseActionResult());
    *out_command = std::move(command);
  } else {
    PrintError("unknown command type");
    return wabt::Result::Error;
  }
  EXPECT("}");
  return wabt::Result::Ok;
}

wabt::Result JSONParser::ParseScript(Script* out_script) {
  EXPECT("{");
  PARSE_KEY_STRING_VALUE("source_filename", &out_script->filename);
  EXPECT(",");
  EXPECT_KEY("commands");
  EXPECT("[");
  bool first = true;
  while (!Match("]")) {
    CommandPtr command;
    if (!first) {
      EXPECT(",");
    }
    CHECK_RESULT(ParseCommand(&command));
    out_script->commands.push_back(std::move(command));
    first = false;
  }
  EXPECT("}");
  return wabt::Result::Ok;
}

class CommandRunner {
 public:
  CommandRunner();

  wabt::Result Run(const Script& script);

  int passed() const { return passed_; }
  int total() const { return total_; }

 private:
  void WABT_PRINTF_FORMAT(3, 4)
      PrintError(uint32_t line_number, const char* format, ...);
  ExecResult RunAction(int line_number,
                       const Action* action,
                       RunVerbosity verbose);

  wabt::Result OnModuleCommand(const ModuleCommand*);
  wabt::Result OnActionCommand(const ActionCommand*);
  wabt::Result OnRegisterCommand(const RegisterCommand*);
  wabt::Result OnAssertMalformedCommand(const AssertMalformedCommand*);
  wabt::Result OnAssertUnlinkableCommand(const AssertUnlinkableCommand*);
  wabt::Result OnAssertInvalidCommand(const AssertInvalidCommand*);
  wabt::Result OnAssertUninstantiableCommand(
      const AssertUninstantiableCommand*);
  wabt::Result OnAssertReturnCommand(const AssertReturnCommand*);
  template <typename NanCommand>
  wabt::Result OnAssertReturnNanCommand(const NanCommand*);
  wabt::Result OnAssertTrapCommand(const AssertTrapCommand*);
  wabt::Result OnAssertExhaustionCommand(const AssertExhaustionCommand*);

  void TallyCommand(wabt::Result);

  wabt::Result ReadInvalidTextModule(string_view module_filename,
                                     Environment* env,
                                     const std::string& header);
  wabt::Result ReadInvalidModule(int line_number,
                                 string_view module_filename,
                                 Environment* env,
                                 ModuleType module_type,
                                 const char* desc);

  Environment env_;
  Executor executor_;
  DefinedModule* last_module_ = nullptr;
  int passed_ = 0;
  int total_ = 0;

  std::string source_filename_;
};

static interp::Result PrintCallback(const HostFunc* func,
                                    const interp::FuncSignature* sig,
                                    const TypedValues& args,
                                    TypedValues& results) {
  printf("called host ");
  WriteCall(s_stdout_stream.get(), func->module_name, func->field_name, args,
            results, interp::Result::Ok);
  return interp::Result::Ok;
}

static void InitEnvironment(Environment* env) {
  HostModule* host_module = env->AppendHostModule("spectest");
  host_module->AppendFuncExport("print", {{}, {}}, PrintCallback);
  host_module->AppendFuncExport("print_i32", {{Type::I32}, {}}, PrintCallback);
  host_module->AppendFuncExport("print_f32", {{Type::F32}, {}}, PrintCallback);
  host_module->AppendFuncExport("print_f64", {{Type::F64}, {}}, PrintCallback);
  host_module->AppendFuncExport("print_i32_f32", {{Type::I32, Type::F32}, {}},
                                PrintCallback);
  host_module->AppendFuncExport("print_f64_f64", {{Type::F64, Type::F64}, {}},
                                PrintCallback);

  host_module->AppendTableExport("table", Limits(10, 20));
  host_module->AppendMemoryExport("memory", Limits(1, 2));

  host_module->AppendGlobalExport("global_i32", false, uint32_t(666));
  host_module->AppendGlobalExport("global_i64", false, uint64_t(666));
  host_module->AppendGlobalExport("global_f32", false, float(666.6f));
  host_module->AppendGlobalExport("global_f64", false, double(666.6));
}

CommandRunner::CommandRunner()
    : executor_(&env_, s_trace_stream, s_thread_options) {
  InitEnvironment(&env_);
}

wabt::Result CommandRunner::Run(const Script& script) {
  source_filename_ = script.filename;

  for (const CommandPtr& command : script.commands) {
    switch (command->type) {
      case CommandType::Module:
        OnModuleCommand(cast<ModuleCommand>(command.get()));
        break;

      case CommandType::Action:
        TallyCommand(OnActionCommand(cast<ActionCommand>(command.get())));
        break;

      case CommandType::Register:
        OnRegisterCommand(cast<RegisterCommand>(command.get()));
        break;

      case CommandType::AssertMalformed:
        TallyCommand(OnAssertMalformedCommand(
            cast<AssertMalformedCommand>(command.get())));
        break;

      case CommandType::AssertInvalid:
        TallyCommand(
            OnAssertInvalidCommand(cast<AssertInvalidCommand>(command.get())));
        break;

      case CommandType::AssertUnlinkable:
        TallyCommand(OnAssertUnlinkableCommand(
            cast<AssertUnlinkableCommand>(command.get())));
        break;

      case CommandType::AssertUninstantiable:
        TallyCommand(OnAssertUninstantiableCommand(
            cast<AssertUninstantiableCommand>(command.get())));
        break;

      case CommandType::AssertReturn:
        TallyCommand(
            OnAssertReturnCommand(cast<AssertReturnCommand>(command.get())));
        break;

      case CommandType::AssertReturnCanonicalNan:
        TallyCommand(OnAssertReturnNanCommand(
            cast<AssertReturnCanonicalNanCommand>(command.get())));
        break;

      case CommandType::AssertReturnArithmeticNan:
        TallyCommand(OnAssertReturnNanCommand(
            cast<AssertReturnArithmeticNanCommand>(command.get())));
        break;

      case CommandType::AssertTrap:
        TallyCommand(
            OnAssertTrapCommand(cast<AssertTrapCommand>(command.get())));
        break;

      case CommandType::AssertExhaustion:
        TallyCommand(OnAssertExhaustionCommand(
            cast<AssertExhaustionCommand>(command.get())));
        break;
    }
  }

  return wabt::Result::Ok;
}

void CommandRunner::PrintError(uint32_t line_number, const char* format, ...) {
  WABT_SNPRINTF_ALLOCA(buffer, length, format);
  printf("%s:%u: %s\n", source_filename_.c_str(), line_number, buffer);
}

static ExecResult GetGlobalExportByName(Environment* env,
                                        interp::Module* module,
                                        string_view name) {
  interp::Export* export_ = module->GetExport(name);
  if (!export_) {
    return ExecResult(interp::Result::UnknownExport);
  }
  if (export_->kind != ExternalKind::Global) {
    return ExecResult(interp::Result::ExportKindMismatch);
  }

  interp::Global* global = env->GetGlobal(export_->index);
  return ExecResult(interp::Result::Ok, {global->typed_value});
}

ExecResult CommandRunner::RunAction(int line_number,
                                    const Action* action,
                                    RunVerbosity verbose) {
  interp::Module* module;
  if (!action->module_name.empty()) {
    module = env_.FindModule(action->module_name);
  } else {
    module = env_.GetLastModule();
  }
  assert(module);

  ExecResult exec_result;

  switch (action->type) {
    case ActionType::Invoke:
      exec_result =
          executor_.RunExportByName(module, action->field_name, action->args);
      if (verbose == RunVerbosity::Verbose) {
        WriteCall(s_stdout_stream.get(), string_view(), action->field_name,
                  action->args, exec_result.values, exec_result.result);
      }
      break;

    case ActionType::Get:
      exec_result = GetGlobalExportByName(&env_, module, action->field_name);
      break;

    default:
      WABT_UNREACHABLE;
  }

  return exec_result;
}

wabt::Result CommandRunner::ReadInvalidTextModule(string_view module_filename,
                                                  Environment* env,
                                                  const std::string& header) {
  std::unique_ptr<WastLexer> lexer =
      WastLexer::CreateFileLexer(module_filename);
  Errors errors;
  std::unique_ptr<::Script> script;
  wabt::Result result = ParseWastScript(lexer.get(), &script, &errors);
  if (Succeeded(result)) {
    wabt::Module* module = script->GetFirstModule();
    result = ResolveNamesModule(module, &errors);
    if (Succeeded(result)) {
      ValidateOptions options(s_features);
      // Don't do a full validation, just validate the function signatures.
      result = ValidateFuncSignatures(module, &errors, options);
    }
  }

  auto line_finder = lexer->MakeLineFinder();
  FormatErrorsToFile(errors, Location::Type::Text, line_finder.get(), stdout,
                     header, PrintHeader::Once);
  return result;
}

static wabt::Result ReadModule(string_view module_filename,
                               Environment* env,
                               Errors* errors,
                               DefinedModule** out_module) {
  wabt::Result result;
  std::vector<uint8_t> file_data;

  *out_module = nullptr;

  result = ReadFile(module_filename, &file_data);
  if (Succeeded(result)) {
    const bool kReadDebugNames = true;
    const bool kStopOnFirstError = true;
    const bool kFailOnCustomSectionError = true;
    ReadBinaryOptions options(s_features, s_log_stream.get(), kReadDebugNames,
                              kStopOnFirstError, kFailOnCustomSectionError);
    result = ReadBinaryInterp(env, file_data.data(), file_data.size(), options,
                              errors, out_module);

    if (Succeeded(result)) {
      if (s_verbose) {
        env->DisassembleModule(s_stdout_stream.get(), *out_module);
      }
    }
  }
  return result;
}

wabt::Result CommandRunner::ReadInvalidModule(int line_number,
                                              string_view module_filename,
                                              Environment* env,
                                              ModuleType module_type,
                                              const char* desc) {
  std::string header = StringPrintf(
      "%s:%d: %s passed", source_filename_.c_str(), line_number, desc);

  switch (module_type) {
    case ModuleType::Text: {
      return ReadInvalidTextModule(module_filename, env, header);
    }

    case ModuleType::Binary: {
      DefinedModule* module;
      Errors errors;
      wabt::Result result = ReadModule(module_filename, env, &errors, &module);
      FormatErrorsToFile(errors, Location::Type::Binary, {}, stdout, header,
                         PrintHeader::Once);
      return result;
    }
  }

  WABT_UNREACHABLE;
}

wabt::Result CommandRunner::OnModuleCommand(const ModuleCommand* command) {
  Environment::MarkPoint mark = env_.Mark();
  Errors errors;
  wabt::Result result = ReadModule(command->filename, &env_,
                                   &errors, &last_module_);
  FormatErrorsToFile(errors, Location::Type::Binary);

  if (Failed(result)) {
    env_.ResetToMarkPoint(mark);
    PrintError(command->line, "error reading module: \"%s\"",
               command->filename.c_str());
    return wabt::Result::Error;
  }

  ExecResult exec_result = executor_.RunStartFunction(last_module_);
  if (exec_result.result != interp::Result::Ok) {
    env_.ResetToMarkPoint(mark);
    WriteResult(s_stdout_stream.get(), "error running start function",
                exec_result.result);
    return wabt::Result::Error;
  }

  if (!command->name.empty()) {
    last_module_->name = command->name;
    env_.EmplaceModuleBinding(command->name,
                              Binding(env_.GetModuleCount() - 1));
  }

  return wabt::Result::Ok;
}

wabt::Result CommandRunner::OnActionCommand(const ActionCommand* command) {
  ExecResult exec_result =
      RunAction(command->line, &command->action, RunVerbosity::Verbose);

  if (exec_result.result != interp::Result::Ok) {
    PrintError(command->line, "unexpected trap: %s",
               ResultToString(exec_result.result));
    return wabt::Result::Error;
  }

  return wabt::Result::Ok;
}

wabt::Result CommandRunner::OnAssertMalformedCommand(
    const AssertMalformedCommand* command) {
  Environment env;
  InitEnvironment(&env);

  wabt::Result result =
      ReadInvalidModule(command->line, command->filename, &env, command->type,
                        "assert_malformed");
  if (Succeeded(result)) {
    PrintError(command->line, "expected module to be malformed: \"%s\"",
               command->filename.c_str());
    return wabt::Result::Error;
  }

  return wabt::Result::Ok;
}

wabt::Result CommandRunner::OnRegisterCommand(const RegisterCommand* command) {
  Index module_index;
  if (!command->name.empty()) {
    module_index = env_.FindModuleIndex(command->name);
  } else {
    module_index = env_.GetLastModuleIndex();
  }

  if (module_index == kInvalidIndex) {
    PrintError(command->line, "unknown module in register");
    return wabt::Result::Error;
  }

  env_.EmplaceRegisteredModuleBinding(command->as, Binding(module_index));
  return wabt::Result::Ok;
}

wabt::Result CommandRunner::OnAssertUnlinkableCommand(
    const AssertUnlinkableCommand* command) {
  Environment::MarkPoint mark = env_.Mark();
  wabt::Result result =
      ReadInvalidModule(command->line, command->filename, &env_, command->type,
                        "assert_unlinkable");
  env_.ResetToMarkPoint(mark);

  if (Succeeded(result)) {
    PrintError(command->line, "expected module to be unlinkable: \"%s\"",
               command->filename.c_str());
    return wabt::Result::Error;
  }

  return wabt::Result::Ok;
}

wabt::Result CommandRunner::OnAssertInvalidCommand(
    const AssertInvalidCommand* command) {
  Environment env;
  InitEnvironment(&env);

  wabt::Result result = ReadInvalidModule(
      command->line, command->filename, &env, command->type, "assert_invalid");
  if (Succeeded(result)) {
    PrintError(command->line, "expected module to be invalid: \"%s\"",
               command->filename.c_str());
    return wabt::Result::Error;
  }

  return wabt::Result::Ok;
}

wabt::Result CommandRunner::OnAssertUninstantiableCommand(
    const AssertUninstantiableCommand* command) {
  Errors errors;
  DefinedModule* module;
  Environment::MarkPoint mark = env_.Mark();
  wabt::Result result = ReadModule(command->filename, &env_, &errors, &module);
  FormatErrorsToFile(errors, Location::Type::Binary);

  if (Succeeded(result)) {
    ExecResult exec_result = executor_.RunStartFunction(module);
    if (exec_result.result == interp::Result::Ok) {
      PrintError(command->line, "expected error running start function: \"%s\"",
                 command->filename.c_str());
      result = wabt::Result::Error;
    } else {
      result = wabt::Result::Ok;
    }
  } else {
    PrintError(command->line, "error reading module: \"%s\"",
               command->filename.c_str());
    result = wabt::Result::Error;
  }

  env_.ResetToMarkPoint(mark);
  return result;
}

static bool TypedValuesAreEqual(const TypedValue& tv1, const TypedValue& tv2) {
  if (tv1.type != tv2.type) {
    return false;
  }

  switch (tv1.type) {
    case Type::I32:
      return tv1.value.i32 == tv2.value.i32;
    case Type::F32:
      return tv1.value.f32_bits == tv2.value.f32_bits;
    case Type::I64:
      return tv1.value.i64 == tv2.value.i64;
    case Type::F64:
      return tv1.value.f64_bits == tv2.value.f64_bits;
    default:
      WABT_UNREACHABLE;
  }
}

wabt::Result CommandRunner::OnAssertReturnCommand(
    const AssertReturnCommand* command) {
  ExecResult exec_result =
      RunAction(command->line, &command->action, RunVerbosity::Quiet);

  if (exec_result.result != interp::Result::Ok) {
    PrintError(command->line, "unexpected trap: %s",
               ResultToString(exec_result.result));
    return wabt::Result::Error;
  }

  if (exec_result.values.size() != command->expected.size()) {
    PrintError(command->line,
               "result length mismatch in assert_return: expected %" PRIzd
               ", got %" PRIzd,
               command->expected.size(), exec_result.values.size());
    return wabt::Result::Error;
  }

  wabt::Result result = wabt::Result::Ok;
  for (size_t i = 0; i < exec_result.values.size(); ++i) {
    const TypedValue& expected_tv = command->expected[i];
    const TypedValue& actual_tv = exec_result.values[i];
    if (!TypedValuesAreEqual(expected_tv, actual_tv)) {
      PrintError(command->line,
                 "mismatch in result %" PRIzd
                 " of assert_return: expected %s, got %s",
                 i, TypedValueToString(expected_tv).c_str(),
                 TypedValueToString(actual_tv).c_str());
      result = wabt::Result::Error;
    }
  }

  return result;
}

template <typename NanCommand>
wabt::Result CommandRunner::OnAssertReturnNanCommand(
    const NanCommand* command) {
  ExecResult exec_result =
      RunAction(command->line, &command->action, RunVerbosity::Quiet);

  if (exec_result.result != interp::Result::Ok) {
    PrintError(command->line, "unexpected trap: %s",
               ResultToString(exec_result.result));
    return wabt::Result::Error;
  }

  if (exec_result.values.size() != 1) {
    PrintError(command->line, "expected one result, got %" PRIzd,
               exec_result.values.size());
    return wabt::Result::Error;
  }

  const bool is_canonical =
      command->type == CommandType::AssertReturnCanonicalNan;

  const TypedValue& actual = exec_result.values[0];
  switch (actual.type) {
    case Type::F32: {
      bool is_nan = is_canonical ? IsCanonicalNan(actual.value.f32_bits)
                                 : IsArithmeticNan(actual.value.f32_bits);
      if (!is_nan) {
        PrintError(command->line, "expected result to be nan, got %s",
                   TypedValueToString(actual).c_str());
        return wabt::Result::Error;
      }
      break;
    }

    case Type::F64: {
      bool is_nan = is_canonical ? IsCanonicalNan(actual.value.f64_bits)
                                 : IsArithmeticNan(actual.value.f64_bits);
      if (!is_nan) {
        PrintError(command->line, "expected result to be nan, got %s",
                   TypedValueToString(actual).c_str());
        return wabt::Result::Error;
      }
      break;
    }

    default:
      PrintError(command->line, "expected result type to be f32 or f64, got %s",
                 GetTypeName(actual.type));
      return wabt::Result::Error;
  }

  return wabt::Result::Ok;
}

wabt::Result CommandRunner::OnAssertTrapCommand(
    const AssertTrapCommand* command) {
  ExecResult exec_result =
      RunAction(command->line, &command->action, RunVerbosity::Quiet);
  if (exec_result.result == interp::Result::Ok) {
    PrintError(command->line, "expected trap: \"%s\"", command->text.c_str());
    return wabt::Result::Error;
  }

  return wabt::Result::Ok;
}

wabt::Result CommandRunner::OnAssertExhaustionCommand(
    const AssertExhaustionCommand* command) {
  ExecResult exec_result =
      RunAction(command->line, &command->action, RunVerbosity::Quiet);
  if (exec_result.result != interp::Result::TrapCallStackExhausted &&
      exec_result.result != interp::Result::TrapValueStackExhausted) {
    PrintError(command->line, "expected call stack exhaustion");
    return wabt::Result::Error;
  }

  return wabt::Result::Ok;
}

void CommandRunner::TallyCommand(wabt::Result result) {
  if (Succeeded(result)) {
    passed_++;
  }
  total_++;
}

static wabt::Result ReadAndRunSpecJSON(string_view spec_json_filename) {
  JSONParser parser;
  CHECK_RESULT(parser.ReadFile(spec_json_filename));

  Script script;
  CHECK_RESULT(parser.ParseScript(&script));

  CommandRunner runner;
  wabt::Result result = runner.Run(script);

  printf("%d/%d tests passed.\n", runner.passed(), runner.total());
  return result;
}

}  // namespace spectest

int ProgramMain(int argc, char** argv) {
  InitStdio();
  s_stdout_stream = FileStream::CreateStdout();

  ParseOptions(argc, argv);

  wabt::Result result;
  result = spectest::ReadAndRunSpecJSON(s_infile);
  return result != wabt::Result::Ok;
}

int main(int argc, char** argv) {
  WABT_TRY
  return ProgramMain(argc, argv);
  WABT_CATCH_BAD_ALLOC_AND_EXIT
}
