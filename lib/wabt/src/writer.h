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

namespace wabt {

struct Writer {
  void* user_data;
  Result (*write_data)(size_t offset,
                       const void* data,
                       size_t size,
                       void* user_data);
  Result (*move_data)(size_t dst_offset,
                      size_t src_offset,
                      size_t size,
                      void* user_data);
};

struct OutputBuffer {
  char* start;
  size_t size;
  size_t capacity;
};

struct MemoryWriter {
  Writer base;
  OutputBuffer buf;
};

struct FileWriter {
  Writer base;
  FILE* file;
  size_t offset;
};

/* FileWriter */
Result init_file_writer(FileWriter* writer, const char* filename);
void init_file_writer_existing(FileWriter* writer, FILE* file);
void close_file_writer(FileWriter* writer);

/* MemoryWriter */
Result init_mem_writer(MemoryWriter* writer);
/* Passes ownership of the buffer to writer */
Result init_mem_writer_existing(MemoryWriter* writer, OutputBuffer* buf);
void steal_mem_writer_output_buffer(MemoryWriter* writer,
                                    OutputBuffer* out_buf);
void close_mem_writer(MemoryWriter* writer);

/* OutputBuffer */
void init_output_buffer(OutputBuffer* buf, size_t initial_capacity);
Result write_output_buffer_to_file(OutputBuffer* buf, const char* filename);
void destroy_output_buffer(OutputBuffer* buf);

}  // namespace wabt

#endif /* WABT_WRITER_H_ */
