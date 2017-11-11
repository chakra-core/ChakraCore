//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

extern "C"  int
__cdecl
PAL_wcscmp(
       const char16_t *string1,
       const char16_t *string2);

extern "C" const char16_t *
__cdecl
PAL_wcsstr(
       const char16_t *string,
       const char16_t *strCharSet);

extern "C" int
__cdecl
PAL_wprintf(
        const char16_t*, ...);

#ifndef NULL
#if defined(__cplusplus)
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

int validate(const char16_t *actual, const char16_t *expected, const char16_t *testName)
{
    int c = PAL_wcscmp(expected, actual);
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
    result += validate(PAL_wcsstr(u"abcdefg", u"abcdefg"), u"abcdefg", u"test11");
    result += validate(PAL_wcsstr(u"", u""), u"", u"test12");
    result += validate(PAL_wcsstr(u"", u"def"), NULL, u"test13");
    
    if (result == 0)
    {
        PAL_wprintf(u"SUCCESS");
    }
    return result;
}
