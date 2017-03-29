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
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <algorithm>

#include "binary-reader.h"
#include "binary-reader-opcnt.h"
#include "option-parser.h"
#include "stream.h"

#define PROGRAM_NAME "wasmopcodecnt"

#define ERROR(fmt, ...) \
  fprintf(stderr, "%s:%d: " fmt, __FILE__, __LINE__, __VA_ARGS__)

using namespace wabt;

static int s_verbose;
static const char* s_infile;
static const char* s_outfile;
static size_t s_cutoff = 0;
static const char* s_separator = ": ";

static ReadBinaryOptions s_read_binary_options =
    WABT_READ_BINARY_OPTIONS_DEFAULT;

static FileWriter s_log_stream_writer;
static Stream s_log_stream;

#define NOPE HasArgument::No
#define YEP HasArgument::Yes

enum {
  FLAG_VERBOSE,
  FLAG_HELP,
  FLAG_OUTPUT,
  FLAG_CUTOFF,
  FLAG_SEPARATOR,
  NUM_FLAGS
};

static const char s_description[] =
    "  Read a file in the wasm binary format, and count opcode usage for\n"
    "  instructions.\n"
    "\n"
    "examples:\n"
    "  # parse binary file test.wasm and write pcode dist file test.dist\n"
    "  $ wasmopcodecnt test.wasm -o test.dist\n";

static Option s_options[] = {
    {FLAG_VERBOSE, 'v', "verbose", nullptr, NOPE,
     "use multiple times for more info"},
    {FLAG_HELP, 'h', "help", nullptr, NOPE, "print this help message"},
    {FLAG_OUTPUT, 'o', "output", "FILENAME", YEP,
     "output file for the opcode counts, by default use stdout"},
    {FLAG_CUTOFF, 'c', "cutoff", "N", YEP,
     "cutoff for reporting counts less than N"},
    {FLAG_SEPARATOR, 's', "separator", "SEPARATOR", YEP,
     "Separator text between element and count when reporting counts"}};

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

    case FLAG_CUTOFF:
      s_cutoff = atol(argument);
      break;

    case FLAG_SEPARATOR:
      s_separator = argument;
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

typedef int(int_counter_lt_fcn)(const IntCounter&, const IntCounter&);

typedef int(int_pair_counter_lt_fcn)(const IntPairCounter&,
                                     const IntPairCounter&);

typedef void (*display_name_fcn)(FILE* out, intmax_t value);

static void display_opcode_name(FILE* out, intmax_t opcode) {
  if (opcode >= 0 && opcode < kOpcodeCount)
    fprintf(out, "%s", get_opcode_name(static_cast<Opcode>(opcode)));
  else
    fprintf(out, "?(%" PRIdMAX ")", opcode);
}

static void display_intmax(FILE* out, intmax_t value) {
  fprintf(out, "%" PRIdMAX, value);
}

static void display_int_counter_vector(FILE* out,
                                       const IntCounterVector& vec,
                                       display_name_fcn display_fcn,
                                       const char* opcode_name) {
  for (const IntCounter& counter : vec) {
    if (counter.count == 0)
      continue;
    if (opcode_name)
      fprintf(out, "(%s ", opcode_name);
    display_fcn(out, counter.value);
    if (opcode_name)
      fprintf(out, ")");
    fprintf(out, "%s%" PRIzd "\n", s_separator, counter.count);
  }
}

static void display_int_pair_counter_vector(FILE* out,
                                            const IntPairCounterVector& vec,
                                            display_name_fcn display_first_fcn,
                                            display_name_fcn display_second_fcn,
                                            const char* opcode_name) {
  for (const IntPairCounter& pair : vec) {
    if (pair.count == 0)
      continue;
    if (opcode_name)
      fprintf(out, "(%s ", opcode_name);
    display_first_fcn(out, pair.first);
    fputc(' ', out);
    display_second_fcn(out, pair.second);
    if (opcode_name)
      fprintf(out, ")");
    fprintf(out, "%s%" PRIzd "\n", s_separator, pair.count);
  }
}

static int opcode_counter_gt(const IntCounter& counter_1,
                             const IntCounter& counter_2) {
  if (counter_1.count > counter_2.count)
    return 1;
  if (counter_1.count < counter_2.count)
    return 0;
  const char* name_1 = "?1";
  const char* name_2 = "?2";
  if (counter_1.value < kOpcodeCount) {
    const char* opcode_name =
        get_opcode_name(static_cast<Opcode>(counter_1.value));
    if (opcode_name)
      name_1 = opcode_name;
  }
  if (counter_2.value < kOpcodeCount) {
    const char* opcode_name =
        get_opcode_name(static_cast<Opcode>(counter_2.value));
    if (opcode_name)
      name_2 = opcode_name;
  }
  int diff = strcmp(name_1, name_2);
  if (diff > 0)
    return 1;
  return 0;
}

static int int_counter_gt(const IntCounter& counter_1,
                          const IntCounter& counter_2) {
  if (counter_1.count < counter_2.count)
    return 0;
  if (counter_1.count > counter_2.count)
    return 1;
  if (counter_1.value < counter_2.value)
    return 0;
  if (counter_1.value > counter_2.value)
    return 1;
  return 0;
}

static int int_pair_counter_gt(const IntPairCounter& counter_1,
                               const IntPairCounter& counter_2) {
  if (counter_1.count < counter_2.count)
    return 0;
  if (counter_1.count > counter_2.count)
    return 1;
  if (counter_1.first < counter_2.first)
    return 0;
  if (counter_1.first > counter_2.first)
    return 1;
  if (counter_1.second < counter_2.second)
    return 0;
  if (counter_1.second > counter_2.second)
    return 1;
  return 0;
}

static void display_sorted_int_counter_vector(FILE* out,
                                              const char* title,
                                              const IntCounterVector& vec,
                                              int_counter_lt_fcn lt_fcn,
                                              display_name_fcn display_fcn,
                                              const char* opcode_name) {
  if (vec.size() == 0)
    return;

  /* First filter out values less than cutoff. This speeds up sorting. */
  IntCounterVector filtered_vec;
  for (const IntCounter& counter: vec) {
    if (counter.count < s_cutoff)
      continue;
    filtered_vec.push_back(counter);
  }
  std::sort(filtered_vec.begin(), filtered_vec.end(), lt_fcn);
  fprintf(out, "%s\n", title);
  display_int_counter_vector(out, filtered_vec, display_fcn, opcode_name);
}

static void display_sorted_int_pair_counter_vector(
    FILE* out,
    const char* title,
    const IntPairCounterVector& vec,
    int_pair_counter_lt_fcn lt_fcn,
    display_name_fcn display_first_fcn,
    display_name_fcn display_second_fcn,
    const char* opcode_name) {
  if (vec.size() == 0)
    return;

  IntPairCounterVector filtered_vec;
  for (const IntPairCounter& pair : vec) {
    if (pair.count < s_cutoff)
      continue;
    filtered_vec.push_back(pair);
  }
  std::sort(filtered_vec.begin(), filtered_vec.end(), lt_fcn);
  fprintf(out, "%s\n", title);
  display_int_pair_counter_vector(out, filtered_vec, display_first_fcn,
                                  display_second_fcn, opcode_name);
}

int main(int argc, char** argv) {
  init_stdio();
  parse_options(argc, argv);

  char* data;
  size_t size;
  Result result = read_file(s_infile, &data, &size);
  if (WABT_FAILED(result)) {
    const char* input_name = s_infile ? s_infile : "stdin";
    ERROR("Unable to parse: %s", input_name);
    delete[] data;
  }
  FILE* out = stdout;
  if (s_outfile) {
    out = fopen(s_outfile, "w");
    if (!out)
      ERROR("fopen \"%s\" failed, errno=%d\n", s_outfile, errno);
    result = Result::Error;
  }
  if (WABT_SUCCEEDED(result)) {
    OpcntData opcnt_data;
    result = read_binary_opcnt(data, size, &s_read_binary_options, &opcnt_data);
    if (WABT_SUCCEEDED(result)) {
      display_sorted_int_counter_vector(
          out, "Opcode counts:", opcnt_data.opcode_vec, opcode_counter_gt,
          display_opcode_name, nullptr);
      display_sorted_int_counter_vector(
          out, "\ni32.const:", opcnt_data.i32_const_vec, int_counter_gt,
          display_intmax, get_opcode_name(Opcode::I32Const));
      display_sorted_int_counter_vector(
          out, "\nget_local:", opcnt_data.get_local_vec, int_counter_gt,
          display_intmax, get_opcode_name(Opcode::GetLocal));
      display_sorted_int_counter_vector(
          out, "\nset_local:", opcnt_data.set_local_vec, int_counter_gt,
          display_intmax, get_opcode_name(Opcode::SetLocal));
      display_sorted_int_counter_vector(
          out, "\ntee_local:", opcnt_data.tee_local_vec, int_counter_gt,
          display_intmax, get_opcode_name(Opcode::TeeLocal));
      display_sorted_int_pair_counter_vector(
          out, "\ni32.load:", opcnt_data.i32_load_vec, int_pair_counter_gt,
          display_intmax, display_intmax, get_opcode_name(Opcode::I32Load));
      display_sorted_int_pair_counter_vector(
          out, "\ni32.store:", opcnt_data.i32_store_vec, int_pair_counter_gt,
          display_intmax, display_intmax, get_opcode_name(Opcode::I32Store));
    }
  }
  delete[] data;
  return result != Result::Ok;
}
