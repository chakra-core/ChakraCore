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

#include "wast-parser-lexer-shared.h"

#include "common.h"
#include "error-handler.h"
#include "wast-lexer.h"

namespace wabt {

void WastFormatError(ErrorHandler* error_handler,
                     const Location* loc,
                     WastLexer* lexer,
                     const char* format,
                     va_list args) {
  va_list args_copy;
  va_copy(args_copy, args);
  char fixed_buf[WABT_DEFAULT_SNPRINTF_ALLOCA_BUFSIZE];
  char* buffer = fixed_buf;
  size_t len = wabt_vsnprintf(fixed_buf, sizeof(fixed_buf), format, args);
  if (len + 1 > sizeof(fixed_buf)) {
    buffer = static_cast<char*>(alloca(len + 1));
    len = wabt_vsnprintf(buffer, len + 1, format, args_copy);
  }

  LexerSourceLineFinder::SourceLine source_line;
  if (loc && lexer) {
    size_t source_line_max_length = error_handler->source_line_max_length();
    Result result = lexer->line_finder().GetSourceLine(
        *loc, source_line_max_length, &source_line);
    if (Failed(result)) {
      // If this fails, it means that we've probably screwed up the lexer. Blow
      // up.
      WABT_FATAL("error getting the source line.\n");
    }
  }

  error_handler->OnError(*loc, std::string(buffer), source_line.line,
                         source_line.column_offset);
  va_end(args_copy);
}

}  // namespace wabt
