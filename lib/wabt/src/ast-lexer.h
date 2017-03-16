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

#ifndef WABT_AST_LEXER_H_
#define WABT_AST_LEXER_H_

#include <stddef.h>
#include <stdio.h>

#include "common.h"
#include "vector.h"

namespace wabt {

enum class AstLexerSourceType {
  File,
  Buffer,
};

struct AstLexerSource {
  AstLexerSourceType type;
  union {
    FILE* file;
    struct {
      const void* data;
      size_t size;
      size_t read_offset;
    } buffer;
  };
};

struct AstLexer {
  AstLexerSource source;
  const char* filename;
  int line;
  int comment_nesting;
  size_t buffer_file_offset; /* file offset of the start of the buffer */
  size_t line_file_offset;   /* file offset of the start of the current line */

  /* lexing data needed by re2c */
  bool eof;
  char* buffer;
  size_t buffer_size;
  char* marker;
  char* token;
  char* cursor;
  char* limit;
};

AstLexer* new_ast_file_lexer(const char* filename);
AstLexer* new_ast_buffer_lexer(const char* filename,
                               const void* data,
                               size_t size);
void destroy_ast_lexer(AstLexer*);

}  // namespace wabt

#endif /* WABT_AST_LEXER_H_ */
