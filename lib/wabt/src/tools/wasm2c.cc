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

#include "src/apply-names.h"
#include "src/binary-reader.h"
#include "src/binary-reader-ir.h"
#include "src/error-handler.h"
#include "src/feature.h"
#include "src/generate-names.h"
#include "src/ir.h"
#include "src/option-parser.h"
#include "src/stream.h"
#include "src/validator.h"
#include "src/wast-lexer.h"

#include "src/c-writer.h"

using namespace wabt;

static int s_verbose;
static std::string s_infile;
static std::string s_outfile;
static Features s_features;
static WriteCOptions s_write_c_options;
static bool s_read_debug_names = true;
static std::unique_ptr<FileStream> s_log_stream;

static const char s_description[] =
R"(  Read a file in the WebAssembly binary format, and convert it to
  a C source file and header.

examples:
  # parse binary file test.wasm and write test.c and test.h
  $ wasm2c test.wasm -o test.c

  # parse test.wasm, write test.c and test.h, but ignore the debug names, if any
  $ wasm2c test.wasm --no-debug-names -o test.c
)";

static void ParseOptions(int argc, char** argv) {
  OptionParser parser("wasm2c", s_description);

  parser.AddOption('v', "verbose", "Use multiple times for more info", []() {
    s_verbose++;
    s_log_stream = FileStream::CreateStdout();
  });
  parser.AddHelpOption();
  parser.AddOption(
      'o', "output", "FILENAME",
      "Output file for the generated C source file, by default use stdout",
      [](const char* argument) {
        s_outfile = argument;
        ConvertBackslashToSlash(&s_outfile);
      });
  s_features.AddOptions(&parser);
  parser.AddOption("no-debug-names", "Ignore debug names in the binary file",
                   []() { s_read_debug_names = false; });
  parser.AddArgument("filename", OptionParser::ArgumentCount::One,
                     [](const char* argument) {
                       s_infile = argument;
                       ConvertBackslashToSlash(&s_infile);
                     });
  parser.Parse(argc, argv);

  // TODO(binji): currently wasm2c doesn't support any feature flags.
  bool any_feature_enabled = false;
#define WABT_FEATURE(variable, flag, help) \
  any_feature_enabled |= s_features.variable##_enabled();
#include "src/feature.def"
#undef WABT_FEATURE

  if (any_feature_enabled) {
    fprintf(stderr, "wasm2c doesn't currently support any --enable-* flags.\n");
    exit(1);
  }
}

// TODO(binji): copied from binary-writer-spec.cc, probably should share.
static string_view strip_extension(string_view s) {
  string_view ext = s.substr(s.find_last_of('.'));
  string_view result = s;

  if (ext == ".c")
    result.remove_suffix(ext.length());
  return result;
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
    const bool kFailOnCustomSectionError = true;
    ReadBinaryOptions options(s_features, s_log_stream.get(),
                              s_read_debug_names, kStopOnFirstError,
                              kFailOnCustomSectionError);
    result = ReadBinaryIr(s_infile.c_str(), file_data.data(), file_data.size(),
                          &options, &error_handler, &module);
    if (Succeeded(result)) {
      if (Succeeded(result)) {
        ValidateOptions options(s_features);
        WastLexer* lexer = nullptr;
        result = ValidateModule(lexer, &module, &error_handler, &options);
        result |= GenerateNames(&module);
      }

      if (Succeeded(result)) {
        /* TODO(binji): This shouldn't fail; if a name can't be applied
         * (because the index is invalid, say) it should just be skipped. */
        Result dummy_result = ApplyNames(&module);
        WABT_USE(dummy_result);
      }

      if (Succeeded(result)) {
        if (!s_outfile.empty()) {
          std::string header_name =
              strip_extension(s_outfile).to_string() + ".h";
          FileStream c_stream(s_outfile.c_str());
          FileStream h_stream(header_name);
          result = WriteC(&c_stream, &h_stream, header_name.c_str(), &module,
                          &s_write_c_options);
        } else {
          FileStream stream(stdout);
          result =
              WriteC(&stream, &stream, "wasm.h", &module, &s_write_c_options);
        }
      }
    }
  }
  return result != Result::Ok;
}

int main(int argc, char** argv) {
  WABT_TRY
  return ProgramMain(argc, argv);
  WABT_CATCH_BAD_ALLOC_AND_EXIT
}

