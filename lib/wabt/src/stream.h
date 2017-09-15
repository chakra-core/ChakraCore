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

#ifndef WABT_STREAM_H_
#define WABT_STREAM_H_

#include <cassert>
#include <memory>
#include <vector>

#include "src/common.h"

namespace wabt {

/* whether to display the ASCII characters in the debug output for
 * write_memory */
enum class PrintChars {
  No = 0,
  Yes = 1,
};

class Stream {
 public:
  explicit Stream(Stream* log_stream = nullptr);
  virtual ~Stream() = default;

  size_t offset() { return offset_; }
  Result result() { return result_; }

  void set_log_stream(Stream* stream) {
    assert(stream);
    log_stream_ = stream;
  }

  Stream& log_stream() {
    assert(log_stream_);
    return *log_stream_;
  }

  bool has_log_stream() const { return log_stream_ != nullptr; }

  void AddOffset(ssize_t delta);

  void WriteData(const void* src,
                 size_t size,
                 const char* desc = nullptr,
                 PrintChars = PrintChars::No);

  template <typename T>
  void WriteData(const std::vector<T> src,
                 const char* desc,
                 PrintChars print_chars = PrintChars::No) {
    if (!src.empty())
      WriteData(src.data(), src.size() * sizeof(T), desc, print_chars);
  }

  void WriteDataAt(size_t offset,
                   const void* src,
                   size_t size,
                   const char* desc = nullptr,
                   PrintChars = PrintChars::No);

  void MoveData(size_t dst_offset, size_t src_offset, size_t size);

  void WABT_PRINTF_FORMAT(2, 3) Writef(const char* format, ...);

  // Specified as uint32_t instead of uint8_t so we can check if the value
  // given is in range before wrapping.
  void WriteU8(uint32_t value,
               const char* desc = nullptr,
               PrintChars print_chars = PrintChars::No) {
    assert(value <= UINT8_MAX);
    Write(static_cast<uint8_t>(value), desc, print_chars);
  }
  void WriteU32(uint32_t value,
                const char* desc = nullptr,
                PrintChars print_chars = PrintChars::No) {
    Write(value, desc, print_chars);
  }
  void WriteU64(uint64_t value,
                const char* desc = nullptr,
                PrintChars print_chars = PrintChars::No) {
    Write(value, desc, print_chars);
  }
  void WriteChar(char c,
                 const char* desc = nullptr,
                 PrintChars print_chars = PrintChars::No) {
    WriteU8(c, desc, print_chars);
  }

  // Dump memory as text, similar to the xxd format.
  void WriteMemoryDump(const void* start,
                       size_t size,
                       size_t offset = 0,
                       PrintChars print_chars = PrintChars::No,
                       const char* prefix = nullptr,
                       const char* desc = nullptr);

  // Convenience functions for writing enums.
  template <typename T>
  void WriteU8Enum(T value,
                   const char* desc = nullptr,
                   PrintChars print_chars = PrintChars::No) {
    WriteU8(static_cast<uint32_t>(value), desc, print_chars);
  }

 protected:
  virtual Result WriteDataImpl(size_t offset,
                               const void* data,
                               size_t size) = 0;
  virtual Result MoveDataImpl(size_t dst_offset,
                              size_t src_offset,
                              size_t size) = 0;

 private:
  template <typename T>
  void Write(const T& data, const char* desc, PrintChars print_chars) {
    WriteData(&data, sizeof(data), desc, print_chars);
  }

  size_t offset_;
  Result result_;
  // Not owned. If non-null, log all writes to this stream.
  Stream* log_stream_;
};

struct OutputBuffer {
  Result WriteToFile(string_view filename) const;

  size_t size() const { return data.size(); }

  std::vector<uint8_t> data;
};

class MemoryStream : public Stream {
 public:
  WABT_DISALLOW_COPY_AND_ASSIGN(MemoryStream);
  explicit MemoryStream(Stream* log_stream = nullptr);
  explicit MemoryStream(std::unique_ptr<OutputBuffer>&&,
                        Stream* log_stream = nullptr);

  OutputBuffer& output_buffer() { return *buf_; }
  std::unique_ptr<OutputBuffer> ReleaseOutputBuffer();

  Result WriteToFile(string_view filename) {
    return buf_->WriteToFile(filename);
  }

 protected:
  Result WriteDataImpl(size_t offset, const void* data, size_t size) override;
  Result MoveDataImpl(size_t dst_offset,
                      size_t src_offset,
                      size_t size) override;

 private:
  std::unique_ptr<OutputBuffer> buf_;
};

class FileStream : public Stream {
 public:
  WABT_DISALLOW_COPY_AND_ASSIGN(FileStream);
  explicit FileStream(string_view filename, Stream* log_stream = nullptr);
  explicit FileStream(FILE*, Stream* log_stream = nullptr);
  FileStream(FileStream&&);
  FileStream& operator=(FileStream&&);
  ~FileStream();

  static std::unique_ptr<FileStream> CreateStdout();
  static std::unique_ptr<FileStream> CreateStderr();

  bool is_open() const { return file_ != nullptr; }

 protected:
  Result WriteDataImpl(size_t offset, const void* data, size_t size) override;
  Result MoveDataImpl(size_t dst_offset,
                      size_t src_offset,
                      size_t size) override;

 private:
  FILE* file_;
  size_t offset_;
  bool should_close_;
};

}  // namespace wabt

#endif /* WABT_STREAM_H_ */
