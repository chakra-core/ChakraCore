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

#ifndef WABT_ERROR_FORMATTER_H_
#define WABT_ERROR_FORMATTER_H_

#include <cstdio>
#include <string>
#include <memory>

#include "src/color.h"
#include "src/error.h"
#include "src/lexer-source-line-finder.h"

namespace wabt {

enum class PrintHeader {
  Never,
  Once,
  Always,
};

std::string FormatErrorsToString(const Errors&,
                                 Location::Type,
                                 LexerSourceLineFinder* = nullptr,
                                 const Color& color = Color(nullptr, false),
                                 const std::string& header = {},
                                 PrintHeader print_header = PrintHeader::Never,
                                 int source_line_max_length = 80);

void FormatErrorsToFile(const Errors&,
                        Location::Type,
                        LexerSourceLineFinder* = nullptr,
                        FILE* = stderr,
                        const std::string& header = {},
                        PrintHeader print_header = PrintHeader::Never,
                        int source_line_max_length = 80);

}  // namespace wabt

#endif  // WABT_ERROR_FORMATTER_H_
