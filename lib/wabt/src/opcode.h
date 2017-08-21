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

#ifndef WABT_OPCODE_H_
#define WABT_OPCODE_H_

#include "common.h"

namespace wabt {

class Features;

struct Opcode {
  // Opcode enumerations.
  //
  // NOTE: this enum does not match the binary encoding.
  //
  enum Enum {
#define WABT_OPCODE(rtype, type1, type2, mem_size, prefix, code, Name, text) \
  Name,
#include "opcode.def"
#undef WABT_OPCODE
    Invalid,
  };

  // Static opcode objects.
#define WABT_OPCODE(rtype, type1, type2, mem_size, prefix, code, Name, text) \
  static Opcode Name##_Opcode;
#include "opcode.def"
#undef WABT_OPCODE

  Opcode() = default;  // Provided so Opcode can be member of a union.
  Opcode(Enum e) : enum_(e) {}
  operator Enum() const { return enum_; }

  static Opcode FromCode(uint32_t);
  static Opcode FromCode(uint8_t prefix, uint32_t code);
  bool HasPrefix() const { return GetInfo().prefix != 0; }
  uint8_t GetPrefix() const { return GetInfo().prefix; }
  uint32_t GetCode() const { return GetInfo().code; }
  size_t GetLength() const { return HasPrefix() ? 2 : 1; }
  const char* GetName() const { return GetInfo().name; }
  Type GetResultType() const { return GetInfo().result_type; }
  Type GetParamType1() const { return GetInfo().param1_type; }
  Type GetParamType2() const { return GetInfo().param2_type; }
  Address GetMemorySize() const { return GetInfo().memory_size; }

  // Return 1 if |alignment| matches the alignment of |opcode|, or if
  // |alignment| is WABT_USE_NATURAL_ALIGNMENT.
  bool IsNaturallyAligned(Address alignment) const;

  // If |alignment| is WABT_USE_NATURAL_ALIGNMENT, return the alignment of
  // |opcode|, else return |alignment|.
  Address GetAlignment(Address alignment) const;

  static bool IsPrefixByte(uint8_t byte) { return byte >= kFirstPrefix; }

  bool IsEnabled(const Features& features) const;

 private:
  static const uint32_t kFirstPrefix = 0xfc;

  struct Info {
    const char* name;
    Type result_type;
    Type param1_type;
    Type param2_type;
    Address memory_size;
    uint8_t prefix;
    uint32_t code;
    uint32_t prefix_code;  // See PrefixCode below. Used for fast lookup.
  };

  static uint32_t PrefixCode(uint8_t prefix, uint32_t code) {
    // For now, 8 bits is enough for all codes.
    assert(code < 0x100);
    return (prefix << 8) | code;
  }

  Info GetInfo() const;
  static Info infos_[];
  static Info invalid_info_;

  Enum enum_;
};

}  // end anonymous namespace

#endif  // WABT_OPCODE_H_
