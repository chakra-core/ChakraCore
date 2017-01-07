//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimePlatformAgnosticPch.h"
#include "UnicodeText.h"
#include <ctype.h>

namespace PlatformAgnostic
{
namespace UnicodeText
{
CharacterTypeFlags GetLegacyCharacterTypeFlags(char16 ch)
{
    Assert(ch >= 128);

    const char16 lineSeperatorChar = 0x2028;
    const char16 paraSeperatorChar = 0x2029;

    if (ch == lineSeperatorChar || ch == paraSeperatorChar)
    {
        return LineCharGroup;
    }

    CharacterClassificationType charClass = GetLegacyCharacterClassificationType(ch);

    if (charClass == CharacterClassificationType::Letter)
    {
        return LetterCharGroup;
    }
    else if (charClass == CharacterClassificationType::DigitOrPunct)
    {
        return IdChar;
    }
    else if (charClass == CharacterClassificationType::Whitespace)
    {
        return SpaceChar;
    }

    return UnknownChar;
}

#if !defined(_WIN32) || ENABLE_TEST_PLATFORM_AGNOSTIC
namespace Internal
{
template <typename CharType>
inline bool isDigit(__in CharType c)
{
    return c >= '0' && c <= '9';
}

template <typename CharType>
inline int readNumber(__inout CharType* &str)
{
    int num = 0;

    // Count digits on each string
    while (isDigit(*str))
    {
        int newNum = (num * 10) + (*str - '0');
        Assert(newNum > num);
        if (newNum <= num)
        {
            return num;
        }

        num = newNum;

        str++;
    }

    return num;
}

///
/// Implementation of CompareString(NORM_IGNORECASE | SORT_DIGITSASNUMBERS)
/// This code is in the common library so that we can unit test it on Windows too
///
/// Basic algorithm is as follows:
///
/// Iterate through both strings
///  If either current character is not a number, compare the rest of the strings lexically
///  else if they're both numbers:
///     Count the leading zeroes in both strings and skip over them
///     Read the two numbers
///     If they're not the same, return accordingly
///     If they are the same, but the leading zeroes are different, return whichever has fewer zeroes as "larger"
///     Otherwise, they're the same to continue scanning
///  else they're both not numbers:
///     Compare lexically till we find a number
/// The algorithm treats the characters in a case-insensitive manner
///
template <typename CharType>
int LogicalStringCompareImpl(__in const CharType* p1, __in const CharType* p2)
{
    Assert(p1 != nullptr);
    Assert(p2 != nullptr);

    while (*p1 && *p2)
    {
        bool isDigit1 = isDigit(*p1);
        bool isDigit2 = isDigit(*p2);

        if (isDigit1 ^ isDigit2)
        {
            // If either character exclusively is not a number, compare just the characters
            // since that's enough to sort the string
            CharType c1 = tolower(*p1);
            CharType c2 = tolower(*p2);

            Assert(c1 != c2);

            if (c1 < c2)
            {
                return -1;
            }

            return 1;
        }
        else if (isDigit1 && isDigit2)
        {
            // Both current characters are digits, so we'll compare the numbers
            int numZero1 = 0, numZero2 = 0;

            // Count leading zeroes on each string
            while (*p1 == '0')
            {
                p1++; numZero1++;
            }

            while (*p2 == '0')
            {
                p2++; numZero2++;
            }

            int num1 = readNumber(p1);
            int num2 = readNumber(p2);

            if (num1 != num2)
            {
                // If the numbers are unequal, just compare the numbers
                return (num1 > num2) ? 1 : -1;
            }
            else if (numZero1 != numZero2)
            {
                // The numbers are equal but the number of leading zeroes are not
                // The one with the fewer number of leading zeroes is considered
                // "larger" (gets sorted earlier)
                return (numZero1 < numZero2 ? 1 : -1);
            }
            // else both numbers are the same, keep going
        }
        else
        {
            // Both characters are not digits - scan till we find a
            // digit in the same position in each string
            while (*p1 && !isDigit(*p1) && *p2 && !isDigit(*p2))
            {
                int c1 = tolower(*p1);
                int c2 = tolower(*p2);

                if (c1 < c2)
                {
                    return -1;
                }
                else if (c1 > c2)
                {
                    return 1;
                }

                p1++; p2++;
            }
        }
    }

    if (*p1 == *p2) return 0;

    return ((*p1 > *p2) ? 1 : -1);
}

template
int LogicalStringCompareImpl<char16>(__in const char16* str1, __in const char16* str2);

}; // Internal
#endif // !defined(_WIN32) || ENABLE_TEST_PLATFORM_AGNOSTIC
}; // UnicodeText
}; // PlatformAgnostic

// Unnamespaced test code
#if ENABLE_TEST_PLATFORM_AGNOSTIC
void LogicalStringCompareTest(const WCHAR* str1, const WCHAR* str2, int expected)
{
    int compareStringResult = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_DIGITSASNUMBERS, str1, -1, str2, -1);

    if (compareStringResult == 0)
    {
        printf("ERROR: CompareStringW failed with error: %d\n", ::GetLastError());
        return;
    }

    compareStringResult = compareStringResult - CSTR_EQUAL;

    int res = PlatformAgnostic::UnicodeText::Internal::LogicalStringCompareImpl<WCHAR>(str1, str2);
    bool passed = res == expected;

    if (compareStringResult != expected)
    {
        wprintf(L"WARNING: Incorrect expected value: %d, actual: %d\n", expected, compareStringResult);
    }

    if (passed)
    {
        wprintf(L"[%s]\n", L"PASS");
    }
    else
    {
        wprintf(L"[%s] Expected: %d, Actual: %d)\n", L"FAIL", expected, res);
    }
}

void doLogicalStringTests()
{
    LogicalStringCompareTest(L"a", L"a", 0);
    LogicalStringCompareTest(L"a", L"b", -1);
    LogicalStringCompareTest(L"b", L"a", 1);

    LogicalStringCompareTest(L"a", L"A", 0);
    LogicalStringCompareTest(L"a", L"B", -1);
    LogicalStringCompareTest(L"b", L"A", 1);

    LogicalStringCompareTest(L"1", L"2", -1);
    LogicalStringCompareTest(L"2", L"1", 1);

    LogicalStringCompareTest(L"10", L"01", 1);
    LogicalStringCompareTest(L"01", L"10", -1);

    LogicalStringCompareTest(L"01", L"1", -1);
    LogicalStringCompareTest(L"1", L"01", 1);

    LogicalStringCompareTest(L"1a", L"a1", -1);
    LogicalStringCompareTest(L"aa1", L"a1", 1);
    LogicalStringCompareTest(L"a1", L"a1", 0);
    LogicalStringCompareTest(L"a1", L"b1", -1);
    LogicalStringCompareTest(L"b1", L"a1", 1);
    LogicalStringCompareTest(L"a1", L"a2", -1);
    LogicalStringCompareTest(L"a10", L"a2", 1);
    LogicalStringCompareTest(L"a2", L"a10", -1);

    LogicalStringCompareTest(L"A1", L"a1", 0);
    LogicalStringCompareTest(L"A1", L"b1", -1);
    LogicalStringCompareTest(L"B1", L"a1", 1);
    LogicalStringCompareTest(L"A1", L"a2", -1);
    LogicalStringCompareTest(L"A10", L"a2", 1);
    LogicalStringCompareTest(L"A2", L"a10", -1);

    LogicalStringCompareTest(L"123", L"456", -1);
    LogicalStringCompareTest(L"456", L"123", 1);
    LogicalStringCompareTest(L"abc123", L"def123", -1);
    LogicalStringCompareTest(L"abc123", L"abc123", 0);
    LogicalStringCompareTest(L"abc123", L"abc0123", 1);
    LogicalStringCompareTest(L"abc123", L"abc124", -1);
    LogicalStringCompareTest(L"abc124", L"abc123", 1);

    LogicalStringCompareTest(L"abc123def", L"abc123def", 0);
    LogicalStringCompareTest(L"abc123def", L"abc123eef", -1);
    LogicalStringCompareTest(L"abc123eef", L"abc123def", 1);

    LogicalStringCompareTest(L"abc1def", L"abc10def", -1);
    LogicalStringCompareTest(L"abc1def1", L"abc1def12", -1);

    LogicalStringCompareTest(L"2string", L"3string", -1);
    LogicalStringCompareTest(L"20string", L"3string", 1);
    LogicalStringCompareTest(L"20string", L"st2ring", -1);
    LogicalStringCompareTest(L"st3ring", L"st2ring", 1);

    LogicalStringCompareTest(L"2String", L"3string", -1);
    LogicalStringCompareTest(L"20String", L"3string", 1);
    LogicalStringCompareTest(L"20sTRing", L"st2ring", -1);
    LogicalStringCompareTest(L"st3rING", L"st2riNG", 1);
}
#endif
