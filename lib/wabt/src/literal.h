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

#ifndef WABT_LITERAL_H_
#define WABT_LITERAL_H_

#include <cstdint>

#include "common.h"

namespace wabt {

/* These functions all return Result::Ok on success and Result::Error on failure.
 *
 * NOTE: the functions are written for use with the re2c lexer, assuming that
 * the literal has already matched the regular expressions defined there. As a
 * result, the only validation that is done is for overflow, not for otherwise
 * bogus input. */

enum class ParseIntType {
  UnsignedOnly = 0,
  SignedAndUnsigned = 1,
};

/* Size of char buffer required to hold hex representation of a float/double */
#define WABT_MAX_FLOAT_HEX 20
#define WABT_MAX_DOUBLE_HEX 40

Result parse_hexdigit(char c, uint32_t* out);
Result parse_int32(const char* s,
                   const char* end,
                   uint32_t* out,
                   ParseIntType parse_type);
Result parse_int64(const char* s,
                   const char* end,
                   uint64_t* out,
                   ParseIntType parse_type);
Result parse_uint64(const char* s, const char* end, uint64_t* out);
Result parse_float(LiteralType literal_type,
                   const char* s,
                   const char* end,
                   uint32_t* out_bits);
Result parse_double(LiteralType literal_type,
                    const char* s,
                    const char* end,
                    uint64_t* out_bits);

void write_float_hex(char* buffer, size_t size, uint32_t bits);
void write_double_hex(char* buffer, size_t size, uint64_t bits);

}  // namespace wabt

#endif /* WABT_LITERAL_H_ */
