/*
 * Copyright 2017 WebAssembly Community Group participants
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

#include "src/binary-reader.h"
#include "src/binary-reader-ir.h"
#include "src/error-handler.h"
#include "src/ir.h"
#include "src/option-parser.h"
#include "src/stream.h"
#include "src/validator.h"
#include "src/wast-lexer.h"

using namespace wabt;

static int s_verbose;
static std::string s_infile;
static Features s_features;
static bool s_read_debug_names = true;
static std::unique_ptr<FileStream> s_log_stream;

static const char s_description[] =
R"(  Read a file in the WebAssembly binary format, and validate it.

examples:
  # validate binary file test.wasm
  $ wasm-validate test.wasm
)";

static void ParseOptions(int argc, char** argv) {
  OptionParser parser("wasm-validate", s_description);

  parser.AddOption('v', "verbose", "Use multiple times for more info", []() {
    s_verbose++;
    s_log_stream = FileStream::CreateStdout();
  });
  parser.AddHelpOption();
  s_features.AddOptions(&parser);
  parser.AddOption("no-debug-names", "Ignore debug names in the binary file",
                   []() { s_read_debug_names = false; });
  parser.AddArgument("filename", OptionParser::ArgumentCount::One,
                     [](const char* argument) {
                       s_infile = argument;
                       ConvertBackslashToSlash(&s_infile);
                     });
  parser.Parse(argc, argv);
}

int ProgramMain(int argc, char** argv) {
  Result result;

  InitStdio();
  ParseOptions(argc, argv);

  std::vector<uint8_t> file_data;
  result = ReadFile(s_infile.c_str(), &file_data);
  if (Succeeded(result)) {
    ErrorHandlerFile error_handler(Location::Type::Binary);
    Module module;
    const bool kStopOnFirstError = true;
    ReadBinaryOptions options(s_features, s_log_stream.get(),
                              s_read_debug_names, kStopOnFirstError);
    result = ReadBinaryIr(s_infile.c_str(), file_data.data(),
                          file_data.size(), &options, &error_handler, &module);
    if (Succeeded(result)) {
      WastLexer* lexer = nullptr;
      ValidateOptions options(s_features);
      result = ValidateModule(lexer, &module, &error_handler, &options);
    }
  }
  return result != Result::Ok;
}

int main(int argc, char** argv) {
  WABT_TRY
  return ProgramMain(argc, argv);
  WABT_CATCH_BAD_ALLOC_AND_EXIT
}
