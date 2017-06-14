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

#include "binary-writer-spec.h"

#include <cassert>
#include <cinttypes>

#include "binary.h"
#include "binary-writer.h"
#include "config.h"
#include "ir.h"
#include "stream.h"
#include "writer.h"

namespace wabt {

static void convert_backslash_to_slash(char* s, size_t length) {
  for (size_t i = 0; i < length; ++i)
    if (s[i] == '\\')
      s[i] = '/';
}

static StringSlice strip_extension(const char* s) {
  /* strip .json or .wasm, but leave other extensions, e.g.:
   *
   * s = "foo", => "foo"
   * s = "foo.json" => "foo"
   * s = "foo.wasm" => "foo"
   * s = "foo.bar" => "foo.bar"
   */
  if (!s) {
    StringSlice result;
    result.start = nullptr;
    result.length = 0;
    return result;
  }

  size_t slen = strlen(s);
  const char* ext_start = strrchr(s, '.');
  if (!ext_start)
    ext_start = s + slen;

  StringSlice result;
  result.start = s;

  if (strcmp(ext_start, ".json") == 0 || strcmp(ext_start, ".wasm") == 0) {
    result.length = ext_start - s;
  } else {
    result.length = slen;
  }
  return result;
}

static StringSlice get_basename(const char* s) {
  /* strip everything up to and including the last slash, e.g.:
   *
   * s = "/foo/bar/baz", => "baz"
   * s = "/usr/local/include/stdio.h", => "stdio.h"
   * s = "foo.bar", => "foo.bar"
   */
  size_t slen = strlen(s);
  const char* start = s;
  const char* last_slash = strrchr(s, '/');
  if (last_slash)
    start = last_slash + 1;

  StringSlice result;
  result.start = start;
  result.length = s + slen - start;
  return result;
}

namespace {

class BinaryWriterSpec {
 public:
  BinaryWriterSpec(const char* source_filename,
                   const WriteBinarySpecOptions* spec_options);

  Result WriteScript(Script* script);

 private:
  char* GetModuleFilename(const char* extension);
  void WriteString(const char* s);
  void WriteKey(const char* key);
  void WriteSeparator();
  void WriteEscapedStringSlice(StringSlice ss);
  void WriteCommandType(const Command& command);
  void WriteLocation(const Location* loc);
  void WriteVar(const Var* var);
  void WriteTypeObject(Type type);
  void WriteConst(const Const* const_);
  void WriteConstVector(const ConstVector& consts);
  void WriteAction(const Action* action);
  void WriteActionResultType(Script* script, const Action* action);
  void WriteModule(char* filename, const Module* module);
  void WriteScriptModule(char* filename, const ScriptModule* script_module);
  void WriteInvalidModule(const ScriptModule* module, StringSlice text);
  void WriteCommands(Script* script);

  MemoryStream json_stream_;
  StringSlice source_filename_;
  StringSlice module_filename_noext_;
  bool write_modules_ = false; /* Whether to write the modules files. */
  const WriteBinarySpecOptions* spec_options_ = nullptr;
  Result result_ = Result::Ok;
  size_t num_modules_ = 0;

  static const char* kWasmExtension;
  static const char* kWastExtension;
};

// static
const char* BinaryWriterSpec::kWasmExtension = ".wasm";
// static
const char* BinaryWriterSpec::kWastExtension = ".wast";

BinaryWriterSpec::BinaryWriterSpec(const char* source_filename,
                                   const WriteBinarySpecOptions* spec_options)
    : spec_options_(spec_options) {
  source_filename_.start = source_filename;
  source_filename_.length = strlen(source_filename);
  module_filename_noext_ = strip_extension(spec_options_->json_filename
                                               ? spec_options_->json_filename
                                               : source_filename);
  write_modules_ = !!spec_options_->json_filename;
}

char* BinaryWriterSpec::GetModuleFilename(const char* extension) {
  size_t buflen = module_filename_noext_.length + 20;
  char* str = new char[buflen];
  size_t length =
      wabt_snprintf(str, buflen, PRIstringslice ".%" PRIzd "%s",
                    WABT_PRINTF_STRING_SLICE_ARG(module_filename_noext_),
                    num_modules_, extension);
  convert_backslash_to_slash(str, length);
  return str;
}

void BinaryWriterSpec::WriteString(const char* s) {
  json_stream_.Writef("\"%s\"", s);
}

void BinaryWriterSpec::WriteKey(const char* key) {
  json_stream_.Writef("\"%s\": ", key);
}

void BinaryWriterSpec::WriteSeparator() {
  json_stream_.Writef(", ");
}

void BinaryWriterSpec::WriteEscapedStringSlice(StringSlice ss) {
  json_stream_.WriteChar('"');
  for (size_t i = 0; i < ss.length; ++i) {
    uint8_t c = ss.start[i];
    if (c < 0x20 || c == '\\' || c == '"' || c > 0x7f) {
      json_stream_.Writef("\\u%04x", c);
    } else {
      json_stream_.WriteChar(c);
    }
  }
  json_stream_.WriteChar('"');
}

void BinaryWriterSpec::WriteCommandType(const Command& command) {
  static const char* s_command_names[] = {
      "module",
      "action",
      "register",
      "assert_malformed",
      "assert_invalid",
      nullptr, /* ASSERT_INVALID_NON_BINARY, this command will never be
                  written */
      "assert_unlinkable",
      "assert_uninstantiable",
      "assert_return",
      "assert_return_canonical_nan",
      "assert_return_arithmetic_nan",
      "assert_trap",
      "assert_exhaustion",
  };
  WABT_STATIC_ASSERT(WABT_ARRAY_SIZE(s_command_names) == kCommandTypeCount);

  WriteKey("type");
  assert(s_command_names[static_cast<size_t>(command.type)]);
  WriteString(s_command_names[static_cast<size_t>(command.type)]);
}

void BinaryWriterSpec::WriteLocation(const Location* loc) {
  WriteKey("line");
  json_stream_.Writef("%d", loc->line);
}

void BinaryWriterSpec::WriteVar(const Var* var) {
  if (var->type == VarType::Index)
    json_stream_.Writef("\"%" PRIindex "\"", var->index);
  else
    WriteEscapedStringSlice(var->name);
}

void BinaryWriterSpec::WriteTypeObject(Type type) {
  json_stream_.Writef("{");
  WriteKey("type");
  WriteString(get_type_name(type));
  json_stream_.Writef("}");
}

void BinaryWriterSpec::WriteConst(const Const* const_) {
  json_stream_.Writef("{");
  WriteKey("type");

  /* Always write the values as strings, even though they may be representable
   * as JSON numbers. This way the formatting is consistent. */
  switch (const_->type) {
    case Type::I32:
      WriteString("i32");
      WriteSeparator();
      WriteKey("value");
      json_stream_.Writef("\"%u\"", const_->u32);
      break;

    case Type::I64:
      WriteString("i64");
      WriteSeparator();
      WriteKey("value");
      json_stream_.Writef("\"%" PRIu64 "\"", const_->u64);
      break;

    case Type::F32: {
      /* TODO(binji): write as hex float */
      WriteString("f32");
      WriteSeparator();
      WriteKey("value");
      json_stream_.Writef("\"%u\"", const_->f32_bits);
      break;
    }

    case Type::F64: {
      /* TODO(binji): write as hex float */
      WriteString("f64");
      WriteSeparator();
      WriteKey("value");
      json_stream_.Writef("\"%" PRIu64 "\"", const_->f64_bits);
      break;
    }

    default:
      assert(0);
  }

  json_stream_.Writef("}");
}

void BinaryWriterSpec::WriteConstVector(const ConstVector& consts) {
  json_stream_.Writef("[");
  for (size_t i = 0; i < consts.size(); ++i) {
    const Const* const_ = &consts[i];
    WriteConst(const_);
    if (i != consts.size() - 1)
      WriteSeparator();
  }
  json_stream_.Writef("]");
}

void BinaryWriterSpec::WriteAction(const Action* action) {
  WriteKey("action");
  json_stream_.Writef("{");
  WriteKey("type");
  if (action->type == ActionType::Invoke) {
    WriteString("invoke");
  } else {
    assert(action->type == ActionType::Get);
    WriteString("get");
  }
  WriteSeparator();
  if (action->module_var.type != VarType::Index) {
    WriteKey("module");
    WriteVar(&action->module_var);
    WriteSeparator();
  }
  if (action->type == ActionType::Invoke) {
    WriteKey("field");
    WriteEscapedStringSlice(action->name);
    WriteSeparator();
    WriteKey("args");
    WriteConstVector(action->invoke->args);
  } else {
    WriteKey("field");
    WriteEscapedStringSlice(action->name);
  }
  json_stream_.Writef("}");
}

void BinaryWriterSpec::WriteActionResultType(Script* script,
                                             const Action* action) {
  const Module* module = script->GetModule(action->module_var);
  const Export* export_;
  json_stream_.Writef("[");
  switch (action->type) {
    case ActionType::Invoke: {
      export_ = module->GetExport(action->name);
      assert(export_->kind == ExternalKind::Func);
      const Func* func = module->GetFunc(export_->var);
      Index num_results = func->GetNumResults();
      for (Index i = 0; i < num_results; ++i)
        WriteTypeObject(func->GetResultType(i));
      break;
    }

    case ActionType::Get: {
      export_ = module->GetExport(action->name);
      assert(export_->kind == ExternalKind::Global);
      const Global* global = module->GetGlobal(export_->var);
      WriteTypeObject(global->type);
      break;
    }
  }
  json_stream_.Writef("]");
}

void BinaryWriterSpec::WriteModule(char* filename, const Module* module) {
  MemoryStream memory_stream;
  result_ = write_binary_module(&memory_stream.writer(), module,
                                &spec_options_->write_binary_options);
  if (WABT_SUCCEEDED(result_) && write_modules_)
    result_ = memory_stream.WriteToFile(filename);
}

void BinaryWriterSpec::WriteScriptModule(char* filename,
                                         const ScriptModule* script_module) {
  switch (script_module->type) {
    case ScriptModule::Type::Text:
      WriteModule(filename, script_module->text);
      break;

    case ScriptModule::Type::Binary:
      if (write_modules_) {
        FileStream file_stream(filename);
        if (file_stream.is_open()) {
          file_stream.WriteData(script_module->binary.data,
                                script_module->binary.size, "");
          result_ = file_stream.result();
        } else {
          result_ = Result::Error;
        }
      }
      break;

    case ScriptModule::Type::Quoted:
      if (write_modules_) {
        FileStream file_stream(filename);
        if (file_stream.is_open()) {
          file_stream.WriteData(script_module->quoted.data,
                                script_module->quoted.size, "");
          result_ = file_stream.result();
        } else {
          result_ = Result::Error;
        }
      }
      break;
  }
}

void BinaryWriterSpec::WriteInvalidModule(const ScriptModule* module,
                                          StringSlice text) {
  const char* extension = "";
  const char* module_type = "";
  switch (module->type) {
    case ScriptModule::Type::Text:
      extension = kWasmExtension;
      module_type = "binary";
      break;

    case ScriptModule::Type::Binary:
      extension = kWasmExtension;
      module_type = "binary";
      break;

    case ScriptModule::Type::Quoted:
      extension = kWastExtension;
      module_type = "text";
      break;
  }

  WriteLocation(&module->GetLocation());
  WriteSeparator();
  char* filename = GetModuleFilename(extension);
  WriteKey("filename");
  WriteEscapedStringSlice(get_basename(filename));
  WriteSeparator();
  WriteKey("text");
  WriteEscapedStringSlice(text);
  WriteSeparator();
  WriteKey("module_type");
  WriteString(module_type);
  WriteScriptModule(filename, module);
  delete [] filename;
}

void BinaryWriterSpec::WriteCommands(Script* script) {
  json_stream_.Writef("{\"source_filename\": ");
  WriteEscapedStringSlice(source_filename_);
  json_stream_.Writef(",\n \"commands\": [\n");
  Index last_module_index = kInvalidIndex;
  for (size_t i = 0; i < script->commands.size(); ++i) {
    const Command& command = *script->commands[i].get();

    if (command.type == CommandType::AssertInvalidNonBinary)
      continue;

    if (i != 0) {
      WriteSeparator();
      json_stream_.Writef("\n");
    }

    json_stream_.Writef("  {");
    WriteCommandType(command);
    WriteSeparator();

    switch (command.type) {
      case CommandType::Module: {
        Module* module = command.module;
        char* filename = GetModuleFilename(kWasmExtension);
        WriteLocation(&module->loc);
        WriteSeparator();
        if (module->name.start) {
          WriteKey("name");
          WriteEscapedStringSlice(module->name);
          WriteSeparator();
        }
        WriteKey("filename");
        WriteEscapedStringSlice(get_basename(filename));
        WriteModule(filename, module);
        delete [] filename;
        num_modules_++;
        last_module_index = i;
        break;
      }

      case CommandType::Action:
        WriteLocation(&command.action->loc);
        WriteSeparator();
        WriteAction(command.action);
        break;

      case CommandType::Register:
        WriteLocation(&command.register_.var.loc);
        WriteSeparator();
        if (command.register_.var.type == VarType::Name) {
          WriteKey("name");
          WriteVar(&command.register_.var);
          WriteSeparator();
        } else {
          /* If we're not registering by name, then we should only be
           * registering the last module. */
          WABT_USE(last_module_index);
          assert(command.register_.var.index == last_module_index);
        }
        WriteKey("as");
        WriteEscapedStringSlice(command.register_.module_name);
        break;

      case CommandType::AssertMalformed:
        WriteInvalidModule(command.assert_malformed.module,
                           command.assert_malformed.text);
        num_modules_++;
        break;

      case CommandType::AssertInvalid:
        WriteInvalidModule(command.assert_invalid.module,
                           command.assert_invalid.text);
        num_modules_++;
        break;

      case CommandType::AssertUnlinkable:
        WriteInvalidModule(command.assert_unlinkable.module,
                           command.assert_unlinkable.text);
        num_modules_++;
        break;

      case CommandType::AssertUninstantiable:
        WriteInvalidModule(command.assert_uninstantiable.module,
                           command.assert_uninstantiable.text);
        num_modules_++;
        break;

      case CommandType::AssertReturn:
        WriteLocation(&command.assert_return.action->loc);
        WriteSeparator();
        WriteAction(command.assert_return.action);
        WriteSeparator();
        WriteKey("expected");
        WriteConstVector(*command.assert_return.expected);
        break;

      case CommandType::AssertReturnCanonicalNan:
        WriteLocation(&command.assert_return_canonical_nan.action->loc);
        WriteSeparator();
        WriteAction(command.assert_return_canonical_nan.action);
        WriteSeparator();
        WriteKey("expected");
        WriteActionResultType(script,
                              command.assert_return_canonical_nan.action);
        break;

      case CommandType::AssertReturnArithmeticNan:
        WriteLocation(&command.assert_return_arithmetic_nan.action->loc);
        WriteSeparator();
        WriteAction(command.assert_return_arithmetic_nan.action);
        WriteSeparator();
        WriteKey("expected");
        WriteActionResultType(script,
                              command.assert_return_arithmetic_nan.action);
        break;

      case CommandType::AssertTrap:
        WriteLocation(&command.assert_trap.action->loc);
        WriteSeparator();
        WriteAction(command.assert_trap.action);
        WriteSeparator();
        WriteKey("text");
        WriteEscapedStringSlice(command.assert_trap.text);
        break;

      case CommandType::AssertExhaustion:
        WriteLocation(&command.assert_trap.action->loc);
        WriteSeparator();
        WriteAction(command.assert_trap.action);
        break;

      case CommandType::AssertInvalidNonBinary:
        assert(0);
        break;
    }

    json_stream_.Writef("}");
  }
  json_stream_.Writef("]}\n");
}

Result BinaryWriterSpec::WriteScript(Script* script) {
  WriteCommands(script);
  if (spec_options_->json_filename) {
    json_stream_.WriteToFile(spec_options_->json_filename);
  }
  return result_;
}

}  // namespace

Result write_binary_spec_script(Script* script,
                                const char* source_filename,
                                const WriteBinarySpecOptions* spec_options) {
  assert(source_filename);
  BinaryWriterSpec binary_writer_spec(source_filename, spec_options);
  return binary_writer_spec.WriteScript(script);
}

}  // namespace wabt
