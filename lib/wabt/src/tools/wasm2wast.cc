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

#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>

#include "apply-names.h"
#include "binary-error-handler.h"
#include "binary-reader.h"
#include "binary-reader-ir.h"
#include "generate-names.h"
#include "ir.h"
#include "option-parser.h"
#include "stream.h"
#include "wat-writer.h"
#include "writer.h"

using namespace wabt;

static int s_verbose;
static const char* s_infile;
static const char* s_outfile;
static ReadBinaryOptions s_read_binary_options = {nullptr, true};
static WriteWatOptions s_write_wat_options;
static bool s_generate_names;
static std::unique_ptr<FileStream> s_log_stream;

static const char s_description[] =
R"(  read a file in the wasm binary format, and convert it to the wasm
  s-expression file format.

examples:
  # parse binary file test.wasm and write s-expression file test.wast
  $ wasm2wast test.wasm -o test.wast

  # parse test.wasm, write test.wast, but ignore the debug names, if any
  $ wasm2wast test.wasm --no-debug-names -o test.wast
)";

static void parse_options(int argc, char** argv) {
  OptionParser parser("wasm2wast", s_description);

  parser.AddOption('v', "verbose", "Use multiple times for more info", []() {
    s_verbose++;
    s_log_stream = FileStream::CreateStdout();
    s_read_binary_options.log_stream = s_log_stream.get();
  });
  parser.AddHelpOption();
  parser.AddOption(
      'o', "output", "FILENAME",
      "Output file for the generated wast file, by default use stdout",
      [](const char* argument) { s_outfile = argument; });
  parser.AddOption('f', "fold-exprs", "Write folded expressions where possible",
                   []() { s_write_wat_options.fold_exprs = true; });
  parser.AddOption("inline-exports", "Write all exports inline",
                   []() { s_write_wat_options.inline_export = true; });
  parser.AddOption("no-debug-names", "Ignore debug names in the binary file",
                   []() { s_read_binary_options.read_debug_names = false; });
  parser.AddOption(
      "generate-names",
      "Give auto-generated names to non-named functions, types, etc.",
      []() { s_generate_names = true; });
  parser.AddArgument("filename", OptionParser::ArgumentCount::One,
                     [](const char* argument) { s_infile = argument; });
  parser.Parse(argc, argv);
}

int ProgramMain(int argc, char** argv) {
  Result result;

  init_stdio();
  parse_options(argc, argv);

  char* data;
  size_t size;
  result = read_file(s_infile, &data, &size);
  if (WABT_SUCCEEDED(result)) {
    BinaryErrorHandlerFile error_handler;
    Module module;
    result = read_binary_ir(data, size, &s_read_binary_options, &error_handler,
                            &module);
    if (WABT_SUCCEEDED(result)) {
      if (s_generate_names)
        result = generate_names(&module);

      if (WABT_SUCCEEDED(result)) {
        /* TODO(binji): This shouldn't fail; if a name can't be applied
         * (because the index is invalid, say) it should just be skipped. */
        Result dummy_result = apply_names(&module);
        WABT_USE(dummy_result);
      }

      if (WABT_SUCCEEDED(result)) {
        FileWriter writer(s_outfile ? FileWriter(s_outfile)
                                    : FileWriter(stdout));
        result = write_wat(&writer, &module, &s_write_wat_options);
      }
    }
    delete[] data;
  }
  return result != Result::Ok;
}

int main(int argc, char** argv) {
  WABT_TRY
  return ProgramMain(argc, argv);
  WABT_CATCH_BAD_ALLOC_AND_EXIT
}
