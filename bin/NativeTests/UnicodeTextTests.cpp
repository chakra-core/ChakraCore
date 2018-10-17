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

namespace UnicodeTextTest
{
    void Test(const WCHAR* str1, const WCHAR* str2, int expected)
    {
        REQUIRE(g_testHooksLoaded);

        // there are two tests, one to validate the expected value and validate the result of the call
        int compareStringResult = CompareStringEx(LOCALE_NAME_USER_DEFAULT, NORM_IGNORECASE | SORT_DIGITSASNUMBERS, str1, -1, str2, -1, NULL, NULL, 0);
        CHECK(compareStringResult != 0);
        compareStringResult = compareStringResult - CSTR_EQUAL;

        int res = g_testHooks.pfLogicalCompareStringImpl(str1, static_cast<int>(wcslen(str1)), str2, static_cast<int>(wcslen(str2)));

        //test to check that expected value passed is correct
        REQUIRE(compareStringResult == expected);

        //test to check that the result from call to LogicalStringCompareImpl is the expected value
        REQUIRE(res == expected);
    }

    void TestNullCharacters(const WCHAR* str1, int str1size, const WCHAR* str2, int str2size, int expected)
    {
        REQUIRE(g_testHooksLoaded);

        // there are two tests, one to validate the expected value and validate the result of the call
        int compareStringResult = CompareStringEx(LOCALE_NAME_USER_DEFAULT, NORM_IGNORECASE | SORT_DIGITSASNUMBERS, str1, str1size, str2, str2size, NULL, NULL, 0);
        CHECK(compareStringResult != 0);
        compareStringResult = compareStringResult - CSTR_EQUAL;

        int res = g_testHooks.pfLogicalCompareStringImpl(str1, str1size, str2, str2size);

        //test to check that expected value passed is correct
        REQUIRE(compareStringResult == expected);

        //test to check that the result from call to LogicalStringCompareImpl is the expected value
        REQUIRE(res == expected);
    }

    TEST_CASE("LogicalCompareString_BasicLatinLowercase", "[UnicodeText]")
    {
        Test(_u("a"), _u("a"), 0);
        Test(_u("a"), _u("b"), -1);
        Test(_u("b"), _u("a"), 1);
    }

    TEST_CASE("LogicalCompareString_BasicNumbers", "[UnicodeText]")
    {
        Test(_u("1"), _u("2"), -1);
        Test(_u("2"), _u("1"), 1);
        Test(_u("10"), _u("01"), 1);
        Test(_u("01"), _u("10"), -1);
        Test(_u("01"), _u("1"), -1);
        Test(_u("1"), _u("01"), 1);
    }

    TEST_CASE("LogicalCompareString_Alphanumeric", "[UnicodeText]")
    {
        Test(_u("1a"), _u("a1"), -1);
        Test(_u("aa1"), _u("a1"), 1);
        Test(_u("a1"), _u("a1"), 0);
        Test(_u("a1"), _u("b1"), -1);
        Test(_u("b1"), _u("a1"), 1);
        Test(_u("a1"), _u("a2"), -1);
        Test(_u("a10"), _u("a2"), 1);
        Test(_u("a2"), _u("a10"), -1);
    }

    TEST_CASE("LogicalCompareString_ComplexAlphanumeric", "[UnicodeText]")
    {
        Test(_u("A1"), _u("a1"), 0);
        Test(_u("A1"), _u("b1"), -1);
        Test(_u("B1"), _u("a1"), 1);
        Test(_u("A1"), _u("a2"), -1);
        Test(_u("A10"), _u("a2"), 1);
        Test(_u("A2"), _u("a10"), -1);
        Test(_u("123"), _u("456"), -1);
        Test(_u("456"), _u("123"), 1);
        Test(_u("abc123"), _u("def123"), -1);
        Test(_u("abc123"), _u("abc123"), 0);
        Test(_u("abc123"), _u("abc0123"), 1);
        Test(_u("abc123"), _u("abc124"), -1);
        Test(_u("abc124"), _u("abc123"), 1);
        Test(_u("abc123def"), _u("abc123def"), 0);
        Test(_u("abc123def"), _u("abc123eef"), -1);
        Test(_u("abc123eef"), _u("abc123def"), 1);
        Test(_u("abc1def"), _u("abc10def"), -1);
        Test(_u("abc1def1"), _u("abc1def12"), -1);
        Test(_u("2string"), _u("3string"), -1);
        Test(_u("20string"), _u("3string"), 1);
        Test(_u("20string"), _u("st2ring"), -1);
        Test(_u("st3ring"), _u("st2ring"), 1);
        Test(_u("2String"), _u("3string"), -1);
        Test(_u("20String"), _u("3string"), 1);
        Test(_u("20sTRing"), _u("st2ring"), -1);
        Test(_u("st3rING"), _u("st2riNG"), 1);
    }

    TEST_CASE("LogicalCompareString_EmbeddedNullCharacters", "[UnicodeText]")
    {
        TestNullCharacters(_u("\0\0ab\0\0c123def\0\0"), 15, _u("abc123def"), 9, 0);
    }

}
