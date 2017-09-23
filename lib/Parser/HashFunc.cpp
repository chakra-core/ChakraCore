//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
/*
 * IMPORTANT:
 *  This file does not compile stand alone. It was required so that
 *  the same code could be built into a utility program comphash.exe as well
 *  as the scripting dll's. This file is included in core\comphash.cpp
 *  to be used by comphash.exe. It is included in core\scrutil.cpp where to
 *  be used by jscript.dll and vbscript.dll.
 *
 *  comphash.exe is a utility used in the build to generate a source code file
 *  containing a table of hash values associated with strings needed by
 *  jscript and vbscript. It is highly desirable to have a single definition
 *  of the hash function so things don't go out of sync.
 */

ULONG CaseSensitiveComputeHash(LPCOLESTR prgch, LPCOLESTR end)
{
    ULONG luHash = 0;

    while (prgch < end)
    {
        luHash = 17 * luHash + *(char16 *)prgch++;
    }
    return luHash;
}

ULONG CaseSensitiveComputeHash(LPCUTF8 prgch, LPCUTF8 end)
{
    utf8::DecodeOptions options = utf8::doAllowThreeByteSurrogates;
    ULONG luHash = 0;

    while (prgch < end)
    {
        luHash = 17 * luHash + utf8::Decode(prgch, end, options);
    }
    return luHash;
}

ULONG CaseSensitiveComputeHash(char const * prgch, char const * end)
{
    ULONG luHash = 0;

    while (prgch < end)
    {
        Assert(utf8::IsStartByte(*prgch) && !utf8::IsLeadByte(*prgch));
        luHash = 17 * luHash + *prgch++;
    }
    return luHash;
}

ULONG CaseInsensitiveComputeHash(LPCOLESTR posz)
{
    ULONG luHash = 0;
    char16 ch;
    while (0 != (ch = *(char16 *)posz++))
    {
        if (ch <= 'Z' && ch >= 'A')
            ch += 'a' - 'A';
        luHash = 17 * luHash + ch;
    }
    return luHash;
}
