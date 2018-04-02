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

#include "src/binary-writer-spec.h"

#include <cassert>
#include <cinttypes>

#include "config.h"

#include "src/binary-writer.h"
#include "src/binary.h"
#include "src/cast.h"
#include "src/filenames.h"
#include "src/ir.h"
#include "src/stream.h"
#include "src/string-view.h"

namespace wabt {

namespace {

class BinaryWriterSpec {
 public:
  BinaryWriterSpec(Stream* json_stream,
                   WriteBinarySpecStreamFactory module_stream_factory,
                   string_view source_filename,
                   string_view module_filename_noext,
                   const WriteBinaryOptions* options);

  Result WriteScript(const Script& script);

 private:
  std::string GetModuleFilename(const char* extension);
  void WriteString(const char* s);
  void WriteKey(const char* key);
  void WriteSeparator();
  void WriteEscapedString(string_view);
  void WriteCommandType(const Command& command);
  void WriteLocation(const Location& loc);
  void WriteVar(const Var& var);
  void WriteTypeObject(Type type);
  void WriteConst(const Const& const_);
  void WriteConstVector(const ConstVector& consts);
  void WriteAction(const Action& action);
  void WriteActionResultType(const Action& action);
  void WriteModule(string_view filename, const Module& module);
  void WriteScriptModule(string_view filename,
                         const ScriptModule& script_module);
  void WriteInvalidModule(const ScriptModule& module, string_view text);
  void WriteCommands();

  const Script* script_ = nullptr;
  Stream* json_stream_ = nullptr;
  WriteBinarySpecStreamFactory module_stream_factory_;
  std::string source_filename_;
  std::string module_filename_noext_;
  const WriteBinaryOptions* options_ = nullptr;
  Result result_ = Result::Ok;
  size_t num_modules_ = 0;
};

BinaryWriterSpec::BinaryWriterSpec(
    Stream* json_stream,
    WriteBinarySpecStreamFactory module_stream_factory,
    string_view source_filename,
    string_view module_filename_noext,
    const WriteBinaryOptions* options)
    : json_stream_(json_stream),
      module_stream_factory_(module_stream_factory),
      source_filename_(source_filename),
      module_filename_noext_(module_filename_noext),
      options_(options) {}

std::string BinaryWriterSpec::GetModuleFilename(const char* extension) {
  std::string result = module_filename_noext_;
  result += '.';
  result += std::to_string(num_modules_);
  result += extension;
  ConvertBackslashToSlash(&result);
  return result;
}

void BinaryWriterSpec::WriteString(const char* s) {
  json_stream_->Writef("\"%s\"", s);
}

void BinaryWriterSpec::WriteKey(const char* key) {
  json_stream_->Writef("\"%s\": ", key);
}

void BinaryWriterSpec::WriteSeparator() {
  json_stream_->Writef(", ");
}

void BinaryWriterSpec::WriteEscapedString(string_view s) {
  json_stream_->WriteChar('"');
  for (size_t i = 0; i < s.length(); ++i) {
    uint8_t c = s[i];
    if (c < 0x20 || c == '\\' || c == '"' || c > 0x7f) {
      json_stream_->Writef("\\u%04x", c);
    } else {
      json_stream_->WriteChar(c);
    }
  }
  json_stream_->WriteChar('"');
}

void BinaryWriterSpec::WriteCommandType(const Command& command) {
  static const char* s_command_names[] = {
      "module",
      "action",
      "register",
      "assert_malformed",
      "assert_invalid",
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

void BinaryWriterSpec::WriteLocation(const Location& loc) {
  WriteKey("line");
  json_stream_->Writef("%d", loc.line);
}

void BinaryWriterSpec::WriteVar(const Var& var) {
  if (var.is_index()) {
    json_stream_->Writef("\"%" PRIindex "\"", var.index());
  } else {
    WriteEscapedString(var.name());
  }
}

void BinaryWriterSpec::WriteTypeObject(Type type) {
  json_stream_->Writef("{");
  WriteKey("type");
  WriteString(GetTypeName(type));
  json_stream_->Writef("}");
}

void BinaryWriterSpec::WriteConst(const Const& const_) {
  json_stream_->Writef("{");
  WriteKey("type");

  /* Always write the values as strings, even though they may be representable
   * as JSON numbers. This way the formatting is consistent. */
  switch (const_.type) {
    case Type::I32:
      WriteString("i32");
      WriteSeparator();
      WriteKey("value");
      json_stream_->Writef("\"%u\"", const_.u32);
      break;

    case Type::I64:
      WriteString("i64");
      WriteSeparator();
      WriteKey("value");
      json_stream_->Writef("\"%" PRIu64 "\"", const_.u64);
      break;

    case Type::F32: {
      /* TODO(binji): write as hex float */
      WriteString("f32");
      WriteSeparator();
      WriteKey("value");
      json_stream_->Writef("\"%u\"", const_.f32_bits);
      break;
    }

    case Type::F64: {
      /* TODO(binji): write as hex float */
      WriteString("f64");
      WriteSeparator();
      WriteKey("value");
      json_stream_->Writef("\"%" PRIu64 "\"", const_.f64_bits);
      break;
    }

    default:
      assert(0);
  }

  json_stream_->Writef("}");
}

void BinaryWriterSpec::WriteConstVector(const ConstVector& consts) {
  json_stream_->Writef("[");
  for (size_t i = 0; i < consts.size(); ++i) {
    const Const& const_ = consts[i];
    WriteConst(const_);
    if (i != consts.size() - 1) {
      WriteSeparator();
    }
  }
  json_stream_->Writef("]");
}

void BinaryWriterSpec::WriteAction(const Action& action) {
  WriteKey("action");
  json_stream_->Writef("{");
  WriteKey("type");
  if (action.type() == ActionType::Invoke) {
    WriteString("invoke");
  } else {
    assert(action.type() == ActionType::Get);
    WriteString("get");
  }
  WriteSeparator();
  if (action.module_var.is_name()) {
    WriteKey("module");
    WriteVar(action.module_var);
    WriteSeparator();
  }
  if (action.type() == ActionType::Invoke) {
    WriteKey("field");
    WriteEscapedString(action.name);
    WriteSeparator();
    WriteKey("args");
    WriteConstVector(cast<InvokeAction>(&action)->args);
  } else {
    WriteKey("field");
    WriteEscapedString(action.name);
  }
  json_stream_->Writef("}");
}

void BinaryWriterSpec::WriteActionResultType(const Action& action) {
  const Module* module = script_->GetModule(action.module_var);
  const Export* export_;
  json_stream_->Writef("[");
  switch (action.type()) {
    case ActionType::Invoke: {
      export_ = module->GetExport(action.name);
      assert(export_->kind == ExternalKind::Func);
      const Func* func = module->GetFunc(export_->var);
      Index num_results = func->GetNumResults();
      for (Index i = 0; i < num_results; ++i)
        WriteTypeObject(func->GetResultType(i));
      break;
    }

    case ActionType::Get: {
      export_ = module->GetExport(action.name);
      assert(export_->kind == ExternalKind::Global);
      const Global* global = module->GetGlobal(export_->var);
      WriteTypeObject(global->type);
      break;
    }
  }
  json_stream_->Writef("]");
}

void BinaryWriterSpec::WriteModule(string_view filename, const Module& module) {
  result_ |=
      WriteBinaryModule(module_stream_factory_(filename), &module, options_);
}

void BinaryWriterSpec::WriteScriptModule(string_view filename,
                                         const ScriptModule& script_module) {
  switch (script_module.type()) {
    case ScriptModuleType::Text:
      WriteModule(filename, cast<TextScriptModule>(&script_module)->module);
      break;

    case ScriptModuleType::Binary:
      module_stream_factory_(filename)->WriteData(
          cast<BinaryScriptModule>(&script_module)->data, "");
      break;

    case ScriptModuleType::Quoted:
      module_stream_factory_(filename)->WriteData(
          cast<QuotedScriptModule>(&script_module)->data, "");
      break;
  }
}

void BinaryWriterSpec::WriteInvalidModule(const ScriptModule& module,
                                          string_view text) {
  const char* extension = "";
  const char* module_type = "";
  switch (module.type()) {
    case ScriptModuleType::Text:
      extension = kWasmExtension;
      module_type = "binary";
      break;

    case ScriptModuleType::Binary:
      extension = kWasmExtension;
      module_type = "binary";
      break;

    case ScriptModuleType::Quoted:
      extension = kWatExtension;
      module_type = "text";
      break;
  }

  WriteLocation(module.location());
  WriteSeparator();
  std::string filename = GetModuleFilename(extension);
  WriteKey("filename");
  WriteEscapedString(GetBasename(filename));
  WriteSeparator();
  WriteKey("text");
  WriteEscapedString(text);
  WriteSeparator();
  WriteKey("module_type");
  WriteString(module_type);
  WriteScriptModule(filename, module);
}

void BinaryWriterSpec::WriteCommands() {
  json_stream_->Writef("{\"source_filename\": ");
  WriteEscapedString(source_filename_);
  json_stream_->Writef(",\n \"commands\": [\n");
  Index last_module_index = kInvalidIndex;
  for (size_t i = 0; i < script_->commands.size(); ++i) {
    const Command* command = script_->commands[i].get();

    if (i != 0) {
      WriteSeparator();
      json_stream_->Writef("\n");
    }

    json_stream_->Writef("  {");
    WriteCommandType(*command);
    WriteSeparator();

    switch (command->type) {
      case CommandType::Module: {
        const Module& module = cast<ModuleCommand>(command)->module;
        std::string filename = GetModuleFilename(kWasmExtension);
        WriteLocation(module.loc);
        WriteSeparator();
        if (!module.name.empty()) {
          WriteKey("name");
          WriteEscapedString(module.name);
          WriteSeparator();
        }
        WriteKey("filename");
        WriteEscapedString(GetBasename(filename));
        WriteModule(filename, module);
        num_modules_++;
        last_module_index = i;
        break;
      }

      case CommandType::Action: {
        const Action& action = *cast<ActionCommand>(command)->action;
        WriteLocation(action.loc);
        WriteSeparator();
        WriteAction(action);
        WriteSeparator();
        WriteKey("expected");
        WriteActionResultType(action);
        break;
      }

      case CommandType::Register: {
        auto* register_command = cast<RegisterCommand>(command);
        const Var& var = register_command->var;
        WriteLocation(var.loc);
        WriteSeparator();
        if (var.is_name()) {
          WriteKey("name");
          WriteVar(var);
          WriteSeparator();
        } else {
          /* If we're not registering by name, then we should only be
           * registering the last module. */
          WABT_USE(last_module_index);
          assert(var.index() == last_module_index);
        }
        WriteKey("as");
        WriteEscapedString(register_command->module_name);
        break;
      }

      case CommandType::AssertMalformed: {
        auto* assert_malformed_command = cast<AssertMalformedCommand>(command);
        WriteInvalidModule(*assert_malformed_command->module,
                           assert_malformed_command->text);
        num_modules_++;
        break;
      }

      case CommandType::AssertInvalid: {
        auto* assert_invalid_command = cast<AssertInvalidCommand>(command);
        WriteInvalidModule(*assert_invalid_command->module,
                           assert_invalid_command->text);
        num_modules_++;
        break;
      }

      case CommandType::AssertUnlinkable: {
        auto* assert_unlinkable_command =
            cast<AssertUnlinkableCommand>(command);
        WriteInvalidModule(*assert_unlinkable_command->module,
                           assert_unlinkable_command->text);
        num_modules_++;
        break;
      }

      case CommandType::AssertUninstantiable: {
        auto* assert_uninstantiable_command =
            cast<AssertUninstantiableCommand>(command);
        WriteInvalidModule(*assert_uninstantiable_command->module,
                           assert_uninstantiable_command->text);
        num_modules_++;
        break;
      }

      case CommandType::AssertReturn: {
        auto* assert_return_command = cast<AssertReturnCommand>(command);
        WriteLocation(assert_return_command->action->loc);
        WriteSeparator();
        WriteAction(*assert_return_command->action);
        WriteSeparator();
        WriteKey("expected");
        WriteConstVector(assert_return_command->expected);
        break;
      }

      case CommandType::AssertReturnCanonicalNan: {
        auto* assert_return_canonical_nan_command =
            cast<AssertReturnCanonicalNanCommand>(command);
        WriteLocation(assert_return_canonical_nan_command->action->loc);
        WriteSeparator();
        WriteAction(*assert_return_canonical_nan_command->action);
        WriteSeparator();
        WriteKey("expected");
        WriteActionResultType(*assert_return_canonical_nan_command->action);
        break;
      }

      case CommandType::AssertReturnArithmeticNan: {
        auto* assert_return_arithmetic_nan_command =
            cast<AssertReturnArithmeticNanCommand>(command);
        WriteLocation(assert_return_arithmetic_nan_command->action->loc);
        WriteSeparator();
        WriteAction(*assert_return_arithmetic_nan_command->action);
        WriteSeparator();
        WriteKey("expected");
        WriteActionResultType(*assert_return_arithmetic_nan_command->action);
        break;
      }

      case CommandType::AssertTrap: {
        auto* assert_trap_command = cast<AssertTrapCommand>(command);
        WriteLocation(assert_trap_command->action->loc);
        WriteSeparator();
        WriteAction(*assert_trap_command->action);
        WriteSeparator();
        WriteKey("text");
        WriteEscapedString(assert_trap_command->text);
        WriteSeparator();
        WriteKey("expected");
        WriteActionResultType(*assert_trap_command->action);
        break;
      }

      case CommandType::AssertExhaustion: {
        auto* assert_exhaustion_command =
            cast<AssertExhaustionCommand>(command);
        WriteLocation(assert_exhaustion_command->action->loc);
        WriteSeparator();
        WriteAction(*assert_exhaustion_command->action);
        WriteSeparator();
        WriteKey("expected");
        WriteActionResultType(*assert_exhaustion_command->action);
        break;
      }
    }

    json_stream_->Writef("}");
  }
  json_stream_->Writef("]}\n");
}

Result BinaryWriterSpec::WriteScript(const Script& script) {
  script_ = &script;
  WriteCommands();
  return result_;
}

}  // end anonymous namespace

Result WriteBinarySpecScript(Stream* json_stream,
                             WriteBinarySpecStreamFactory module_stream_factory,
                             Script* script,
                             string_view source_filename,
                             string_view module_filename_noext,
                             const WriteBinaryOptions* options) {
  BinaryWriterSpec binary_writer_spec(json_stream, module_stream_factory,
                                      source_filename, module_filename_noext,
                                      options);
  return binary_writer_spec.WriteScript(*script);
}

Result WriteBinarySpecScript(
    Stream* json_stream,
    Script* script,
    string_view source_filename,
    string_view module_filename_noext,
    const WriteBinaryOptions* options,
    std::vector<FilenameMemoryStreamPair>* out_module_streams,
    Stream* log_stream) {
  WriteBinarySpecStreamFactory module_stream_factory =
      [&](string_view filename) {
        out_module_streams->emplace_back(filename,
                                         MakeUnique<MemoryStream>(log_stream));
        return out_module_streams->back().stream.get();
      };

  BinaryWriterSpec binary_writer_spec(json_stream, module_stream_factory,
                                      source_filename, module_filename_noext,
                                      options);
  return binary_writer_spec.WriteScript(*script);
}

}  // namespace wabt
