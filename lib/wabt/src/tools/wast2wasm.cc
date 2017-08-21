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
#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <string>

#include "config.h"

#include "binary-writer.h"
#include "binary-writer-spec.h"
#include "common.h"
#include "error-handler.h"
#include "feature.h"
#include "ir.h"
#include "option-parser.h"
#include "resolve-names.h"
#include "stream.h"
#include "validator.h"
#include "wast-parser.h"
#include "writer.h"

using namespace wabt;

static const char* s_infile;
static const char* s_outfile;
static bool s_dump_module;
static int s_verbose;
static WriteBinaryOptions s_write_binary_options;
static bool s_spec;
static bool s_validate = true;
static bool s_debug_parsing;
static Features s_features;

static std::unique_ptr<FileStream> s_log_stream;

static const char s_description[] =
R"(  read a file in the wasm s-expression format, check it for errors, and
  convert it to the wasm binary format.

examples:
  # parse and typecheck test.wast
  $ wast2wasm test.wast

  # parse test.wast and write to binary file test.wasm
  $ wast2wasm test.wast -o test.wasm

  # parse spec-test.wast, and write verbose output to stdout (including
  # the meaning of every byte)
  $ wast2wasm spec-test.wast -v

  # parse spec-test.wast, and write files to spec-test.json. Modules are
  # written to spec-test.0.wasm, spec-test.1.wasm, etc.
  $ wast2wasm spec-test.wast --spec -o spec-test.json
)";

static void ParseOptions(int argc, char* argv[]) {
  OptionParser parser("wast2wasm", s_description);

  parser.AddOption('v', "verbose", "Use multiple times for more info", []() {
    s_verbose++;
    s_log_stream = FileStream::CreateStdout();
    s_write_binary_options.log_stream = s_log_stream.get();
  });
  parser.AddHelpOption();
  parser.AddOption("debug-parser", "Turn on debugging the parser of wast files",
                   []() { s_debug_parsing = true; });
  parser.AddOption('d', "dump-module",
                   "Print a hexdump of the module to stdout",
                   []() { s_dump_module = true; });
  s_features.AddOptions(&parser);
  parser.AddOption('o', "output", "FILE", "output wasm binary file",
                   [](const char* argument) { s_outfile = argument; });
  parser.AddOption(
      'r', "relocatable",
      "Create a relocatable wasm binary (suitable for linking with wasm-link)",
      []() { s_write_binary_options.relocatable = true; });
  parser.AddOption(
      "spec",
      "Parse a file with multiple modules and assertions, like the spec tests",
      []() { s_spec = true; });
  parser.AddOption(
      "no-canonicalize-leb128s",
      "Write all LEB128 sizes as 5-bytes instead of their minimal size",
      []() { s_write_binary_options.canonicalize_lebs = false; });
  parser.AddOption("debug-names",
                   "Write debug names to the generated binary file",
                   []() { s_write_binary_options.write_debug_names = true; });
  parser.AddOption("no-check", "Don't check for invalid modules",
                   []() { s_validate = false; });
  parser.AddArgument("filename", OptionParser::ArgumentCount::One,
                     [](const char* argument) { s_infile = argument; });

  parser.Parse(argc, argv);
}

static void WriteBufferToFile(const char* filename,
                              const OutputBuffer& buffer) {
  if (s_dump_module) {
    if (s_verbose)
      s_log_stream->Writef(";; dump\n");
    if (!buffer.data.empty()) {
      s_log_stream->WriteMemoryDump(buffer.data.data(), buffer.data.size());
    }
  }

  if (filename) {
    buffer.WriteToFile(filename);
  }
}

int ProgramMain(int argc, char** argv) {
  InitStdio();

  ParseOptions(argc, argv);

  std::unique_ptr<WastLexer> lexer = WastLexer::CreateFileLexer(s_infile);
  if (!lexer)
    WABT_FATAL("unable to read file: %s\n", s_infile);

  ErrorHandlerFile error_handler(Location::Type::Text);
  Script* script;
  WastParseOptions parse_wast_options(s_features);
  Result result =
      ParseWast(lexer.get(), &script, &error_handler, &parse_wast_options);

  if (Succeeded(result)) {
    result = ResolveNamesScript(lexer.get(), script, &error_handler);

    if (Succeeded(result) && s_validate)
      result = ValidateScript(lexer.get(), script, &error_handler);

    if (Succeeded(result)) {
      if (s_spec) {
        WriteBinarySpecOptions write_binary_spec_options;
        write_binary_spec_options.json_filename = s_outfile;
        write_binary_spec_options.write_binary_options = s_write_binary_options;
        result =
            WriteBinarySpecScript(script, s_infile, &write_binary_spec_options);
      } else {
        MemoryWriter writer;
        const Module* module = script->GetFirstModule();
        if (module) {
          result = WriteBinaryModule(&writer, module, &s_write_binary_options);
        } else {
          WABT_FATAL("no module found\n");
        }

        if (Succeeded(result))
          WriteBufferToFile(s_outfile, writer.output_buffer());
      }
    }
  }

  delete script;
  return result != Result::Ok;
}

int main(int argc, char** argv) {
  WABT_TRY
  return ProgramMain(argc, argv);
  WABT_CATCH_BAD_ALLOC_AND_EXIT
}
