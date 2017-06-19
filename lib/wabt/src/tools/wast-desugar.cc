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

#include "apply-names.h"
#include "common.h"
#include "config.h"
#include "generate-names.h"
#include "ir.h"
#include "option-parser.h"
#include "source-error-handler.h"
#include "stream.h"
#include "wast-parser.h"
#include "wat-writer.h"
#include "writer.h"

using namespace wabt;

static const char* s_infile;
static const char* s_outfile;
static WriteWatOptions s_write_wat_options;
static bool s_generate_names;

static const char s_description[] =
R"(  read a file in the wasm s-expression format and format it.

examples:
  # write output to stdout
  $ wast-desugar test.wast

  # write output to test2.wast
  $ wast-desugar test.wast -o test2.wast

  # generate names for indexed variables
  $ wast-desugar --generate-names test.wast
)";

static void parse_options(int argc, char** argv) {
  OptionParser parser("wast-desugar", s_description);

  parser.AddHelpOption();
  parser.AddOption('o', "output", "FILE", "Output file for the formatted file",
                   [](const char* argument) { s_outfile = argument; });
  parser.AddOption('f', "fold-exprs", "Write folded expressions where possible",
                   []() { s_write_wat_options.fold_exprs = true; });
  parser.AddOption(
      "generate-names",
      "Give auto-generated names to non-named functions, types, etc.",
      []() { s_generate_names = true; });

  parser.AddArgument("filename", OptionParser::ArgumentCount::One,
                     [](const char* argument) { s_infile = argument; });
  parser.Parse(argc, argv);
}

int ProgramMain(int argc, char** argv) {
  init_stdio();
  parse_options(argc, argv);

  std::unique_ptr<WastLexer> lexer(WastLexer::CreateFileLexer(s_infile));
  if (!lexer)
    WABT_FATAL("unable to read %s\n", s_infile);

  SourceErrorHandlerFile error_handler;
  Script* script;
  Result result = parse_wast(lexer.get(), &script, &error_handler);

  if (WABT_SUCCEEDED(result)) {
    Module* module = script->GetFirstModule();
    if (!module)
      WABT_FATAL("no module in file.\n");

    if (s_generate_names)
      result = generate_names(module);

    if (WABT_SUCCEEDED(result))
      result = apply_names(module);

    if (WABT_SUCCEEDED(result)) {
      FileWriter writer(s_outfile ? FileWriter(s_outfile) : FileWriter(stdout));
      result = write_wat(&writer, module, &s_write_wat_options);
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
