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

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "apply-names.h"
#include "ast.h"
#include "ast-writer.h"
#include "binary-reader.h"
#include "binary-reader-ast.h"
#include "generate-names.h"
#include "option-parser.h"
#include "stream.h"
#include "writer.h"

#define PROGRAM_NAME "wasm2wast"

using namespace wabt;

static int s_verbose;
static const char* s_infile;
static const char* s_outfile;
static ReadBinaryOptions s_read_binary_options = {nullptr, true};
static bool s_generate_names;

static BinaryErrorHandler s_error_handler = WABT_BINARY_ERROR_HANDLER_DEFAULT;

static FileWriter s_log_stream_writer;
static Stream s_log_stream;

#define NOPE HasArgument::No
#define YEP HasArgument::Yes

enum {
  FLAG_VERBOSE,
  FLAG_HELP,
  FLAG_OUTPUT,
  FLAG_NO_DEBUG_NAMES,
  FLAG_GENERATE_NAMES,
  NUM_FLAGS
};

static const char s_description[] =
    "  read a file in the wasm binary format, and convert it to the wasm\n"
    "  s-expression file format.\n"
    "\n"
    "examples:\n"
    "  # parse binary file test.wasm and write s-expression file test.wast\n"
    "  $ wasm2wast test.wasm -o test.wast\n"
    "\n"
    "  # parse test.wasm, write test.wast, but ignore the debug names, if any\n"
    "  $ wasm2wast test.wasm --no-debug-names -o test.wast\n";

static Option s_options[] = {
    {FLAG_VERBOSE, 'v', "verbose", nullptr, NOPE,
     "use multiple times for more info"},
    {FLAG_HELP, 'h', "help", nullptr, NOPE, "print this help message"},
    {FLAG_OUTPUT, 'o', "output", "FILENAME", YEP,
     "output file for the generated wast file, by default use stdout"},
    {FLAG_NO_DEBUG_NAMES, 0, "no-debug-names", nullptr, NOPE,
     "Ignore debug names in the binary file"},
    {FLAG_GENERATE_NAMES, 0, "generate-names", nullptr, NOPE,
     "Give auto-generated names to non-named functions, types, etc."},
};
WABT_STATIC_ASSERT(NUM_FLAGS == WABT_ARRAY_SIZE(s_options));

static void on_option(struct OptionParser* parser,
                      struct Option* option,
                      const char* argument) {
  switch (option->id) {
    case FLAG_VERBOSE:
      s_verbose++;
      init_file_writer_existing(&s_log_stream_writer, stdout);
      init_stream(&s_log_stream, &s_log_stream_writer.base, nullptr);
      s_read_binary_options.log_stream = &s_log_stream;
      break;

    case FLAG_HELP:
      print_help(parser, PROGRAM_NAME);
      exit(0);
      break;

    case FLAG_OUTPUT:
      s_outfile = argument;
      break;

    case FLAG_NO_DEBUG_NAMES:
      s_read_binary_options.read_debug_names = false;
      break;

    case FLAG_GENERATE_NAMES:
      s_generate_names = true;
      break;
  }
}

static void on_argument(struct OptionParser* parser, const char* argument) {
  s_infile = argument;
}

static void on_option_error(struct OptionParser* parser, const char* message) {
  WABT_FATAL("%s\n", message);
}

static void parse_options(int argc, char** argv) {
  OptionParser parser;
  WABT_ZERO_MEMORY(parser);
  parser.description = s_description;
  parser.options = s_options;
  parser.num_options = WABT_ARRAY_SIZE(s_options);
  parser.on_option = on_option;
  parser.on_argument = on_argument;
  parser.on_error = on_option_error;
  parse_options(&parser, argc, argv);

  if (!s_infile) {
    print_help(&parser, PROGRAM_NAME);
    WABT_FATAL("No filename given.\n");
  }
}

int main(int argc, char** argv) {
  Result result;

  init_stdio();
  parse_options(argc, argv);

  char* data;
  size_t size;
  result = read_file(s_infile, &data, &size);
  if (WABT_SUCCEEDED(result)) {
    Module module;
    result = read_binary_ast(data, size, &s_read_binary_options,
                             &s_error_handler, &module);
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
        FileWriter file_writer;
        if (s_outfile) {
          result = init_file_writer(&file_writer, s_outfile);
        } else {
          init_file_writer_existing(&file_writer, stdout);
        }

        if (WABT_SUCCEEDED(result)) {
          result = write_ast(&file_writer.base, &module);
          close_file_writer(&file_writer);
        }
      }
    }
    delete[] data;
  }
  return result != Result::Ok;
}
