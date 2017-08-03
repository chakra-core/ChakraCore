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

#include "writer.h"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <utility>

#define ERROR0(msg) fprintf(stderr, "%s:%d: " msg, __FILE__, __LINE__)
#define ERROR(fmt, ...) \
  fprintf(stderr, "%s:%d: " fmt, __FILE__, __LINE__, __VA_ARGS__)

namespace wabt {

Result OutputBuffer::WriteToFile(const char* filename) const {
  FILE* file = fopen(filename, "wb");
  if (!file) {
    ERROR("unable to open %s for writing\n", filename);
    return Result::Error;
  }

  if (data.empty()) {
    return Result::Ok;
  }

  ssize_t bytes = fwrite(data.data(), 1, data.size(), file);
  if (bytes < 0 || static_cast<size_t>(bytes) != data.size()) {
    ERROR("failed to write %" PRIzd " bytes to %s\n", data.size(), filename);
    return Result::Error;
  }

  fclose(file);
  return Result::Ok;
}

MemoryWriter::MemoryWriter() : buf_(new OutputBuffer()) {}

MemoryWriter::MemoryWriter(std::unique_ptr<OutputBuffer> buf)
    : buf_(std::move(buf)) {}

std::unique_ptr<OutputBuffer> MemoryWriter::ReleaseOutputBuffer() {
  return std::move(buf_);
}

Result MemoryWriter::WriteData(size_t dst_offset,
                               const void* src,
                               size_t size) {
  if (size == 0)
    return Result::Ok;
  size_t end = dst_offset + size;
  if (end > buf_->data.size()) {
    buf_->data.resize(end);
  }
  uint8_t* dst = &buf_->data[dst_offset];
  memcpy(dst, src, size);
  return Result::Ok;
}

Result MemoryWriter::MoveData(size_t dst_offset,
                              size_t src_offset,
                              size_t size) {
  if (size == 0)
    return Result::Ok;
  size_t src_end = src_offset + size;
  size_t dst_end = dst_offset + size;
  size_t end = src_end > dst_end ? src_end : dst_end;
  if (end > buf_->data.size()) {
    buf_->data.resize(end);
  }

  uint8_t* dst = &buf_->data[dst_offset];
  uint8_t* src = &buf_->data[src_offset];
  memmove(dst, src, size);
  return Result::Ok;
}

FileWriter::FileWriter(FILE* file)
    : file_(file), offset_(0), should_close_(false) {}

FileWriter::FileWriter(const char* filename)
    : file_(nullptr), offset_(0), should_close_(false) {
  file_ = fopen(filename, "wb");

  // TODO(binji): this is pretty cheesy, should come up with a better API.
  if (file_) {
    should_close_ = true;
  } else {
    ERROR("fopen name=\"%s\" failed, errno=%d\n", filename, errno);
  }
}

FileWriter::FileWriter(FileWriter&& other) {
  *this = std::move(other);
}

FileWriter& FileWriter::operator=(FileWriter&& other) {
  file_ = other.file_;
  offset_ = other.offset_;
  should_close_ = other.should_close_;
  other.file_ = nullptr;
  other.offset_ = 0;
  other.should_close_ = false;
  return *this;
}

FileWriter::~FileWriter() {
  // We don't want to close existing files (stdout/sterr, for example).
  if (should_close_) {
    fclose(file_);
  }
}

Result FileWriter::WriteData(size_t at, const void* data, size_t size) {
  if (!file_)
    return Result::Error;
  if (size == 0)
    return Result::Ok;
  if (at != offset_) {
    if (fseek(file_, at, SEEK_SET) != 0) {
      ERROR("fseek offset=%" PRIzd " failed, errno=%d\n", size, errno);
      return Result::Error;
    }
    offset_ = at;
  }
  if (fwrite(data, size, 1, file_) != 1) {
    ERROR("fwrite size=%" PRIzd " failed, errno=%d\n", size, errno);
    return Result::Error;
  }
  offset_ += size;
  return Result::Ok;
}

Result FileWriter::MoveData(size_t dst_offset, size_t src_offset, size_t size) {
  if (!file_)
    return Result::Error;
  if (size == 0)
    return Result::Ok;
  // TODO(binji): implement if needed.
  ERROR0("FileWriter::MoveData not implemented!\n");
  return Result::Error;
}

}  // namespace wabt
