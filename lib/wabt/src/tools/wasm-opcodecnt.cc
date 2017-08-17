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

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>

#include "binary-reader.h"
#include "binary-reader-opcnt.h"
#include "option-parser.h"
#include "stream.h"

#define ERROR(fmt, ...) \
  fprintf(stderr, "%s:%d: " fmt, __FILE__, __LINE__, __VA_ARGS__)

using namespace wabt;

static int s_verbose;
static const char* s_infile;
static const char* s_outfile;
static size_t s_cutoff = 0;
static const char* s_separator = ": ";

static ReadBinaryOptions s_read_binary_options;
static std::unique_ptr<FileStream> s_log_stream;

static const char s_description[] =
R"(  Read a file in the wasm binary format, and count opcode usage for
  instructions.

examples:
  # parse binary file test.wasm and write pcode dist file test.dist
  $ wasm-opcodecnt test.wasm -o test.dist
)";

static void ParseOptions(int argc, char** argv) {
  OptionParser parser("wasm-opcodecnt", s_description);

  parser.AddOption('v', "verbose", "Use multiple times for more info", []() {
    s_verbose++;
    s_log_stream = FileStream::CreateStdout();
    s_read_binary_options.log_stream = s_log_stream.get();
  });
  parser.AddOption('h', "help", "Print this help message", [&parser]() {
    parser.PrintHelp();
    exit(0);
  });
  parser.AddOption('o', "output", "FILENAME",
                   "Output file for the opcode counts, by default use stdout",
                   [](const char* argument) { s_outfile = argument; });
  parser.AddOption(
      'c', "cutoff", "N", "Cutoff for reporting counts less than N",
      [](const std::string& argument) { s_cutoff = atol(argument.c_str()); });
  parser.AddOption(
      's', "separator", "SEPARATOR",
      "Separator text between element and count when reporting counts",
      [](const char* argument) { s_separator = argument; });
  parser.AddArgument("filename", OptionParser::ArgumentCount::OneOrMore,
                     [](const char* argument) { s_infile = argument; });
  parser.Parse(argc, argv);
}

template <typename T>
struct SortByCountDescending {
  bool operator()(const T& lhs, const T& rhs) const {
    return lhs.second > rhs.second;
  }
};

template <typename T>
struct WithinCutoff {
  bool operator()(const T& pair) const {
    return pair.second >= s_cutoff;
  }
};

void WriteCounts(Stream& stream, const OpcodeInfoCounts& info_counts) {
  typedef std::pair<Opcode, size_t> OpcodeCountPair;

  std::map<Opcode, size_t> counts;
  for (auto& info_count_pair: info_counts) {
    Opcode opcode = info_count_pair.first.opcode();
    size_t count = info_count_pair.second;
    counts[opcode] += count;
  }

  std::vector<OpcodeCountPair> sorted;
  std::copy_if(counts.begin(), counts.end(), std::back_inserter(sorted),
               WithinCutoff<OpcodeCountPair>());

  // Use a stable sort to keep the elements with the same count in opcode
  // order (since the Opcode map is sorted).
  std::stable_sort(sorted.begin(), sorted.end(),
                   SortByCountDescending<OpcodeCountPair>());

  for (auto& pair : sorted) {
    Opcode opcode = pair.first;
    size_t count = pair.second;
    stream.Writef("%s%s%" PRIzd "\n", opcode.GetName(), s_separator, count);
  }
}

void WriteCountsWithImmediates(Stream& stream,
                               const OpcodeInfoCounts& counts) {
  // Remove const from the key type so we can sort below.
  typedef std::pair<std::remove_const<OpcodeInfoCounts::key_type>::type,
                    OpcodeInfoCounts::mapped_type>
      OpcodeInfoCountPair;

  std::vector<OpcodeInfoCountPair> sorted;
  std::copy_if(counts.begin(), counts.end(), std::back_inserter(sorted),
               WithinCutoff<OpcodeInfoCountPair>());

  // Use a stable sort to keep the elements with the same count in opcode info
  // order (since the OpcodeInfoCounts map is sorted).
  std::stable_sort(sorted.begin(), sorted.end(),
                   SortByCountDescending<OpcodeInfoCountPair>());

  for (auto& pair : sorted) {
    auto&& info = pair.first;
    size_t count = pair.second;
    info.Write(stream);
    stream.Writef("%s%" PRIzd "\n", s_separator, count);
  }
}

int ProgramMain(int argc, char** argv) {
  InitStdio();
  ParseOptions(argc, argv);

  std::vector<uint8_t> file_data;
  Result result = ReadFile(s_infile, &file_data);
  if (Failed(result)) {
    const char* input_name = s_infile ? s_infile : "stdin";
    ERROR("Unable to parse: %s", input_name);
    return 1;
  }

  FileStream stream(s_outfile ? FileStream(s_outfile) : FileStream(stdout));

  if (Succeeded(result)) {
    OpcodeInfoCounts counts;
    result = ReadBinaryOpcnt(DataOrNull(file_data), file_data.size(),
                             &s_read_binary_options, &counts);
    if (Succeeded(result)) {
      stream.Writef("Opcode counts:\n");
      WriteCounts(stream, counts);

      stream.Writef("\nOpcode counts with immediates:\n");
      WriteCountsWithImmediates(stream, counts);
    }
  }

  return result != Result::Ok;
}

int main(int argc, char** argv) {
  WABT_TRY
  return ProgramMain(argc, argv);
  WABT_CATCH_BAD_ALLOC_AND_EXIT
}
