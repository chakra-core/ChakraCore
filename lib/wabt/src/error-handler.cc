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

#include "src/error-handler.h"

#include <algorithm>

namespace wabt {

ErrorHandler::ErrorHandler(Location::Type location_type)
    : location_type_(location_type) {}

std::string ErrorHandler::DefaultErrorMessage(const Color& color,
                                              const Location& loc,
                                              const std::string& error,
                                              const std::string& source_line,
                                              size_t source_line_column_offset,
                                              int indent) {
  std::string indent_str(indent, ' ');
  std::string result = indent_str;

  result += color.MaybeBoldCode();

  if (!loc.filename.empty()) {
    result += loc.filename.to_string();
    result += ":";
  }

  if (location_type_ == Location::Type::Text) {
    result += StringPrintf("%d:%d: ", loc.line, loc.first_column);
  } else if (loc.offset != kInvalidOffset) {
    result += StringPrintf("%07" PRIzx ": ", loc.offset);
  }

  result += color.MaybeRedCode();
  result += "error: ";
  result += color.MaybeDefaultCode();

  result += error;
  result += '\n';

  if (!source_line.empty()) {
    result += indent_str;
    result += source_line;
    result += '\n';
    result += indent_str;

    size_t num_spaces = (loc.first_column - 1) - source_line_column_offset;
    size_t num_carets = loc.last_column - loc.first_column;
    num_carets = std::min(num_carets, source_line.size() - num_spaces);
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

ErrorHandlerNop::ErrorHandlerNop()
    // The Location::Type is irrelevant since we never display an error.
    : ErrorHandler(Location::Type::Text) {}

ErrorHandlerFile::ErrorHandlerFile(Location::Type location_type,
                                   FILE* file,
                                   const std::string& header,
                                   PrintHeader print_header,
                                   size_t source_line_max_length)
    : ErrorHandler(location_type),
      file_(file),
      header_(header),
      print_header_(print_header),
      source_line_max_length_(source_line_max_length),
      color_(file) {}

bool ErrorHandlerFile::OnError(const Location& loc,
                               const std::string& error,
                               const std::string& source_line,
                               size_t source_line_column_offset) {
  PrintErrorHeader();
  int indent = header_.empty() ? 0 : 2;

  std::string message = DefaultErrorMessage(color_, loc, error, source_line,
                                            source_line_column_offset, indent);
  fwrite(message.data(), 1, message.size(), file_);
  return true;
}

void ErrorHandlerFile::PrintErrorHeader() {
  if (header_.empty()) {
    return;
  }

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

ErrorHandlerBuffer::ErrorHandlerBuffer(Location::Type location_type,
                                       size_t source_line_max_length)
    : ErrorHandler(location_type),
      source_line_max_length_(source_line_max_length),
      color_(nullptr, false) {}

bool ErrorHandlerBuffer::OnError(const Location& loc,
                                 const std::string& error,
                                 const std::string& source_line,
                                 size_t source_line_column_offset) {
  buffer_ += DefaultErrorMessage(color_, loc, error, source_line,
                                 source_line_column_offset, 0);
  return true;
}

}  // namespace wabt
