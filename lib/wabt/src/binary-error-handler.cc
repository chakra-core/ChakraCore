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

#include "binary-error-handler.h"

#include "common.h"

namespace wabt {

std::string BinaryErrorHandler::DefaultErrorMessage(Offset offset,
                                                    const std::string& error) {
  if (offset == kInvalidOffset)
    return string_printf("error: %s\n", error.c_str());
  else
    return string_printf("error: @0x%08" PRIzx ": %s\n", offset, error.c_str());
}

BinaryErrorHandlerFile::BinaryErrorHandlerFile(FILE* file,
                                               const std::string& header,
                                               PrintHeader print_header)
    : file_(file), header_(header), print_header_(print_header) {}

bool BinaryErrorHandlerFile::OnError(Offset offset, const std::string& error) {
  PrintErrorHeader();
  std::string message = DefaultErrorMessage(offset, error);
  fwrite(message.data(), 1, message.size(), file_);
  fflush(file_);
  return true;
}

void BinaryErrorHandlerFile::PrintErrorHeader() {
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
  // If there's a header, indent the following message.
  fprintf(file_, "  ");
}

bool BinaryErrorHandlerBuffer::OnError(Offset offset,
                                       const std::string& error) {
  buffer_ += DefaultErrorMessage(offset, error);
  return true;
}

}  // namespace wabt
