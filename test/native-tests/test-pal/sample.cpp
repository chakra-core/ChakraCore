//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "pal.h"

int wscmp(const char16_t *left, const char16_t *right)
{
    if (!left && !right)
    {
        return 0;
    }
    else if (left && right)
    {
        while (*left != u'\0' && *right != u'\0')
        {
            int diff = *right - *left;
            if (diff != 0)
            {
                return diff;
            }
            ++left;
            ++right;
        }

        int diff = *right - *left;
        return diff;
    }
    else
    {
        return (left == NULL ? -1 : 1);
    }
}

int validate(const char16_t *actual, const char16_t *expected, const char16_t *testName)
{
    int c = wscmp(expected, actual);
    if (c == 0)
    {
        return 0;
    }
    else
    {
        PAL_wprintf(u"Test Failed: %s\n", testName);
        return 1;
    }
}

int main()
{
    int result = 0;

    result += validate(PAL_wcsstr(u"targetstringxxx", u"searchStringxxx"), NULL, u"test1");
    result += validate(PAL_wcsstr(u"abcdefg", u"def"), u"defg", u"test2");
    result += validate(PAL_wcsstr(u"abcdefg", u"a"), u"abcdefg", u"test3");
    result += validate(PAL_wcsstr(u"abcdefg", u"g"), u"g", u"test4");
    result += validate(PAL_wcsstr(u"abcdefg", u"gh"), NULL, u"test5");
    result += validate(PAL_wcsstr(u"abcdefg", u"ah"), NULL, u"test6");
    result += validate(PAL_wcsstr((const char16_t *)NULL, (const char16_t *)NULL), NULL, u"test7");
    result += validate(PAL_wcsstr((const char16_t *)NULL, u""), NULL, u"test8");
    result += validate(PAL_wcsstr(u"", (const char16_t *)NULL), NULL, u"test9");
    result += validate(PAL_wcsstr(u"abcdefg", u""), u"abcdefg", u"test10");

    if (result == 0)
    {
        printf("SUCCESS");
    }
    return result;
}
