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

#include "ast-parser-lexer-shared.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace wabt {

void ast_parser_error(Location* loc,
                      AstLexer* lexer,
                      AstParser* parser,
                      const char* format,
                      ...) {
  parser->errors++;
  va_list args;
  va_start(args, format);
  ast_format_error(parser->error_handler, loc, lexer, format, args);
  va_end(args);
}

void ast_format_error(SourceErrorHandler* error_handler,
                      const struct Location* loc,
                      AstLexer* lexer,
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

  char* source_line = nullptr;
  size_t source_line_length = 0;
  int source_line_column_offset = 0;
  size_t source_line_max_length = error_handler->source_line_max_length;
  if (loc && lexer) {
    source_line = static_cast<char*>(alloca(source_line_max_length + 1));
    Result result = ast_lexer_get_source_line(
        lexer, loc, source_line_max_length, source_line, &source_line_length,
        &source_line_column_offset);
    if (WABT_FAILED(result)) {
      /* if this fails, it means that we've probably screwed up the lexer. blow
       * up. */
      WABT_FATAL("error getting the source line.\n");
    }
  }

  if (error_handler->on_error) {
    error_handler->on_error(loc, buffer, source_line, source_line_length,
                            source_line_column_offset,
                            error_handler->user_data);
  }
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

FuncField::FuncField()
    : type(FuncFieldType::Exprs), first_expr(nullptr), next(nullptr) {}

FuncField::~FuncField() {
  switch (type) {
    case FuncFieldType::Exprs:
      destroy_expr_list(first_expr);
      break;

    case FuncFieldType::ParamTypes:
    case FuncFieldType::LocalTypes:
    case FuncFieldType::ResultTypes:
      delete types;
      break;

    case FuncFieldType::BoundParam:
    case FuncFieldType::BoundLocal:
      destroy_string_slice(&bound_type.name);
      break;
  }
}

void destroy_func_fields(FuncField* func_field) {
  /* destroy the entire linked-list */
  while (func_field) {
    FuncField* next_func_field = func_field->next;
    delete func_field;
    func_field = next_func_field;
  }
}

}  // namespace wabt
