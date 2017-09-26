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

#ifndef WABT_LEB128_H_
#define WABT_LEB128_H_

#include <cstdint>

#include "src/common.h"

namespace wabt {

class Stream;

// Returns the length of the leb128.
Offset U32Leb128Length(uint32_t value);

void WriteU32Leb128(Stream* stream, uint32_t value, const char* desc);
void WriteS32Leb128(Stream* stream, uint32_t value, const char* desc);
void WriteS64Leb128(Stream* stream, uint64_t value, const char* desc);
void WriteFixedU32Leb128(Stream* stream, uint32_t value, const char* desc);

Offset WriteU32Leb128At(Stream* stream,
                        Offset offset,
                        uint32_t value,
                        const char* desc);

Offset WriteFixedU32Leb128At(Stream* stream,
                             Offset offset,
                             uint32_t value,
                             const char* desc);

Offset WriteFixedU32Leb128Raw(uint8_t* data, uint8_t* end, uint32_t value);

// Convenience functions for writing enums as LEB128s.
template <typename T>
void WriteU32Leb128(Stream* stream, T value, const char* desc) {
  WriteU32Leb128(stream, static_cast<uint32_t>(value), desc);
}

template <typename T>
void WriteS32Leb128(Stream* stream, T value, const char* desc) {
  WriteS32Leb128(stream, static_cast<uint32_t>(value), desc);
}

}  // namespace wabt

#endif // WABT_LEB128_H_
