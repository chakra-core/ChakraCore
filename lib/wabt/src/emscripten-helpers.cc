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

#ifndef WABT_EMSCRIPTEN_HELPERS_H_
#define WABT_EMSCRIPTEN_HELPERS_H_

#include <cstddef>

#include <algorithm>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include "src/apply-names.h"
#include "src/binary-reader-ir.h"
#include "src/binary-reader.h"
#include "src/binary-writer-spec.h"
#include "src/binary-writer.h"
#include "src/common.h"
#include "src/error-formatter.h"
#include "src/feature.h"
#include "src/filenames.h"
#include "src/generate-names.h"
#include "src/ir.h"
#include "src/resolve-names.h"
#include "src/stream.h"
#include "src/validator.h"
#include "src/wast-lexer.h"
#include "src/wast-parser.h"
#include "src/wat-writer.h"

typedef std::unique_ptr<wabt::OutputBuffer> WabtOutputBufferPtr;
typedef std::pair<std::string, WabtOutputBufferPtr>
    WabtFilenameOutputBufferPair;

struct WabtParseWatResult {
  wabt::Result result;
  std::unique_ptr<wabt::Module> module;
};

struct WabtReadBinaryResult {
  wabt::Result result;
  std::unique_ptr<wabt::Module> module;
};

struct WabtWriteModuleResult {
  wabt::Result result;
  WabtOutputBufferPtr buffer;
  WabtOutputBufferPtr log_buffer;
};

struct WabtWriteScriptResult {
  wabt::Result result;
  WabtOutputBufferPtr json_buffer;
  WabtOutputBufferPtr log_buffer;
  std::vector<WabtFilenameOutputBufferPair> module_buffers;
};

struct WabtParseWastResult {
  wabt::Result result;
  std::unique_ptr<wabt::Script> script;
};

extern "C" {

wabt::Features* wabt_new_features(void) {
  return new wabt::Features();
}

void wabt_destroy_features(wabt::Features* f) {
  delete f;
}

#define WABT_FEATURE(variable, flag, default_, help)                   \
  bool wabt_##variable##_enabled(wabt::Features* f) {                  \
    return f->variable##_enabled();                                    \
  }                                                                    \
  void wabt_set_##variable##_enabled(wabt::Features* f, int enabled) { \
    f->set_##variable##_enabled(enabled);                              \
  }
#include "src/feature.def"
#undef WABT_FEATURE

wabt::WastLexer* wabt_new_wast_buffer_lexer(const char* filename,
                                            const void* data,
                                            size_t size) {
  std::unique_ptr<wabt::WastLexer> lexer =
      wabt::WastLexer::CreateBufferLexer(filename, data, size);
  return lexer.release();
}

WabtParseWatResult* wabt_parse_wat(wabt::WastLexer* lexer,
                                   wabt::Features* features,
                                   wabt::Errors* errors) {
  wabt::WastParseOptions options(*features);
  WabtParseWatResult* result = new WabtParseWatResult();
  std::unique_ptr<wabt::Module> module;
  result->result = wabt::ParseWatModule(lexer, &module, errors, &options);
  result->module = std::move(module);
  return result;
}

WabtParseWastResult* wabt_parse_wast(wabt::WastLexer* lexer,
                                     wabt::Features* features,
                                     wabt::Errors* errors) {
  wabt::WastParseOptions options(*features);
  WabtParseWastResult* result = new WabtParseWastResult();
  std::unique_ptr<wabt::Script> script;
  result->result = wabt::ParseWastScript(lexer, &script, errors, &options);
  result->script = std::move(script);
  return result;
}

WabtReadBinaryResult* wabt_read_binary(const void* data,
                                       size_t size,
                                       int read_debug_names,
                                       wabt::Features* features,
                                       wabt::Errors* errors) {
  wabt::ReadBinaryOptions options;
  options.features = *features;
  options.read_debug_names = read_debug_names;

  WabtReadBinaryResult* result = new WabtReadBinaryResult();
  wabt::Module* module = new wabt::Module();
  // TODO(binji): Pass through from wabt_read_binary parameter.
  const char* filename = "<binary>";
  result->result =
      wabt::ReadBinaryIr(filename, data, size, options, errors, module);
  result->module.reset(module);
  return result;
}

wabt::Result::Enum wabt_resolve_names_module(wabt::Module* module,
                                             wabt::Errors* errors) {
  return ResolveNamesModule(module, errors);
}

wabt::Result::Enum wabt_validate_module(wabt::Module* module,
                                        wabt::Features* features,
                                        wabt::Errors* errors) {
  wabt::ValidateOptions options;
  options.features = *features;
  return ValidateModule(module, errors, options);
}

wabt::Result::Enum wabt_validate_script(wabt::Script* script,
                                        wabt::Features* features,
                                        wabt::Errors* errors) {
  wabt::ValidateOptions options;
  options.features = *features;
  return ValidateScript(script, errors, options);
}

WabtWriteScriptResult* wabt_write_binary_spec_script(
    wabt::Script* script,
    const char* source_filename,
    const char* out_filename,
    int log,
    int canonicalize_lebs,
    int relocatable,
    int write_debug_names) {
  wabt::MemoryStream log_stream;
  wabt::MemoryStream* log_stream_p = log ? &log_stream : nullptr;

  wabt::WriteBinaryOptions options;
  options.canonicalize_lebs = canonicalize_lebs;
  options.relocatable = relocatable;
  options.write_debug_names = write_debug_names;

  std::vector<wabt::FilenameMemoryStreamPair> module_streams;
  wabt::MemoryStream json_stream(log_stream_p);

  std::string module_filename_noext =
      wabt::StripExtension(out_filename ? out_filename : source_filename)
          .to_string();

  WabtWriteScriptResult* result = new WabtWriteScriptResult();
  result->result = WriteBinarySpecScript(&json_stream, script, source_filename,
                                         module_filename_noext, options,
                                         &module_streams, log_stream_p);

  if (result->result == wabt::Result::Ok) {
    result->json_buffer = json_stream.ReleaseOutputBuffer();
    result->log_buffer = log ? log_stream.ReleaseOutputBuffer() : nullptr;
    std::transform(module_streams.begin(), module_streams.end(),
                   std::back_inserter(result->module_buffers),
                   [](wabt::FilenameMemoryStreamPair& pair) {
                     return WabtFilenameOutputBufferPair(
                         pair.filename, pair.stream->ReleaseOutputBuffer());
                   });
  }
  return result;
}

wabt::Result::Enum wabt_apply_names_module(wabt::Module* module) {
  return ApplyNames(module);
}

wabt::Result::Enum wabt_generate_names_module(wabt::Module* module) {
  return GenerateNames(module);
}

WabtWriteModuleResult* wabt_write_binary_module(wabt::Module* module,
                                                int log,
                                                int canonicalize_lebs,
                                                int relocatable,
                                                int write_debug_names) {
  wabt::MemoryStream log_stream;
  wabt::WriteBinaryOptions options;
  options.canonicalize_lebs = canonicalize_lebs;
  options.relocatable = relocatable;
  options.write_debug_names = write_debug_names;

  wabt::MemoryStream stream(log ? &log_stream : nullptr);
  WabtWriteModuleResult* result = new WabtWriteModuleResult();
  result->result = WriteBinaryModule(&stream, module, options);
  if (result->result == wabt::Result::Ok) {
    result->buffer = stream.ReleaseOutputBuffer();
    result->log_buffer = log ? log_stream.ReleaseOutputBuffer() : nullptr;
  }
  return result;
}

WabtWriteModuleResult* wabt_write_text_module(wabt::Module* module,
                                              int fold_exprs,
                                              int inline_export) {
  wabt::WriteWatOptions options;
  options.fold_exprs = fold_exprs;
  options.inline_export = inline_export;

  wabt::MemoryStream stream;
  WabtWriteModuleResult* result = new WabtWriteModuleResult();
  result->result = WriteWat(&stream, module, options);
  if (result->result == wabt::Result::Ok) {
    result->buffer = stream.ReleaseOutputBuffer();
  }
  return result;
}

void wabt_destroy_module(wabt::Module* module) {
  delete module;
}

void wabt_destroy_wast_lexer(wabt::WastLexer* lexer) {
  delete lexer;
}

// Errors
wabt::Errors* wabt_new_errors(void) {
  return new wabt::Errors();
}

wabt::OutputBuffer* wabt_format_text_errors(wabt::Errors* errors,
                                            wabt::WastLexer* lexer) {
  auto line_finder = lexer->MakeLineFinder();
  std::string string_result = FormatErrorsToString(
      *errors, wabt::Location::Type::Text, line_finder.get());

  wabt::OutputBuffer* result = new wabt::OutputBuffer();
  std::copy(string_result.begin(), string_result.end(),
            std::back_inserter(result->data));
  return result;
}

wabt::OutputBuffer* wabt_format_binary_errors(wabt::Errors* errors) {
  std::string string_result =
      FormatErrorsToString(*errors, wabt::Location::Type::Binary);

  wabt::OutputBuffer* result = new wabt::OutputBuffer();
  std::copy(string_result.begin(), string_result.end(),
            std::back_inserter(result->data));
  return result;
}

void wabt_destroy_errors(wabt::Errors* errors) {
  delete errors;
}

// WabtParseWatResult
wabt::Result::Enum wabt_parse_wat_result_get_result(
    WabtParseWatResult* result) {
  return result->result;
}

wabt::Module* wabt_parse_wat_result_release_module(WabtParseWatResult* result) {
  return result->module.release();
}

void wabt_destroy_parse_wat_result(WabtParseWatResult* result) {
  delete result;
}

// WabtParseWastResult
wabt::Result::Enum wabt_parse_wast_result_get_result(
    WabtParseWastResult* result) {
  return result->result;
}

wabt::Script* wabt_parse_wast_result_release_module(
    WabtParseWastResult* result) {
  return result->script.release();
}

void wabt_destroy_parse_wast_result(WabtParseWastResult* result) {
  delete result;
}

// WabtReadBinaryResult
wabt::Result::Enum wabt_read_binary_result_get_result(
    WabtReadBinaryResult* result) {
  return result->result;
}

wabt::Module* wabt_read_binary_result_release_module(
    WabtReadBinaryResult* result) {
  return result->module.release();
}

void wabt_destroy_read_binary_result(WabtReadBinaryResult* result) {
  delete result;
}

// WabtWriteModuleResult
wabt::Result::Enum wabt_write_module_result_get_result(
    WabtWriteModuleResult* result) {
  return result->result;
}

wabt::OutputBuffer* wabt_write_module_result_release_output_buffer(
    WabtWriteModuleResult* result) {
  return result->buffer.release();
}

wabt::OutputBuffer* wabt_write_module_result_release_log_output_buffer(
    WabtWriteModuleResult* result) {
  return result->log_buffer.release();
}

void wabt_destroy_write_module_result(WabtWriteModuleResult* result) {
  delete result;
}

// WabtWriteScriptResult
wabt::Result::Enum wabt_write_script_result_get_result(
    WabtWriteScriptResult* result) {
  return result->result;
}

wabt::OutputBuffer* wabt_write_script_result_release_json_output_buffer(
    WabtWriteScriptResult* result) {
  return result->json_buffer.release();
}

wabt::OutputBuffer* wabt_write_script_result_release_log_output_buffer(
    WabtWriteScriptResult* result) {
  return result->log_buffer.release();
}

size_t wabt_write_script_result_get_module_count(
    WabtWriteScriptResult* result) {
  return result->module_buffers.size();
}

const char* wabt_write_script_result_get_module_filename(
    WabtWriteScriptResult* result,
    size_t index) {
  return result->module_buffers[index].first.c_str();
}

wabt::OutputBuffer* wabt_write_script_result_release_module_output_buffer(
    WabtWriteScriptResult* result,
    size_t index) {
  return result->module_buffers[index].second.release();
}

void wabt_destroy_write_script_result(WabtWriteScriptResult* result) {
  delete result;
}

// wabt::OutputBuffer*
const void* wabt_output_buffer_get_data(wabt::OutputBuffer* output_buffer) {
  return output_buffer->data.data();
}

size_t wabt_output_buffer_get_size(wabt::OutputBuffer* output_buffer) {
  return output_buffer->data.size();
}

void wabt_destroy_output_buffer(wabt::OutputBuffer* output_buffer) {
  delete output_buffer;
}

// See https://github.com/kripken/emscripten/issues/7073.
void dummy_workaround_for_emscripten_issue_7073(void) {}

}  // extern "C"

#endif /* WABT_EMSCRIPTEN_HELPERS_H_ */
