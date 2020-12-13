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

#pragma warning(disable:4100) // unreferenced formal parameter
#pragma warning(disable:6387) // suppressing preFAST which raises warning for passing null to the JsRT APIs
#pragma warning(disable:6262) // CATCH is using stack variables to report errors, suppressing the preFAST warning.

namespace JavascriptBigIntTests
{
    void Test_AddDigit(digit_t digit1, digit_t digit2, digit_t * carry, digit_t expectedResult, digit_t expectedCarry)
    {
        REQUIRE(g_testHooksLoaded);

        digit_t res = g_testHooks.pfAddDigit(digit1, digit2, carry);

        //test to check that the result from call to AddDigit is the expected value
        REQUIRE(res == expectedResult);
        REQUIRE(expectedCarry == *carry);
    }

    void Test_SubDigit(digit_t digit1, digit_t digit2, digit_t * borrow, digit_t expectedResult, digit_t expectedBorrow)
    {
        REQUIRE(g_testHooksLoaded);

        digit_t res = g_testHooks.pfSubDigit(digit1, digit2, borrow);

        //test to check that the result from call to SubtractDigit is the expected value
        REQUIRE(res == expectedResult);
        REQUIRE(*borrow == expectedBorrow);
    }

    void Test_MulDigit(digit_t digit1, digit_t digit2, digit_t * high, digit_t expectedResult, digit_t expectedHigh)
    {
        REQUIRE(g_testHooksLoaded);

        digit_t res = g_testHooks.pfMulDigit(digit1, digit2, high);

        //test to check that the result from call to SubtractDigit is the expected value
        REQUIRE(res == expectedResult);
        REQUIRE(*high == expectedHigh);
    }

    TEST_CASE("AddDigit", "[JavascriptBigIntTests]")
    {
        digit_t carry = 0;
        Test_AddDigit(1, 2, &carry, 3, 0);

        digit_t d1 = UINTPTR_MAX;
        digit_t d2 = UINTPTR_MAX;
        carry = 0;
        Test_AddDigit(d1, d2, &carry, UINTPTR_MAX-1, 1);
    }

    TEST_CASE("SubDigit", "[JavascriptBigIntTests]")
    {
        digit_t borrow = 0;
        Test_SubDigit(3, 2, &borrow, 1, 0);

        digit_t d1 = 0;
        digit_t d2 = 1;
        borrow = 0;
        Test_SubDigit(d1, d2, &borrow, UINTPTR_MAX, 1);
    }

    TEST_CASE("MulDigit", "[JavascriptBigIntTests]")
    {
        digit_t high = 0;
        Test_MulDigit(3, 2, &high, 6, 0);

        digit_t d1 = UINTPTR_MAX;
        digit_t d2 = 2;
        high = 0;
        Test_MulDigit(d1, d2, &high, UINTPTR_MAX-1, 1);
    }
}
