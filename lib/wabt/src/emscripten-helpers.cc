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

#include "apply-names.h"
#include "binary-reader.h"
#include "binary-reader-ir.h"
#include "binary-writer.h"
#include "binary-error-handler.h"
#include "common.h"
#include "ir.h"
#include "generate-names.h"
#include "resolve-names.h"
#include "source-error-handler.h"
#include "stream.h"
#include "validator.h"
#include "wast-lexer.h"
#include "wast-parser.h"
#include "wat-writer.h"
#include "writer.h"

struct WabtParseWastResult {
  wabt::Result result;
  std::unique_ptr<wabt::Script> script;
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

WabtParseWastResult* wabt_parse_wast(
    wabt::WastLexer* lexer,
    wabt::SourceErrorHandlerBuffer* error_handler) {
  WabtParseWastResult* result = new WabtParseWastResult();
  wabt::Script* script = nullptr;
  result->result = wabt::parse_wast(lexer, &script, error_handler);
  result->script.reset(script);
  return result;
}

WabtReadBinaryResult* wabt_read_binary(
    const void* data,
    size_t size,
    int read_debug_names,
    wabt::BinaryErrorHandlerBuffer* error_handler) {
  wabt::ReadBinaryOptions options;
  options.log_stream = nullptr;
  options.read_debug_names = read_debug_names;

  WabtReadBinaryResult* result = new WabtReadBinaryResult();
  wabt::Module* module = new wabt::Module();
  result->result =
      wabt::read_binary_ir(data, size, &options, error_handler, module);
  result->module.reset(module);
  return result;
}

wabt::Result wabt_resolve_names_script(
    wabt::WastLexer* lexer,
    wabt::Script* script,
    wabt::SourceErrorHandlerBuffer* error_handler) {
  return resolve_names_script(lexer, script, error_handler);
}

wabt::Result wabt_resolve_names_module(
    wabt::WastLexer* lexer,
    wabt::Module* module,
    wabt::SourceErrorHandlerBuffer* error_handler) {
  return resolve_names_module(lexer, module, error_handler);
}

wabt::Result wabt_validate_script(
    wabt::WastLexer* lexer,
    wabt::Script* script,
    wabt::SourceErrorHandlerBuffer* error_handler) {
  return validate_script(lexer, script, error_handler);
}

wabt::Result wabt_apply_names_module(wabt::Module* module) {
  return apply_names(module);
}

wabt::Result wabt_generate_names_module(wabt::Module* module) {
  return generate_names(module);
}

wabt::Module* wabt_get_first_module(wabt::Script* script) {
  return script->GetFirstModule();
}

WabtWriteModuleResult* wabt_write_binary_module(wabt::Module* module,
                                                int log,
                                                int canonicalize_lebs,
                                                int relocatable,
                                                int write_debug_names) {
  wabt::MemoryStream stream;
  wabt::WriteBinaryOptions options;
  options.log_stream = log ? &stream : nullptr;
  options.canonicalize_lebs = canonicalize_lebs;
  options.relocatable = relocatable;
  options.write_debug_names = write_debug_names;

  wabt::MemoryWriter writer;
  WabtWriteModuleResult* result = new WabtWriteModuleResult();
  result->result = write_binary_module(&writer, module, &options);
  if (result->result == wabt::Result::Ok) {
    result->buffer = writer.ReleaseOutputBuffer();
    result->log_buffer = log ? stream.ReleaseOutputBuffer() : nullptr;
  }
  return result;
}

WabtWriteModuleResult* wabt_write_text_module(wabt::Module* module,
                                              int fold_exprs,
                                              int inline_export) {
  wabt::MemoryStream stream;
  wabt::WriteWatOptions options;
  options.fold_exprs = fold_exprs;
  options.inline_export = inline_export;

  wabt::MemoryWriter writer;
  WabtWriteModuleResult* result = new WabtWriteModuleResult();
  result->result = write_wat(&writer, module, &options);
  if (result->result == wabt::Result::Ok) {
    result->buffer = writer.ReleaseOutputBuffer();
  }
  return result;
}

void wabt_destroy_script(wabt::Script* script) {
  delete script;
}

void wabt_destroy_module(wabt::Module* module) {
  delete module;
}

void wabt_destroy_wast_lexer(wabt::WastLexer* lexer) {
  delete lexer;
}

// SourceErrorHandlerBuffer
wabt::SourceErrorHandlerBuffer* wabt_new_source_error_handler_buffer(void) {
  return new wabt::SourceErrorHandlerBuffer();
}

const void* wabt_source_error_handler_buffer_get_data(
    wabt::SourceErrorHandlerBuffer* error_handler) {
  return error_handler->buffer().data();
}

size_t wabt_source_error_handler_buffer_get_size(
    wabt::SourceErrorHandlerBuffer* error_handler) {
  return error_handler->buffer().size();
}

void wabt_destroy_source_error_handler_buffer(
    wabt::SourceErrorHandlerBuffer* error_handler) {
  delete error_handler;
}

// BinaryErrorHandlerBuffer
wabt::BinaryErrorHandlerBuffer* wabt_new_binary_error_handler_buffer(void) {
  return new wabt::BinaryErrorHandlerBuffer();
}

const void* wabt_binary_error_handler_buffer_get_data(
    wabt::BinaryErrorHandlerBuffer* error_handler) {
  return error_handler->buffer().data();
}

size_t wabt_binary_error_handler_buffer_get_size(
    wabt::BinaryErrorHandlerBuffer* error_handler) {
  return error_handler->buffer().size();
}

void wabt_destroy_binary_error_handler_buffer(
    wabt::BinaryErrorHandlerBuffer* error_handler) {
  delete error_handler;
}

// WabtParseWastResult
wabt::Result wabt_parse_wast_result_get_result(WabtParseWastResult* result) {
  return result->result;
}

wabt::Script* wabt_parse_wast_result_release_script(
    WabtParseWastResult* result) {
  return result->script.release();
}

void wabt_destroy_parse_wast_result(WabtParseWastResult* result) {
  delete result;
}

// WabtReadBinaryResult
wabt::Result wabt_read_binary_result_get_result(WabtReadBinaryResult* result) {
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
wabt::Result wabt_write_module_result_get_result(
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
