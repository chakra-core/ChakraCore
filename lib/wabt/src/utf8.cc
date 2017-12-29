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

#include "src/utf8.h"

#include <cstdint>

namespace wabt {

namespace {

const int s_utf8_length[256] = {
 // 0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x00
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x10
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x20
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x30
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x40
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x50
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x60
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 0x70
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x80
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x90
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xa0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xb0
    0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  // 0xc0
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  // 0xd0
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  // 0xe0
    4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xf0
};

// Returns true if this is a valid continuation byte.
bool IsCont(uint8_t c) {
  return (c & 0xc0) == 0x80;
}

}  // end anonymous namespace

bool IsValidUtf8(const char* s, size_t s_length) {
  const uint8_t* p = reinterpret_cast<const uint8_t*>(s);
  const uint8_t* end = p + s_length;
  while (p < end) {
    uint8_t cu0 = *p;
    int length = s_utf8_length[cu0];
    if (p + length > end) {
      return false;
    }

    switch (length) {
      case 0:
        return false;

      case 1:
        p++;
        break;

      case 2:
        p++;
        if (!IsCont(*p++)) {
          return false;
        }
        break;

      case 3: {
        p++;
        uint8_t cu1 = *p++;
        uint8_t cu2 = *p++;
        if (!(IsCont(cu1) && IsCont(cu2)) ||
            (cu0 == 0xe0 && cu1 < 0xa0) ||  // Overlong encoding.
            (cu0 == 0xed && cu1 >= 0xa0))   // UTF-16 surrogate halves.
          return false;
        break;
      }

      case 4: {
        p++;
        uint8_t cu1 = *p++;
        uint8_t cu2 = *p++;
        uint8_t cu3 = *p++;
        if (!(IsCont(cu1) && IsCont(cu2) && IsCont(cu3)) ||
            (cu0 == 0xf0 && cu1 < 0x90) ||  // Overlong encoding.
            (cu0 == 0xf4 && cu1 >= 0x90))   // Code point >= 0x11000.
          return false;
        break;
      }
    }
  }
  return true;
}

}  // namespace wabt
