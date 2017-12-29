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

#include <memory>

#include "src/apply-names.h"
#include "src/binary-reader.h"
#include "src/binary-reader-ir.h"
#include "src/binary-writer.h"
#include "src/common.h"
#include "src/error-handler.h"
#include "src/ir.h"
#include "src/generate-names.h"
#include "src/resolve-names.h"
#include "src/stream.h"
#include "src/validator.h"
#include "src/wast-lexer.h"
#include "src/wast-parser.h"
#include "src/wat-writer.h"

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
  std::unique_ptr<wabt::OutputBuffer> buffer;
  std::unique_ptr<wabt::OutputBuffer> log_buffer;
};

extern "C" {

wabt::WastLexer* wabt_new_wast_buffer_lexer(const char* filename,
                                            const void* data,
                                            size_t size) {
  std::unique_ptr<wabt::WastLexer> lexer =
      wabt::WastLexer::CreateBufferLexer(filename, data, size);
  return lexer.release();
}

WabtParseWatResult* wabt_parse_wat(wabt::WastLexer* lexer,
                                   wabt::ErrorHandlerBuffer* error_handler) {
  WabtParseWatResult* result = new WabtParseWatResult();
  std::unique_ptr<wabt::Module> module;
  result->result = wabt::ParseWatModule(lexer, &module, error_handler);
  result->module = std::move(module);
  return result;
}

WabtReadBinaryResult* wabt_read_binary(
    const void* data,
    size_t size,
    int read_debug_names,
    wabt::ErrorHandlerBuffer* error_handler) {
  wabt::ReadBinaryOptions options;
  options.read_debug_names = read_debug_names;

  WabtReadBinaryResult* result = new WabtReadBinaryResult();
  wabt::Module* module = new wabt::Module();
  // TODO(binji): Pass through from wabt_read_binary parameter.
  const char* filename = "<binary>";
  result->result =
      wabt::ReadBinaryIr(filename, data, size, &options, error_handler, module);
  result->module.reset(module);
  return result;
}

wabt::Result::Enum wabt_resolve_names_module(
    wabt::WastLexer* lexer,
    wabt::Module* module,
    wabt::ErrorHandlerBuffer* error_handler) {
  return ResolveNamesModule(lexer, module, error_handler);
}

wabt::Result::Enum wabt_validate_module(
    wabt::WastLexer* lexer,
    wabt::Module* module,
    wabt::ErrorHandlerBuffer* error_handler) {
  wabt::ValidateOptions options;
  return ValidateModule(lexer, module, error_handler, &options);
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
  result->result = WriteBinaryModule(&stream, module, &options);
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
  result->result = WriteWat(&stream, module, &options);
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

// ErrorHandlerBuffer
wabt::ErrorHandlerBuffer* wabt_new_text_error_handler_buffer(void) {
  return new wabt::ErrorHandlerBuffer(wabt::Location::Type::Text);
}

wabt::ErrorHandlerBuffer* wabt_new_binary_error_handler_buffer(void) {
  return new wabt::ErrorHandlerBuffer(wabt::Location::Type::Binary);
}

const void* wabt_error_handler_buffer_get_data(
    wabt::ErrorHandlerBuffer* error_handler) {
  return error_handler->buffer().data();
}

size_t wabt_error_handler_buffer_get_size(
    wabt::ErrorHandlerBuffer* error_handler) {
  return error_handler->buffer().size();
}

void wabt_destroy_error_handler_buffer(
    wabt::ErrorHandlerBuffer* error_handler) {
  delete error_handler;
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

}  // extern "C"

#endif /* WABT_EMSCRIPTEN_HELPERS_H_ */
