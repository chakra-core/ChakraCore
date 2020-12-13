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

#include <type_traits>

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

Offset WriteU32Leb128Raw(uint8_t* dest, uint8_t* dest_end, uint32_t value) {
  uint8_t data[MAX_U32_LEB128_BYTES];
  Offset length = 0;
  LEB128_LOOP_UNTIL(value == 0);
  if (static_cast<Offset>(dest_end - dest) < length) {
    return 0;
  }
  memcpy(dest, data, length);
  return length;
}

Offset WriteFixedU32Leb128Raw(uint8_t* data, uint8_t* end, uint32_t value) {
  if (end - data < MAX_U32_LEB128_BYTES) {
    return 0;
  }
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
  if (value < 0) {
    LEB128_LOOP_UNTIL(value == -1 && (byte & 0x40));
  } else {
    LEB128_LOOP_UNTIL(value == 0 && !(byte & 0x40));
  }

  stream->WriteData(data, length, desc);
}

static void WriteS64Leb128(Stream* stream, int64_t value, const char* desc) {
  uint8_t data[MAX_U64_LEB128_BYTES];
  Offset length = 0;
  if (value < 0) {
    LEB128_LOOP_UNTIL(value == -1 && (byte & 0x40));
  } else {
    LEB128_LOOP_UNTIL(value == 0 && !(byte & 0x40));
  }

  stream->WriteData(data, length, desc);
}

void WriteS32Leb128(Stream* stream, uint32_t value, const char* desc) {
  WriteS32Leb128(stream, Bitcast<int32_t>(value), desc);
}

void WriteS64Leb128(Stream* stream, uint64_t value, const char* desc) {
  WriteS64Leb128(stream, Bitcast<int64_t>(value), desc);
}

#undef LEB128_LOOP_UNTIL

#define BYTE_AT(type, i, shift) ((static_cast<type>(p[i]) & 0x7f) << (shift))

#define LEB128_1(type) (BYTE_AT(type, 0, 0))
#define LEB128_2(type) (BYTE_AT(type, 1, 7) | LEB128_1(type))
#define LEB128_3(type) (BYTE_AT(type, 2, 14) | LEB128_2(type))
#define LEB128_4(type) (BYTE_AT(type, 3, 21) | LEB128_3(type))
#define LEB128_5(type) (BYTE_AT(type, 4, 28) | LEB128_4(type))
#define LEB128_6(type) (BYTE_AT(type, 5, 35) | LEB128_5(type))
#define LEB128_7(type) (BYTE_AT(type, 6, 42) | LEB128_6(type))
#define LEB128_8(type) (BYTE_AT(type, 7, 49) | LEB128_7(type))
#define LEB128_9(type) (BYTE_AT(type, 8, 56) | LEB128_8(type))
#define LEB128_10(type) (BYTE_AT(type, 9, 63) | LEB128_9(type))

#define SHIFT_AMOUNT(type, sign_bit) (sizeof(type) * 8 - 1 - (sign_bit))
#define SIGN_EXTEND(type, value, sign_bit)                       \
  (static_cast<type>((value) << SHIFT_AMOUNT(type, sign_bit)) >> \
   SHIFT_AMOUNT(type, sign_bit))

size_t ReadU32Leb128(const uint8_t* p,
                     const uint8_t* end,
                     uint32_t* out_value) {
  if (p < end && (p[0] & 0x80) == 0) {
    *out_value = LEB128_1(uint32_t);
    return 1;
  } else if (p + 1 < end && (p[1] & 0x80) == 0) {
    *out_value = LEB128_2(uint32_t);
    return 2;
  } else if (p + 2 < end && (p[2] & 0x80) == 0) {
    *out_value = LEB128_3(uint32_t);
    return 3;
  } else if (p + 3 < end && (p[3] & 0x80) == 0) {
    *out_value = LEB128_4(uint32_t);
    return 4;
  } else if (p + 4 < end && (p[4] & 0x80) == 0) {
    // The top bits set represent values > 32 bits.
    if (p[4] & 0xf0) {
      return 0;
    }
    *out_value = LEB128_5(uint32_t);
    return 5;
  } else {
    // past the end.
    *out_value = 0;
    return 0;
  }
}

size_t ReadS32Leb128(const uint8_t* p,
                     const uint8_t* end,
                     uint32_t* out_value) {
  if (p < end && (p[0] & 0x80) == 0) {
    uint32_t result = LEB128_1(uint32_t);
    *out_value = SIGN_EXTEND(int32_t, result, 6);
    return 1;
  } else if (p + 1 < end && (p[1] & 0x80) == 0) {
    uint32_t result = LEB128_2(uint32_t);
    *out_value = SIGN_EXTEND(int32_t, result, 13);
    return 2;
  } else if (p + 2 < end && (p[2] & 0x80) == 0) {
    uint32_t result = LEB128_3(uint32_t);
    *out_value = SIGN_EXTEND(int32_t, result, 20);
    return 3;
  } else if (p + 3 < end && (p[3] & 0x80) == 0) {
    uint32_t result = LEB128_4(uint32_t);
    *out_value = SIGN_EXTEND(int32_t, result, 27);
    return 4;
  } else if (p + 4 < end && (p[4] & 0x80) == 0) {
    // The top bits should be a sign-extension of the sign bit.
    bool sign_bit_set = (p[4] & 0x8);
    int top_bits = p[4] & 0xf0;
    if ((sign_bit_set && top_bits != 0x70) ||
        (!sign_bit_set && top_bits != 0)) {
      return 0;
    }
    uint32_t result = LEB128_5(uint32_t);
    *out_value = result;
    return 5;
  } else {
    // Past the end.
    return 0;
  }
}

size_t ReadS64Leb128(const uint8_t* p,
                     const uint8_t* end,
                     uint64_t* out_value) {
  if (p < end && (p[0] & 0x80) == 0) {
    uint64_t result = LEB128_1(uint64_t);
    *out_value = SIGN_EXTEND(int64_t, result, 6);
    return 1;
  } else if (p + 1 < end && (p[1] & 0x80) == 0) {
    uint64_t result = LEB128_2(uint64_t);
    *out_value = SIGN_EXTEND(int64_t, result, 13);
    return 2;
  } else if (p + 2 < end && (p[2] & 0x80) == 0) {
    uint64_t result = LEB128_3(uint64_t);
    *out_value = SIGN_EXTEND(int64_t, result, 20);
    return 3;
  } else if (p + 3 < end && (p[3] & 0x80) == 0) {
    uint64_t result = LEB128_4(uint64_t);
    *out_value = SIGN_EXTEND(int64_t, result, 27);
    return 4;
  } else if (p + 4 < end && (p[4] & 0x80) == 0) {
    uint64_t result = LEB128_5(uint64_t);
    *out_value = SIGN_EXTEND(int64_t, result, 34);
    return 5;
  } else if (p + 5 < end && (p[5] & 0x80) == 0) {
    uint64_t result = LEB128_6(uint64_t);
    *out_value = SIGN_EXTEND(int64_t, result, 41);
    return 6;
  } else if (p + 6 < end && (p[6] & 0x80) == 0) {
    uint64_t result = LEB128_7(uint64_t);
    *out_value = SIGN_EXTEND(int64_t, result, 48);
    return 7;
  } else if (p + 7 < end && (p[7] & 0x80) == 0) {
    uint64_t result = LEB128_8(uint64_t);
    *out_value = SIGN_EXTEND(int64_t, result, 55);
    return 8;
  } else if (p + 8 < end && (p[8] & 0x80) == 0) {
    uint64_t result = LEB128_9(uint64_t);
    *out_value = SIGN_EXTEND(int64_t, result, 62);
    return 9;
  } else if (p + 9 < end && (p[9] & 0x80) == 0) {
    // The top bits should be a sign-extension of the sign bit.
    bool sign_bit_set = (p[9] & 0x1);
    int top_bits = p[9] & 0xfe;
    if ((sign_bit_set && top_bits != 0x7e) ||
        (!sign_bit_set && top_bits != 0)) {
      return 0;
    }
    uint64_t result = LEB128_10(uint64_t);
    *out_value = result;
    return 10;
  } else {
    // Past the end.
    return 0;
  }
}

#undef BYTE_AT
#undef LEB128_1
#undef LEB128_2
#undef LEB128_3
#undef LEB128_4
#undef LEB128_5
#undef LEB128_6
#undef LEB128_7
#undef LEB128_8
#undef LEB128_9
#undef LEB128_10
#undef SHIFT_AMOUNT
#undef SIGN_EXTEND

}  // namespace wabt
