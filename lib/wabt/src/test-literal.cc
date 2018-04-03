/*
 * Copyright 2018 WebAssembly Community Group participants
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

#include <cstdio>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

#include "src/literal.h"

using namespace wabt;

namespace {

void AssertHexFloatEquals(uint32_t expected_bits, const char* s) {
  uint32_t actual_bits;
  ASSERT_EQ(Result::Ok,
            ParseFloat(LiteralType::Hexfloat, s, s + strlen(s), &actual_bits));
  ASSERT_EQ(expected_bits, actual_bits);
}

void AssertHexFloatFails(const char* s) {
  uint32_t actual_bits;
  ASSERT_EQ(Result::Error,
            ParseFloat(LiteralType::Hexfloat, s, s + strlen(s), &actual_bits));
}

void AssertHexDoubleEquals(uint64_t expected_bits, const char* s) {
  uint64_t actual_bits;
  ASSERT_EQ(Result::Ok,
            ParseDouble(LiteralType::Hexfloat, s, s + strlen(s), &actual_bits));
  ASSERT_EQ(expected_bits, actual_bits);
}

void AssertHexDoubleFails(const char* s) {
  uint64_t actual_bits;
  ASSERT_EQ(Result::Error,
            ParseDouble(LiteralType::Hexfloat, s, s + strlen(s), &actual_bits));
}

}  // end anonymous namespace

TEST(ParseFloat, NonCanonical) {
  AssertHexFloatEquals(0x3f800000, "0x00000000000000000000001.0p0");
  AssertHexFloatEquals(0x3f800000, "0x1.00000000000000000000000p0");
  AssertHexFloatEquals(0x3f800000, "0x0.0000000000000000000001p88");
}

TEST(ParseFloat, Rounding) {
  // |------- 23 bits -----| V-- extra bit
  //
  // 11111111111111111111101 0  ==> no rounding
  AssertHexFloatEquals(0x7f7ffffd, "0x1.fffffap127");
  // 11111111111111111111101 1  ==> round up
  AssertHexFloatEquals(0x7f7ffffe, "0x1.fffffbp127");
  // 11111111111111111111110 0  ==> no rounding
  AssertHexFloatEquals(0x7f7ffffe, "0x1.fffffcp127");
  // 11111111111111111111110 1  ==> round down
  AssertHexFloatEquals(0x7f7ffffe, "0x1.fffffdp127");
  // 11111111111111111111111 0  ==> no rounding
  AssertHexFloatEquals(0x7f7fffff, "0x1.fffffep127");
}

TEST(ParseFloat, OutOfRange) {
  AssertHexFloatFails("0x1p128");
  AssertHexFloatFails("-0x1p128");
  AssertHexFloatFails("0x1.ffffffp127");
  AssertHexFloatFails("-0x1.ffffffp127");
}

TEST(ParseDouble, NonCanonical) {
  AssertHexDoubleEquals(0x3ff0000000000000, "0x00000000000000000000001.0p0");
  AssertHexDoubleEquals(0x3ff0000000000000, "0x1.00000000000000000000000p0");
  AssertHexDoubleEquals(0x3ff0000000000000, "0x0.0000000000000000000001p88");
}

TEST(ParseDouble, Rounding) {
  // |-------------------- 52 bits ---------------------| V-- extra bit
  //
  // 1111111111111111111111111111111111111111111111111101 0  ==> no rounding
  AssertHexDoubleEquals(0x7feffffffffffffd, "0x1.ffffffffffffd0p1023");
  // 1111111111111111111111111111111111111111111111111101 1  ==> round up
  AssertHexDoubleEquals(0x7feffffffffffffe, "0x1.ffffffffffffd8p1023");
  // 1111111111111111111111111111111111111111111111111110 0  ==> no rounding
  AssertHexDoubleEquals(0x7feffffffffffffe, "0x1.ffffffffffffe0p1023");
  // 1111111111111111111111111111111111111111111111111110 1  ==> round down
  AssertHexDoubleEquals(0x7feffffffffffffe, "0x1.ffffffffffffe8p1023");
  // 1111111111111111111111111111111111111111111111111111 0  ==> no rounding
  AssertHexDoubleEquals(0x7fefffffffffffff, "0x1.fffffffffffff0p1023");
}

TEST(ParseDouble, OutOfRange) {
  AssertHexDoubleFails("0x1p1024");
  AssertHexDoubleFails("-0x1p1024");
  AssertHexDoubleFails("0x1.fffffffffffff8p1023");
  AssertHexDoubleFails("-0x1.fffffffffffff8p1023");
}
