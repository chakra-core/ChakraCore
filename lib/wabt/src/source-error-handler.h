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

#ifndef WABT_SOURCE_ERROR_HANDLER_H_
#define WABT_SOURCE_ERROR_HANDLER_H_

#include <string>

#include "common.h"

namespace wabt {

class SourceErrorHandler {
 public:
  virtual ~SourceErrorHandler() {}

  // Returns true if the error was handled.
  virtual bool OnError(const Location*,
                       const std::string& error,
                       const std::string& source_line,
                       size_t source_line_column_offset) = 0;

  // OnError will be called with with source_line trimmed to this length.
  virtual size_t source_line_max_length() const = 0;

  std::string DefaultErrorMessage(const Location*,
                                  const std::string& error,
                                  const std::string& source_line,
                                  size_t source_line_column_offset,
                                  int indent);
};

class SourceErrorHandlerNop : public SourceErrorHandler {
 public:
  bool OnError(const Location*,
               const std::string& error,
               const std::string& source_line,
               size_t source_line_column_offset) override {
    return false;
  }

  size_t source_line_max_length() const override { return 80; }
};

class SourceErrorHandlerFile : public SourceErrorHandler {
 public:
  enum class PrintHeader {
    Never,
    Once,
    Always,
  };

  SourceErrorHandlerFile(FILE* file = stderr,
                         const std::string& header = std::string(),
                         PrintHeader print_header = PrintHeader::Never,
                         size_t source_line_max_length = 80);

  bool OnError(const Location*,
               const std::string& error,
               const std::string& source_line,
               size_t source_line_column_offset) override;

  size_t source_line_max_length() const override {
    return source_line_max_length_;
  }

 private:
  void PrintErrorHeader();
  void PrintSourceError();

  FILE* file_;
  std::string header_;
  PrintHeader print_header_;
  size_t source_line_max_length_;
};

class SourceErrorHandlerBuffer : public SourceErrorHandler {
 public:
  explicit SourceErrorHandlerBuffer(size_t source_line_max_length = 80);

  bool OnError(const Location*,
               const std::string& error,
               const std::string& source_line,
               size_t source_line_column_offset) override;

  size_t source_line_max_length() const override {
    return source_line_max_length_;
  }

  const std::string& buffer() const { return buffer_; }

 private:
  size_t source_line_max_length_;
  std::string buffer_;
};

}  // namespace wabt

#endif // WABT_SOURCE_ERROR_HANDLER_H_
