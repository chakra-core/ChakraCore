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

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "src/common.h"
#include "src/option-parser.h"
#include "src/stream.h"
#include "src/binary-reader.h"
#include "src/binary-reader-objdump.h"

using namespace wabt;

static const char s_description[] =
R"(  Print information about the contents of wasm binaries.

examples:
  $ wasm-objdump test.wasm
)";

static ObjdumpOptions s_objdump_options;

static std::vector<const char*> s_infiles;

static std::unique_ptr<FileStream> s_log_stream;

static void ParseOptions(int argc, char** argv) {
  OptionParser parser("wasm-objdump", s_description);

  parser.AddOption('h', "headers", "Print headers",
                   []() { s_objdump_options.headers = true; });
  parser.AddOption(
      'j', "section", "SECTION", "Select just one section",
      [](const char* argument) { s_objdump_options.section_name = argument; });
  parser.AddOption('s', "full-contents", "Print raw section contents",
                   []() { s_objdump_options.raw = true; });
  parser.AddOption('d', "disassemble", "Disassemble function bodies",
                   []() { s_objdump_options.disassemble = true; });
  parser.AddOption("debug", "Print extra debug information", []() {
    s_objdump_options.debug = true;
    s_log_stream = FileStream::CreateStdout();
    s_objdump_options.log_stream = s_log_stream.get();
  });
  parser.AddOption('x', "details", "Show section details",
                   []() { s_objdump_options.details = true; });
  parser.AddOption('r', "reloc", "Show relocations inline with disassembly",
                   []() { s_objdump_options.relocs = true; });
  parser.AddHelpOption();
  parser.AddArgument(
      "filename", OptionParser::ArgumentCount::OneOrMore,
      [](const char* argument) { s_infiles.push_back(argument); });

  parser.Parse(argc, argv);
}

Result dump_file(const char* filename) {
  std::vector<uint8_t> file_data;
  CHECK_RESULT(ReadFile(filename, &file_data));

  uint8_t* data = DataOrNull(file_data);
  size_t size = file_data.size();

  // Perform serveral passed over the binary in order to print out different
  // types of information.
  s_objdump_options.filename = filename;
  printf("\n");

  ObjdumpState state;

  Result result = Result::Ok;

  // Pass 0: Prepass
  s_objdump_options.mode = ObjdumpMode::Prepass;
  result |= ReadBinaryObjdump(data, size, &s_objdump_options, &state);
  s_objdump_options.log_stream = nullptr;

  // Pass 1: Print the section headers
  if (s_objdump_options.headers) {
    s_objdump_options.mode = ObjdumpMode::Headers;
    result |= ReadBinaryObjdump(data, size, &s_objdump_options, &state);
  }

  // Pass 2: Print extra information based on section type
  if (s_objdump_options.details) {
    s_objdump_options.mode = ObjdumpMode::Details;
    result |= ReadBinaryObjdump(data, size, &s_objdump_options, &state);
  }

  // Pass 3: Disassemble code section
  if (s_objdump_options.disassemble) {
    s_objdump_options.mode = ObjdumpMode::Disassemble;
    result |= ReadBinaryObjdump(data, size, &s_objdump_options, &state);
  }

  // Pass 4: Dump to raw contents of the sections
  if (s_objdump_options.raw) {
    s_objdump_options.mode = ObjdumpMode::RawData;
    result |= ReadBinaryObjdump(data, size, &s_objdump_options, &state);
  }

  return result;
}

int ProgramMain(int argc, char** argv) {
  InitStdio();

  ParseOptions(argc, argv);
  if (!s_objdump_options.headers && !s_objdump_options.details &&
      !s_objdump_options.disassemble && !s_objdump_options.raw) {
    fprintf(stderr, "At least one of the following switches must be given:\n");
    fprintf(stderr, " -d/--disassemble\n");
    fprintf(stderr, " -h/--headers\n");
    fprintf(stderr, " -x/--details\n");
    fprintf(stderr, " -s/--full-contents\n");
    return 1;
  }

  for (const char* filename: s_infiles) {
    if (Failed(dump_file(filename))) {
      return 1;
    }
  }

  return 0;
}

int main(int argc, char** argv) {
  WABT_TRY
  return ProgramMain(argc, argv);
  WABT_CATCH_BAD_ALLOC_AND_EXIT
}
