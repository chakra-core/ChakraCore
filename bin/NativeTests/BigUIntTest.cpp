//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "stdafx.h"
#pragma warning(disable:26434) // Function definition hides non-virtual function in base class
#pragma warning(disable:26439) // Implicit noexcept
#pragma warning(disable:26451) // Arithmetic overflow
#pragma warning(disable:26495) // Uninitialized member variable
#include "catch.hpp"
#include "BigUIntTest.h"

#pragma warning(disable:4100) // unreferenced formal parameter
#pragma warning(disable:6387) // suppressing preFAST which raises warning for passing null to the JsRT APIs
#pragma warning(disable:6262) // CATCH is using stack variables to report errors, suppressing the preFAST warning.

namespace BigUIntTest
{
    TEST_CASE("Init_Compare", "[BigUIntTest]")
    {
        uint32 digits[1];
        int32 length = 1;
        Js::BigUInt bi1, bi2;
        BOOL f;
        int result;

        digits[0] = 0x00001111;
        f = bi1.FInitFromRglu(digits, length);
        REQUIRE(f);

        SECTION("Equal number init from the same array and length")
        {
            f = bi2.FInitFromRglu(digits, length);
            REQUIRE(f);
            result = bi1.Compare(&bi2);
            CHECK(result == 0);
        }

        SECTION("Equal number init from other big int number")
        {
            f = bi2.FInitFromBigint(&bi1);
            REQUIRE(f);
            result = bi1.Compare(&bi2);
            CHECK(result == 0);
        }

        SECTION("Greater number")
        {
            digits[0] = 0x00000001;
            f = bi2.FInitFromRglu(digits, length);
            REQUIRE(f);
            result = bi1.Compare(&bi2);
            CHECK(result == 1);
        }

        SECTION("Smaller number")
        {
            digits[0] = 0x00000001;
            f = bi2.FInitFromRglu(digits, length);
            REQUIRE(f);
            result = bi2.Compare(&bi1);
            CHECK(result == -1);
        }
    }

    TEST_CASE("Addition", "[BigUIntTest]")
    {
        uint32 digits[1], digit1s[2];
        int32 length = 1;
        Js::BigUInt bi1, bi2, bi3;
        BOOL f;
        int result;

        SECTION("Check 0x33331111 + 0x33331111 = 0x66662222")
        {
            digits[0] = 0x33331111;
            f = bi1.FInitFromRglu(digits, length);
            REQUIRE(f);
            f = bi2.FInitFromBigint(&bi1);
            REQUIRE(f);
            f = bi1.FAdd(&bi2);
            REQUIRE(f);
            digits[0] = 0x66662222;
            f = bi3.FInitFromRglu(digits, length);
            REQUIRE(f);
            result = bi1.Compare(&bi3);
            CHECK(result == 0);
        }

        SECTION("Check 0xffffffff + 0x1 = 0x100000000")
        {
            digits[0] = 0xffffffff;
            f = bi1.FInitFromRglu(digits, length);
            REQUIRE(f);
            digits[0] = 0x00000001;
            f = bi2.FInitFromRglu(digits, length);
            REQUIRE(f);
            f = bi1.FAdd(&bi2);
            digit1s[0] = 0x0;
            digit1s[1] = 0x1;
            f = bi3.FInitFromRglu(digit1s, 2);
            REQUIRE(f);
            result = bi1.Compare(&bi3);
            CHECK(result == 0);
        }

        SECTION("Check 0xffffffffffffffff + 0x1 = 0x10000000000000000")
        {
            digit1s[0] = 0xffffffff;
            digit1s[1] = 0xffffffff;
            f = bi1.FInitFromRglu(digit1s, 2);
            REQUIRE(f);
            digits[0] = 0x00000001;
            f = bi2.FInitFromRglu(digits, 1);
            REQUIRE(f);
            f = bi1.FAdd(&bi2);
            uint32 digit2s[3];
            digit2s[0] = 0x0;
            digit2s[1] = 0x0;
            digit2s[2] = 0x1;
            f = bi3.FInitFromRglu(digit2s, 3);
            REQUIRE(f);
            result = bi1.Compare(&bi3);
            CHECK(result == 0);
        }
    }

    TEST_CASE("Addition_Subtraction_Large_Number", "[BigUIntTest]")
    {
        const int l1 = 50, l2 = 1;
        uint32 digit1s[l1], digit2s[l2];
        Js::BigUInt bi1, bi2;
        BOOL f;

        SECTION("Check 0xf...0xf + 0x1 = 0x1_0x0...0x0")
        {
            for (int i = 0; i < l1; i++)
            {
                digit1s[i] = 0xffffffff;
            }
            f = bi1.FInitFromRglu(digit1s, l1);
            REQUIRE(f);
            digit2s[0] = 0x1;
            f = bi2.FInitFromRglu(digit2s, l2);
            REQUIRE(f);
            f = bi1.FAdd(&bi2);
            REQUIRE(f);
            int32 length = bi1.Clu();
            CHECK(length == l1 + 1);
            uint32 digit = bi1.Lu(length - 1);
            CHECK(digit == 1);
            for (int i = 0; i < length - 1; i++)
            {
                digit = bi1.Lu(i);
                CHECK(digit == 0);
            }
        }
    }

    TEST_CASE("Subtraction", "[BigUIntTest]")
    {
        uint32 digits[1], digit1s[2];
        int32 length = 1;
        Js::BigUInt bi1, bi2, bi3;
        BOOL f;
        int result;

        SECTION("Check 0x66662222 - 0x33331111 = 0x33331111")
        {
            digits[0] = 0x33331111;
            f = bi1.FInitFromRglu(digits, length);
            REQUIRE(f);
            f = bi2.FInitFromBigint(&bi1);
            REQUIRE(f);
            digits[0] = 0x66662222;
            f = bi3.FInitFromRglu(digits, length);
            REQUIRE(f);
            bi3.Subtract(&bi2);
            result = bi1.Compare(&bi3);
            CHECK(result == 0);
        }

        SECTION("Check 0x3_0x1 - 0x1_0x0 = 0x2_0x1")
        {
            digit1s[0] = 0x1;
            digit1s[1] = 0x3;
            f = bi3.FInitFromRglu(digit1s, 2);
            REQUIRE(f);
            digit1s[0] = 0x0;
            digit1s[1] = 0x1;
            f = bi2.FInitFromRglu(digit1s, 2);
            REQUIRE(f);
            bi3.Subtract(&bi2);
            int l = bi3.Clu();
            CHECK(l == 2);
            int digit = bi3.Lu(1);
            CHECK(digit == 2);
            digit = bi3.Lu(0);
            CHECK(digit == 1);
        }

        SECTION("Check 0x2_0x0 - 0x1 = 0x1_0xfffffff")
        {
            digit1s[0] = 0x0;
            digit1s[1] = 0x2;
            f = bi3.FInitFromRglu(digit1s, 2);
            REQUIRE(f);
            digits[0] = 0x1;
            f = bi2.FInitFromRglu(digits, 1);
            REQUIRE(f);
            bi3.Subtract(&bi2);
            int l = bi3.Clu();
            CHECK(l == 2);
            int digit = bi3.Lu(1);
            CHECK(digit == 1);
            digit = bi3.Lu(0);
            CHECK(digit == 0xffffffff);
        }

        SECTION("Currently 0x1_0x0 - 0x1 is overflow")
        {
        }
    }

    TEST_CASE("Init_From_Char_Of_Digits", "[BigUIntTest]")
    {
        BigUInt biDec;
        const char *charDigit;
        bool result;
        int charDigitLength;

        SECTION("2**32-1 should have length = 1")
        {
            charDigit = "4294967295";
            charDigitLength = 10;
            result = biDec.FInitFromDigits(charDigit, charDigitLength, &charDigitLength);
            REQUIRE(result);
            int length = biDec.Clu();
            CHECK(length == 1);
            uint32 digit = biDec.Lu(0);
            CHECK(digit == 4294967295);
        }

        SECTION("2**32+2 should have length = 2")
        {
            charDigit = "4294967298";
            charDigitLength = 10;
            result = biDec.FInitFromDigits(charDigit, charDigitLength, &charDigitLength);
            REQUIRE(result);
            int length = biDec.Clu();
            CHECK(length == 2);
            uint32 digit = biDec.Lu(0);
            CHECK(digit == 2);
            digit = biDec.Lu(1);
            CHECK(digit == 1);
        }
        
        SECTION("2**64 should have length = 3")
        {
            charDigit = "18446744073709551616";
            charDigitLength = 20;
            result = biDec.FInitFromDigits(charDigit, charDigitLength, &charDigitLength);
            REQUIRE(result);
            int length = biDec.Clu();
            CHECK(length == 3);
            uint32 digit = biDec.Lu(0);
            CHECK(digit == 0);
            digit = biDec.Lu(1);
            CHECK(digit == 0);
            digit = biDec.Lu(2);
            CHECK(digit == 1);
        }
    }
}
