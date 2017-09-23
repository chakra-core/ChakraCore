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

#include "src/leb128.h"

#include "src/stream.h"

#define MAX_U32_LEB128_BYTES 5
#define MAX_U64_LEB128_BYTES 10

namespace wabt {

Offset U32Leb128Length(uint32_t value) {
  uint32_t size = 0;
  do {
    value >>= 7;
    size++;
  } while (value != 0);
  return size;
}

#define LEB128_LOOP_UNTIL(end_cond) \
  do {                              \
    uint8_t byte = value & 0x7f;    \
    value >>= 7;                    \
    if (end_cond) {                 \
      data[length++] = byte;        \
      break;                        \
    } else {                        \
      data[length++] = byte | 0x80; \
    }                               \
  } while (1)

Offset WriteFixedU32Leb128At(Stream* stream,
                             Offset offset,
                             uint32_t value,
                             const char* desc) {
  uint8_t data[MAX_U32_LEB128_BYTES];
  Offset length =
      WriteFixedU32Leb128Raw(data, data + MAX_U32_LEB128_BYTES, value);
  stream->WriteDataAt(offset, data, length, desc);
  return length;
}

void WriteU32Leb128(Stream* stream, uint32_t value, const char* desc) {
  uint8_t data[MAX_U32_LEB128_BYTES];
  Offset length = 0;
  LEB128_LOOP_UNTIL(value == 0);
  stream->WriteData(data, length, desc);
}

void WriteFixedU32Leb128(Stream* stream, uint32_t value, const char* desc) {
  uint8_t data[MAX_U32_LEB128_BYTES];
  Offset length =
      WriteFixedU32Leb128Raw(data, data + MAX_U32_LEB128_BYTES, value);
  stream->WriteData(data, length, desc);
}

// returns the length of the leb128.
Offset WriteU32Leb128At(Stream* stream,
                        Offset offset,
                        uint32_t value,
                        const char* desc) {
  uint8_t data[MAX_U32_LEB128_BYTES];
  Offset length = 0;
  LEB128_LOOP_UNTIL(value == 0);
  stream->WriteDataAt(offset, data, length, desc);
  return length;
}

Offset WriteFixedU32Leb128Raw(uint8_t* data, uint8_t* end, uint32_t value) {
  if (end - data < MAX_U32_LEB128_BYTES)
    return 0;
  data[0] = (value & 0x7f) | 0x80;
  data[1] = ((value >> 7) & 0x7f) | 0x80;
  data[2] = ((value >> 14) & 0x7f) | 0x80;
  data[3] = ((value >> 21) & 0x7f) | 0x80;
  data[4] = ((value >> 28) & 0x0f);
  return MAX_U32_LEB128_BYTES;
}

static void WriteS32Leb128(Stream* stream, int32_t value, const char* desc) {
  uint8_t data[MAX_U32_LEB128_BYTES];
  Offset length = 0;
  if (value < 0)
    LEB128_LOOP_UNTIL(value == -1 && (byte & 0x40));
  else
    LEB128_LOOP_UNTIL(value == 0 && !(byte & 0x40));

  stream->WriteData(data, length, desc);
}

static void WriteS64Leb128(Stream* stream, int64_t value, const char* desc) {
  uint8_t data[MAX_U64_LEB128_BYTES];
  Offset length = 0;
  if (value < 0)
    LEB128_LOOP_UNTIL(value == -1 && (byte & 0x40));
  else
    LEB128_LOOP_UNTIL(value == 0 && !(byte & 0x40));

  stream->WriteData(data, length, desc);
}

void WriteS32Leb128(Stream* stream, uint32_t value, const char* desc) {
  WriteS32Leb128(stream, Bitcast<int32_t>(value), desc);
}

void WriteS64Leb128(Stream* stream, uint64_t value, const char* desc) {
  WriteS64Leb128(stream, Bitcast<int64_t>(value), desc);
}

#undef LEB128_LOOP_UNTIL

}  // namespace wabt
