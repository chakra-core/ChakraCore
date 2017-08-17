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

#include "lexer-source-line-finder.h"

#include <algorithm>

#include "lexer-source.h"

#define CHECK_RESULT(expr)  \
  do {                      \
    if (Failed(expr))       \
      return Result::Error; \
  } while (0)

namespace wabt {

LexerSourceLineFinder::LexerSourceLineFinder(
    std::unique_ptr<LexerSource> source)
    : source_(std::move(source)),
      next_line_start_(0),
      last_cr_(false),
      eof_(false) {
  // Line 0 should not be used; but it makes indexing simpler.
  line_ranges_.emplace_back(0, 0);
}

Result LexerSourceLineFinder::GetSourceLine(const Location& loc,
                                            Offset max_line_length,
                                            SourceLine* out_source_line) {
  ColumnRange column_range(loc.first_column, loc.last_column);
  OffsetRange original;
  CHECK_RESULT(GetLineOffsets(loc.line, &original));

  OffsetRange clamped =
      ClampSourceLineOffsets(original, column_range, max_line_length);
  bool has_start_ellipsis = original.start != clamped.start;
  bool has_end_ellipsis = original.end != clamped.end;

  out_source_line->column_offset = clamped.start - original.start;

  if (has_start_ellipsis) {
    out_source_line->line += "...";
    clamped.start += 3;
  }
  if (has_end_ellipsis)
    clamped.end -= 3;

  std::vector<char> read_line;
  CHECK_RESULT(source_->ReadRange(clamped, &read_line));
  out_source_line->line.append(read_line.begin(), read_line.end());

  if (has_end_ellipsis)
    out_source_line->line += "...";

  return Result::Ok;
}

bool LexerSourceLineFinder::IsLineCached(int line) const {
  return static_cast<size_t>(line) < line_ranges_.size();
}

OffsetRange LexerSourceLineFinder::GetCachedLine(int line) const {
  assert(IsLineCached(line));
  return line_ranges_[line];
}

Result LexerSourceLineFinder::GetLineOffsets(int find_line,
                                             OffsetRange* out_range) {
  if (IsLineCached(find_line)) {
    *out_range = GetCachedLine(find_line);
    return Result::Ok;
  }

  const size_t kBufferSize = 1 << 16;
  std::vector<char> buffer(kBufferSize);

  assert(!line_ranges_.empty());
  Offset buffer_file_offset = 0;
  CHECK_RESULT(source_->Tell(&buffer_file_offset));
  while (!IsLineCached(find_line) && !eof_) {
    size_t read_size = source_->Fill(buffer.data(), buffer.size());
    if (read_size < buffer.size())
      eof_ = true;

    for (auto iter = buffer.begin(), end = iter + read_size; iter < end;
         ++iter) {
      if (*iter == '\n') {
        // Don't include \n or \r in the line range.
        Offset line_offset =
            buffer_file_offset + (iter - buffer.begin()) - last_cr_;
        line_ranges_.emplace_back(next_line_start_, line_offset);
        next_line_start_ = line_offset + last_cr_ + 1;
      }
      last_cr_ = *iter == '\r';
    }

    if (eof_) {
      // Add the final line as an empty range.
      Offset end = buffer_file_offset + read_size;
      line_ranges_.emplace_back(next_line_start_, end);
    }
  }

  if (IsLineCached(find_line)) {
    *out_range = GetCachedLine(find_line);
    return Result::Ok;
  } else {
    assert(eof_);
    return Result::Error;
  }
}

// static
OffsetRange LexerSourceLineFinder::ClampSourceLineOffsets(
    OffsetRange offset_range,
    ColumnRange column_range,
    Offset max_line_length) {
  Offset line_length = offset_range.size();
  if (line_length > max_line_length) {
    size_t column_count = column_range.size();
    size_t center_on;
    if (column_count > max_line_length) {
      // The column range doesn't fit, just center on first_column.
      center_on = column_range.start - 1;
    } else {
      // the entire range fits, display it all in the center.
      center_on = (column_range.start + column_range.end) / 2 - 1;
    }
    if (center_on > max_line_length / 2)
      offset_range.start += center_on - max_line_length / 2;
    offset_range.start =
        std::min(offset_range.start, offset_range.end - max_line_length);
    offset_range.end = offset_range.start + max_line_length;
  }

  return offset_range;
}

}  // namespace wabt
