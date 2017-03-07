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

#include <stdio.h>

#include "common.h"
#include "writer.h"

namespace wabt {

struct Stream {
  Writer* writer;
  size_t offset;
  Result result;
  /* if non-null, log all writes to this stream */
  struct Stream* log_stream;
};

struct FileStream {
  Stream base;
  FileWriter writer;
};

/* whether to display the ASCII characters in the debug output for
 * write_memory */
enum class PrintChars {
  No = 0,
  Yes = 1,
};

void init_stream(Stream* stream, Writer* writer, Stream* log_stream);
void init_file_stream_from_existing(FileStream* stream, FILE* file);
Stream* init_stdout_stream(void);
Stream* init_stderr_stream(void);

/* helper functions for writing to a Stream. the |desc| parameter is
 * optional, and will be appended to the log stream if |stream.log_stream| is
 * non-null. */
void write_data_at(Stream*,
                   size_t offset,
                   const void* src,
                   size_t size,
                   PrintChars print_chars,
                   const char* desc);
void write_data(Stream*, const void* src, size_t size, const char* desc);
void move_data(Stream*, size_t dst_offset, size_t src_offset, size_t size);

void WABT_PRINTF_FORMAT(2, 3) writef(Stream*, const char* format, ...);
/* specified as uint32_t instead of uint8_t so we can check if the value given
 * is in range before wrapping */
void write_u8(Stream*, uint32_t value, const char* desc);
void write_u32(Stream*, uint32_t value, const char* desc);
void write_u64(Stream*, uint64_t value, const char* desc);

static WABT_INLINE void write_char(Stream* stream, char c) {
  write_u8(stream, c, nullptr);
}

/* dump memory as text, similar to the xxd format */
void write_memory_dump(Stream*,
                       const void* start,
                       size_t size,
                       size_t offset,
                       PrintChars print_chars,
                       const char* prefix,
                       const char* desc);

static WABT_INLINE void write_output_buffer_memory_dump(
    Stream* stream,
    struct OutputBuffer* buf) {
  write_memory_dump(stream, buf->start, buf->size, 0, PrintChars::No, nullptr,
                    nullptr);
}

/* Helper function for writing enums as u8. */
template <typename T>
void write_u8_enum(Stream* stream, T value, const char* desc) {
  write_u8(stream, static_cast<uint32_t>(value), desc);
}

}  // namespace wabt

#endif /* WABT_STREAM_H_ */
