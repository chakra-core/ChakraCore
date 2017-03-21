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

#include "literal.h"

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define HEX_DIGIT_BITS 4

/* The PLUS_ONE values are used because normal IEEE floats have an implicit
 * leading one, so they have an additional bit of precision. */

#define F32_SIGN_SHIFT 31
#define F32_SIG_BITS 23
#define F32_SIG_MASK 0x7fffff
#define F32_SIG_PLUS_ONE_BITS 24
#define F32_SIG_PLUS_ONE_MASK 0xffffff
#define F32_EXP_MASK 0xff
#define F32_MIN_EXP -127
#define F32_MAX_EXP 128
#define F32_EXP_BIAS 127
#define F32_QUIET_NAN_TAG 0x400000

#define F64_SIGN_SHIFT 63
#define F64_SIG_BITS 52
#define F64_SIG_MASK 0xfffffffffffffULL
#define F64_SIG_PLUS_ONE_BITS 53
#define F64_SIG_PLUS_ONE_MASK 0x1fffffffffffffULL
#define F64_EXP_MASK 0x7ff
#define F64_MIN_EXP -1023
#define F64_MAX_EXP 1024
#define F64_EXP_BIAS 1023
#define F64_QUIET_NAN_TAG 0x8000000000000ULL

namespace wabt {

static const char s_hex_digits[] = "0123456789abcdef";

Result parse_hexdigit(char c, uint32_t* out) {
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

/* return 1 if the non-NULL-terminated string starting with |start| and ending
 with |end| starts with the NULL-terminated string |prefix|. */
static bool string_starts_with(const char* start,
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

Result parse_uint64(const char* s, const char* end, uint64_t* out) {
  if (s == end)
    return Result::Error;
  uint64_t value = 0;
  if (*s == '0' && s + 1 < end && s[1] == 'x') {
    s += 2;
    if (s == end)
      return Result::Error;
    for (; s < end; ++s) {
      uint32_t digit;
      if (WABT_FAILED(parse_hexdigit(*s, &digit)))
        return Result::Error;
      uint64_t old_value = value;
      value = value * 16 + digit;
      /* check for overflow */
      if (old_value > value)
        return Result::Error;
    }
  } else {
    for (; s < end; ++s) {
      uint32_t digit = (*s - '0');
      if (digit > 9)
        return Result::Error;
      uint64_t old_value = value;
      value = value * 10 + digit;
      /* check for overflow */
      if (old_value > value)
        return Result::Error;
    }
  }
  if (s != end)
    return Result::Error;
  *out = value;
  return Result::Ok;
}

Result parse_int64(const char* s,
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
  Result result = parse_uint64(s, end, &value);
  if (has_sign) {
    /* abs(INT64_MIN) == INT64_MAX + 1 */
    if (value > static_cast<uint64_t>(INT64_MAX) + 1)
      return Result::Error;
    value = UINT64_MAX - value + 1;
  }
  *out = value;
  return result;
}

Result parse_int32(const char* s,
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
  if (WABT_FAILED(parse_uint64(s, end, &value)))
    return Result::Error;

  if (has_sign) {
    /* abs(INT32_MIN) == INT32_MAX + 1 */
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

/* floats */
static uint32_t make_float(bool sign, int exp, uint32_t sig) {
  assert(exp >= F32_MIN_EXP && exp <= F32_MAX_EXP);
  assert(sig <= F32_SIG_MASK);
  return (static_cast<uint32_t>(sign) << F32_SIGN_SHIFT) |
         (static_cast<uint32_t>(exp + F32_EXP_BIAS) << F32_SIG_BITS) | sig;
}

static uint32_t shift_float_and_round_to_nearest(uint32_t significand,
                                                 int shift) {
  assert(shift > 0);
  /* round ties to even */
  if (significand & (1U << shift))
    significand += 1U << (shift - 1);
  significand >>= shift;
  return significand;
}

static Result parse_float_nan(const char* s,
                              const char* end,
                              uint32_t* out_bits) {
  bool is_neg = false;
  if (*s == '-') {
    is_neg = true;
    s++;
  } else if (*s == '+') {
    s++;
  }
  assert(string_starts_with(s, end, "nan"));
  s += 3;

  uint32_t tag;
  if (s != end) {
    tag = 0;
    assert(string_starts_with(s, end, ":0x"));
    s += 3;

    for (; s < end; ++s) {
      uint32_t digit;
      if (WABT_FAILED(parse_hexdigit(*s, &digit)))
        return Result::Error;
      tag = tag * 16 + digit;
      /* check for overflow */
      if (tag > F32_SIG_MASK)
        return Result::Error;
    }

    /* NaN cannot have a zero tag, that is reserved for infinity */
    if (tag == 0)
      return Result::Error;
  } else {
    tag = F32_QUIET_NAN_TAG;
  }

  *out_bits = make_float(is_neg, F32_MAX_EXP, tag);
  return Result::Ok;
}

static void parse_float_hex(const char* s,
                            const char* end,
                            uint32_t* out_bits) {
  bool is_neg = false;
  if (*s == '-') {
    is_neg = true;
    s++;
  } else if (*s == '+') {
    s++;
  }
  assert(string_starts_with(s, end, "0x"));
  s += 2;

  /* loop over the significand; everything up to the 'p'.
   this code is a bit nasty because we want to support extra zeroes anywhere
   without having to use many significand bits.
   e.g.
   0x00000001.0p0 => significand = 1, significand_exponent = 0
   0x10000000.0p0 => significand = 1, significand_exponent = 28
   0x0.000001p0 => significand = 1, significand_exponent = -24
   */
  bool seen_dot = false;
  uint32_t significand = 0;
  /* how much to shift |significand| if a non-zero value is appended */
  int significand_shift = 0;
  int significand_bits = 0;     /* bits of |significand| */
  int significand_exponent = 0; /* exponent adjustment due to dot placement */
  for (; s < end; ++s) {
    uint32_t digit;
    if (*s == '.') {
      if (significand != 0)
        significand_exponent += significand_shift;
      significand_shift = 0;
      seen_dot = true;
      continue;
    } else if (WABT_FAILED(parse_hexdigit(*s, &digit))) {
      break;
    }
    significand_shift += HEX_DIGIT_BITS;
    if (digit != 0 && (significand == 0 ||
                       significand_bits + significand_shift <=
                           F32_SIG_BITS + 1 + HEX_DIGIT_BITS)) {
      if (significand != 0)
        significand <<= significand_shift;
      if (seen_dot)
        significand_exponent -= significand_shift;
      significand += digit;
      significand_shift = 0;
      significand_bits += HEX_DIGIT_BITS;
    }
  }

  if (!seen_dot)
    significand_exponent += significand_shift;

  if (significand == 0) {
    /* 0 or -0 */
    *out_bits = make_float(is_neg, F32_MIN_EXP, 0);
    return;
  }

  int exponent = 0;
  bool exponent_is_neg = false;
  if (s < end) {
    assert(*s == 'p');
    s++;
    /* exponent is always positive, but significand_exponent is signed.
     significand_exponent_add is negated if exponent will be negative, so it  can
     be easily summed to see if the exponent is too large (see below) */
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
      uint32_t digit = (*s - '0');
      assert(digit <= 9);
      exponent = exponent * 10 + digit;
      if (exponent + significand_exponent_add >= F32_MAX_EXP)
        break;
    }
  }

  if (exponent_is_neg)
    exponent = -exponent;

  significand_bits = sizeof(uint32_t) * 8 - wabt_clz_u32(significand);
  /* -1 for the implicit 1 bit of the significand */
  exponent += significand_exponent + significand_bits - 1;

  if (exponent >= F32_MAX_EXP) {
    /* inf or -inf */
    *out_bits = make_float(is_neg, F32_MAX_EXP, 0);
  } else if (exponent <= F32_MIN_EXP) {
    /* maybe subnormal */
    if (significand_bits > F32_SIG_BITS) {
      significand = shift_float_and_round_to_nearest(
          significand, significand_bits - F32_SIG_BITS);
    } else if (significand_bits < F32_SIG_BITS) {
      significand <<= (F32_SIG_BITS - significand_bits);
    }

    int shift = F32_MIN_EXP - exponent;
    if (shift < F32_SIG_BITS) {
      if (shift) {
        significand =
            shift_float_and_round_to_nearest(significand, shift) & F32_SIG_MASK;
      }
      exponent = F32_MIN_EXP;

      if (significand != 0) {
        *out_bits = make_float(is_neg, exponent, significand);
        return;
      }
    }

    /* not subnormal, too small; return 0 or -0 */
    *out_bits = make_float(is_neg, F32_MIN_EXP, 0);
  } else {
    /* normal value */
    if (significand_bits > F32_SIG_PLUS_ONE_BITS) {
      significand = shift_float_and_round_to_nearest(
          significand, significand_bits - F32_SIG_PLUS_ONE_BITS);
      if (significand > F32_SIG_PLUS_ONE_MASK)
        exponent++;
    } else if (significand_bits < F32_SIG_PLUS_ONE_BITS) {
      significand <<= (F32_SIG_PLUS_ONE_BITS - significand_bits);
    }

    *out_bits = make_float(is_neg, exponent, significand & F32_SIG_MASK);
  }
}

static void parse_float_infinity(const char* s,
                                 const char* end,
                                 uint32_t* out_bits) {
  bool is_neg = false;
  if (*s == '-') {
    is_neg = true;
    s++;
  } else if (*s == '+') {
    s++;
  }
  assert(string_starts_with(s, end, "infinity"));
  *out_bits = make_float(is_neg, F32_MAX_EXP, 0);
}

Result parse_float(LiteralType literal_type,
                   const char* s,
                   const char* end,
                   uint32_t* out_bits) {
  switch (literal_type) {
    case LiteralType::Int:
    case LiteralType::Float: {
      errno = 0;
      char* endptr;
      float value;
      value = strtof(s, &endptr);
      if (endptr != end ||
          ((value == 0 || value == HUGE_VALF || value == -HUGE_VALF) &&
           errno != 0))
        return Result::Error;

      memcpy(out_bits, &value, sizeof(value));
      return Result::Ok;
    }

    case LiteralType::Hexfloat:
      parse_float_hex(s, end, out_bits);
      return Result::Ok;

    case LiteralType::Infinity:
      parse_float_infinity(s, end, out_bits);
      return Result::Ok;

    case LiteralType::Nan:
      return parse_float_nan(s, end, out_bits);

    default:
      assert(0);
      return Result::Error;
  }
}

void write_float_hex(char* out, size_t size, uint32_t bits) {
  /* 1234567890123456 */
  /* -0x#.######p-### */
  /* -nan:0x###### */
  /* -infinity */
  char buffer[WABT_MAX_FLOAT_HEX];
  char* p = buffer;
  bool is_neg = (bits >> F32_SIGN_SHIFT);
  int exp = ((bits >> F32_SIG_BITS) & F32_EXP_MASK) - F32_EXP_BIAS;
  uint32_t sig = bits & F32_SIG_MASK;

  if (is_neg)
    *p++ = '-';
  if (exp == F32_MAX_EXP) {
    /* infinity or nan */
    if (sig == 0) {
      strcpy(p, "infinity");
      p += 8;
    } else {
      strcpy(p, "nan");
      p += 3;
      if (sig != F32_QUIET_NAN_TAG) {
        strcpy(p, ":0x");
        p += 3;
        /* skip leading zeroes */
        int num_nybbles = sizeof(uint32_t) * 8 / 4;
        while ((sig & 0xf0000000) == 0) {
          sig <<= 4;
          num_nybbles--;
        }
        while (num_nybbles) {
          uint32_t nybble = (sig >> (sizeof(uint32_t) * 8 - 4)) & 0xf;
          *p++ = s_hex_digits[nybble];
          sig <<= 4;
          --num_nybbles;
        }
      }
    }
  } else {
    bool is_zero = sig == 0 && exp == F32_MIN_EXP;
    strcpy(p, "0x");
    p += 2;
    *p++ = is_zero ? '0' : '1';

    /* shift sig up so the top 4-bits are at the top of the uint32 */
    sig <<= sizeof(uint32_t) * 8 - F32_SIG_BITS;

    if (sig) {
      if (exp == F32_MIN_EXP) {
        /* subnormal; shift the significand up, and shift out the implicit 1 */
        uint32_t leading_zeroes = wabt_clz_u32(sig);
        if (leading_zeroes < 31)
          sig <<= leading_zeroes + 1;
        else
          sig = 0;
        exp -= leading_zeroes;
      }

      *p++ = '.';
      while (sig) {
        uint32_t nybble = (sig >> (sizeof(uint32_t) * 8 - 4)) & 0xf;
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
      if (exp >= 100)
        *p++ = '1';
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

/* doubles */
static uint64_t make_double(bool sign, int exp, uint64_t sig) {
  assert(exp >= F64_MIN_EXP && exp <= F64_MAX_EXP);
  assert(sig <= F64_SIG_MASK);
  return (static_cast<uint64_t>(sign) << F64_SIGN_SHIFT) |
         (static_cast<uint64_t>(exp + F64_EXP_BIAS) << F64_SIG_BITS) | sig;
}

static uint64_t shift_double_and_round_to_nearest(uint64_t significand,
                                                  int shift) {
  assert(shift > 0);
  /* round ties to even */
  if (significand & (static_cast<uint64_t>(1) << shift))
    significand += static_cast<uint64_t>(1) << (shift - 1);
  significand >>= shift;
  return significand;
}

static Result parse_double_nan(const char* s,
                               const char* end,
                               uint64_t* out_bits) {
  bool is_neg = false;
  if (*s == '-') {
    is_neg = true;
    s++;
  } else if (*s == '+') {
    s++;
  }
  assert(string_starts_with(s, end, "nan"));
  s += 3;

  uint64_t tag;
  if (s != end) {
    tag = 0;
    if (!string_starts_with(s, end, ":0x"))
      return Result::Error;
    s += 3;

    for (; s < end; ++s) {
      uint32_t digit;
      if (WABT_FAILED(parse_hexdigit(*s, &digit)))
        return Result::Error;
      tag = tag * 16 + digit;
      /* check for overflow */
      if (tag > F64_SIG_MASK)
        return Result::Error;
    }

    /* NaN cannot have a zero tag, that is reserved for infinity */
    if (tag == 0)
      return Result::Error;
  } else {
    tag = F64_QUIET_NAN_TAG;
  }

  *out_bits = make_double(is_neg, F64_MAX_EXP, tag);
  return Result::Ok;
}

static void parse_double_hex(const char* s,
                             const char* end,
                             uint64_t* out_bits) {
  bool is_neg = false;
  if (*s == '-') {
    is_neg = true;
    s++;
  } else if (*s == '+') {
    s++;
  }
  assert(string_starts_with(s, end, "0x"));
  s += 2;

  /* see the similar comment in parse_float_hex */
  bool seen_dot = false;
  uint64_t significand = 0;
  /* how much to shift |significand| if a non-zero value is appended */
  int significand_shift = 0;
  int significand_bits = 0;     /* bits of |significand| */
  int significand_exponent = 0; /* exponent adjustment due to dot placement */
  for (; s < end; ++s) {
    uint32_t digit;
    if (*s == '.') {
      if (significand != 0)
        significand_exponent += significand_shift;
      significand_shift = 0;
      seen_dot = true;
      continue;
    } else if (WABT_FAILED(parse_hexdigit(*s, &digit))) {
      break;
    }
    significand_shift += HEX_DIGIT_BITS;
    if (digit != 0 && (significand == 0 ||
                       significand_bits + significand_shift <=
                           F64_SIG_BITS + 1 + HEX_DIGIT_BITS)) {
      if (significand != 0)
        significand <<= significand_shift;
      if (seen_dot)
        significand_exponent -= significand_shift;
      significand += digit;
      significand_shift = 0;
      significand_bits += HEX_DIGIT_BITS;
    }
  }

  if (!seen_dot)
    significand_exponent += significand_shift;

  if (significand == 0) {
    /* 0 or -0 */
    *out_bits = make_double(is_neg, F64_MIN_EXP, 0);
    return;
  }

  int exponent = 0;
  bool exponent_is_neg = false;
  if (s < end) {
    assert(*s == 'p');
    s++;

    /* exponent is always positive, but significand_exponent is signed.
     significand_exponent_add is negated if exponent will be negative, so it  can
     be easily summed to see if the exponent is too large (see below) */
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
      uint32_t digit = (*s - '0');
      assert(digit <= 9);
      exponent = exponent * 10 + digit;
      if (exponent + significand_exponent_add >= F64_MAX_EXP)
        break;
    }
  }

  if (exponent_is_neg)
    exponent = -exponent;

  significand_bits = sizeof(uint64_t) * 8 - wabt_clz_u64(significand);
  /* -1 for the implicit 1 bit of the significand */
  exponent += significand_exponent + significand_bits - 1;

  if (exponent >= F64_MAX_EXP) {
    /* inf or -inf */
    *out_bits = make_double(is_neg, F64_MAX_EXP, 0);
  } else if (exponent <= F64_MIN_EXP) {
    /* maybe subnormal */
    if (significand_bits > F64_SIG_BITS) {
      significand = shift_double_and_round_to_nearest(
          significand, significand_bits - F64_SIG_BITS);
    } else if (significand_bits < F64_SIG_BITS) {
      significand <<= (F64_SIG_BITS - significand_bits);
    }

    int shift = F64_MIN_EXP - exponent;
    if (shift < F64_SIG_BITS) {
      if (shift) {
        significand = shift_double_and_round_to_nearest(significand, shift) &
                      F64_SIG_MASK;
      }
      exponent = F64_MIN_EXP;

      if (significand != 0) {
        *out_bits = make_double(is_neg, exponent, significand);
        return;
      }
    }

    /* not subnormal, too small; return 0 or -0 */
    *out_bits = make_double(is_neg, F64_MIN_EXP, 0);
  } else {
    /* normal value */
    if (significand_bits > F64_SIG_PLUS_ONE_BITS) {
      significand = shift_double_and_round_to_nearest(
          significand, significand_bits - F64_SIG_PLUS_ONE_BITS);
      if (significand > F64_SIG_PLUS_ONE_MASK)
        exponent++;
    } else if (significand_bits < F64_SIG_PLUS_ONE_BITS) {
      significand <<= (F64_SIG_PLUS_ONE_BITS - significand_bits);
    }

    *out_bits = make_double(is_neg, exponent, significand & F64_SIG_MASK);
  }
}

static void parse_double_infinity(const char* s,
                                  const char* end,
                                  uint64_t* out_bits) {
  bool is_neg = false;
  if (*s == '-') {
    is_neg = true;
    s++;
  } else if (*s == '+') {
    s++;
  }
  assert(string_starts_with(s, end, "infinity"));
  *out_bits = make_double(is_neg, F64_MAX_EXP, 0);
}

Result parse_double(LiteralType literal_type,
                    const char* s,
                    const char* end,
                    uint64_t* out_bits) {
  switch (literal_type) {
    case LiteralType::Int:
    case LiteralType::Float: {
      errno = 0;
      char* endptr;
      double value;
      value = strtod(s, &endptr);
      if (endptr != end ||
          ((value == 0 || value == HUGE_VAL || value == -HUGE_VAL) &&
           errno != 0))
        return Result::Error;

      memcpy(out_bits, &value, sizeof(value));
      return Result::Ok;
    }

    case LiteralType::Hexfloat:
      parse_double_hex(s, end, out_bits);
      return Result::Ok;

    case LiteralType::Infinity:
      parse_double_infinity(s, end, out_bits);
      return Result::Ok;

    case LiteralType::Nan:
      return parse_double_nan(s, end, out_bits);

    default:
      assert(0);
      return Result::Error;
  }
}

void write_double_hex(char* out, size_t size, uint64_t bits) {
  /* 123456789012345678901234 */
  /* -0x#.#############p-#### */
  /* -nan:0x############# */
  /* -infinity */
  char buffer[WABT_MAX_DOUBLE_HEX];
  char* p = buffer;
  bool is_neg = (bits >> F64_SIGN_SHIFT);
  int exp = ((bits >> F64_SIG_BITS) & F64_EXP_MASK) - F64_EXP_BIAS;
  uint64_t sig = bits & F64_SIG_MASK;

  if (is_neg)
    *p++ = '-';
  if (exp == F64_MAX_EXP) {
    /* infinity or nan */
    if (sig == 0) {
      strcpy(p, "infinity");
      p += 8;
    } else {
      strcpy(p, "nan");
      p += 3;
      if (sig != F64_QUIET_NAN_TAG) {
        strcpy(p, ":0x");
        p += 3;
        /* skip leading zeroes */
        int num_nybbles = sizeof(uint64_t) * 8 / 4;
        while ((sig & 0xf000000000000000ULL) == 0) {
          sig <<= 4;
          num_nybbles--;
        }
        while (num_nybbles) {
          uint32_t nybble = (sig >> (sizeof(uint64_t) * 8 - 4)) & 0xf;
          *p++ = s_hex_digits[nybble];
          sig <<= 4;
          --num_nybbles;
        }
      }
    }
  } else {
    bool is_zero = sig == 0 && exp == F64_MIN_EXP;
    strcpy(p, "0x");
    p += 2;
    *p++ = is_zero ? '0' : '1';

    /* shift sig up so the top 4-bits are at the top of the uint32 */
    sig <<= sizeof(uint64_t) * 8 - F64_SIG_BITS;

    if (sig) {
      if (exp == F64_MIN_EXP) {
        /* subnormal; shift the significand up, and shift out the implicit 1 */
        uint32_t leading_zeroes = wabt_clz_u64(sig);
        if (leading_zeroes < 63)
          sig <<= leading_zeroes + 1;
        else
          sig = 0;
        exp -= leading_zeroes;
      }

      *p++ = '.';
      while (sig) {
        uint32_t nybble = (sig >> (sizeof(uint64_t) * 8 - 4)) & 0xf;
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

#if COMPILER_IS_MSVC
#if _MSC_VER <= 1800
float strtof(const char *nptr, char **endptr) {
  const char* end = nptr + strlen(nptr);
  // review:: should we check for leading whitespaces ?
  if (string_starts_with(nptr, end, "0x")) {
    uint32_t out_bits = 0;
    parse_float_hex(nptr, end, &out_bits);
    float value;
    memcpy((void*)&value, &out_bits, sizeof(value));

    *endptr = (char*)end;
    return value;
  }
  return ::strtof(nptr, endptr);
}
double strtod(const char *nptr, char **endptr) {
  const char* end = nptr + strlen(nptr);
  // review:: should we check for leading whitespaces ?
  if (string_starts_with(nptr, end, "0x")) {
    uint64_t out_bits = 0;
    parse_double_hex(nptr, end, &out_bits);
    double value;
    memcpy((void*)&value, &out_bits, sizeof(value));

    *endptr = (char*)end;
    return value;
  }
  return ::strtod(nptr, endptr);
}
#endif
#endif
}  // namespace wabt

