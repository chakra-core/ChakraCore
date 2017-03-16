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

#include <assert.h>
#include <ctype.h>

#define DUMP_OCTETS_PER_LINE 16
#define DUMP_OCTETS_PER_GROUP 2

namespace wabt {

static FileStream s_stdout_stream;
static FileStream s_stderr_stream;

void init_stream(Stream* stream, Writer* writer, Stream* log_stream) {
  stream->writer = writer;
  stream->offset = 0;
  stream->result = Result::Ok;
  stream->log_stream = log_stream;
}

void init_file_stream_from_existing(FileStream* stream, FILE* file) {
  init_file_writer_existing(&stream->writer, file);
  init_stream(&stream->base, &stream->writer.base, nullptr);
}

Stream* init_stdout_stream(void) {
  init_file_stream_from_existing(&s_stdout_stream, stdout);
  return &s_stdout_stream.base;
}

Stream* init_stderr_stream(void) {
  init_file_stream_from_existing(&s_stderr_stream, stderr);
  return &s_stderr_stream.base;
}

void write_data_at(Stream* stream,
                   size_t offset,
                   const void* src,
                   size_t size,
                   PrintChars print_chars,
                   const char* desc) {
  if (WABT_FAILED(stream->result))
    return;
  if (stream->log_stream) {
    write_memory_dump(stream->log_stream, src, size, offset, print_chars,
                      nullptr, desc);
  }
  if (stream->writer->write_data) {
    stream->result = stream->writer->write_data(offset, src, size,
                                                stream->writer->user_data);
  }
}

void write_data(Stream* stream,
                const void* src,
                size_t size,
                const char* desc) {
  write_data_at(stream, stream->offset, src, size, PrintChars::No, desc);
  stream->offset += size;
}

void move_data(Stream* stream,
               size_t dst_offset,
               size_t src_offset,
               size_t size) {
  if (WABT_FAILED(stream->result))
    return;
  if (stream->log_stream) {
    writef(stream->log_stream, "; move data: [%" PRIzx ", %" PRIzx
                               ") -> [%" PRIzx ", %" PRIzx ")\n",
           src_offset, src_offset + size, dst_offset, dst_offset + size);
  }
  if (stream->writer->write_data) {
    stream->result = stream->writer->move_data(dst_offset, src_offset, size,
                                               stream->writer->user_data);
  }
}

void writef(Stream* stream, const char* format, ...) {
  WABT_SNPRINTF_ALLOCA(buffer, length, format);
  write_data(stream, buffer, length, nullptr);
}

void write_u8(Stream* stream, uint32_t value, const char* desc) {
  assert(value <= UINT8_MAX);
  uint8_t value8 = value;
  write_data_at(stream, stream->offset, &value8, sizeof(value8), PrintChars::No,
                desc);
  stream->offset += sizeof(value8);
}

void write_u32(Stream* stream, uint32_t value, const char* desc) {
  write_data_at(stream, stream->offset, &value, sizeof(value), PrintChars::No,
                desc);
  stream->offset += sizeof(value);
}

void write_u64(Stream* stream, uint64_t value, const char* desc) {
  write_data_at(stream, stream->offset, &value, sizeof(value), PrintChars::No,
                desc);
  stream->offset += sizeof(value);
}

void write_memory_dump(Stream* stream,
                       const void* start,
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
      writef(stream, "%s", prefix);
    writef(stream, "%07" PRIzx ": ", reinterpret_cast<intptr_t>(p) -
                                         reinterpret_cast<intptr_t>(start) +
                                         offset);
    while (p < line_end) {
      int i;
      for (i = 0; i < DUMP_OCTETS_PER_GROUP; ++i, ++p) {
        if (p < end) {
          writef(stream, "%02x", *p);
        } else {
          write_char(stream, ' ');
          write_char(stream, ' ');
        }
      }
      write_char(stream, ' ');
    }

    if (print_chars == PrintChars::Yes) {
      write_char(stream, ' ');
      p = line;
      int i;
      for (i = 0; i < DUMP_OCTETS_PER_LINE && p < end; ++i, ++p)
        write_char(stream, isprint(*p) ? *p : '.');
    }

    /* if there are multiple lines, only print the desc on the last one */
    if (p >= end && desc)
      writef(stream, "  ; %s", desc);
    write_char(stream, '\n');
  }
}

}  // namespace wabt
