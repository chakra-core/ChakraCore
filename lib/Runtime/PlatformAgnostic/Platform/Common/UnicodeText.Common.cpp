//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
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

            const char16 lineSeparatorChar = 0x2028;
            const char16 paraSeparatorChar = 0x2029;

            if (ch == lineSeparatorChar || ch == paraSeparatorChar)
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

        namespace Internal
        {
            template <typename CharType>
            bool isDigit(_In_ CharType c)
            {
                return c >= '0' && c <= '9';
            }

            template <typename CharType>
            int readNumber(__inout CharType* &str)
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

            template <typename CharType>
            bool isNull(_In_ CharType c)
            {
                return c == '\0';
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
            ///  if either of the strings current character is a null character continue scanning
            /// The algorithm treats the characters in a case-insensitive manner
            ///
            int LogicalStringCompareImpl(const char16* p1, int p1size, const char16* p2, int p2size)
            {
                Assert(p1 != nullptr);
                Assert(p2 != nullptr);

                const char16* str1End = p1 + p1size;
                const char16* str2End = p2 + p2size;

                while (p1 < str1End && p2 < str2End)
                {
                    bool isDigit1 = isDigit(*p1);
                    bool isDigit2 = isDigit(*p2);

                    if (isDigit1 ^ isDigit2)
                    {
                        // If either character exclusively is not a number, compare just the characters
                        // since that's enough to sort the string
                        int c1 = tolower(*p1);
                        int c2 = tolower(*p2);

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

                    while (isNull(*p1) && p1 < str1End)
                    {
                        p1++;
                    }

                    while (isNull(*p2) && p2 < str2End)
                    {
                        p2++;
                    }
                }

                if (*p1 == *p2) return 0;

                return ((*p1 > *p2) ? 1 : -1);
            }
        }; // Internal
    }; // UnicodeText
}; // PlatformAgnostic

// Unnamespaced test code
#if ENABLE_TEST_PLATFORM_AGNOSTIC
void LogicalStringCompareTest(const WCHAR* str1, const WCHAR* str2, int expected)
{
    int compareStringResult = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_DIGITSASNUMBERS, str1, -1, str2, -1);

    if (compareStringResult == 0)
    {
        Output::Print(_u("ERROR: CompareStringW failed with error: %d\n"), ::GetLastError());
        return;
    }

    compareStringResult = compareStringResult - CSTR_EQUAL;

    int res = PlatformAgnostic::UnicodeText::Internal::LogicalStringCompareImpl(str1, str2);
    bool passed = res == expected;

    if (compareStringResult != expected)
    {
        Output::Print(_u("WARNING: Incorrect expected value: %d, actual: %d\n"), expected, compareStringResult);
    }

    if (passed)
    {
        Output::Print(_u("[PASS]\n"));
    }
    else
    {
        Output::Print(_u("[FAIL] Expected: %d, Actual: %d)\n"), expected, res);
    }
}

void doLogicalStringTests()
{
    LogicalStringCompareTest(_u("a"), _u("a"), 0);
    LogicalStringCompareTest(_u("a"), _u("b"), -1);
    LogicalStringCompareTest(_u("b"), _u("a"), 1);

    LogicalStringCompareTest(_u("a"), _u("A"), 0);
    LogicalStringCompareTest(_u("a"), _u("B"), -1);
    LogicalStringCompareTest(_u("b"), _u("A"), 1);

    LogicalStringCompareTest(_u("1"), _u("2"), -1);
    LogicalStringCompareTest(_u("2"), _u("1"), 1);

    LogicalStringCompareTest(_u("10"), _u("01"), 1);
    LogicalStringCompareTest(_u("01"), _u("10"), -1);

    LogicalStringCompareTest(_u("01"), _u("1"), -1);
    LogicalStringCompareTest(_u("1"), _u("01"), 1);

    LogicalStringCompareTest(_u("1a"), _u("a1"), -1);
    LogicalStringCompareTest(_u("aa1"), _u("a1"), 1);
    LogicalStringCompareTest(_u("a1"), _u("a1"), 0);
    LogicalStringCompareTest(_u("a1"), _u("b1"), -1);
    LogicalStringCompareTest(_u("b1"), _u("a1"), 1);
    LogicalStringCompareTest(_u("a1"), _u("a2"), -1);
    LogicalStringCompareTest(_u("a10"), _u("a2"), 1);
    LogicalStringCompareTest(_u("a2"), _u("a10"), -1);

    LogicalStringCompareTest(_u("A1"), _u("a1"), 0);
    LogicalStringCompareTest(_u("A1"), _u("b1"), -1);
    LogicalStringCompareTest(_u("B1"), _u("a1"), 1);
    LogicalStringCompareTest(_u("A1"), _u("a2"), -1);
    LogicalStringCompareTest(_u("A10"), _u("a2"), 1);
    LogicalStringCompareTest(_u("A2"), _u("a10"), -1);

    LogicalStringCompareTest(_u("123"), _u("456"), -1);
    LogicalStringCompareTest(_u("456"), _u("123"), 1);
    LogicalStringCompareTest(_u("abc123"), _u("def123"), -1);
    LogicalStringCompareTest(_u("abc123"), _u("abc123"), 0);
    LogicalStringCompareTest(_u("abc123"), _u("abc0123"), 1);
    LogicalStringCompareTest(_u("abc123"), _u("abc124"), -1);
    LogicalStringCompareTest(_u("abc124"), _u("abc123"), 1);

    LogicalStringCompareTest(_u("abc123def"), _u("abc123def"), 0);
    LogicalStringCompareTest(_u("abc123def"), _u("abc123eef"), -1);
    LogicalStringCompareTest(_u("abc123eef"), _u("abc123def"), 1);

    LogicalStringCompareTest(_u("abc1def"), _u("abc10def"), -1);
    LogicalStringCompareTest(_u("abc1def1"), _u("abc1def12"), -1);

    LogicalStringCompareTest(_u("2string"), _u("3string"), -1);
    LogicalStringCompareTest(_u("20string"), _u("3string"), 1);
    LogicalStringCompareTest(_u("20string"), _u("st2ring"), -1);
    LogicalStringCompareTest(_u("st3ring"), _u("st2ring"), 1);

    LogicalStringCompareTest(_u("2String"), _u("3string"), -1);
    LogicalStringCompareTest(_u("20String"), _u("3string"), 1);
    LogicalStringCompareTest(_u("20sTRing"), _u("st2ring"), -1);
    LogicalStringCompareTest(_u("st3rING"), _u("st2riNG"), 1);
}
#endif
