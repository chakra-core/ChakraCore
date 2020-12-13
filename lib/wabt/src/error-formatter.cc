/*
 * Copyright 2018 WebAssembly Community Group participants
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

#include "src/error-formatter.h"

namespace wabt {

namespace {

std::string FormatError(const Error& error,
                        Location::Type location_type,
                        const Color& color,
                        LexerSourceLineFinder* line_finder,
                        int source_line_max_length,
                        int indent) {
  std::string indent_str(indent, ' ');
  std::string result = indent_str;

  result += color.MaybeBoldCode();

  const Location& loc = error.loc;
  if (!loc.filename.empty()) {
    result += loc.filename.to_string();
    result += ":";
  }

  if (location_type == Location::Type::Text) {
    result += StringPrintf("%d:%d: ", loc.line, loc.first_column);
  } else if (loc.offset != kInvalidOffset) {
    result += StringPrintf("%07" PRIzx ": ", loc.offset);
  }

  result += color.MaybeRedCode();
  result += GetErrorLevelName(error.error_level);
  result += ": ";
  result += color.MaybeDefaultCode();

  result += error.message;
  result += '\n';

  LexerSourceLineFinder::SourceLine source_line;
  if (line_finder) {
    line_finder->GetSourceLine(loc, source_line_max_length, &source_line);
  }

  if (!source_line.line.empty()) {
    result += indent_str;
    result += source_line.line;
    result += '\n';
    result += indent_str;

    size_t num_spaces = (loc.first_column - 1) - source_line.column_offset;
    size_t num_carets = loc.last_column - loc.first_column;
    num_carets = std::min(num_carets, source_line.line.size() - num_spaces);
    num_carets = std::max<size_t>(num_carets, 1);
    result.append(num_spaces, ' ');
    result += color.MaybeBoldCode();
    result += color.MaybeGreenCode();
    result.append(num_carets, '^');
    result += color.MaybeDefaultCode();
    result += '\n';
  }

  return result;
}

}  // End of anonymous namespace

std::string FormatErrorsToString(const Errors& errors,
                                 Location::Type location_type,
                                 LexerSourceLineFinder* line_finder,
                                 const Color& color,
                                 const std::string& header,
                                 PrintHeader print_header,
                                 int source_line_max_length) {
  std::string result;
  for (const auto& error : errors) {
    if (!header.empty()) {
      switch (print_header) {
        case PrintHeader::Never:
          break;
        case PrintHeader::Once:
          print_header = PrintHeader::Never;
          // Fallthrough.
        case PrintHeader::Always:
          result += header;
          result += ":\n";
          break;
      }
    }

    int indent = header.empty() ? 0 : 2;

    result += FormatError(error, location_type, color, line_finder,
                          source_line_max_length, indent);
  }
  return result;
}

void FormatErrorsToFile(const Errors& errors,
                        Location::Type location_type,
                        LexerSourceLineFinder* line_finder,
                        FILE* file,
                        const std::string& header,
                        PrintHeader print_header,
                        int source_line_max_length) {
  Color color(file);
  std::string s =
      FormatErrorsToString(errors, location_type, line_finder, color, header,
                           print_header, source_line_max_length);
  fwrite(s.data(), 1, s.size(), file);
}

}  // namespace wabt
