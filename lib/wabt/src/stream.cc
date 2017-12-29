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

#include "src/stream.h"

#include <cassert>
#include <cctype>
#include <cerrno>

#define DUMP_OCTETS_PER_LINE 16
#define DUMP_OCTETS_PER_GROUP 2

#define ERROR0(msg) fprintf(stderr, "%s:%d: " msg, __FILE__, __LINE__)
#define ERROR(fmt, ...) \
  fprintf(stderr, "%s:%d: " fmt, __FILE__, __LINE__, __VA_ARGS__)

namespace wabt {

Stream::Stream(Stream* log_stream)
    : offset_(0), result_(Result::Ok), log_stream_(log_stream) {}

void Stream::AddOffset(ssize_t delta) {
  offset_ += delta;
}

void Stream::WriteDataAt(size_t at,
                         const void* src,
                         size_t size,
                         const char* desc,
                         PrintChars print_chars) {
  if (Failed(result_)) {
    return;
  }
  if (log_stream_) {
    log_stream_->WriteMemoryDump(src, size, at, print_chars, nullptr, desc);
  }
  result_ = WriteDataImpl(at, src, size);
}

void Stream::WriteData(const void* src,
                       size_t size,
                       const char* desc,
                       PrintChars print_chars) {
  WriteDataAt(offset_, src, size, desc, print_chars);
  offset_ += size;
}

void Stream::MoveData(size_t dst_offset, size_t src_offset, size_t size) {
  if (Failed(result_)) {
    return;
  }
  if (log_stream_) {
    log_stream_->Writef(
        "; move data: [%" PRIzx ", %" PRIzx ") -> [%" PRIzx ", %" PRIzx ")\n",
        src_offset, src_offset + size, dst_offset, dst_offset + size);
  }
  result_ = MoveDataImpl(dst_offset, src_offset, size);
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
    if (prefix) {
      Writef("%s", prefix);
    }
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
    if (p >= end && desc) {
      Writef("  ; %s", desc);
    }
    WriteChar('\n');
  }
}

Result OutputBuffer::WriteToFile(string_view filename) const {
  std::string filename_str = filename.to_string();
  FILE* file = fopen(filename_str.c_str(), "wb");
  if (!file) {
    ERROR("unable to open %s for writing\n", filename_str.c_str());
    return Result::Error;
  }

  if (data.empty()) {
    fclose(file);
    return Result::Ok;
  }

  ssize_t bytes = fwrite(data.data(), 1, data.size(), file);
  if (bytes < 0 || static_cast<size_t>(bytes) != data.size()) {
    ERROR("failed to write %" PRIzd " bytes to %s\n", data.size(),
          filename_str.c_str());
    fclose(file);
    return Result::Error;
  }

  fclose(file);
  return Result::Ok;
}

MemoryStream::MemoryStream(Stream* log_stream)
    : Stream(log_stream), buf_(new OutputBuffer()) {}

MemoryStream::MemoryStream(std::unique_ptr<OutputBuffer>&& buf,
                           Stream* log_stream)
    : Stream(log_stream), buf_(std::move(buf)) {}

std::unique_ptr<OutputBuffer> MemoryStream::ReleaseOutputBuffer() {
  return std::move(buf_);
}

Result MemoryStream::WriteDataImpl(size_t dst_offset,
                                   const void* src,
                                   size_t size) {
  if (size == 0) {
    return Result::Ok;
  }
  size_t end = dst_offset + size;
  if (end > buf_->data.size()) {
    buf_->data.resize(end);
  }
  uint8_t* dst = &buf_->data[dst_offset];
  memcpy(dst, src, size);
  return Result::Ok;
}

Result MemoryStream::MoveDataImpl(size_t dst_offset,
                                  size_t src_offset,
                                  size_t size) {
  if (size == 0) {
    return Result::Ok;
  }
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

FileStream::FileStream(string_view filename, Stream* log_stream)
    : Stream(log_stream), file_(nullptr), offset_(0), should_close_(false) {
  std::string filename_str = filename.to_string();
  file_ = fopen(filename_str.c_str(), "wb");

  // TODO(binji): this is pretty cheesy, should come up with a better API.
  if (file_) {
    should_close_ = true;
  } else {
    ERROR("fopen name=\"%s\" failed, errno=%d\n", filename_str.c_str(), errno);
  }
}

FileStream::FileStream(FILE* file, Stream* log_stream)
    : Stream(log_stream), file_(file), offset_(0), should_close_(false) {}

FileStream::FileStream(FileStream&& other) {
  *this = std::move(other);
}

FileStream& FileStream::operator=(FileStream&& other) {
  file_ = other.file_;
  offset_ = other.offset_;
  should_close_ = other.should_close_;
  other.file_ = nullptr;
  other.offset_ = 0;
  other.should_close_ = false;
  return *this;
}

FileStream::~FileStream() {
  // We don't want to close existing files (stdout/sterr, for example).
  if (should_close_) {
    fclose(file_);
  }
}

Result FileStream::WriteDataImpl(size_t at, const void* data, size_t size) {
  if (!file_) {
    return Result::Error;
  }
  if (size == 0) {
    return Result::Ok;
  }
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

Result FileStream::MoveDataImpl(size_t dst_offset,
                                size_t src_offset,
                                size_t size) {
  if (!file_) {
    return Result::Error;
  }
  if (size == 0) {
    return Result::Ok;
  }
  // TODO(binji): implement if needed.
  ERROR0("FileWriter::MoveData not implemented!\n");
  return Result::Error;
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
