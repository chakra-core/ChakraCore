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

#include "src/common.h"

namespace wabt {

class Features;

struct Opcode {
  // Opcode enumerations.
  //
  // NOTE: this enum does not match the binary encoding.
  //
  enum Enum : uint32_t {
#define WABT_OPCODE(rtype, type1, type2, type3, mem_size, prefix, code, Name, \
                    text)                                                     \
  Name,
#include "src/opcode.def"
#undef WABT_OPCODE
    Invalid,
  };

  // Static opcode objects.
#define WABT_OPCODE(rtype, type1, type2, type3, mem_size, prefix, code, Name, \
                    text)                                                     \
  static Opcode Name##_Opcode;
#include "src/opcode.def"
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
  Type GetParamType3() const { return GetInfo().param3_type; }
  Address GetMemorySize() const { return GetInfo().memory_size; }

  // Return 1 if |alignment| matches the alignment of |opcode|, or if
  // |alignment| is WABT_USE_NATURAL_ALIGNMENT.
  bool IsNaturallyAligned(Address alignment) const;

  // If |alignment| is WABT_USE_NATURAL_ALIGNMENT, return the alignment of
  // |opcode|, else return |alignment|.
  Address GetAlignment(Address alignment) const;

  static bool IsPrefixByte(uint8_t byte) {
    return byte == kMathPrefix || byte == kThreadsPrefix;
  }

  bool IsEnabled(const Features& features) const;

 private:
  static const uint32_t kMathPrefix = 0xfc;
  static const uint32_t kThreadsPrefix = 0xfe;

  struct Info {
    const char* name;
    Type result_type;
    Type param1_type;
    Type param2_type;
    Type param3_type;
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

  // The Opcode struct only stores an enumeration (Opcode::Enum) of all valid
  // opcodes, densely packed. We want to be able to store invalid opcodes as
  // well, for display to the user. To encode these, we use PrefixCode() to
  // generate a uint32_t of the prefix/code pair, then negate the value so it
  // doesn't overlap with the valid enum values. The negation is done using
  // `~code + 1` since prefix_code is unsigned, and MSVC warns if you use - on
  // an unsigned value.
  //
  // | 0             | Opcode::Invalid         | INT32_MAX+1    UINT32_MAX |
  // |---------------|-------------------------|---------------------------|
  // | valid opcodes |      unused space       |      invalid opcodes      |
  //
  static Enum EncodeInvalidOpcode(uint32_t prefix_code) {
    Enum result = static_cast<Enum>(~prefix_code + 1);
    assert(result >= Invalid);
    return result;
  }

  static void DecodeInvalidOpcode(Enum e,
                                  uint8_t* out_prefix,
                                  uint32_t* out_code) {
    uint32_t prefix_code = ~static_cast<uint32_t>(e) + 1;
    *out_prefix = prefix_code >> 8;
    *out_code = prefix_code & 0xff;
  }

  Info GetInfo() const;
  static Info infos_[];

  Enum enum_;
};

}  // end anonymous namespace

#endif  // WABT_OPCODE_H_
