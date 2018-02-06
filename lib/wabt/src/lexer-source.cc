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

#include "src/lexer-source.h"

#include <algorithm>

namespace wabt {

LexerSourceFile::LexerSourceFile(string_view filename)
  : filename_(filename) {
  file_ = fopen(filename_.c_str(), "rb");
}

LexerSourceFile::~LexerSourceFile() {
  if (file_) {
    fclose(file_);
  }
}

std::unique_ptr<LexerSource> LexerSourceFile::Clone() {
  std::unique_ptr<LexerSourceFile> result(new LexerSourceFile(filename_));

  Offset offset = 0;
  if (Failed(Tell(&offset)) || Failed(result->Seek(offset))) {
    result.reset();
  }

  return std::move(result);
}

Result LexerSourceFile::Tell(Offset* out_offset) {
  if (!file_) {
    return Result::Error;
  }

  long offset = ftell(file_);
  if (offset < 0) {
    return Result::Error;
  }

  *out_offset = offset;
  return Result::Ok;
}

size_t LexerSourceFile::Fill(void* dest, size_t size)  {
  if (!file_) {
    return 0;
  }
  return fread(dest, 1, size, file_);
}

Result LexerSourceFile::ReadRange(OffsetRange range,
                                  std::vector<char>* out_data) {
  Offset old_offset = 0;
  CHECK_RESULT(Tell(&old_offset));
  CHECK_RESULT(Seek(range.start));

  std::vector<char> result(range.size());
  if (range.size() > 0) {
    size_t read_size = Fill(result.data(), range.size());
    if (read_size < range.size()) {
      result.resize(read_size);
    }
  }

  CHECK_RESULT(Seek(old_offset));

  *out_data = std::move(result);
  return Result::Ok;
}

Result LexerSourceFile::Seek(Offset offset) {
  if (!file_) {
    return Result::Error;
  }

  int result = fseek(file_, offset, SEEK_SET);
  return result < 0 ? Result::Error : Result::Ok;
}

LexerSourceBuffer::LexerSourceBuffer(const void* data, Offset size)
    : data_(data), size_(size), read_offset_(0) {}

std::unique_ptr<LexerSource> LexerSourceBuffer::Clone() {
  LexerSourceBuffer* result = new LexerSourceBuffer(data_, size_);
  result->read_offset_ = read_offset_;
  return std::unique_ptr<LexerSource>(result);
}

Result LexerSourceBuffer::Tell(Offset* out_offset) {
  *out_offset = read_offset_;
  return Result::Ok;
}

size_t LexerSourceBuffer::Fill(void* dest, Offset size) {
  Offset read_size = std::min(size, size_ - read_offset_);
  if (read_size > 0) {
    const void* src = static_cast<const char*>(data_) + read_offset_;
    memcpy(dest, src, read_size);
    read_offset_ += read_size;
  }
  return read_size;
}

Result LexerSourceBuffer::ReadRange(OffsetRange range,
                                    std::vector<char>* out_data) {
  OffsetRange clamped = range;
  clamped.start = std::min(clamped.start, size_);
  clamped.end = std::min(clamped.end, size_);
  if (clamped.size()) {
    out_data->resize(clamped.size());
    const void* src = static_cast<const char*>(data_) + clamped.start;
    memcpy(out_data->data(), src, clamped.size());
  }
  return Result::Ok;
}

}  // namespace wabt
