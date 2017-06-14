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

#ifndef WABT_WRITER_H_
#define WABT_WRITER_H_

#include <stdio.h>

#include "common.h"

#include <memory>
#include <vector>

namespace wabt {

struct OutputBuffer {
  Result WriteToFile(const char* filename) const;

  size_t size() const { return data.size(); }

  std::vector<uint8_t> data;
};

class Writer {
 public:
  virtual ~Writer() {}
  virtual Result WriteData(size_t offset, const void* data, size_t size) = 0;
  virtual Result MoveData(size_t dst_offset,
                          size_t src_offset,
                          size_t size) = 0;
};

class MemoryWriter : public Writer {
 public:
  MemoryWriter();
  explicit MemoryWriter(std::unique_ptr<OutputBuffer>);

  OutputBuffer& output_buffer() { return *buf_; }
  std::unique_ptr<OutputBuffer> ReleaseOutputBuffer();

  virtual Result WriteData(size_t offset, const void* data, size_t size);
  virtual Result MoveData(size_t dst_offset, size_t src_offset, size_t size);

 private:
  std::unique_ptr<OutputBuffer> buf_;
};

class FileWriter : public Writer {
  WABT_DISALLOW_COPY_AND_ASSIGN(FileWriter);

 public:
  explicit FileWriter(const char* filename);
  explicit FileWriter(FILE* file);
  FileWriter(FileWriter&&);
  FileWriter& operator=(FileWriter&&);
  ~FileWriter();

  bool is_open() const { return file_ != nullptr; }

  virtual Result WriteData(size_t offset, const void* data, size_t size);
  virtual Result MoveData(size_t dst_offset, size_t src_offset, size_t size);

 private:
  FILE* file_;
  size_t offset_;
  bool should_close_;
};

}  // namespace wabt

#endif /* WABT_WRITER_H_ */
