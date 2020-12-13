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

#ifndef WABT_LEXER_SOURCE_LINE_FINDER_H_
#define WABT_LEXER_SOURCE_LINE_FINDER_H_

#include <memory>
#include <string>
#include <vector>

#include "src/common.h"
#include "src/lexer-source.h"
#include "src/range.h"

namespace wabt {

class LexerSourceLineFinder {
 public:
  struct SourceLine {
    std::string line;
    int column_offset;
  };

  explicit LexerSourceLineFinder(std::unique_ptr<LexerSource>);

  Result GetSourceLine(const Location& loc,
                       Offset max_line_length,
                       SourceLine* out_source_line);
  Result GetLineOffsets(int line, OffsetRange* out_offsets);

 private:
  static OffsetRange ClampSourceLineOffsets(OffsetRange line_offset_range,
                                            ColumnRange column_range,
                                            Offset max_line_length);

  bool IsLineCached(int line) const;
  OffsetRange GetCachedLine(int line) const;

  std::unique_ptr<LexerSource> source_;
  std::vector<OffsetRange> line_ranges_;
  Offset next_line_start_;
  bool last_cr_;  // Last read character was a '\r' (carriage return).
  bool eof_;
};

}  // namespace wabt

#endif  // WABT_LEXER_SOURCE_LINE_FINDER_H_
