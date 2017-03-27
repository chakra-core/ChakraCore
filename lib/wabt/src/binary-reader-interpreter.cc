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

#include "binary-reader-interpreter.h"

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

#include <vector>

#include "binary-reader.h"
#include "interpreter.h"
#include "type-checker.h"
#include "writer.h"

#define CHECK_RESULT(expr)  \
  do {                      \
    if (WABT_FAILED(expr))  \
      return Result::Error; \
  } while (0)

#define CHECK_LOCAL(ctx, local_index)                                       \
  do {                                                                      \
    uint32_t max_local_index =                                              \
        (ctx)->current_func->param_and_local_types.size();                  \
    if ((local_index) >= max_local_index) {                                 \
      print_error((ctx), "invalid local_index: %d (max %d)", (local_index), \
                  max_local_index);                                         \
      return Result::Error;                                                 \
    }                                                                       \
  } while (0)

#define CHECK_GLOBAL(ctx, global_index)                                       \
  do {                                                                        \
    uint32_t max_global_index = (ctx)->global_index_mapping.size();           \
    if ((global_index) >= max_global_index) {                                 \
      print_error((ctx), "invalid global_index: %d (max %d)", (global_index), \
                  max_global_index);                                          \
      return Result::Error;                                                   \
    }                                                                         \
  } while (0)

namespace wabt {

namespace {

typedef std::vector<uint32_t> Uint32Vector;
typedef std::vector<Uint32Vector> Uint32VectorVector;

struct Label {
  Label(uint32_t offset, uint32_t fixup_offset);

  uint32_t offset; /* branch location in the istream */
  uint32_t fixup_offset;
};

Label::Label(uint32_t offset, uint32_t fixup_offset)
    : offset(offset), fixup_offset(fixup_offset) {}

struct Context {
  Context();

  BinaryReader* reader = nullptr;
  BinaryErrorHandler* error_handler = nullptr;
  InterpreterEnvironment* env = nullptr;
  DefinedInterpreterModule* module = nullptr;
  DefinedInterpreterFunc* current_func = nullptr;
  TypeChecker typechecker;
  std::vector<Label> label_stack;
  Uint32VectorVector func_fixups;
  Uint32VectorVector depth_fixups;
  MemoryWriter istream_writer;
  uint32_t istream_offset = 0;
  /* mappings from module index space to env index space; this won't just be a
   * translation, because imported values will be resolved as well */
  Uint32Vector sig_index_mapping;
  Uint32Vector func_index_mapping;
  Uint32Vector global_index_mapping;

  uint32_t num_func_imports = 0;
  uint32_t num_global_imports = 0;

  /* values cached in the Context so they can be shared between callbacks */
  InterpreterTypedValue init_expr_value;
  uint32_t table_offset = 0;
  bool is_host_import = false;
  HostInterpreterModule* host_import_module = nullptr;
  uint32_t import_env_index = 0;
};

Context::Context() {
  WABT_ZERO_MEMORY(istream_writer);
}

}  // namespace

static Label* get_label(Context* ctx, uint32_t depth) {
  assert(depth < ctx->label_stack.size());
  return &ctx->label_stack[ctx->label_stack.size() - depth - 1];
}

static Label* top_label(Context* ctx) {
  return get_label(ctx, 0);
}

static bool handle_error(uint32_t offset, const char* message, Context* ctx) {
  if (ctx->error_handler->on_error) {
    return ctx->error_handler->on_error(offset, message,
                                        ctx->error_handler->user_data);
  }
  return false;
}

static void WABT_PRINTF_FORMAT(2, 3)
    print_error(Context* ctx, const char* format, ...) {
  WABT_SNPRINTF_ALLOCA(buffer, length, format);
  handle_error(WABT_INVALID_OFFSET, buffer, ctx);
}

static void on_typechecker_error(const char* msg, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  print_error(ctx, "%s", msg);
}

static uint32_t translate_sig_index_to_env(Context* ctx, uint32_t sig_index) {
  assert(sig_index < ctx->sig_index_mapping.size());
  return ctx->sig_index_mapping[sig_index];
}

static InterpreterFuncSignature* get_signature_by_env_index(
    Context* ctx,
    uint32_t sig_index) {
  return &ctx->env->sigs[sig_index];
}

static InterpreterFuncSignature* get_signature_by_module_index(
    Context* ctx,
    uint32_t sig_index) {
  return get_signature_by_env_index(ctx,
                                    translate_sig_index_to_env(ctx, sig_index));
}

static uint32_t translate_func_index_to_env(Context* ctx, uint32_t func_index) {
  assert(func_index < ctx->func_index_mapping.size());
  return ctx->func_index_mapping[func_index];
}

static uint32_t translate_module_func_index_to_defined(Context* ctx,
                                                       uint32_t func_index) {
  assert(func_index >= ctx->num_func_imports);
  return func_index - ctx->num_func_imports;
}

static InterpreterFunc* get_func_by_env_index(Context* ctx,
                                              uint32_t func_index) {
  return ctx->env->funcs[func_index].get();
}

static InterpreterFunc* get_func_by_module_index(Context* ctx,
                                                 uint32_t func_index) {
  return get_func_by_env_index(ctx,
                               translate_func_index_to_env(ctx, func_index));
}

static uint32_t translate_global_index_to_env(Context* ctx,
                                              uint32_t global_index) {
  return ctx->global_index_mapping[global_index];
}

static InterpreterGlobal* get_global_by_env_index(Context* ctx,
                                                  uint32_t global_index) {
  return &ctx->env->globals[global_index];
}

static InterpreterGlobal* get_global_by_module_index(Context* ctx,
                                                     uint32_t global_index) {
  return get_global_by_env_index(
      ctx, translate_global_index_to_env(ctx, global_index));
}

static Type get_global_type_by_module_index(Context* ctx,
                                            uint32_t global_index) {
  return get_global_by_module_index(ctx, global_index)->typed_value.type;
}

static Type get_local_type_by_index(InterpreterFunc* func,
                                    uint32_t local_index) {
  assert(!func->is_host);
  return func->as_defined()->param_and_local_types[local_index];
}

static uint32_t get_istream_offset(Context* ctx) {
  return ctx->istream_offset;
}

static Result emit_data_at(Context* ctx,
                           size_t offset,
                           const void* data,
                           size_t size) {
  return ctx->istream_writer.base.write_data(
      offset, data, size, ctx->istream_writer.base.user_data);
}

static Result emit_data(Context* ctx, const void* data, size_t size) {
  CHECK_RESULT(emit_data_at(ctx, ctx->istream_offset, data, size));
  ctx->istream_offset += size;
  return Result::Ok;
}

static Result emit_opcode(Context* ctx, Opcode opcode) {
  return emit_data(ctx, &opcode, sizeof(uint8_t));
}

static Result emit_opcode(Context* ctx, InterpreterOpcode opcode) {
  return emit_data(ctx, &opcode, sizeof(uint8_t));
}

static Result emit_i8(Context* ctx, uint8_t value) {
  return emit_data(ctx, &value, sizeof(value));
}

static Result emit_i32(Context* ctx, uint32_t value) {
  return emit_data(ctx, &value, sizeof(value));
}

static Result emit_i64(Context* ctx, uint64_t value) {
  return emit_data(ctx, &value, sizeof(value));
}

static Result emit_i32_at(Context* ctx, uint32_t offset, uint32_t value) {
  return emit_data_at(ctx, offset, &value, sizeof(value));
}

static Result emit_drop_keep(Context* ctx, uint32_t drop, uint8_t keep) {
  assert(drop != UINT32_MAX);
  assert(keep <= 1);
  if (drop > 0) {
    if (drop == 1 && keep == 0) {
      CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::Drop));
    } else {
      CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::DropKeep));
      CHECK_RESULT(emit_i32(ctx, drop));
      CHECK_RESULT(emit_i8(ctx, keep));
    }
  }
  return Result::Ok;
}

static Result append_fixup(Context* ctx,
                           Uint32VectorVector* fixups_vector,
                           uint32_t index) {
  if (index >= fixups_vector->size())
    fixups_vector->resize(index + 1);
  (*fixups_vector)[index].push_back(get_istream_offset(ctx));
  return Result::Ok;
}

static Result emit_br_offset(Context* ctx, uint32_t depth, uint32_t offset) {
  if (offset == WABT_INVALID_OFFSET) {
    /* depth_fixups stores the depth counting up from zero, where zero is the
     * top-level function scope. */
    depth = ctx->label_stack.size() - 1 - depth;
    CHECK_RESULT(append_fixup(ctx, &ctx->depth_fixups, depth));
  }
  CHECK_RESULT(emit_i32(ctx, offset));
  return Result::Ok;
}

static Result get_br_drop_keep_count(Context* ctx,
                                     uint32_t depth,
                                     uint32_t* out_drop_count,
                                     uint32_t* out_keep_count) {
  TypeCheckerLabel* label;
  CHECK_RESULT(typechecker_get_label(&ctx->typechecker, depth, &label));
  *out_keep_count =
      label->label_type != LabelType::Loop ? label->sig.size() : 0;
  if (typechecker_is_unreachable(&ctx->typechecker)) {
    *out_drop_count = 0;
  } else {
    *out_drop_count =
        (ctx->typechecker.type_stack.size() - label->type_stack_limit) -
        *out_keep_count;
  }
  return Result::Ok;
}

static Result get_return_drop_keep_count(Context* ctx,
                                         uint32_t* out_drop_count,
                                         uint32_t* out_keep_count) {
  if (WABT_FAILED(get_br_drop_keep_count(ctx, ctx->label_stack.size() - 1,
                                         out_drop_count, out_keep_count))) {
    return Result::Error;
  }

  *out_drop_count += ctx->current_func->param_and_local_types.size();
  return Result::Ok;
}

static Result emit_br(Context* ctx,
                      uint32_t depth,
                      uint32_t drop_count,
                      uint32_t keep_count) {
  CHECK_RESULT(emit_drop_keep(ctx, drop_count, keep_count));
  CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::Br));
  CHECK_RESULT(emit_br_offset(ctx, depth, get_label(ctx, depth)->offset));
  return Result::Ok;
}

static Result emit_br_table_offset(Context* ctx, uint32_t depth) {
  uint32_t drop_count, keep_count;
  CHECK_RESULT(get_br_drop_keep_count(ctx, depth, &drop_count, &keep_count));
  CHECK_RESULT(emit_br_offset(ctx, depth, get_label(ctx, depth)->offset));
  CHECK_RESULT(emit_i32(ctx, drop_count));
  CHECK_RESULT(emit_i8(ctx, keep_count));
  return Result::Ok;
}

static Result fixup_top_label(Context* ctx) {
  uint32_t offset = get_istream_offset(ctx);
  uint32_t top = ctx->label_stack.size() - 1;
  if (top >= ctx->depth_fixups.size()) {
    /* nothing to fixup */
    return Result::Ok;
  }

  Uint32Vector& fixups = ctx->depth_fixups[top];
  for (uint32_t fixup: fixups)
    CHECK_RESULT(emit_i32_at(ctx, fixup, offset));
  fixups.clear();
  return Result::Ok;
}

static Result emit_func_offset(Context* ctx,
                               DefinedInterpreterFunc* func,
                               uint32_t func_index) {
  if (func->offset == WABT_INVALID_OFFSET) {
    uint32_t defined_index =
        translate_module_func_index_to_defined(ctx, func_index);
    CHECK_RESULT(append_fixup(ctx, &ctx->func_fixups, defined_index));
  }
  CHECK_RESULT(emit_i32(ctx, func->offset));
  return Result::Ok;
}

static bool on_error(BinaryReaderContext* ctx, const char* message) {
  return handle_error(ctx->offset, message,
                      static_cast<Context*>(ctx->user_data));
}

static Result on_signature_count(uint32_t count, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  ctx->sig_index_mapping.resize(count);
  for (uint32_t i = 0; i < count; ++i)
    ctx->sig_index_mapping[i] = ctx->env->sigs.size() + i;
  ctx->env->sigs.resize(ctx->env->sigs.size() + count);
  return Result::Ok;
}

static Result on_signature(uint32_t index,
                           uint32_t param_count,
                           Type* param_types,
                           uint32_t result_count,
                           Type* result_types,
                           void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  InterpreterFuncSignature* sig = get_signature_by_module_index(ctx, index);
  sig->param_types.insert(sig->param_types.end(), param_types,
                          param_types + param_count);
  sig->result_types.insert(sig->result_types.end(), result_types,
                           result_types + result_count);
  return Result::Ok;
}

static Result on_import_count(uint32_t count, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  ctx->module->imports.resize(count);
  return Result::Ok;
}

static Result on_import(uint32_t index,
                        StringSlice module_name,
                        StringSlice field_name,
                        void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  InterpreterImport* import = &ctx->module->imports[index];
  import->module_name = dup_string_slice(module_name);
  import->field_name = dup_string_slice(field_name);
  int module_index =
      ctx->env->registered_module_bindings.find_index(import->module_name);
  if (module_index < 0) {
    print_error(ctx, "unknown import module \"" PRIstringslice "\"",
                WABT_PRINTF_STRING_SLICE_ARG(import->module_name));
    return Result::Error;
  }

  InterpreterModule* module = ctx->env->modules[module_index].get();
  if (module->is_host) {
    /* We don't yet know the kind of a host import module, so just assume it
     * exists for now. We'll fail later (in on_import_* below) if it doesn't
     * exist). */
    ctx->is_host_import = true;
    ctx->host_import_module = module->as_host();
  } else {
    InterpreterExport* export_ =
        get_interpreter_export_by_name(module, &import->field_name);
    if (!export_) {
      print_error(ctx, "unknown module field \"" PRIstringslice "\"",
                  WABT_PRINTF_STRING_SLICE_ARG(import->field_name));
      return Result::Error;
    }

    import->kind = export_->kind;
    ctx->is_host_import = false;
    ctx->import_env_index = export_->index;
  }
  return Result::Ok;
}

static Result check_import_kind(Context* ctx,
                                InterpreterImport* import,
                                ExternalKind expected_kind) {
  if (import->kind != expected_kind) {
    print_error(ctx, "expected import \"" PRIstringslice "." PRIstringslice
                     "\" to have kind %s, not %s",
                WABT_PRINTF_STRING_SLICE_ARG(import->module_name),
                WABT_PRINTF_STRING_SLICE_ARG(import->field_name),
                get_kind_name(expected_kind), get_kind_name(import->kind));
    return Result::Error;
  }
  return Result::Ok;
}

static Result check_import_limits(Context* ctx,
                                  const Limits* declared_limits,
                                  const Limits* actual_limits) {
  if (actual_limits->initial < declared_limits->initial) {
    print_error(ctx,
                "actual size (%" PRIu64 ") smaller than declared (%" PRIu64 ")",
                actual_limits->initial, declared_limits->initial);
    return Result::Error;
  }

  if (declared_limits->has_max) {
    if (!actual_limits->has_max) {
      print_error(ctx,
                  "max size (unspecified) larger than declared (%" PRIu64 ")",
                  declared_limits->max);
      return Result::Error;
    } else if (actual_limits->max > declared_limits->max) {
      print_error(ctx,
                  "max size (%" PRIu64 ") larger than declared (%" PRIu64 ")",
                  actual_limits->max, declared_limits->max);
      return Result::Error;
    }
  }

  return Result::Ok;
}

static Result append_export(Context* ctx,
                            InterpreterModule* module,
                            ExternalKind kind,
                            uint32_t item_index,
                            StringSlice name) {
  if (module->export_bindings.find_index(name) != -1) {
    print_error(ctx, "duplicate export \"" PRIstringslice "\"",
                WABT_PRINTF_STRING_SLICE_ARG(name));
    return Result::Error;
  }

  module->exports.emplace_back(dup_string_slice(name), kind, item_index);
  InterpreterExport* export_ = &module->exports.back();

  module->export_bindings.emplace(string_slice_to_string(export_->name),
                                  Binding(module->exports.size() - 1));
  return Result::Ok;
}

static void on_host_import_print_error(const char* msg, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  print_error(ctx, "%s", msg);
}

static PrintErrorCallback make_print_error_callback(Context* ctx) {
  PrintErrorCallback result;
  result.print_error = on_host_import_print_error;
  result.user_data = ctx;
  return result;
}

static Result on_import_func(uint32_t import_index,
                             StringSlice module_name,
                             StringSlice field_name,
                             uint32_t func_index,
                             uint32_t sig_index,
                             void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  InterpreterImport* import = &ctx->module->imports[import_index];
  import->func.sig_index = translate_sig_index_to_env(ctx, sig_index);

  uint32_t func_env_index;
  if (ctx->is_host_import) {
    HostInterpreterFunc* func = new HostInterpreterFunc(
        import->module_name, import->field_name, import->func.sig_index);

    ctx->env->funcs.emplace_back(func);

    InterpreterHostImportDelegate* host_delegate =
        &ctx->host_import_module->import_delegate;
    InterpreterFuncSignature* sig = &ctx->env->sigs[func->sig_index];
    CHECK_RESULT(host_delegate->import_func(import, func, sig,
                                            make_print_error_callback(ctx),
                                            host_delegate->user_data));
    assert(func->callback);

    func_env_index = ctx->env->funcs.size() - 1;
    append_export(ctx, ctx->host_import_module, ExternalKind::Func,
                  func_env_index, import->field_name);
  } else {
    CHECK_RESULT(check_import_kind(ctx, import, ExternalKind::Func));
    InterpreterFunc* func = ctx->env->funcs[ctx->import_env_index].get();
    if (!func_signatures_are_equal(ctx->env, import->func.sig_index,
                                   func->sig_index)) {
      print_error(ctx, "import signature mismatch");
      return Result::Error;
    }

    func_env_index = ctx->import_env_index;
  }
  ctx->func_index_mapping.push_back(func_env_index);
  ctx->num_func_imports++;
  return Result::Ok;
}

static Result on_import_table(uint32_t import_index,
                              StringSlice module_name,
                              StringSlice field_name,
                              uint32_t table_index,
                              Type elem_type,
                              const Limits* elem_limits,
                              void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  if (ctx->module->table_index != WABT_INVALID_INDEX) {
    print_error(ctx, "only one table allowed");
    return Result::Error;
  }

  InterpreterImport* import = &ctx->module->imports[import_index];

  if (ctx->is_host_import) {
    ctx->env->tables.emplace_back(*elem_limits);
    InterpreterTable* table = &ctx->env->tables.back();

    InterpreterHostImportDelegate* host_delegate =
        &ctx->host_import_module->import_delegate;
    CHECK_RESULT(host_delegate->import_table(import, table,
                                             make_print_error_callback(ctx),
                                             host_delegate->user_data));

    CHECK_RESULT(check_import_limits(ctx, elem_limits, &table->limits));

    ctx->module->table_index = ctx->env->tables.size() - 1;
    append_export(ctx, ctx->host_import_module, ExternalKind::Table,
                  ctx->module->table_index, import->field_name);
  } else {
    CHECK_RESULT(check_import_kind(ctx, import, ExternalKind::Table));
    InterpreterTable* table = &ctx->env->tables[ctx->import_env_index];
    CHECK_RESULT(check_import_limits(ctx, elem_limits, &table->limits));

    import->table.limits = *elem_limits;
    ctx->module->table_index = ctx->import_env_index;
  }
  return Result::Ok;
}

static Result on_import_memory(uint32_t import_index,
                               StringSlice module_name,
                               StringSlice field_name,
                               uint32_t memory_index,
                               const Limits* page_limits,
                               void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  if (ctx->module->memory_index != WABT_INVALID_INDEX) {
    print_error(ctx, "only one memory allowed");
    return Result::Error;
  }

  InterpreterImport* import = &ctx->module->imports[import_index];

  if (ctx->is_host_import) {
    ctx->env->memories.emplace_back();
    InterpreterMemory* memory = &ctx->env->memories.back();

    InterpreterHostImportDelegate* host_delegate =
        &ctx->host_import_module->import_delegate;
    CHECK_RESULT(host_delegate->import_memory(import, memory,
                                              make_print_error_callback(ctx),
                                              host_delegate->user_data));

    CHECK_RESULT(check_import_limits(ctx, page_limits, &memory->page_limits));

    ctx->module->memory_index = ctx->env->memories.size() - 1;
    append_export(ctx, ctx->host_import_module, ExternalKind::Memory,
                  ctx->module->memory_index, import->field_name);
  } else {
    CHECK_RESULT(check_import_kind(ctx, import, ExternalKind::Memory));
    InterpreterMemory* memory = &ctx->env->memories[ctx->import_env_index];
    CHECK_RESULT(check_import_limits(ctx, page_limits, &memory->page_limits));

    import->memory.limits = *page_limits;
    ctx->module->memory_index = ctx->import_env_index;
  }
  return Result::Ok;
}

static Result on_import_global(uint32_t import_index,
                               StringSlice module_name,
                               StringSlice field_name,
                               uint32_t global_index,
                               Type type,
                               bool mutable_,
                               void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  InterpreterImport* import = &ctx->module->imports[import_index];

  uint32_t global_env_index = ctx->env->globals.size() - 1;
  if (ctx->is_host_import) {
    ctx->env->globals.emplace_back(InterpreterTypedValue(type), mutable_);
    InterpreterGlobal* global = &ctx->env->globals.back();

    InterpreterHostImportDelegate* host_delegate =
        &ctx->host_import_module->import_delegate;
    CHECK_RESULT(host_delegate->import_global(import, global,
                                              make_print_error_callback(ctx),
                                              host_delegate->user_data));

    global_env_index = ctx->env->globals.size() - 1;
    append_export(ctx, ctx->host_import_module, ExternalKind::Global,
                  global_env_index, import->field_name);
  } else {
    CHECK_RESULT(check_import_kind(ctx, import, ExternalKind::Global));
    // TODO: check type and mutability
    import->global.type = type;
    import->global.mutable_ = mutable_;
    global_env_index = ctx->import_env_index;
  }
  ctx->global_index_mapping.push_back(global_env_index);
  ctx->num_global_imports++;
  return Result::Ok;
}

static Result on_function_signatures_count(uint32_t count, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  for (uint32_t i = 0; i < count; ++i)
    ctx->func_index_mapping.push_back(ctx->env->funcs.size() + i);
  ctx->env->funcs.reserve(ctx->env->funcs.size() + count);
  ctx->func_fixups.resize(count);
  return Result::Ok;
}

static Result on_function_signature(uint32_t index,
                                    uint32_t sig_index,
                                    void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  DefinedInterpreterFunc* func =
      new DefinedInterpreterFunc(translate_sig_index_to_env(ctx, sig_index));
  ctx->env->funcs.emplace_back(func);
  return Result::Ok;
}

static Result on_table(uint32_t index,
                       Type elem_type,
                       const Limits* elem_limits,
                       void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  if (ctx->module->table_index != WABT_INVALID_INDEX) {
    print_error(ctx, "only one table allowed");
    return Result::Error;
  }
  ctx->env->tables.emplace_back(*elem_limits);
  ctx->module->table_index = ctx->env->tables.size() - 1;
  return Result::Ok;
}

static Result on_memory(uint32_t index,
                        const Limits* page_limits,
                        void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  if (ctx->module->memory_index != WABT_INVALID_INDEX) {
    print_error(ctx, "only one memory allowed");
    return Result::Error;
  }
  ctx->env->memories.emplace_back(*page_limits);
  ctx->module->memory_index = ctx->env->memories.size() - 1;
  return Result::Ok;
}

static Result on_global_count(uint32_t count, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  for (uint32_t i = 0; i < count; ++i)
    ctx->global_index_mapping.push_back(ctx->env->globals.size() + i);
  ctx->env->globals.resize(ctx->env->globals.size() + count);
  return Result::Ok;
}

static Result begin_global(uint32_t index,
                           Type type,
                           bool mutable_,
                           void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  InterpreterGlobal* global = get_global_by_module_index(ctx, index);
  global->typed_value.type = type;
  global->mutable_ = mutable_;
  return Result::Ok;
}

static Result end_global_init_expr(uint32_t index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  InterpreterGlobal* global = get_global_by_module_index(ctx, index);
  if (ctx->init_expr_value.type != global->typed_value.type) {
    print_error(ctx, "type mismatch in global, expected %s but got %s.",
                get_type_name(global->typed_value.type),
                get_type_name(ctx->init_expr_value.type));
    return Result::Error;
  }
  global->typed_value = ctx->init_expr_value;
  return Result::Ok;
}

static Result on_init_expr_f32_const_expr(uint32_t index,
                                          uint32_t value_bits,
                                          void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  ctx->init_expr_value.type = Type::F32;
  ctx->init_expr_value.value.f32_bits = value_bits;
  return Result::Ok;
}

static Result on_init_expr_f64_const_expr(uint32_t index,
                                          uint64_t value_bits,
                                          void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  ctx->init_expr_value.type = Type::F64;
  ctx->init_expr_value.value.f64_bits = value_bits;
  return Result::Ok;
}

static Result on_init_expr_get_global_expr(uint32_t index,
                                           uint32_t global_index,
                                           void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  if (global_index >= ctx->num_global_imports) {
    print_error(ctx,
                "initializer expression can only reference an imported global");
    return Result::Error;
  }
  InterpreterGlobal* ref_global = get_global_by_module_index(ctx, global_index);
  if (ref_global->mutable_) {
    print_error(ctx,
                "initializer expression cannot reference a mutable global");
    return Result::Error;
  }
  ctx->init_expr_value = ref_global->typed_value;
  return Result::Ok;
}

static Result on_init_expr_i32_const_expr(uint32_t index,
                                          uint32_t value,
                                          void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  ctx->init_expr_value.type = Type::I32;
  ctx->init_expr_value.value.i32 = value;
  return Result::Ok;
}

static Result on_init_expr_i64_const_expr(uint32_t index,
                                          uint64_t value,
                                          void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  ctx->init_expr_value.type = Type::I64;
  ctx->init_expr_value.value.i64 = value;
  return Result::Ok;
}

static Result on_export(uint32_t index,
                        ExternalKind kind,
                        uint32_t item_index,
                        StringSlice name,
                        void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  switch (kind) {
    case ExternalKind::Func:
      item_index = translate_func_index_to_env(ctx, item_index);
      break;

    case ExternalKind::Table:
      item_index = ctx->module->table_index;
      break;

    case ExternalKind::Memory:
      item_index = ctx->module->memory_index;
      break;

    case ExternalKind::Global: {
      item_index = translate_global_index_to_env(ctx, item_index);
      InterpreterGlobal* global = &ctx->env->globals[item_index];
      if (global->mutable_) {
        print_error(ctx, "mutable globals cannot be exported");
        return Result::Error;
      }
      break;
    }
  }
  return append_export(ctx, ctx->module, kind, item_index, name);
}

static Result on_start_function(uint32_t func_index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  uint32_t start_func_index = translate_func_index_to_env(ctx, func_index);
  InterpreterFunc* start_func = get_func_by_env_index(ctx, start_func_index);
  InterpreterFuncSignature* sig =
      get_signature_by_env_index(ctx, start_func->sig_index);
  if (sig->param_types.size() != 0) {
    print_error(ctx, "start function must be nullary");
    return Result::Error;
  }
  if (sig->result_types.size() != 0) {
    print_error(ctx, "start function must not return anything");
    return Result::Error;
  }
  ctx->module->start_func_index = start_func_index;
  return Result::Ok;
}

static Result end_elem_segment_init_expr(uint32_t index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  if (ctx->init_expr_value.type != Type::I32) {
    print_error(ctx, "type mismatch in elem segment, expected i32 but got %s",
                get_type_name(ctx->init_expr_value.type));
    return Result::Error;
  }
  ctx->table_offset = ctx->init_expr_value.value.i32;
  return Result::Ok;
}

static Result on_elem_segment_function_index_check(uint32_t index,
                                                   uint32_t func_index,
                                                   void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  assert(ctx->module->table_index != WABT_INVALID_INDEX);
  InterpreterTable* table = &ctx->env->tables[ctx->module->table_index];
  if (ctx->table_offset >= table->func_indexes.size()) {
    print_error(ctx,
                "elem segment offset is out of bounds: %u >= max value %" PRIzd,
                ctx->table_offset, table->func_indexes.size());
    return Result::Error;
  }

  uint32_t max_func_index = ctx->func_index_mapping.size();
  if (func_index >= max_func_index) {
    print_error(ctx, "invalid func_index: %d (max %d)", func_index,
                max_func_index);
    return Result::Error;
  }

  return Result::Ok;
}

static Result on_elem_segment_function_index(uint32_t index,
                                             uint32_t func_index,
                                             void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  assert(ctx->module->table_index != WABT_INVALID_INDEX);
  InterpreterTable* table = &ctx->env->tables[ctx->module->table_index];
  table->func_indexes[ctx->table_offset++] =
      translate_func_index_to_env(ctx, func_index);
  return Result::Ok;
}

static Result on_data_segment_data_check(uint32_t index,
                                         const void* src_data,
                                         uint32_t size,
                                         void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  assert(ctx->module->memory_index != WABT_INVALID_INDEX);
  InterpreterMemory* memory = &ctx->env->memories[ctx->module->memory_index];
  if (ctx->init_expr_value.type != Type::I32) {
    print_error(ctx, "type mismatch in data segment, expected i32 but got %s",
                get_type_name(ctx->init_expr_value.type));
    return Result::Error;
  }
  uint32_t address = ctx->init_expr_value.value.i32;
  uint64_t end_address =
      static_cast<uint64_t>(address) + static_cast<uint64_t>(size);
  if (end_address > memory->data.size()) {
    print_error(ctx,
                "data segment is out of bounds: [%u, %" PRIu64
                ") >= max value %" PRIzd,
                address, end_address, memory->data.size());
    return Result::Error;
  }
  return Result::Ok;
}

static Result on_data_segment_data(uint32_t index,
                                   const void* src_data,
                                   uint32_t size,
                                   void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  if (size > 0) {
    assert(ctx->module->memory_index != WABT_INVALID_INDEX);
    InterpreterMemory* memory = &ctx->env->memories[ctx->module->memory_index];
    uint32_t address = ctx->init_expr_value.value.i32;
    memcpy(&memory->data[address], src_data, size);
  }
  return Result::Ok;
}

static void push_label(Context* ctx, uint32_t offset, uint32_t fixup_offset) {
  ctx->label_stack.emplace_back(offset, fixup_offset);
}

static void pop_label(Context* ctx) {
  ctx->label_stack.pop_back();
  /* reduce the depth_fixups stack as well, but it may be smaller than
   * label_stack so only do it conditionally. */
  if (ctx->depth_fixups.size() > ctx->label_stack.size()) {
    ctx->depth_fixups.erase(ctx->depth_fixups.begin() + ctx->label_stack.size(),
                            ctx->depth_fixups.end());
  }
}

// TODO(binji): remove this when the rest of the code is using std::vector
#define INTERPRETER_TYPE_VECTOR_TO_TYPE_VECTOR(out, in)                \
  TypeVector out;                                                      \
  {                                                                    \
    size_t byte_size = (in).size() * sizeof((in)[0]);                  \
    (out).data = static_cast<decltype((out).data)>(alloca(byte_size)); \
    (out).size = (in).size();                                          \
    if (byte_size) {                                                   \
      memcpy((out).data, (in).data(), byte_size);                      \
    }                                                                  \
  }

static Result begin_function_body(BinaryReaderContext* context,
                                  uint32_t index) {
  Context* ctx = static_cast<Context*>(context->user_data);
  DefinedInterpreterFunc* func =
      get_func_by_module_index(ctx, index)->as_defined();
  InterpreterFuncSignature* sig =
      get_signature_by_env_index(ctx, func->sig_index);

  func->offset = get_istream_offset(ctx);
  func->local_decl_count = 0;
  func->local_count = 0;

  ctx->current_func = func;
  ctx->depth_fixups.clear();
  ctx->label_stack.clear();

  /* fixup function references */
  uint32_t defined_index = translate_module_func_index_to_defined(ctx, index);
  Uint32Vector& fixups = ctx->func_fixups[defined_index];
  for (uint32_t fixup: fixups)
    CHECK_RESULT(emit_i32_at(ctx, fixup, func->offset));

  /* append param types */
  for (Type param_type: sig->param_types)
    func->param_and_local_types.push_back(param_type);

  CHECK_RESULT(
      typechecker_begin_function(&ctx->typechecker, &sig->result_types));

  /* push implicit func label (equivalent to return) */
  push_label(ctx, WABT_INVALID_OFFSET, WABT_INVALID_OFFSET);
  return Result::Ok;
}

static Result end_function_body(uint32_t index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  fixup_top_label(ctx);
  uint32_t drop_count, keep_count;
  CHECK_RESULT(get_return_drop_keep_count(ctx, &drop_count, &keep_count));
  CHECK_RESULT(typechecker_end_function(&ctx->typechecker));
  CHECK_RESULT(emit_drop_keep(ctx, drop_count, keep_count));
  CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::Return));
  pop_label(ctx);
  ctx->current_func = nullptr;
  return Result::Ok;
}

static Result on_local_decl_count(uint32_t count, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  ctx->current_func->local_decl_count = count;
  return Result::Ok;
}

static Result on_local_decl(uint32_t decl_index,
                            uint32_t count,
                            Type type,
                            void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  ctx->current_func->local_count += count;

  for (uint32_t i = 0; i < count; ++i)
    ctx->current_func->param_and_local_types.push_back(type);

  if (decl_index == ctx->current_func->local_decl_count - 1) {
    /* last local declaration, allocate space for all locals. */
    CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::Alloca));
    CHECK_RESULT(emit_i32(ctx, ctx->current_func->local_count));
  }
  return Result::Ok;
}

static Result check_has_memory(Context* ctx, Opcode opcode) {
  if (ctx->module->memory_index == WABT_INVALID_INDEX) {
    print_error(ctx, "%s requires an imported or defined memory.",
                get_opcode_name(opcode));
    return Result::Error;
  }
  return Result::Ok;
}

static Result check_align(Context* ctx,
                          uint32_t alignment_log2,
                          uint32_t natural_alignment) {
  if (alignment_log2 >= 32 || (1U << alignment_log2) > natural_alignment) {
    print_error(ctx, "alignment must not be larger than natural alignment (%u)",
                natural_alignment);
    return Result::Error;
  }
  return Result::Ok;
}

static Result on_unary_expr(Opcode opcode, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_RESULT(typechecker_on_unary(&ctx->typechecker, opcode));
  CHECK_RESULT(emit_opcode(ctx, opcode));
  return Result::Ok;
}

static Result on_binary_expr(Opcode opcode, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_RESULT(typechecker_on_binary(&ctx->typechecker, opcode));
  CHECK_RESULT(emit_opcode(ctx, opcode));
  return Result::Ok;
}

static Result on_block_expr(uint32_t num_types,
                            Type* sig_types,
                            void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  TypeVector sig(sig_types, sig_types + num_types);
  CHECK_RESULT(typechecker_on_block(&ctx->typechecker, &sig));
  push_label(ctx, WABT_INVALID_OFFSET, WABT_INVALID_OFFSET);
  return Result::Ok;
}

static Result on_loop_expr(uint32_t num_types,
                           Type* sig_types,
                           void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  TypeVector sig(sig_types, sig_types + num_types);
  CHECK_RESULT(typechecker_on_loop(&ctx->typechecker, &sig));
  push_label(ctx, get_istream_offset(ctx), WABT_INVALID_OFFSET);
  return Result::Ok;
}

static Result on_if_expr(uint32_t num_types, Type* sig_types, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  TypeVector sig(sig_types, sig_types + num_types);
  CHECK_RESULT(typechecker_on_if(&ctx->typechecker, &sig));
  CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::BrUnless));
  uint32_t fixup_offset = get_istream_offset(ctx);
  CHECK_RESULT(emit_i32(ctx, WABT_INVALID_OFFSET));
  push_label(ctx, WABT_INVALID_OFFSET, fixup_offset);
  return Result::Ok;
}

static Result on_else_expr(void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_RESULT(typechecker_on_else(&ctx->typechecker));
  Label* label = top_label(ctx);
  uint32_t fixup_cond_offset = label->fixup_offset;
  CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::Br));
  label->fixup_offset = get_istream_offset(ctx);
  CHECK_RESULT(emit_i32(ctx, WABT_INVALID_OFFSET));
  CHECK_RESULT(emit_i32_at(ctx, fixup_cond_offset, get_istream_offset(ctx)));
  return Result::Ok;
}

static Result on_end_expr(void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  TypeCheckerLabel* label;
  CHECK_RESULT(typechecker_get_label(&ctx->typechecker, 0, &label));
  LabelType label_type = label->label_type;
  CHECK_RESULT(typechecker_on_end(&ctx->typechecker));
  if (label_type == LabelType::If || label_type == LabelType::Else) {
    CHECK_RESULT(emit_i32_at(ctx, top_label(ctx)->fixup_offset,
                             get_istream_offset(ctx)));
  }
  fixup_top_label(ctx);
  pop_label(ctx);
  return Result::Ok;
}

static Result on_br_expr(uint32_t depth, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  uint32_t drop_count, keep_count;
  CHECK_RESULT(get_br_drop_keep_count(ctx, depth, &drop_count, &keep_count));
  CHECK_RESULT(typechecker_on_br(&ctx->typechecker, depth));
  CHECK_RESULT(emit_br(ctx, depth, drop_count, keep_count));
  return Result::Ok;
}

static Result on_br_if_expr(uint32_t depth, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  uint32_t drop_count, keep_count;
  CHECK_RESULT(typechecker_on_br_if(&ctx->typechecker, depth));
  CHECK_RESULT(get_br_drop_keep_count(ctx, depth, &drop_count, &keep_count));
  /* flip the br_if so if <cond> is true it can drop values from the stack */
  CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::BrUnless));
  uint32_t fixup_br_offset = get_istream_offset(ctx);
  CHECK_RESULT(emit_i32(ctx, WABT_INVALID_OFFSET));
  CHECK_RESULT(emit_br(ctx, depth, drop_count, keep_count));
  CHECK_RESULT(emit_i32_at(ctx, fixup_br_offset, get_istream_offset(ctx)));
  return Result::Ok;
}

static Result on_br_table_expr(BinaryReaderContext* context,
                               uint32_t num_targets,
                               uint32_t* target_depths,
                               uint32_t default_target_depth) {
  Context* ctx = static_cast<Context*>(context->user_data);
  CHECK_RESULT(typechecker_begin_br_table(&ctx->typechecker));
  CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::BrTable));
  CHECK_RESULT(emit_i32(ctx, num_targets));
  uint32_t fixup_table_offset = get_istream_offset(ctx);
  CHECK_RESULT(emit_i32(ctx, WABT_INVALID_OFFSET));
  /* not necessary for the interpreter, but it makes it easier to disassemble.
   * This opcode specifies how many bytes of data follow. */
  CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::Data));
  CHECK_RESULT(emit_i32(ctx, (num_targets + 1) * WABT_TABLE_ENTRY_SIZE));
  CHECK_RESULT(emit_i32_at(ctx, fixup_table_offset, get_istream_offset(ctx)));

  for (uint32_t i = 0; i <= num_targets; ++i) {
    uint32_t depth = i != num_targets ? target_depths[i] : default_target_depth;
    CHECK_RESULT(typechecker_on_br_table_target(&ctx->typechecker, depth));
    CHECK_RESULT(emit_br_table_offset(ctx, depth));
  }

  CHECK_RESULT(typechecker_end_br_table(&ctx->typechecker));
  return Result::Ok;
}

static Result on_call_expr(uint32_t func_index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  InterpreterFunc* func = get_func_by_module_index(ctx, func_index);
  InterpreterFuncSignature* sig =
      get_signature_by_env_index(ctx, func->sig_index);
  CHECK_RESULT(typechecker_on_call(&ctx->typechecker, &sig->param_types,
                                   &sig->result_types));

  if (func->is_host) {
    CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::CallHost));
    CHECK_RESULT(emit_i32(ctx, translate_func_index_to_env(ctx, func_index)));
  } else {
    CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::Call));
    CHECK_RESULT(emit_func_offset(ctx, func->as_defined(), func_index));
  }

  return Result::Ok;
}

static Result on_call_indirect_expr(uint32_t sig_index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  if (ctx->module->table_index == WABT_INVALID_INDEX) {
    print_error(ctx, "found call_indirect operator, but no table");
    return Result::Error;
  }
  InterpreterFuncSignature* sig = get_signature_by_module_index(ctx, sig_index);
  CHECK_RESULT(typechecker_on_call_indirect(
      &ctx->typechecker, &sig->param_types, &sig->result_types));

  CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::CallIndirect));
  CHECK_RESULT(emit_i32(ctx, ctx->module->table_index));
  CHECK_RESULT(emit_i32(ctx, translate_sig_index_to_env(ctx, sig_index)));
  return Result::Ok;
}

static Result on_drop_expr(void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_RESULT(typechecker_on_drop(&ctx->typechecker));
  CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::Drop));
  return Result::Ok;
}

static Result on_i32_const_expr(uint32_t value, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_RESULT(typechecker_on_const(&ctx->typechecker, Type::I32));
  CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::I32Const));
  CHECK_RESULT(emit_i32(ctx, value));
  return Result::Ok;
}

static Result on_i64_const_expr(uint64_t value, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_RESULT(typechecker_on_const(&ctx->typechecker, Type::I64));
  CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::I64Const));
  CHECK_RESULT(emit_i64(ctx, value));
  return Result::Ok;
}

static Result on_f32_const_expr(uint32_t value_bits, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_RESULT(typechecker_on_const(&ctx->typechecker, Type::F32));
  CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::F32Const));
  CHECK_RESULT(emit_i32(ctx, value_bits));
  return Result::Ok;
}

static Result on_f64_const_expr(uint64_t value_bits, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_RESULT(typechecker_on_const(&ctx->typechecker, Type::F64));
  CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::F64Const));
  CHECK_RESULT(emit_i64(ctx, value_bits));
  return Result::Ok;
}

static Result on_get_global_expr(uint32_t global_index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_GLOBAL(ctx, global_index);
  Type type = get_global_type_by_module_index(ctx, global_index);
  CHECK_RESULT(typechecker_on_get_global(&ctx->typechecker, type));
  CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::GetGlobal));
  CHECK_RESULT(emit_i32(ctx, translate_global_index_to_env(ctx, global_index)));
  return Result::Ok;
}

static Result on_set_global_expr(uint32_t global_index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_GLOBAL(ctx, global_index);
  InterpreterGlobal* global = get_global_by_module_index(ctx, global_index);
  if (!global->mutable_) {
    print_error(ctx, "can't set_global on immutable global at index %u.",
                global_index);
    return Result::Error;
  }
  CHECK_RESULT(
      typechecker_on_set_global(&ctx->typechecker, global->typed_value.type));
  CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::SetGlobal));
  CHECK_RESULT(emit_i32(ctx, translate_global_index_to_env(ctx, global_index)));
  return Result::Ok;
}

static uint32_t translate_local_index(Context* ctx, uint32_t local_index) {
  return ctx->typechecker.type_stack.size() +
         ctx->current_func->param_and_local_types.size() - local_index;
}

static Result on_get_local_expr(uint32_t local_index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_LOCAL(ctx, local_index);
  Type type = get_local_type_by_index(ctx->current_func, local_index);
  /* Get the translated index before calling typechecker_on_get_local
   * because it will update the type stack size. We need the index to be
   * relative to the old stack size. */
  uint32_t translated_local_index = translate_local_index(ctx, local_index);
  CHECK_RESULT(typechecker_on_get_local(&ctx->typechecker, type));
  CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::GetLocal));
  CHECK_RESULT(emit_i32(ctx, translated_local_index));
  return Result::Ok;
}

static Result on_set_local_expr(uint32_t local_index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_LOCAL(ctx, local_index);
  Type type = get_local_type_by_index(ctx->current_func, local_index);
  CHECK_RESULT(typechecker_on_set_local(&ctx->typechecker, type));
  CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::SetLocal));
  CHECK_RESULT(emit_i32(ctx, translate_local_index(ctx, local_index)));
  return Result::Ok;
}

static Result on_tee_local_expr(uint32_t local_index, void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_LOCAL(ctx, local_index);
  Type type = get_local_type_by_index(ctx->current_func, local_index);
  CHECK_RESULT(typechecker_on_tee_local(&ctx->typechecker, type));
  CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::TeeLocal));
  CHECK_RESULT(emit_i32(ctx, translate_local_index(ctx, local_index)));
  return Result::Ok;
}

static Result on_grow_memory_expr(void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_RESULT(check_has_memory(ctx, Opcode::GrowMemory));
  CHECK_RESULT(typechecker_on_grow_memory(&ctx->typechecker));
  CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::GrowMemory));
  CHECK_RESULT(emit_i32(ctx, ctx->module->memory_index));
  return Result::Ok;
}

static Result on_load_expr(Opcode opcode,
                           uint32_t alignment_log2,
                           uint32_t offset,
                           void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_RESULT(check_has_memory(ctx, opcode));
  CHECK_RESULT(
      check_align(ctx, alignment_log2, get_opcode_memory_size(opcode)));
  CHECK_RESULT(typechecker_on_load(&ctx->typechecker, opcode));
  CHECK_RESULT(emit_opcode(ctx, opcode));
  CHECK_RESULT(emit_i32(ctx, ctx->module->memory_index));
  CHECK_RESULT(emit_i32(ctx, offset));
  return Result::Ok;
}

static Result on_store_expr(Opcode opcode,
                            uint32_t alignment_log2,
                            uint32_t offset,
                            void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_RESULT(check_has_memory(ctx, opcode));
  CHECK_RESULT(
      check_align(ctx, alignment_log2, get_opcode_memory_size(opcode)));
  CHECK_RESULT(typechecker_on_store(&ctx->typechecker, opcode));
  CHECK_RESULT(emit_opcode(ctx, opcode));
  CHECK_RESULT(emit_i32(ctx, ctx->module->memory_index));
  CHECK_RESULT(emit_i32(ctx, offset));
  return Result::Ok;
}

static Result on_current_memory_expr(void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_RESULT(check_has_memory(ctx, Opcode::CurrentMemory));
  CHECK_RESULT(typechecker_on_current_memory(&ctx->typechecker));
  CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::CurrentMemory));
  CHECK_RESULT(emit_i32(ctx, ctx->module->memory_index));
  return Result::Ok;
}

static Result on_nop_expr(void* user_data) {
  return Result::Ok;
}

static Result on_return_expr(void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  uint32_t drop_count, keep_count;
  CHECK_RESULT(get_return_drop_keep_count(ctx, &drop_count, &keep_count));
  CHECK_RESULT(typechecker_on_return(&ctx->typechecker));
  CHECK_RESULT(emit_drop_keep(ctx, drop_count, keep_count));
  CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::Return));
  return Result::Ok;
}

static Result on_select_expr(void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_RESULT(typechecker_on_select(&ctx->typechecker));
  CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::Select));
  return Result::Ok;
}

static Result on_unreachable_expr(void* user_data) {
  Context* ctx = static_cast<Context*>(user_data);
  CHECK_RESULT(typechecker_on_unreachable(&ctx->typechecker));
  CHECK_RESULT(emit_opcode(ctx, InterpreterOpcode::Unreachable));
  return Result::Ok;
}

Result read_binary_interpreter(InterpreterEnvironment* env,
                               const void* data,
                               size_t size,
                               const ReadBinaryOptions* options,
                               BinaryErrorHandler* error_handler,
                               DefinedInterpreterModule** out_module) {
  Context ctx;
  BinaryReader reader;

  InterpreterEnvironmentMark mark = mark_interpreter_environment(env);

  DefinedInterpreterModule* module =
      new DefinedInterpreterModule(env->istream.size);
  env->modules.emplace_back(module);

  WABT_ZERO_MEMORY(reader);

  ctx.reader = &reader;
  ctx.error_handler = error_handler;
  ctx.env = env;
  ctx.module = module;
  ctx.istream_offset = env->istream.size;
  CHECK_RESULT(init_mem_writer_existing(&ctx.istream_writer, &env->istream));

  TypeCheckerErrorHandler tc_error_handler;
  tc_error_handler.on_error = on_typechecker_error;
  tc_error_handler.user_data = &ctx;
  ctx.typechecker.error_handler = &tc_error_handler;

  WABT_ZERO_MEMORY(reader);
  reader.user_data = &ctx;
  reader.on_error = on_error;
  reader.on_signature_count = on_signature_count;
  reader.on_signature = on_signature;
  reader.on_import_count = on_import_count;
  reader.on_import = on_import;
  reader.on_import_func = on_import_func;
  reader.on_import_table = on_import_table;
  reader.on_import_memory = on_import_memory;
  reader.on_import_global = on_import_global;
  reader.on_function_signatures_count = on_function_signatures_count;
  reader.on_function_signature = on_function_signature;
  reader.on_table = on_table;
  reader.on_memory = on_memory;
  reader.on_global_count = on_global_count;
  reader.begin_global = begin_global;
  reader.end_global_init_expr = end_global_init_expr;
  reader.on_export = on_export;
  reader.on_start_function = on_start_function;
  reader.begin_function_body = begin_function_body;
  reader.on_local_decl_count = on_local_decl_count;
  reader.on_local_decl = on_local_decl;
  reader.on_binary_expr = on_binary_expr;
  reader.on_block_expr = on_block_expr;
  reader.on_br_expr = on_br_expr;
  reader.on_br_if_expr = on_br_if_expr;
  reader.on_br_table_expr = on_br_table_expr;
  reader.on_call_expr = on_call_expr;
  reader.on_call_indirect_expr = on_call_indirect_expr;
  reader.on_compare_expr = on_binary_expr;
  reader.on_convert_expr = on_unary_expr;
  reader.on_current_memory_expr = on_current_memory_expr;
  reader.on_drop_expr = on_drop_expr;
  reader.on_else_expr = on_else_expr;
  reader.on_end_expr = on_end_expr;
  reader.on_f32_const_expr = on_f32_const_expr;
  reader.on_f64_const_expr = on_f64_const_expr;
  reader.on_get_global_expr = on_get_global_expr;
  reader.on_get_local_expr = on_get_local_expr;
  reader.on_grow_memory_expr = on_grow_memory_expr;
  reader.on_i32_const_expr = on_i32_const_expr;
  reader.on_i64_const_expr = on_i64_const_expr;
  reader.on_if_expr = on_if_expr;
  reader.on_load_expr = on_load_expr;
  reader.on_loop_expr = on_loop_expr;
  reader.on_nop_expr = on_nop_expr;
  reader.on_return_expr = on_return_expr;
  reader.on_select_expr = on_select_expr;
  reader.on_set_global_expr = on_set_global_expr;
  reader.on_set_local_expr = on_set_local_expr;
  reader.on_store_expr = on_store_expr;
  reader.on_tee_local_expr = on_tee_local_expr;
  reader.on_unary_expr = on_unary_expr;
  reader.on_unreachable_expr = on_unreachable_expr;
  reader.end_function_body = end_function_body;
  reader.end_elem_segment_init_expr = end_elem_segment_init_expr;
  reader.on_elem_segment_function_index = on_elem_segment_function_index_check;
  reader.on_data_segment_data = on_data_segment_data_check;
  reader.on_init_expr_f32_const_expr = on_init_expr_f32_const_expr;
  reader.on_init_expr_f64_const_expr = on_init_expr_f64_const_expr;
  reader.on_init_expr_get_global_expr = on_init_expr_get_global_expr;
  reader.on_init_expr_i32_const_expr = on_init_expr_i32_const_expr;
  reader.on_init_expr_i64_const_expr = on_init_expr_i64_const_expr;

  const uint32_t num_function_passes = 1;
  Result result =
      read_binary(data, size, &reader, num_function_passes, options);
  steal_mem_writer_output_buffer(&ctx.istream_writer, &env->istream);
  if (WABT_SUCCEEDED(result)) {
    /* Another pass on the read binary to assign data and elem segments. */
    WABT_ZERO_MEMORY(reader);
    reader.user_data = &ctx;
    reader.on_error = on_error;
    reader.end_elem_segment_init_expr = end_elem_segment_init_expr;
    reader.on_elem_segment_function_index = on_elem_segment_function_index;
    reader.on_data_segment_data = on_data_segment_data;
    reader.on_init_expr_f32_const_expr = on_init_expr_f32_const_expr;
    reader.on_init_expr_f64_const_expr = on_init_expr_f64_const_expr;
    reader.on_init_expr_get_global_expr = on_init_expr_get_global_expr;
    reader.on_init_expr_i32_const_expr = on_init_expr_i32_const_expr;
    reader.on_init_expr_i64_const_expr = on_init_expr_i64_const_expr;

    result = read_binary(data, size, &reader, num_function_passes, options);
    assert(WABT_SUCCEEDED(result));

    env->istream.size = ctx.istream_offset;
    ctx.module->istream_end = env->istream.size;
    *out_module = module;
  } else {
    reset_interpreter_environment_to_mark(env, mark);
    *out_module = nullptr;
  }
  return result;
}

}  // namespace wabt
