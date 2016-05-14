//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

namespace Js
{
    Var UriHelper::EncodeCoreURI(ScriptContext* scriptContext, Arguments& args, unsigned char flags )
    {
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        JavascriptString * strURI;
        //TODO make sure this string is pinned when the memory recycler is in
        if(args.Info.Count < 2)
        {
            strURI = scriptContext->GetLibrary()->GetUndefinedDisplayString();
        }
        else
        {

            if (JavascriptString::Is(args[1]))
            {
                strURI = JavascriptString::FromVar(args[1]);
            }
            else
            {
                strURI = JavascriptConversion::ToString(args[1], scriptContext);
            }
        }
        return Encode(strURI->GetSz(), strURI->GetLength(), flags, scriptContext);
    }

    unsigned char UriHelper::s_uriProps[128] =
    {
        //0x00  0x01  0x02  0x03  0x04  0x05  0x06  0x07  0x08  0x09  0x0a  0x0b  0x0c  0x0d  0x0e  0x0f
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        //0x10  0x11  0x12  0x13  0x14  0x15  0x16  0x17  0x18  0x19  0x1a  0x1b  0x1c  0x1d  0x1e  0x1f
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        //         !     "     #     $     %     &     '     (     )     *     +     ,     -     .     /
        0, 0x02,    0, 0x01, 0x01,    0, 0x01, 0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 0x02, 0x02, 0x01,
        //   0     1     2     3     4     5     6     7     8     9     :     ;     <     =     >     ?
        0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x01, 0x01,    0, 0x01,    0, 0x01,
        //   @     A     B     C     D     E     F     G     H     I     J     K     L     M     N     O
        0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
        //   P     Q     R     S     T     U     V     W     X     Y     Z     [     \     ]     ^     _
        0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,    0,    0,    0,    0, 0x02,
        //   `     a     b     c     d     e     f     g     h     i     j     k     l     m     n     o
        0, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
        //   p     q     r     s     t     u     v     w     x     y     z     {     |     }     ~  0x7f
        0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,    0,    0,    0, 0x02,    0,
    };

    // Convert 'uVal' to it's UTF-8 encoding in the array 'bUTF8'. Returns
    // the number of characters in the output array.
    // This routine assumes that it's input 'uVal' is a valid Unicode code-point value
    // and does no error checking.
    uint32 UriHelper::ToUTF8( uint32 uVal, BYTE bUTF8[MaxUTF8Len])
    {
        uint32 uRet;
        if( uVal <= 0x007F )
        {
            bUTF8[0] = (BYTE)uVal;
            uRet = 1;
        }
        else if( uVal <= 0x07FF )
        {
            uint32 z = uVal & 0x3F;
            uint32 y = uVal >> 6;
            bUTF8[0] = (BYTE) (0xC0 | y);
            bUTF8[1] = (BYTE) (0x80 | z);
            uRet = 2;
        }
        else if( uVal <= 0xFFFF )
        {
            Assert( uVal <= 0xD7FF || uVal >= 0xE000 );
            uint32 z = uVal & 0x3F;
            uint32 y = (uVal >> 6) & 0x3F;
            uint32 x = (uVal >> 12);
            bUTF8[0] = (BYTE) (0xE0 | x);
            bUTF8[1] = (BYTE) (0x80 | y);
            bUTF8[2] = (BYTE) (0x80 | z);
            uRet = 3;
        }
        else
        {
            uint32 z = uVal & 0x3F;
            uint32 y = (uVal >> 6) &0x3F;
            uint32 x = (uVal >> 12) &0x3F;
            uint32 w = (uVal >> 18);
            bUTF8[0] = (BYTE) (0xF0 | w);
            bUTF8[1] = (BYTE) (0x80 | x);
            bUTF8[2] = (BYTE) (0x80 | y);
            bUTF8[3] = (BYTE) (0x80 | z);
            uRet = 4;
        }

        return uRet;
    }

    // Return the Unicode code-point value of the UTF-8 encoding passed in as the
    // array 'bUTF8'. uLen is the number of characters in the UTF-8 encoding.
    // This routine assumes that a valid UTF-8 encoding of a character is passed in
    // and does no error checking.
    uint32 UriHelper::FromUTF8( BYTE bUTF8[MaxUTF8Len], uint32 uLen )
    {
        Assert( 1 <= uLen && uLen <= MaxUTF8Len );
        if( uLen == 1 )
        {
            return bUTF8[0];
        }
        else if( uLen == 2 )
        {
            return ((bUTF8[0] & 0x1F) << 6 ) | (bUTF8[1] & 0x3F);
        }
        else if( uLen == 3 )
        {
            return ((bUTF8[0] & 0x0F) << 12) | ((bUTF8[1] & 0x3F) << 6) | (bUTF8[2] & 0x3F);
        }
        else
        {
            Assert( uLen == 4 );
            return ((bUTF8[0] & 0x07) << 18) | ((bUTF8[1] & 0x3F) << 12) | ((bUTF8[2] & 0x3F) << 6 ) | (bUTF8[3] & 0x3F) ;
        }
    }

    // The Encode algorithm described in sec. 15.1.3 of the spec. The input string is
    // 'pSz' and the Unescaped set is described by the flags 'unescapedFlags'. The
    // output is a string var.
    Var UriHelper::Encode(__in_ecount(len) const  char16* pSz, uint32 len, unsigned char unescapedFlags, ScriptContext* scriptContext )
    {
        BYTE bUTF8[MaxUTF8Len];

        // pass 1 calculate output length and error check
        uint32 outputLen = 0;
        for( uint32 k = 0; k < len; k++ )
        {
            char16 c = pSz[k];
            uint32 uVal;
            if( InURISet(c, unescapedFlags) )
            {
                outputLen = UInt32Math::Add(outputLen, 1);
            }
            else
            {
                if( c >= 0xDC00 && c <= 0xDFFF )
                {
                    JavascriptError::ThrowURIError(scriptContext, JSERR_URIEncodeError /* TODO-ERROR: _u("NEED MESSAGE") */);
                }
                else if( c < 0xD800 || c > 0xDBFF )
                {
                    uVal = (uint32)c;
                }
                else
                {
                    ++k;
                    if(k == len)
                    {
                        JavascriptError::ThrowURIError(scriptContext, JSERR_URIEncodeError /* TODO-ERROR: _u("NEED MESSAGE") */);
                    }
                    __analysis_assume(k < len); // because we throw exception if k==len
                    char16 c1 = pSz[k];
                    if( c1 < 0xDC00 || c1 > 0xDFFF )
                    {
                        JavascriptError::ThrowURIError(scriptContext, JSERR_URIEncodeError /* TODO-ERROR: _u("NEED MESSAGE") */);
                    }
                    uVal = (c - 0xD800) * 0x400 + (c1 - 0xDC00) + 0x10000;
                }
                uint32 utfLen = ToUTF8(uVal, bUTF8);
                utfLen = UInt32Math::Mul(utfLen, 3);
                outputLen = UInt32Math::Add(outputLen, utfLen);
            }
        }

        //pass 2 generate the encoded URI

        uint32 allocSize = UInt32Math::Add(outputLen, 1);
        char16* outURI = RecyclerNewArrayLeaf(scriptContext->GetRecycler(), char16, allocSize);
        char16* outCurrent = outURI;

        for( uint32 k = 0; k < len; k++ )
        {
            char16 c = pSz[k];
            uint32 uVal;
            if( InURISet(c, unescapedFlags) )
            {
                __analysis_assume(outCurrent < outURI + allocSize);
                *outCurrent++ = c;
            }
            else
            {
#if DBG
                if( c >= 0xDC00 && c <= 0xDFFF )
                {
                    JavascriptError::ThrowURIError(scriptContext, VBSERR_InternalError /* TODO-ERROR: _u("NEED MESSAGE") */);
                }
#endif
                if( c < 0xD800 || c > 0xDBFF )
                {
                    uVal = (uint32)c;
                }
                else
                {
                    ++k;
#if DBG
                    if(k == len)
                    {
                        JavascriptError::ThrowURIError(scriptContext, VBSERR_InternalError /* TODO-ERROR: _u("NEED MESSAGE") */);
                    }
#endif
                    __analysis_assume(k < len);// because we throw exception if k==len
                    char16 c1 = pSz[k];

#if DBG
                    if( c1 < 0xDC00 || c1 > 0xDFFF )
                    {
                        JavascriptError::ThrowURIError(scriptContext, VBSERR_InternalError /* TODO-ERROR: _u("NEED MESSAGE") */);
                    }
#endif
                    uVal = (c - 0xD800) * 0x400 + (c1 - 0xDC00) + 0x10000;
                }

                uint32 utfLen = ToUTF8(uVal, bUTF8);
                for( uint32 j = 0; j < utfLen; j++ )
                {
#pragma prefast(suppress: 26014, "buffer length was calculated earlier");
                    swprintf_s(outCurrent, 4, _u("%%%02X"), (int)bUTF8[j] );
                    outCurrent +=3;
#pragma prefast(default: 26014);
                }
            }
        }
        AssertMsg(outURI + outputLen == outCurrent, " URI out buffer out of sync");
        __analysis_assume(outputLen + 1 == allocSize);
        outURI[outputLen] = _u('\0');

        return JavascriptString::NewCopyBuffer(outURI, outputLen, scriptContext);
    }

    Var UriHelper::DecodeCoreURI(ScriptContext* scriptContext, Arguments& args, unsigned char reservedFlags )
    {
        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
        JavascriptString * strURI;
        //TODO make sure this string is pinned when the memory recycler is in
        if(args.Info.Count < 2)
        {
            strURI = scriptContext->GetLibrary()->GetUndefinedDisplayString();
        }
        else
        {

            if (JavascriptString::Is(args[1]))
            {
                strURI = JavascriptString::FromVar(args[1]);
            }
            else
            {
                strURI = JavascriptConversion::ToString(args[1], scriptContext);
            }
        }
        return Decode(strURI->GetSz(), strURI->GetLength(), reservedFlags, scriptContext);
    }

    // The Decode algorithm described in sec. 15.1.3 of the spec. The input string is
    // 'pSZ' and the Reserved set is described by the flags 'reservedFlags'. The
    // output is a string var.
    Var UriHelper::Decode(__in_ecount(len) const char16* pSz, uint32 len, unsigned char reservedFlags, ScriptContext* scriptContext)
    {
        char16 c1;
        char16 c;
        // pass 1 calculate output length and error check
        uint32 outputLen = 0;
        for( uint32 k = 0; k < len; k++ )
        {
            c = pSz[k];

            if( c == '%')
            {
                uint32 start = k;
                if( k + 2 >= len )
                {
                    JavascriptError::ThrowURIError(scriptContext, JSERR_URIDecodeError /* TODO-ERROR: _u("NEED MESSAGE") */);
                }

                // %-encoded components in a URI may only contain hexadecimal digits from the ASCII character set. 'swscanf_s'
                // only supports those characters when decoding hexadecimal integers. 'iswxdigit' on the other hand, uses the
                // current locale to see if the specified character maps to a hexadecimal digit, which causes it to consider some
                // characters outside the ASCII character set to be hexadecimal digits, so we can't use that. 'swscanf_s' seems
                // to be overkill for this, so using a simple function that parses two hex digits and produces their value.
                BYTE b;
                if(!DecodeByteFromHex(pSz[k + 1], pSz[k + 2], b))
                {
                    JavascriptError::ThrowURIError(scriptContext, JSERR_URIDecodeError);
                }

                k += 2;

                if( (b & 0x80) ==  0)
                {
                    c1 = b;
                }
                else
                {
                    int n;
                    for( n = 1; ((b << n) & 0x80) != 0; n++ )
                        ;

                    if( n == 1 || n > UriHelper::MaxUTF8Len )
                    {
                        JavascriptError::ThrowURIError(scriptContext, JSERR_URIDecodeError /* TODO-ERROR: _u("NEED MESSAGE") */);
                    }

                    BYTE bOctets[UriHelper::MaxUTF8Len];
                    bOctets[0] = b;

                    if( k + 3 * (n-1) >= len )
                    {
                        JavascriptError::ThrowURIError(scriptContext, JSERR_URIDecodeError /* TODO-ERROR: _u("NEED MESSAGE") */);
                    }

                    for( int j = 1; j < n; j++ )
                    {
                        if( pSz[++k] != '%' )
                        {
                            JavascriptError::ThrowURIError(scriptContext, JSERR_URIDecodeError /* TODO-ERROR: _u("NEED MESSAGE") */);
                        }

                        if(!DecodeByteFromHex(pSz[k + 1], pSz[k + 2], b))
                        {
                            JavascriptError::ThrowURIError(scriptContext, JSERR_URIDecodeError /* TODO-ERROR: _u("NEED MESSAGE") */);
                        }

                        // The two leading bits should be 10 for a valid UTF-8 encoding
                        if( (b & 0xC0) != 0x80)
                        {
                            JavascriptError::ThrowURIError(scriptContext, JSERR_URIDecodeError /* TODO-ERROR: _u("NEED MESSAGE") */);
                        }
                        k += 2;

                        bOctets[j] = b;
                    }

                    uint32 uVal = UriHelper::FromUTF8( bOctets, n );

                    if( uVal >= 0xD800 && uVal <= 0xDFFF)
                    {
                        JavascriptError::ThrowURIError(scriptContext, JSERR_URIDecodeError /* TODO-ERROR: _u("NEED MESSAGE") */);
                    }
                    if( uVal < 0x10000 )
                    {
                        c1 = (char16)uVal;
                    }
                    else if( uVal > 0x10ffff )
                    {
                        JavascriptError::ThrowURIError(scriptContext, JSERR_URIDecodeError /* TODO-ERROR: _u("NEED MESSAGE") */);
                    }
                    else
                    {
                        outputLen +=2;
                        continue;
                    }
                }

                if( ! UriHelper::InURISet( c1, reservedFlags ))
                {
                    outputLen++;
                }
                else
                {
                    outputLen += k - start + 1;
                }
            }
            else // c is not '%'
            {
                outputLen++;
            }
        }

        //pass 2 generate the decoded URI
        uint32 allocSize = UInt32Math::Add(outputLen, 1);
        char16* outURI = RecyclerNewArrayLeaf(scriptContext->GetRecycler(), char16, allocSize);
        char16* outCurrent = outURI;


        for( uint32 k = 0; k < len; k++ )
        {
            c = pSz[k];
            if( c == '%')
            {
                uint32 start = k;
#if DBG
                Assert(!(k + 2 >= len));
                if( k + 2 >= len )
                {
                    JavascriptError::ThrowURIError(scriptContext, VBSERR_InternalError /* TODO-ERROR: _u("NEED MESSAGE") */);
                }
#endif
                // Let OACR know some things about 'k' that we checked just above, to let it know that we are not going to
                // overflow later. The same checks are done in the first pass in non-debug builds, and the conditions
                // checked upon in the first and second pass are the same.
                __analysis_assume(!(k + 2 >= len));

                BYTE b;
                if(!DecodeByteFromHex(pSz[k + 1], pSz[k + 2], b))
                {
#if DBG
                    AssertMsg(false, "!DecodeByteFromHex(pSz[k + 1], pSz[k + 2], b)");
                    JavascriptError::ThrowURIError(scriptContext, VBSERR_InternalError /* TODO-ERROR: _u("NEED MESSAGE") */);
#endif
                }

                k += 2;

                if( (b & 0x80) ==  0)
                {
                    c1 = b;
                }
                else
                {
                    int n;
                    for( n = 1; ((b << n) & 0x80) != 0; n++ )
                        ;

                    if( n == 1 || n > UriHelper::MaxUTF8Len )
                    {
                        JavascriptError::ThrowURIError(scriptContext, VBSERR_InternalError /* TODO-ERROR: _u("NEED MESSAGE") */);
                    }

                    BYTE bOctets[UriHelper::MaxUTF8Len];
                    bOctets[0] = b;

#if DBG
                    Assert(!(k + 3 * (n-1) >= len));
                    if( k + 3 * (n-1) >= len )
                    {
                        JavascriptError::ThrowURIError(scriptContext, VBSERR_InternalError /* TODO-ERROR: _u("NEED MESSAGE") */);
                    }
#endif
                    // Let OACR know some things about 'k' that we checked just above, to let it know that we are not going to
                    // overflow later. The same checks are done in the first pass in non-debug builds, and the conditions
                    // checked upon in the first and second pass are the same.
                    __analysis_assume(!(k + 3 * (n-1) >= len));

                    for( int j = 1; j < n; j++ )
                    {
                        ++k;

#if DBG
                        Assert(!(pSz[k] != '%'));
                        if( pSz[k] != '%' )
                        {
                            JavascriptError::ThrowURIError(scriptContext, VBSERR_InternalError /* TODO-ERROR: _u("NEED MESSAGE") */);
                        }
#endif

                        if(!DecodeByteFromHex(pSz[k + 1], pSz[k + 2], b))
                        {
#if DBG
                            AssertMsg(false, "!DecodeByteFromHex(pSz[k + 1], pSz[k + 2], b)");
                            JavascriptError::ThrowURIError(scriptContext, VBSERR_InternalError /* TODO-ERROR: _u("NEED MESSAGE") */);
#endif
                        }

#if DBG
                        // The two leading bits should be 10 for a valid UTF-8 encoding
                        Assert(!((b & 0xC0) != 0x80));
                        if( (b & 0xC0) != 0x80)
                        {
                            JavascriptError::ThrowURIError(scriptContext, VBSERR_InternalError /* TODO-ERROR: _u("NEED MESSAGE") */);
                        }
#endif

                        k += 2;

                        bOctets[j] = b;
                    }

                    uint32 uVal = UriHelper::FromUTF8( bOctets, n );

#if DBG
                    Assert(!(uVal >= 0xD800 && uVal <= 0xDFFF));
                    if( uVal >= 0xD800 && uVal <= 0xDFFF)
                    {
                        JavascriptError::ThrowURIError(scriptContext, VBSERR_InternalError /* TODO-ERROR: _u("NEED MESSAGE") */);
                    }
#endif

                    if( uVal < 0x10000 )
                    {
                        c1 = (char16)uVal;
                    }

#if DBG
                    else if( uVal > 0x10ffff )
                    {
                        AssertMsg(false, "uVal > 0x10ffff");
                        JavascriptError::ThrowURIError(scriptContext, VBSERR_InternalError /* TODO-ERROR: _u("NEED MESSAGE") */);
                    }
#endif
                    else
                    {
                        uint32 l = (( uVal - 0x10000) & 0x3ff) + 0xdc00;
                        uint32 h = ((( uVal - 0x10000) >> 10) & 0x3ff) + 0xd800;

                        __analysis_assume(outCurrent + 2 <= outURI + allocSize);
                        *outCurrent++ = (char16)h;
                        *outCurrent++ = (char16)l;
                        continue;
                    }
                }

                if( !UriHelper::InURISet( c1, reservedFlags ))
                {
                    __analysis_assume(outCurrent < outURI + allocSize);
                    *outCurrent++ = c1;
                }
                else
                {
                    js_memcpy_s(outCurrent, (allocSize - (outCurrent - outURI)) * sizeof(char16), &pSz[start], (k - start + 1)*sizeof(char16));
                    outCurrent += k - start + 1;
                }
            }
            else // c is not '%'
            {
                __analysis_assume(outCurrent < outURI + allocSize);
                *outCurrent++ = c;
            }
        }

        AssertMsg(outURI + outputLen == outCurrent, " URI out buffer out of sync");
        __analysis_assume(outputLen + 1 == allocSize);
        outURI[outputLen] = _u('\0');

        return JavascriptString::NewCopyBuffer(outURI, outputLen, scriptContext);
    }

    // Decodes a two-hexadecimal-digit wide character pair into the byte value it represents
    bool UriHelper::DecodeByteFromHex(const char16 digit1, const char16 digit2, unsigned char &value)
    {
        int x;
        if(!Js::NumberUtilities::FHexDigit(digit1, &x))
        {
            return false;
        }
        Assert(static_cast<unsigned int>(x) <= 0xfU);
        value = static_cast<unsigned char>(x) << 4;

        if(!Js::NumberUtilities::FHexDigit(digit2, &x))
        {
            return false;
        }
        Assert(static_cast<unsigned int>(x) <= 0xfU);
        value += static_cast<unsigned char>(x);

        return true;
    }
}
