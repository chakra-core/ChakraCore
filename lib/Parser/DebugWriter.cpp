//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "ParserPch.h"

#if ENABLE_REGEX_CONFIG_OPTIONS

namespace UnifiedRegex
{
    const char16* const DebugWriter::hex = _u("0123456789abcdef");

    DebugWriter::DebugWriter() : indent(0), nlPending(false)
    {
    }

#define PRINT_DEBUG_WRITER()                                        \
    va_list argptr;                                                 \
    va_start(argptr, form);                                         \
    int len = _vsnwprintf_s(buf, bufLen, _TRUNCATE, form, argptr);  \
    if (len < 0 || len >= bufLen - 1)                               \
        Output::Print(_u("<not enough buffer space to format>"));   \
    else                                                            \
    {                                                               \
        if (len > 0)                                                \
            CheckForNewline();                                      \
        Output::Print(_u("%s"), buf);                               \
    }                                                               \
    va_end(argptr)

    void __cdecl DebugWriter::Print(const Char *form, ...)
    {
        PRINT_DEBUG_WRITER();
    }

    void __cdecl DebugWriter::PrintEOL(const Char *form, ...)
    {
        PRINT_DEBUG_WRITER();
        EOL();
    }

    void DebugWriter::PrintEscapedString(const Char* str, CharCount len)
    {
        Assert(str != 0);
        CheckForNewline();

        const Char* pl = str + len;
        for (const Char* p = str; p < pl; p++)
        {
            if (*p == '"')
                Output::Print(_u("\\\""));
            else
                PrintEscapedChar(*p);
        }
    }

    void DebugWriter::PrintQuotedString(const Char* str, CharCount len)
    {
        CheckForNewline();
        if (str == 0)
            Output::Print(_u("null"));
        else
        {
            Output::Print(_u("\""));
            PrintEscapedString(str, len);
            Output::Print(_u("\""));
        }
    }

    void DebugWriter::PrintEscapedChar(const Char c)
    {
        CheckForNewline();
        if (c > 0xff)
            Output::Print(_u("\\u%lc%lc%lc%lc"), hex[c >> 12], hex[(c >> 8) & 0xf], hex[(c >> 4) & 0xf], hex[c & 0xf]);
        else if (c < ' ' || c > '~')
            Output::Print(_u("\\x%lc%lc"), hex[c >> 4], hex[c & 0xf]);
        else
            Output::Print(_u("%lc"), c);
    }

    void DebugWriter::PrintQuotedChar(const Char c)
    {
        CheckForNewline();
        Output::Print(_u("'"));
        if (c == '\'')
            Output::Print(_u("\\'"));
        else
            PrintEscapedChar(c);
        Output::Print(_u("'"));
    }

    void DebugWriter::EOL()
    {
        CheckForNewline();
        nlPending = true;
    }

    void DebugWriter::Indent()
    {
        indent++;
    }

    void DebugWriter::Unindent()
    {
        indent--;
    }

    void DebugWriter::Flush()
    {
        Output::Print(_u("\n"));
        Output::Flush();
        nlPending = false;
    }

    void DebugWriter::BeginLine()
    {
        Output::Print(_u("\n%*s"), indent * 4, _u(""));
    }
}

#endif
