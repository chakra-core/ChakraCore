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

#include <assert.h>
#include <inttypes.h>

#include "ast.h"
#include "binary.h"
#include "binary-writer.h"
#include "config.h"
#include "stream.h"
#include "writer.h"

namespace wabt {

struct Context {
  MemoryWriter json_writer;
  Stream json_stream;
  StringSlice source_filename;
  StringSlice module_filename_noext;
  bool write_modules; /* Whether to write the modules files. */
  const WriteBinarySpecOptions* spec_options;
  Result result;
  size_t num_modules;
};

static void convert_backslash_to_slash(char* s, size_t length) {
  size_t i = 0;
  for (; i < length; ++i)
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

static char* get_module_filename(Context* ctx) {
  size_t buflen = ctx->module_filename_noext.length + 20;
  char* str = new char[buflen];
  size_t length =
      wabt_snprintf(str, buflen, PRIstringslice ".%" PRIzd ".wasm",
               WABT_PRINTF_STRING_SLICE_ARG(ctx->module_filename_noext),
               ctx->num_modules);
  convert_backslash_to_slash(str, length);
  return str;
}

static void write_string(Context* ctx, const char* s) {
  writef(&ctx->json_stream, "\"%s\"", s);
}

static void write_key(Context* ctx, const char* key) {
  writef(&ctx->json_stream, "\"%s\": ", key);
}

static void write_separator(Context* ctx) {
  writef(&ctx->json_stream, ", ");
}

static void write_escaped_string_slice(Context* ctx, StringSlice ss) {
  size_t i;
  write_char(&ctx->json_stream, '"');
  for (i = 0; i < ss.length; ++i) {
    uint8_t c = ss.start[i];
    if (c < 0x20 || c == '\\' || c == '"') {
      writef(&ctx->json_stream, "\\u%04x", c);
    } else {
      write_char(&ctx->json_stream, c);
    }
  }
  write_char(&ctx->json_stream, '"');
}

static void write_command_type(Context* ctx, const Command* command) {
  static const char* s_command_names[] = {
      "module", "action", "register", "assert_malformed", "assert_invalid",
      nullptr, /* ASSERT_INVALID_NON_BINARY, this command will never be
                  written */
      "assert_unlinkable", "assert_uninstantiable", "assert_return",
      "assert_return_nan", "assert_trap", "assert_exhaustion",
  };
  WABT_STATIC_ASSERT(WABT_ARRAY_SIZE(s_command_names) == kCommandTypeCount);

  write_key(ctx, "type");
  assert(s_command_names[static_cast<size_t>(command->type)]);
  write_string(ctx, s_command_names[static_cast<size_t>(command->type)]);
}

static void write_location(Context* ctx, const Location* loc) {
  write_key(ctx, "line");
  writef(&ctx->json_stream, "%d", loc->line);
}

static void write_var(Context* ctx, const Var* var) {
  if (var->type == VarType::Index)
    writef(&ctx->json_stream, "\"%" PRIu64 "\"", var->index);
  else
    write_escaped_string_slice(ctx, var->name);
}

static void write_type_object(Context* ctx, Type type) {
  writef(&ctx->json_stream, "{");
  write_key(ctx, "type");
  write_string(ctx, get_type_name(type));
  writef(&ctx->json_stream, "}");
}

static void write_const(Context* ctx, const Const* const_) {
  writef(&ctx->json_stream, "{");
  write_key(ctx, "type");

  /* Always write the values as strings, even though they may be representable
   * as JSON numbers. This way the formatting is consistent. */
  switch (const_->type) {
    case Type::I32:
      write_string(ctx, "i32");
      write_separator(ctx);
      write_key(ctx, "value");
      writef(&ctx->json_stream, "\"%u\"", const_->u32);
      break;

    case Type::I64:
      write_string(ctx, "i64");
      write_separator(ctx);
      write_key(ctx, "value");
      writef(&ctx->json_stream, "\"%" PRIu64 "\"", const_->u64);
      break;

    case Type::F32: {
      /* TODO(binji): write as hex float */
      write_string(ctx, "f32");
      write_separator(ctx);
      write_key(ctx, "value");
      writef(&ctx->json_stream, "\"%u\"", const_->f32_bits);
      break;
    }

    case Type::F64: {
      /* TODO(binji): write as hex float */
      write_string(ctx, "f64");
      write_separator(ctx);
      write_key(ctx, "value");
      writef(&ctx->json_stream, "\"%" PRIu64 "\"", const_->f64_bits);
      break;
    }

    default:
      assert(0);
  }

  writef(&ctx->json_stream, "}");
}

static void write_const_vector(Context* ctx, const ConstVector* consts) {
  writef(&ctx->json_stream, "[");
  size_t i;
  for (i = 0; i < consts->size; ++i) {
    const Const* const_ = &consts->data[i];
    write_const(ctx, const_);
    if (i != consts->size - 1)
      write_separator(ctx);
  }
  writef(&ctx->json_stream, "]");
}

static void write_action(Context* ctx, const Action* action) {
  write_key(ctx, "action");
  writef(&ctx->json_stream, "{");
  write_key(ctx, "type");
  if (action->type == ActionType::Invoke) {
    write_string(ctx, "invoke");
  } else {
    assert(action->type == ActionType::Get);
    write_string(ctx, "get");
  }
  write_separator(ctx);
  if (action->module_var.type != VarType::Index) {
    write_key(ctx, "module");
    write_var(ctx, &action->module_var);
    write_separator(ctx);
  }
  if (action->type == ActionType::Invoke) {
    write_key(ctx, "field");
    write_escaped_string_slice(ctx, action->invoke.name);
    write_separator(ctx);
    write_key(ctx, "args");
    write_const_vector(ctx, &action->invoke.args);
  } else {
    write_key(ctx, "field");
    write_escaped_string_slice(ctx, action->get.name);
  }
  writef(&ctx->json_stream, "}");
}

static void write_action_result_type(Context* ctx,
                                     Script* script,
                                     const Action* action) {
  const Module* module = get_module_by_var(script, &action->module_var);
  const Export* export_;
  writef(&ctx->json_stream, "[");
  switch (action->type) {
    case ActionType::Invoke: {
      export_ = get_export_by_name(module, &action->invoke.name);
      assert(export_->kind == ExternalKind::Func);
      Func* func = get_func_by_var(module, &export_->var);
      size_t num_results = get_num_results(func);
      size_t i;
      for (i = 0; i < num_results; ++i)
        write_type_object(ctx, get_result_type(func, i));
      break;
    }

    case ActionType::Get: {
      export_ = get_export_by_name(module, &action->get.name);
      assert(export_->kind == ExternalKind::Global);
      Global* global = get_global_by_var(module, &export_->var);
      write_type_object(ctx, global->type);
      break;
    }
  }
  writef(&ctx->json_stream, "]");
}

static void write_module(Context* ctx, char* filename, const Module* module) {
  MemoryWriter writer;
  Result result = init_mem_writer(&writer);
  if (WABT_SUCCEEDED(result)) {
    WriteBinaryOptions options = ctx->spec_options->write_binary_options;
    result = write_binary_module(&writer.base, module, &options);
    if (WABT_SUCCEEDED(result) && ctx->write_modules)
      result = write_output_buffer_to_file(&writer.buf, filename);
    close_mem_writer(&writer);
  }

  ctx->result = result;
}

static void write_raw_module(Context* ctx,
                             char* filename,
                             const RawModule* raw_module) {
  if (raw_module->type == RawModuleType::Text) {
    write_module(ctx, filename, raw_module->text);
  } else if (ctx->write_modules) {
    FileStream stream;
    Result result = init_file_writer(&stream.writer, filename);
    if (WABT_SUCCEEDED(result)) {
      init_stream(&stream.base, &stream.writer.base, nullptr);
      write_data(&stream.base, raw_module->binary.data, raw_module->binary.size,
                 "");
      close_file_writer(&stream.writer);
    }
    ctx->result = result;
  }
}

static void write_invalid_module(Context* ctx,
                                 const RawModule* module,
                                 StringSlice text) {
  char* filename = get_module_filename(ctx);
  write_location(ctx, get_raw_module_location(module));
  write_separator(ctx);
  write_key(ctx, "filename");
  write_escaped_string_slice(ctx, get_basename(filename));
  write_separator(ctx);
  write_key(ctx, "text");
  write_escaped_string_slice(ctx, text);
  write_raw_module(ctx, filename, module);
  delete [] filename;
}

static void write_commands(Context* ctx, Script* script) {
  writef(&ctx->json_stream, "{\"source_filename\": ");
  write_escaped_string_slice(ctx, ctx->source_filename);
  writef(&ctx->json_stream, ",\n \"commands\": [\n");
  size_t i;
  int last_module_index = -1;
  for (i = 0; i < script->commands.size; ++i) {
    Command* command = &script->commands.data[i];

    if (command->type == CommandType::AssertInvalidNonBinary)
      continue;

    if (i != 0)
      write_separator(ctx);
    writef(&ctx->json_stream, "\n");

    writef(&ctx->json_stream, "  {");
    write_command_type(ctx, command);
    write_separator(ctx);

    switch (command->type) {
      case CommandType::Module: {
        Module* module = &command->module;
        char* filename = get_module_filename(ctx);
        write_location(ctx, &module->loc);
        write_separator(ctx);
        if (module->name.start) {
          write_key(ctx, "name");
          write_escaped_string_slice(ctx, module->name);
          write_separator(ctx);
        }
        write_key(ctx, "filename");
        write_escaped_string_slice(ctx, get_basename(filename));
        write_module(ctx, filename, module);
        delete [] filename;
        ctx->num_modules++;
        last_module_index = static_cast<int>(i);
        break;
      }

      case CommandType::Action:
        write_location(ctx, &command->action.loc);
        write_separator(ctx);
        write_action(ctx, &command->action);
        break;

      case CommandType::Register:
        write_location(ctx, &command->register_.var.loc);
        write_separator(ctx);
        if (command->register_.var.type == VarType::Name) {
          write_key(ctx, "name");
          write_var(ctx, &command->register_.var);
          write_separator(ctx);
        } else {
          /* If we're not registering by name, then we should only be
           * registering the last module. */
          WABT_USE(last_module_index);
          assert(command->register_.var.index == last_module_index);
        }
        write_key(ctx, "as");
        write_escaped_string_slice(ctx, command->register_.module_name);
        break;

      case CommandType::AssertMalformed:
        write_invalid_module(ctx, &command->assert_malformed.module,
                             command->assert_malformed.text);
        ctx->num_modules++;
        break;

      case CommandType::AssertInvalid:
        write_invalid_module(ctx, &command->assert_invalid.module,
                             command->assert_invalid.text);
        ctx->num_modules++;
        break;

      case CommandType::AssertUnlinkable:
        write_invalid_module(ctx, &command->assert_unlinkable.module,
                             command->assert_unlinkable.text);
        ctx->num_modules++;
        break;

      case CommandType::AssertUninstantiable:
        write_invalid_module(ctx, &command->assert_uninstantiable.module,
                             command->assert_uninstantiable.text);
        ctx->num_modules++;
        break;

      case CommandType::AssertReturn:
        write_location(ctx, &command->assert_return.action.loc);
        write_separator(ctx);
        write_action(ctx, &command->assert_return.action);
        write_separator(ctx);
        write_key(ctx, "expected");
        write_const_vector(ctx, &command->assert_return.expected);
        break;

      case CommandType::AssertReturnNan:
        write_location(ctx, &command->assert_return_nan.action.loc);
        write_separator(ctx);
        write_action(ctx, &command->assert_return_nan.action);
        write_separator(ctx);
        write_key(ctx, "expected");
        write_action_result_type(ctx, script,
                                 &command->assert_return_nan.action);
        break;

      case CommandType::AssertTrap:
        write_location(ctx, &command->assert_trap.action.loc);
        write_separator(ctx);
        write_action(ctx, &command->assert_trap.action);
        write_separator(ctx);
        write_key(ctx, "text");
        write_escaped_string_slice(ctx, command->assert_trap.text);
        break;

      case CommandType::AssertExhaustion:
        write_location(ctx, &command->assert_trap.action.loc);
        write_separator(ctx);
        write_action(ctx, &command->assert_trap.action);
        break;

      case CommandType::AssertInvalidNonBinary:
        assert(0);
        break;
    }

    writef(&ctx->json_stream, "}");
  }
  writef(&ctx->json_stream, "]}\n");
}

Result write_binary_spec_script(Script* script,
                                const char* source_filename,
                                const WriteBinarySpecOptions* spec_options) {
  assert(source_filename);
  Context ctx;
  WABT_ZERO_MEMORY(ctx);
  ctx.spec_options = spec_options;
  ctx.result = Result::Ok;
  ctx.source_filename.start = source_filename;
  ctx.source_filename.length = strlen(source_filename);
  ctx.module_filename_noext = strip_extension(
      ctx.spec_options->json_filename ? ctx.spec_options->json_filename
                                      : source_filename);
  ctx.write_modules = !!ctx.spec_options->json_filename;

  Result result = init_mem_writer(&ctx.json_writer);
  if (WABT_SUCCEEDED(result)) {
    init_stream(&ctx.json_stream, &ctx.json_writer.base, nullptr);
    write_commands(&ctx, script);
    if (ctx.spec_options->json_filename) {
      write_output_buffer_to_file(&ctx.json_writer.buf,
                                  ctx.spec_options->json_filename);
    }
    close_mem_writer(&ctx.json_writer);
  }

  return ctx.result;
}

}  // namespace wabt
