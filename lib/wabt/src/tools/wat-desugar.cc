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
#include <cstdio>
#include <cstdlib>

#include "config.h"

#include "src/apply-names.h"
#include "src/common.h"
#include "src/error-handler.h"
#include "src/feature.h"
#include "src/generate-names.h"
#include "src/ir.h"
#include "src/option-parser.h"
#include "src/stream.h"
#include "src/wast-parser.h"
#include "src/wat-writer.h"

using namespace wabt;

static const char* s_infile;
static const char* s_outfile;
static WriteWatOptions s_write_wat_options;
static bool s_generate_names;
static bool s_debug_parsing;
static Features s_features;

static const char s_description[] =
R"(  read a file in the wasm s-expression format and format it.

examples:
  # write output to stdout
  $ wat-desugar test.wat

  # write output to test2.wat
  $ wat-desugar test.wat -o test2.wat

  # generate names for indexed variables
  $ wat-desugar --generate-names test.wat
)";

static void ParseOptions(int argc, char** argv) {
  OptionParser parser("wat-desugar", s_description);

  parser.AddHelpOption();
  parser.AddOption('o', "output", "FILE", "Output file for the formatted file",
                   [](const char* argument) { s_outfile = argument; });
  parser.AddOption("debug-parser", "Turn on debugging the parser of wat files",
                   []() { s_debug_parsing = true; });
  parser.AddOption('f', "fold-exprs", "Write folded expressions where possible",
                   []() { s_write_wat_options.fold_exprs = true; });
  s_features.AddOptions(&parser);
  parser.AddOption(
      "generate-names",
      "Give auto-generated names to non-named functions, types, etc.",
      []() { s_generate_names = true; });

  parser.AddArgument("filename", OptionParser::ArgumentCount::One,
                     [](const char* argument) { s_infile = argument; });
  parser.Parse(argc, argv);
}

int ProgramMain(int argc, char** argv) {
  InitStdio();
  ParseOptions(argc, argv);

  std::unique_ptr<WastLexer> lexer(WastLexer::CreateFileLexer(s_infile));
  if (!lexer) {
    WABT_FATAL("unable to read %s\n", s_infile);
  }

  ErrorHandlerFile error_handler(Location::Type::Text);
  std::unique_ptr<Script> script;
  WastParseOptions parse_wast_options(s_features);
  Result result = ParseWastScript(lexer.get(), &script, &error_handler,
                                  &parse_wast_options);

  if (Succeeded(result)) {
    Module* module = script->GetFirstModule();
    if (!module) {
      WABT_FATAL("no module in file.\n");
    }

    if (s_generate_names) {
      result = GenerateNames(module);
    }

    if (Succeeded(result)) {
      result = ApplyNames(module);
    }

    if (Succeeded(result)) {
      FileStream stream(s_outfile ? FileStream(s_outfile) : FileStream(stdout));
      result = WriteWat(&stream, module, &s_write_wat_options);
    }
  }

  return result != Result::Ok;
}

int main(int argc, char** argv) {
  WABT_TRY
  return ProgramMain(argc, argv);
  WABT_CATCH_BAD_ALLOC_AND_EXIT
}
