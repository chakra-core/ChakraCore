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

#ifndef WABT_WAST_LEXER_H_
#define WABT_WAST_LEXER_H_

#include <cstddef>
#include <cstdio>
#include <memory>

#include "src/common.h"
#include "src/lexer-source-line-finder.h"
#include "src/literal.h"
#include "src/opcode.h"
#include "src/token.h"

namespace wabt {

class ErrorHandler;
class LexerSource;
class WastParser;

class WastLexer {
 public:
  WABT_DISALLOW_COPY_AND_ASSIGN(WastLexer);

  WastLexer(std::unique_ptr<LexerSource> source, const char* filename);
  ~WastLexer();

  // Convenience functions.
  static std::unique_ptr<WastLexer> CreateFileLexer(const char* filename);
  static std::unique_ptr<WastLexer> CreateBufferLexer(const char* filename,
                                                      const void* data,
                                                      size_t size);

  Token GetToken(WastParser* parser);
  Result Fill(size_t need);

  // TODO(binji): Move this out of the lexer.
  LexerSourceLineFinder& line_finder() { return line_finder_; }

 private:
  Location GetLocation();
  Literal MakeLiteral(LiteralType);
  std::string GetText(size_t at = 0);

  std::unique_ptr<LexerSource> source_;
  LexerSourceLineFinder line_finder_;
  const char* filename_;
  int line_;
  int comment_nesting_;
  size_t buffer_file_offset_; // File offset of the start of the buffer.
  size_t line_file_offset_;   // File offset of the start of the current line.

  // Lexing data needed by re2c.
  bool eof_;
  char* buffer_;
  size_t buffer_size_;
  char* marker_;
  char* next_pos_;
  char* cursor_;
  char* limit_;
};

}  // namespace wabt

#endif /* WABT_WAST_LEXER_H_ */
