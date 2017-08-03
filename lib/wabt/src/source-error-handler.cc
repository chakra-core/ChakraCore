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

#include "source-error-handler.h"

#include <algorithm>

namespace wabt {

std::string SourceErrorHandler::DefaultErrorMessage(
    const Location* loc,
    const std::string& error,
    const std::string& source_line,
    size_t source_line_column_offset,
    int indent) {
  std::string indent_str(indent, ' ');
  std::string result =
      string_printf("%s%s:%d:%d: %s\n", indent_str.c_str(), loc->filename,
                    loc->line, loc->first_column, error.c_str());
  result += indent_str;
  if (!source_line.empty()) {
    result += source_line;
    result += '\n';
    result += indent_str;

    size_t num_spaces = (loc->first_column - 1) - source_line_column_offset;
    size_t num_carets = loc->last_column - loc->first_column;
    num_carets = std::min(num_carets, source_line.size() - num_spaces);
    num_carets = std::max<size_t>(num_carets, 1);
    result.append(num_spaces, ' ');
    result.append(num_carets, '^');
    result += '\n';
  }
  return result;
}

SourceErrorHandlerFile::SourceErrorHandlerFile(FILE* file,
                                               const std::string& header,
                                               PrintHeader print_header,
                                               size_t source_line_max_length)
    : file_(file),
      header_(header),
      print_header_(print_header),
      source_line_max_length_(source_line_max_length) {}

bool SourceErrorHandlerFile::OnError(const Location* loc,
                                     const std::string& error,
                                     const std::string& source_line,
                                     size_t source_line_column_offset) {
  PrintErrorHeader();
  int indent = header_.empty() ? 0 : 2;
  std::string message = DefaultErrorMessage(loc, error, source_line,
                                            source_line_column_offset, indent);
  fwrite(message.data(), 1, message.size(), file_);
  return true;
}

void SourceErrorHandlerFile::PrintErrorHeader() {
  if (header_.empty())
    return;

  switch (print_header_) {
    case PrintHeader::Never:
      break;

    case PrintHeader::Once:
      print_header_ = PrintHeader::Never;
    // Fallthrough.

    case PrintHeader::Always:
      fprintf(file_, "%s:\n", header_.c_str());
      break;
  }
}

SourceErrorHandlerBuffer::SourceErrorHandlerBuffer(
    size_t source_line_max_length)
    : source_line_max_length_(source_line_max_length) {}

bool SourceErrorHandlerBuffer::OnError(const Location* loc,
                                       const std::string& error,
                                       const std::string& source_line,
                                       size_t source_line_column_offset) {
  buffer_ += DefaultErrorMessage(loc, error, source_line,
                                 source_line_column_offset, 0);
  return true;
}

}  // namespace wabt
