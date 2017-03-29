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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "option-parser.h"
#include "stream.h"
#include "writer.h"
#include "binary-reader.h"
#include "binary-reader-objdump.h"

#define PROGRAM_NAME "wasmdump"

#define NOPE HasArgument::No
#define YEP HasArgument::Yes

using namespace wabt;

enum {
  FLAG_HEADERS,
  FLAG_SECTION,
  FLAG_RAW,
  FLAG_DISASSEMBLE,
  FLAG_DEBUG,
  FLAG_DETAILS,
  FLAG_RELOCS,
  FLAG_HELP,
  NUM_FLAGS
};

static const char s_description[] =
    "  Print information about the contents of a wasm binary file.\n"
    "\n"
    "examples:\n"
    "  $ wasmdump test.wasm\n";

static Option s_options[] = {
    {FLAG_HEADERS, 'h', "headers", nullptr, NOPE, "print headers"},
    {FLAG_SECTION, 'j', "section", nullptr, YEP, "select just one section"},
    {FLAG_RAW, 's', "full-contents", nullptr, NOPE,
     "print raw section contents"},
    {FLAG_DISASSEMBLE, 'd', "disassemble", nullptr, NOPE,
     "disassemble function bodies"},
    {FLAG_DEBUG, '\0', "debug", nullptr, NOPE, "print extra debug information"},
    {FLAG_DETAILS, 'x', "details", nullptr, NOPE, "Show section details"},
    {FLAG_RELOCS, 'r', "reloc", nullptr, NOPE,
     "show relocations inline with disassembly"},
    {FLAG_HELP, 'h', "help", nullptr, NOPE, "print this help message"},
};

WABT_STATIC_ASSERT(NUM_FLAGS == WABT_ARRAY_SIZE(s_options));

static ObjdumpOptions s_objdump_options;
static FileWriter s_log_stream_writer;
static Stream s_log_stream;

static void on_argument(struct OptionParser* parser, const char* argument) {
  s_objdump_options.infile = argument;
}

static void on_option(struct OptionParser* parser,
                      struct Option* option,
                      const char* argument) {
  switch (option->id) {
    case FLAG_HEADERS:
      s_objdump_options.headers = true;
      break;

    case FLAG_RAW:
      s_objdump_options.raw = true;
      break;

    case FLAG_DEBUG:
      s_objdump_options.debug = true;
      init_file_writer_existing(&s_log_stream_writer, stdout);
      init_stream(&s_log_stream, &s_log_stream_writer.base, nullptr);
      s_objdump_options.log_stream = &s_log_stream;
      break;

    case FLAG_DISASSEMBLE:
      s_objdump_options.disassemble = true;
      break;

    case FLAG_DETAILS:
      s_objdump_options.details = true;
      break;

    case FLAG_RELOCS:
      s_objdump_options.relocs = true;
      break;

    case FLAG_SECTION:
      s_objdump_options.section_name = argument;
      break;

    case FLAG_HELP:
      print_help(parser, PROGRAM_NAME);
      exit(0);
      break;
  }
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

  if (!s_objdump_options.infile) {
    print_help(&parser, PROGRAM_NAME);
    WABT_FATAL("No filename given.\n");
  }
}

int main(int argc, char** argv) {
  init_stdio();

  parse_options(argc, argv);
  if (!s_objdump_options.headers && !s_objdump_options.details &&
      !s_objdump_options.disassemble && !s_objdump_options.raw) {
    fprintf(stderr, "At least one of the following switches must be given:\n");
    fprintf(stderr, " -d/--disassemble\n");
    fprintf(stderr, " -h/--headers\n");
    fprintf(stderr, " -x/--details\n");
    fprintf(stderr, " -s/--full-contents\n");
    return 1;
  }

  char* char_data;
  size_t size;
  Result result = read_file(s_objdump_options.infile, &char_data, &size);
  if (WABT_FAILED(result))
    return result != Result::Ok;

  uint8_t* data = reinterpret_cast<uint8_t*>(char_data);

  // Perform serveral passed over the binary in order to print out different
  // types of information.
  s_objdump_options.print_header = true;
  if (!s_objdump_options.headers && !s_objdump_options.details &&
      !s_objdump_options.disassemble && !s_objdump_options.raw) {
    printf("At least one of the following switches must be given:\n");
    printf(" -d/--disassemble\n");
    printf(" -h/--headers\n");
    printf(" -x/--details\n");
    printf(" -s/--full-contents\n");
    return 1;
  }

  s_objdump_options.mode = ObjdumpMode::Prepass;
  result = read_binary_objdump(data, size, &s_objdump_options);
  if (WABT_FAILED(result))
    goto done;
  s_objdump_options.log_stream = nullptr;

  // Pass 1: Print the section headers
  if (s_objdump_options.headers) {
    s_objdump_options.mode = ObjdumpMode::Headers;
    result = read_binary_objdump(data, size, &s_objdump_options);
    if (WABT_FAILED(result))
      goto done;
    s_objdump_options.print_header = false;
  }
  // Pass 2: Print extra information based on section type
  if (s_objdump_options.details) {
    s_objdump_options.mode = ObjdumpMode::Details;
    result = read_binary_objdump(data, size, &s_objdump_options);
    if (WABT_FAILED(result))
      goto done;
    s_objdump_options.print_header = false;
  }
  if (s_objdump_options.disassemble) {
    s_objdump_options.mode = ObjdumpMode::Disassemble;
    result = read_binary_objdump(data, size, &s_objdump_options);
    if (WABT_FAILED(result))
      goto done;
    s_objdump_options.print_header = false;
  }
  // Pass 3: Dump to raw contents of the sections
  if (s_objdump_options.raw) {
    s_objdump_options.mode = ObjdumpMode::RawData;
    result = read_binary_objdump(data, size, &s_objdump_options);
  }

done:
  delete[] data;
  return result != Result::Ok;
}
