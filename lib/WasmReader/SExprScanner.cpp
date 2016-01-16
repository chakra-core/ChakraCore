//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"
#include "SExprScanner.h"

#ifdef ENABLE_WASM

namespace Wasm
{

SExprScanner::SExprScanner(ArenaAllocator * alloc) : m_alloc(alloc)
{
}

void
SExprScanner::Init(SExprParseContext * context, SExprToken * token)
{
    m_context = context;
    m_token = token;
    m_currentChar = m_context->source + m_context->offset;
}

SExprTokenType
SExprScanner::Scan()
{
    if (m_nextToken.tk != wtkNONE)
    {
        CompileAssert(sizeof(int64) >= sizeof(double));
        CompileAssert(sizeof(int64) >= sizeof(LPUTF8));

        m_token->u.lng = m_nextToken.u.lng;
        m_token->tk = m_nextToken.tk;
        m_nextToken.tk = wtkNONE;
        return m_token->tk;
    }
    utf8char_t ch;
    utf8char_t firstChar;
    LPCUTF8 pchT = NULL;
    LPCUTF8 p = m_currentChar;
    LPCUTF8 last = m_context->source + m_context->length;
    if (p >= last)
    {
        goto LEof;
    }
    SExprTokenType token = wtkNONE;

    for (;;)
    {
        ch = *p++;
        switch (ch)
        {
        default:
            AssertMsg(UNREACHED, "UNIMPLEMENTED ERROR");

        case '\0':
            // Put back the null in case we get called again.
            p--;
        LEof:
            token = wtkEOF;
            goto LDone;

        case 0x0009:
        case 0x000B:
        case 0x000C:
        case 0x0020:
            continue;

        case '.':
            if (!Js::NumberUtilities::IsDigit(*p))
            {
                token = wtkDOT;
                goto LDone;
            }
            // May be a double, fall thru
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':

        {
            bool likelyInt = true;

            double dbl = Js::NumberUtilities::StrToDbl((p-1), &pchT, likelyInt);
            if ((p - 1) == pchT)
            {
                AssertMsg(false, "UNIMPLEMENTED");
                //Error(ERRbadNumber);
            }

            p = pchT;

            long value;
            if (likelyInt && Js::NumberUtilities::FDblIsLong(dbl, &value))
            {
                m_token->u.lng = value;
                token = wtkINTLIT;
            }
            else
            {
                m_token->u.dbl = dbl;
                token = wtkFLOATLIT;
            }

            goto LDone;
        }
        case '(':
            token = wtkLPAREN;
            goto LDone;
        case ')':
            token = wtkRPAREN;
            goto LDone;
        case ';':
            if (*p == ';')
            {
                ch = *++p;
                firstChar = ch;
                pchT = NULL;
                for (;;)
                {
                    switch ((ch = *p++))
                    {
                    case '\r':
                    case '\n':
                        p--;
                    LCommentLineBreak:
                        break;
                    case '\0':
                        if (p >= last)
                        {
                            p--;
                            goto LCommentLineBreak;
                        }
                        continue;

                    default:
                        continue;
                    }

                    break;
                }

                continue;
            }
            else
            {
                AssertMsg(UNREACHED, "UNIMPLEMENTED ERROR");
                continue;
            }

        case '\r':
        case '\n':
                continue;

            LKeyword:
                {
                    // We will derive the PID from the token
                    Assert(token < wtkID);
                    goto LDone;
                }

#include "WasmKeywordSwitch.h"

        case '"':
        {
            LPCUTF8 strStart = p;
            size_t length = 0;
            while (*p++ != '"')
            {
                length++;
            }
            LPUTF8 str = AnewArray(m_alloc, utf8char_t, length + 1);
            p = strStart;
            for (size_t i = 0; i < length; i++)
            {
                str[i] = *p++;
            }
            str[length] = '\0';
            p++;
            m_token->u.m_sz = str;
            token = wtkSTRINGLIT;
            goto LDone;
        }
        case '$':
        {
            LPCUTF8 strStart = p;
            size_t length = 1;
            p++;
            for (;;)
            {
                switch (ch = *p++)
                {
                case 'a': case 'b': case 'c': case 'd': case 'e':
                case 'f': case 'g': case 'h': case 'i': case 'j':
                case 'k': case 'l': case 'm': case 'n': case 'o':
                case 'p': case 'q': case 'r': case 's': case 't':
                case 'u': case 'v': case 'w': case 'x': case 'y':
                case 'z':
                case 'A': case 'B': case 'C': case 'D': case 'E':
                case 'F': case 'G': case 'H': case 'I': case 'J':
                case 'K': case 'L': case 'M': case 'N': case 'O':
                case 'P': case 'Q': case 'R': case 'S': case 'T':
                case 'U': case 'V': case 'W': case 'X': case 'Y':
                case 'Z':
                case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                case '_':
                    length++;
                    break;
                default:
                    p--;
                    LPUTF8 str = AnewArray(m_alloc, utf8char_t, length + 1);
                    p = strStart;
                    for (size_t i = 0; i < length; i++)
                    {
                        str[i] = *p++;
                    }
                    str[length] = '\0';
                    p++;
                    m_token->u.m_sz = str;
                    token = wtkID;
                    goto LDone;
                }
            }
        }
        }
    }

LError:
    AssertMsg(UNREACHED, "Unimplemented error");
LDone:
    m_currentChar = p;
    m_token->tk = token;
    return token;
}

void
SExprScanner::UndoScan()
{
    Assert(m_nextToken.tk == wtkNONE);

    m_nextToken.u.lng = m_token->u.lng;
    m_nextToken.tk = m_token->tk;
}

void
SExprScanner::ScanToken(SExprTokenType tok)
{
    if (Scan() != tok)
    {
        SExprParser::ThrowSyntaxError();
    }
}

} // namespace Wasm

#endif // ENABLE_WASM
