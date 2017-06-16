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

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

namespace wabt {

void wast_parser_error(Location* loc,
                       WastLexer* lexer,
                       WastParser* parser,
                       const char* format,
                       ...) {
  parser->errors++;
  va_list args;
  va_start(args, format);
  wast_format_error(parser->error_handler, loc, lexer, format, args);
  va_end(args);
}

void wast_format_error(SourceErrorHandler* error_handler,
                       const struct Location* loc,
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
    if (WABT_FAILED(result)) {
      // If this fails, it means that we've probably screwed up the lexer. Blow
      // up.
      WABT_FATAL("error getting the source line.\n");
    }
  }

  error_handler->OnError(loc, std::string(buffer), source_line.line,
                         source_line.column_offset);
  va_end(args_copy);
}

void destroy_text_list(TextList* text_list) {
  TextListNode* node = text_list->first;
  while (node) {
    TextListNode* next = node->next;
    destroy_string_slice(&node->text);
    delete node;
    node = next;
  }
}

void destroy_module_field_list(ModuleFieldList* fields) {
  ModuleField* field = fields->first;
  while (field) {
    ModuleField* next = field->next;
    delete field;
    field = next;
  }
}

}  // namespace wabt
