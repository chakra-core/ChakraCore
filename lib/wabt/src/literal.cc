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

#include "src/literal.h"

#include <cassert>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace wabt {

namespace {

int Clz(uint32_t value) {
  return value == 0 ? 32 : wabt_clz_u32(value);
}

int Clz(uint64_t value) {
  return value == 0 ? 64 : wabt_clz_u64(value);
}

template <typename T>
struct FloatTraitsBase {};

// The "PlusOne" values are used because normal IEEE floats have an implicit
// leading one, so they have an additional bit of precision.

template <>
struct FloatTraitsBase<float> {
  typedef uint32_t Uint;
  static constexpr int kBits = sizeof(Uint) * 8;
  static constexpr int kSigBits = 23;
  static constexpr float kHugeVal = HUGE_VALF;
  static constexpr int kMaxHexBufferSize = WABT_MAX_FLOAT_HEX;

  static float Strto(const char* s, char** endptr) { return strtof(s, endptr); }
};

template <>
struct FloatTraitsBase<double> {
  typedef uint64_t Uint;
  static constexpr int kBits = sizeof(Uint) * 8;
  static constexpr int kSigBits = 52;
  static constexpr float kHugeVal = HUGE_VAL;
  static constexpr int kMaxHexBufferSize = WABT_MAX_DOUBLE_HEX;

  static double Strto(const char* s, char** endptr) {
    return strtod(s, endptr);
  }
};

template <typename T>
struct FloatTraits : FloatTraitsBase<T> {
  typedef typename FloatTraitsBase<T>::Uint Uint;
  using FloatTraitsBase<T>::kBits;
  using FloatTraitsBase<T>::kSigBits;

  static constexpr int kExpBits = kBits - kSigBits - 1;
  static constexpr int kSignShift = kBits - 1;
  static constexpr Uint kSigMask = (Uint(1) << kSigBits) - 1;
  static constexpr int kSigPlusOneBits = kSigBits + 1;
  static constexpr Uint kSigPlusOneMask = (Uint(1) << kSigPlusOneBits) - 1;
  static constexpr int kExpMask = (1 << kExpBits) - 1;
  static constexpr int kMaxExp = 1 << (kExpBits - 1);
  static constexpr int kMinExp = -kMaxExp + 1;
  static constexpr int kExpBias = -kMinExp;
  static constexpr Uint kQuietNanTag = Uint(1) << (kSigBits - 1);
};

template <typename T>
class FloatParser {
 public:
  typedef FloatTraits<T> Traits;
  typedef typename Traits::Uint Uint;
  typedef T Float;

  static Result Parse(LiteralType,
                      const char* s,
                      const char* end,
                      Uint* out_bits);

 private:
  static bool StringStartsWith(const char* start,
                               const char* end,
                               const char* prefix);
  static Uint Make(bool sign, int exp, Uint sig);
  static Uint ShiftAndRoundToNearest(Uint significand, int shift);

  static Result ParseFloat(const char* s, const char* end, Uint* out_bits);
  static Result ParseNan(const char* s, const char* end, Uint* out_bits);
  static Result ParseHex(const char* s, const char* end, Uint* out_bits);
  static void ParseInfinity(const char* s, const char* end, Uint* out_bits);
};

template <typename T>
class FloatWriter {
 public:
  typedef FloatTraits<T> Traits;
  typedef typename Traits::Uint Uint;

  static void WriteHex(char* out, size_t size, Uint bits);
};

// Return 1 if the non-NULL-terminated string starting with |start| and ending
// with |end| starts with the NULL-terminated string |prefix|.
template <typename T>
// static
bool FloatParser<T>::StringStartsWith(const char* start,
                                      const char* end,
                                      const char* prefix) {
  while (start < end && *prefix) {
    if (*start != *prefix)
      return false;
    start++;
    prefix++;
  }
  return *prefix == 0;
}

// static
template <typename T>
Result FloatParser<T>::ParseFloat(const char* s,
                                  const char* end,
                                  Uint* out_bits) {
  // Here is the normal behavior for strtof/strtod:
  //
  // input     | errno  |   output   |
  // ---------------------------------
  // overflow  | ERANGE | +-HUGE_VAL |
  // underflow | ERANGE |        0.0 |
  // otherwise |      0 |      value |
  //
  // So normally we need to clear errno before calling strto{f,d}, and check
  // afterward whether it was set to ERANGE.
  //
  // glibc seems to have a bug where
  // strtof("340282356779733661637539395458142568448") will return HUGE_VAL,
  // but will not set errno to ERANGE. Since this function is only called when
  // we know that we have parsed a "normal" number (i.e. not "inf"), we know
  // that if we ever get HUGE_VAL, it must be overflow.
  //
  // The WebAssembly spec also ignores underflow, so we don't need to check for
  // ERANGE at all.

  // WebAssembly floats can contain underscores, but strto* can't parse those,
  // so remove them first.
  assert(s <= end);
  const size_t kBufferSize = end - s + 1;  // +1 for \0.
  char* buffer = static_cast<char*>(alloca(kBufferSize));
  auto buffer_end =
      std::copy_if(s, end, buffer, [](char c) -> bool { return c != '_'; });
  assert(buffer_end < buffer + kBufferSize);
  *buffer_end = 0;

  char* endptr;
  Float value = Traits::Strto(buffer, &endptr);
  if (endptr != buffer_end ||
      (value == Traits::kHugeVal || value == -Traits::kHugeVal)) {
    return Result::Error;
  }

  memcpy(out_bits, &value, sizeof(value));
  return Result::Ok;
}

// static
template <typename T>
typename FloatParser<T>::Uint FloatParser<T>::Make(bool sign,
                                                   int exp,
                                                   Uint sig) {
  assert(exp >= Traits::kMinExp && exp <= Traits::kMaxExp);
  assert(sig <= Traits::kSigMask);
  return (Uint(sign) << Traits::kSignShift) |
         (Uint(exp + Traits::kExpBias) << Traits::kSigBits) | sig;
}

// static
template <typename T>
typename FloatParser<T>::Uint FloatParser<T>::ShiftAndRoundToNearest(
    Uint significand,
    int shift) {
  assert(shift > 0);
  // Round ties to even.
  if (significand & (Uint(1) << shift))
    significand += Uint(1) << (shift - 1);
  significand >>= shift;
  return significand;
}

// static
template <typename T>
Result FloatParser<T>::ParseNan(const char* s,
                                const char* end,
                                Uint* out_bits) {
  bool is_neg = false;
  if (*s == '-') {
    is_neg = true;
    s++;
  } else if (*s == '+') {
    s++;
  }
  assert(StringStartsWith(s, end, "nan"));
  s += 3;

  Uint tag;
  if (s != end) {
    tag = 0;
    assert(StringStartsWith(s, end, ":0x"));
    s += 3;

    for (; s < end; ++s) {
      uint32_t digit;
      CHECK_RESULT(ParseHexdigit(*s, &digit));
      tag = tag * 16 + digit;
      // Check for overflow.
      if (tag > Traits::kSigMask)
        return Result::Error;
    }

    // NaN cannot have a zero tag, that is reserved for infinity.
    if (tag == 0)
      return Result::Error;
  } else {
    tag = Traits::kQuietNanTag;
  }

  *out_bits = Make(is_neg, Traits::kMaxExp, tag);
  return Result::Ok;
}

// static
template <typename T>
Result FloatParser<T>::ParseHex(const char* s,
                                const char* end,
                                Uint* out_bits) {
  bool is_neg = false;
  if (*s == '-') {
    is_neg = true;
    s++;
  } else if (*s == '+') {
    s++;
  }
  assert(StringStartsWith(s, end, "0x"));
  s += 2;

  // Loop over the significand; everything up to the 'p'.
  // This code is a bit nasty because we want to support extra zeroes anywhere
  // without having to use many significand bits.
  // e.g.
  // 0x00000001.0p0 => significand = 1, significand_exponent = 0
  // 0x10000000.0p0 => significand = 1, significand_exponent = 28
  // 0x0.000001p0 => significand = 1, significand_exponent = -24
  bool seen_dot = false;
  Uint significand = 0;
  int significand_exponent = 0;  // Exponent adjustment due to dot placement.
  for (; s < end; ++s) {
    uint32_t digit;
    if (*s == '_') {
      continue;
    } else if (*s == '.') {
      seen_dot = true;
    } else if (Succeeded(ParseHexdigit(*s, &digit))) {
      if (Traits::kBits - Clz(significand) <= Traits::kSigPlusOneBits) {
        significand = (significand << 4) + digit;
        if (seen_dot)
          significand_exponent -= 4;
      } else if (!seen_dot) {
        significand_exponent += 4;
      }
    } else {
      break;
    }
  }

  if (significand == 0) {
    // 0 or -0.
    *out_bits = Make(is_neg, Traits::kMinExp, 0);
    return Result::Ok;
  }

  int exponent = 0;
  bool exponent_is_neg = false;
  if (s < end) {
    assert(*s == 'p' || *s == 'P');
    s++;
    // Exponent is always positive, but significand_exponent is signed.
    // significand_exponent_add is negated if exponent will be negative, so it
    // can be easily summed to see if the exponent is too large (see below).
    int significand_exponent_add = 0;
    if (*s == '-') {
      exponent_is_neg = true;
      significand_exponent_add = -significand_exponent;
      s++;
    } else if (*s == '+') {
      s++;
      significand_exponent_add = significand_exponent;
    }

    for (; s < end; ++s) {
      if (*s == '_')
        continue;

      uint32_t digit = (*s - '0');
      assert(digit <= 9);
      exponent = exponent * 10 + digit;
      if (exponent + significand_exponent_add >= Traits::kMaxExp)
        break;
    }
  }

  if (exponent_is_neg)
    exponent = -exponent;

  int significand_bits = Traits::kBits - Clz(significand);
  // -1 for the implicit 1 bit of the significand.
  exponent += significand_exponent + significand_bits - 1;

  if (exponent <= Traits::kMinExp) {
    // Maybe subnormal.
    if (significand_bits > Traits::kSigBits) {
      significand = ShiftAndRoundToNearest(significand,
                                           significand_bits - Traits::kSigBits);
    } else if (significand_bits < Traits::kSigBits) {
      significand <<= (Traits::kSigBits - significand_bits);
    }

    int shift = Traits::kMinExp - exponent;
    if (shift < Traits::kSigBits) {
      if (shift) {
        significand =
            ShiftAndRoundToNearest(significand, shift) & Traits::kSigMask;
      }
      exponent = Traits::kMinExp;

      if (significand != 0) {
        *out_bits = Make(is_neg, exponent, significand);
        return Result::Ok;
      }
    }

    // Not subnormal, too small; return 0 or -0.
    *out_bits = Make(is_neg, Traits::kMinExp, 0);
  } else {
    // Maybe Normal value.
    if (significand_bits > Traits::kSigPlusOneBits) {
      significand = ShiftAndRoundToNearest(
          significand, significand_bits - Traits::kSigPlusOneBits);
      if (significand > Traits::kSigPlusOneMask)
        exponent++;
    } else if (significand_bits < Traits::kSigPlusOneBits) {
      significand <<= (Traits::kSigPlusOneBits - significand_bits);
    }

    if (exponent >= Traits::kMaxExp) {
      // Would be inf or -inf, but the spec doesn't allow rounding hex-floats to
      // infinity.
      return Result::Error;
    }

    *out_bits = Make(is_neg, exponent, significand & Traits::kSigMask);
  }

  return Result::Ok;
}

// static
template <typename T>
void FloatParser<T>::ParseInfinity(const char* s,
                                   const char* end,
                                   Uint* out_bits) {
  bool is_neg = false;
  if (*s == '-') {
    is_neg = true;
    s++;
  } else if (*s == '+') {
    s++;
  }
  assert(StringStartsWith(s, end, "inf"));
  *out_bits = Make(is_neg, Traits::kMaxExp, 0);
}

// static
template <typename T>
Result FloatParser<T>::Parse(LiteralType literal_type,
                             const char* s,
                             const char* end,
                             Uint* out_bits) {
#if COMPILER_IS_MSVC
  if (literal_type == LiteralType::Int && StringStartsWith(s, end, "0x")) {
    // Some MSVC crt implementation of strtof doesn't support hex strings
    literal_type = LiteralType::Hexfloat;
  }
#endif
  switch (literal_type) {
    case LiteralType::Int:
    case LiteralType::Float:
      return ParseFloat(s, end, out_bits);

    case LiteralType::Hexfloat:
      return ParseHex(s, end, out_bits);

    case LiteralType::Infinity:
      ParseInfinity(s, end, out_bits);
      return Result::Ok;

    case LiteralType::Nan:
      return ParseNan(s, end, out_bits);
  }

  WABT_UNREACHABLE;
}

// static
template <typename T>
void FloatWriter<T>::WriteHex(char* out, size_t size, Uint bits) {
  static constexpr int kNumNybbles = Traits::kBits / 4;
  static constexpr int kTopNybbleShift = Traits::kBits - 4;
  static constexpr Uint kTopNybble = Uint(0xf) << kTopNybbleShift;
  static const char s_hex_digits[] = "0123456789abcdef";

  char buffer[Traits::kMaxHexBufferSize];
  char* p = buffer;
  bool is_neg = (bits >> Traits::kSignShift);
  int exp = ((bits >> Traits::kSigBits) & Traits::kExpMask) - Traits::kExpBias;
  Uint sig = bits & Traits::kSigMask;

  if (is_neg)
    *p++ = '-';
  if (exp == Traits::kMaxExp) {
    // Infinity or nan.
    if (sig == 0) {
      strcpy(p, "inf");
      p += 3;
    } else {
      strcpy(p, "nan");
      p += 3;
      if (sig != Traits::kQuietNanTag) {
        strcpy(p, ":0x");
        p += 3;
        // Skip leading zeroes.
        int num_nybbles = kNumNybbles;
        while ((sig & kTopNybble) == 0) {
          sig <<= 4;
          num_nybbles--;
        }
        while (num_nybbles) {
          Uint nybble = (sig >> kTopNybbleShift) & 0xf;
          *p++ = s_hex_digits[nybble];
          sig <<= 4;
          --num_nybbles;
        }
      }
    }
  } else {
    bool is_zero = sig == 0 && exp == Traits::kMinExp;
    strcpy(p, "0x");
    p += 2;
    *p++ = is_zero ? '0' : '1';

    // Shift sig up so the top 4-bits are at the top of the Uint.
    sig <<= Traits::kBits - Traits::kSigBits;

    if (sig) {
      if (exp == Traits::kMinExp) {
        // Subnormal; shift the significand up, and shift out the implicit 1.
        Uint leading_zeroes = Clz(sig);
        if (leading_zeroes < Traits::kSignShift)
          sig <<= leading_zeroes + 1;
        else
          sig = 0;
        exp -= leading_zeroes;
      }

      *p++ = '.';
      while (sig) {
        int nybble = (sig >> kTopNybbleShift) & 0xf;
        *p++ = s_hex_digits[nybble];
        sig <<= 4;
      }
    }
    *p++ = 'p';
    if (is_zero) {
      strcpy(p, "+0");
      p += 2;
    } else {
      if (exp < 0) {
        *p++ = '-';
        exp = -exp;
      } else {
        *p++ = '+';
      }
      if (exp >= 1000)
        *p++ = '1';
      if (exp >= 100)
        *p++ = '0' + (exp / 100) % 10;
      if (exp >= 10)
        *p++ = '0' + (exp / 10) % 10;
      *p++ = '0' + exp % 10;
    }
  }

  size_t len = p - buffer;
  if (len >= size)
    len = size - 1;
  memcpy(out, buffer, len);
  out[len] = '\0';
}

}  // end anonymous namespace

Result ParseHexdigit(char c, uint32_t* out) {
  if (static_cast<unsigned int>(c - '0') <= 9) {
    *out = c - '0';
    return Result::Ok;
  } else if (static_cast<unsigned int>(c - 'a') <= 6) {
    *out = 10 + (c - 'a');
    return Result::Ok;
  } else if (static_cast<unsigned int>(c - 'A') <= 6) {
    *out = 10 + (c - 'A');
    return Result::Ok;
  }
  return Result::Error;
}

Result ParseUint64(const char* s, const char* end, uint64_t* out) {
  if (s == end)
    return Result::Error;
  uint64_t value = 0;
  if (*s == '0' && s + 1 < end && s[1] == 'x') {
    s += 2;
    if (s == end)
      return Result::Error;
    for (; s < end; ++s) {
      uint32_t digit;
      if (*s == '_')
        continue;
      CHECK_RESULT(ParseHexdigit(*s, &digit));
      uint64_t old_value = value;
      value = value * 16 + digit;
      // Check for overflow.
      if (old_value > value)
        return Result::Error;
    }
  } else {
    for (; s < end; ++s) {
      if (*s == '_')
        continue;
      uint32_t digit = (*s - '0');
      if (digit > 9)
        return Result::Error;
      uint64_t old_value = value;
      value = value * 10 + digit;
      // Check for overflow.
      if (old_value > value)
        return Result::Error;
    }
  }
  if (s != end)
    return Result::Error;
  *out = value;
  return Result::Ok;
}

Result ParseInt64(const char* s,
                  const char* end,
                  uint64_t* out,
                  ParseIntType parse_type) {
  bool has_sign = false;
  if (*s == '-' || *s == '+') {
    if (parse_type == ParseIntType::UnsignedOnly)
      return Result::Error;
    if (*s == '-')
      has_sign = true;
    s++;
  }
  uint64_t value = 0;
  Result result = ParseUint64(s, end, &value);
  if (has_sign) {
    // abs(INT64_MIN) == INT64_MAX + 1.
    if (value > static_cast<uint64_t>(INT64_MAX) + 1)
      return Result::Error;
    value = UINT64_MAX - value + 1;
  }
  *out = value;
  return result;
}

Result ParseInt32(const char* s,
                  const char* end,
                  uint32_t* out,
                  ParseIntType parse_type) {
  uint64_t value;
  bool has_sign = false;
  if (*s == '-' || *s == '+') {
    if (parse_type == ParseIntType::UnsignedOnly)
      return Result::Error;
    if (*s == '-')
      has_sign = true;
    s++;
  }
  CHECK_RESULT(ParseUint64(s, end, &value));

  if (has_sign) {
    // abs(INT32_MIN) == INT32_MAX + 1.
    if (value > static_cast<uint64_t>(INT32_MAX) + 1)
      return Result::Error;
    value = UINT32_MAX - value + 1;
  } else {
    if (value > static_cast<uint64_t>(UINT32_MAX))
      return Result::Error;
  }
  *out = static_cast<uint32_t>(value);
  return Result::Ok;
}

Result ParseFloat(LiteralType literal_type,
                  const char* s,
                  const char* end,
                  uint32_t* out_bits) {
  return FloatParser<float>::Parse(literal_type, s, end, out_bits);
}

Result ParseDouble(LiteralType literal_type,
                   const char* s,
                   const char* end,
                   uint64_t* out_bits) {
  return FloatParser<double>::Parse(literal_type, s, end, out_bits);
}

void WriteFloatHex(char* buffer, size_t size, uint32_t bits) {
  return FloatWriter<float>::WriteHex(buffer, size, bits);
}

void WriteDoubleHex(char* buffer, size_t size, uint64_t bits) {
  return FloatWriter<double>::WriteHex(buffer, size, bits);
}

}  // namespace wabt
