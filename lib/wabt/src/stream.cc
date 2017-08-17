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

#include "stream.h"

#include <cassert>
#include <cctype>

#define DUMP_OCTETS_PER_LINE 16
#define DUMP_OCTETS_PER_GROUP 2

namespace wabt {

Stream::Stream(Writer* writer, Stream* log_stream)
    : writer_(writer),
      offset_(0),
      result_(Result::Ok),
      log_stream_(log_stream) {}

void Stream::AddOffset(ssize_t delta) {
  offset_ += delta;
}

void Stream::WriteDataAt(size_t at,
                         const void* src,
                         size_t size,
                         const char* desc,
                         PrintChars print_chars) {
  if (Failed(result_))
    return;
  if (log_stream_) {
    log_stream_->WriteMemoryDump(src, size, at, print_chars, nullptr, desc);
  }
  result_ = writer_->WriteData(at, src, size);
}

void Stream::WriteData(const void* src,
                       size_t size,
                       const char* desc,
                       PrintChars print_chars) {
  WriteDataAt(offset_, src, size, desc, print_chars);
  offset_ += size;
}

void Stream::MoveData(size_t dst_offset, size_t src_offset, size_t size) {
  if (Failed(result_))
    return;
  if (log_stream_) {
    log_stream_->Writef(
        "; move data: [%" PRIzx ", %" PRIzx ") -> [%" PRIzx ", %" PRIzx ")\n",
        src_offset, src_offset + size, dst_offset, dst_offset + size);
  }
  result_ = writer_->MoveData(dst_offset, src_offset, size);
}

void Stream::Writef(const char* format, ...) {
  WABT_SNPRINTF_ALLOCA(buffer, length, format);
  WriteData(buffer, length);
}

void Stream::WriteMemoryDump(const void* start,
                             size_t size,
                             size_t offset,
                             PrintChars print_chars,
                             const char* prefix,
                             const char* desc) {
  const uint8_t* p = static_cast<const uint8_t*>(start);
  const uint8_t* end = p + size;
  while (p < end) {
    const uint8_t* line = p;
    const uint8_t* line_end = p + DUMP_OCTETS_PER_LINE;
    if (prefix)
      Writef("%s", prefix);
    Writef("%07" PRIzx ": ", reinterpret_cast<intptr_t>(p) -
                                 reinterpret_cast<intptr_t>(start) + offset);
    while (p < line_end) {
      for (int i = 0; i < DUMP_OCTETS_PER_GROUP; ++i, ++p) {
        if (p < end) {
          Writef("%02x", *p);
        } else {
          WriteChar(' ');
          WriteChar(' ');
        }
      }
      WriteChar(' ');
    }

    if (print_chars == PrintChars::Yes) {
      WriteChar(' ');
      p = line;
      for (int i = 0; i < DUMP_OCTETS_PER_LINE && p < end; ++i, ++p)
        WriteChar(isprint(*p) ? *p : '.');
    }

    /* if there are multiple lines, only print the desc on the last one */
    if (p >= end && desc)
      Writef("  ; %s", desc);
    WriteChar('\n');
  }
}

MemoryStream::MemoryStream() : Stream(&writer_) {}

FileStream::FileStream(string_view filename)
    : Stream(&writer_), writer_(filename) {}

FileStream::FileStream(FILE* file) : Stream(&writer_), writer_(file) {}

FileStream::FileStream(FileStream&& other)
    : Stream(&writer_), writer_(std::move(other.writer_)) {}

FileStream& FileStream::operator=(FileStream&& other) {
  writer_ = std::move(other.writer_);
  return *this;
}

// static
std::unique_ptr<FileStream> FileStream::CreateStdout() {
  return std::unique_ptr<FileStream>(new FileStream(stdout));
}

// static
std::unique_ptr<FileStream> FileStream::CreateStderr() {
  return std::unique_ptr<FileStream>(new FileStream(stderr));
}


}  // namespace wabt
