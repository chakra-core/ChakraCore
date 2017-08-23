//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Utf8Codex.h"

#ifndef _WIN32
#undef _Analysis_assume_
#define _Analysis_assume_(expr)
#endif

#ifdef _MSC_VER
//=============================
// Disabled Warnings
//=============================

#pragma warning(push)

#pragma warning(disable: 4127)  // constant expression for template parameter
#endif

extern void CodexAssert(bool condition);

namespace utf8
{
    const unsigned int mAlignmentMask = 0x3;

    inline bool IsAligned(LPCUTF8 pch)
    {
        return (reinterpret_cast<size_t>(pch) & mAlignmentMask) == 0;
    }

    inline bool IsAligned(LPCOLESTR pch)
    {
        return (reinterpret_cast<size_t>(pch) & mAlignmentMask) == 0;
    }

    inline bool ShouldFastPath(LPCUTF8 pb, LPCOLESTR pch)
    {
        return (reinterpret_cast<size_t>(pb) & mAlignmentMask) == 0 && (reinterpret_cast<size_t>(pch) & mAlignmentMask) == 0;
    }

    inline size_t EncodedBytes(char16 prefix)
    {
         CodexAssert(0 == (prefix & 0xFF00)); // prefix must really be a byte. We use char16 for as a convenience for the API.

        // The number of bytes in an UTF8 encoding is determined by the 4 high-order bits of the first byte.
        // 0xxx -> 1
        // 10xx -> 1 (invalid)
        // 110x -> 2
        // 1110 -> 3
        // 1111 -> 4

        // If this value is XOR with 0xF0 and shift 3 bits to the right it can be used as an
        // index into a 16 element 2 bit array encoded as a uint32 of n - 1 where n is the number
        // of bits in the encoding.

        // The XOR prefix bits mapped to n - 1.
        // 1xxx -> 00 (8 - 15)
        // 01xx -> 00 (4  - 7)
        // 001x -> 01 (2  - 3)
        // 0001 -> 10 (1)
        // 0000 -> 11 (0)

        // This produces the following bit sequence:
        //  15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
        //  00 00 00 00 00 00 00 00 00 00 00 00 01 01 10 11
        // which is 0x5B

        return ((0x5B >> (((prefix ^ 0xF0) >> 3) & 0x1E)) & 0x03) + 1;
    }

    const char16 WCH_UTF16_HIGH_FIRST  =  char16(0xd800);
    const char16 WCH_UTF16_HIGH_LAST   =  char16(0xdbff);
    const char16 WCH_UTF16_LOW_FIRST   =  char16(0xdc00);
    const char16 WCH_UTF16_LOW_LAST    =  char16(0xdfff);

    char16 GetUnknownCharacter(DecodeOptions options = doDefault)
    {
        if ((options & doThrowOnInvalidWCHARs) != 0)
        {
            throw InvalidWideCharException();
        }
        return char16(UNICODE_UNKNOWN_CHAR_MARK);
    }

    inline BOOL InRange(const char16 ch, const char16 chMin, const char16 chMax)
    {
        return (unsigned)(ch - chMin) <= (unsigned)(chMax - chMin);
    }

    BOOL IsValidWideChar(char16 ch)
    {
        return (ch < 0xfdd0) || ((ch > 0xfdef) && (ch <= 0xffef)) || ((ch >= 0xfff9) && (ch <= 0xfffd));
    }

    inline BOOL IsHighSurrogateChar(char16 ch)
    {
        return InRange( ch, WCH_UTF16_HIGH_FIRST, WCH_UTF16_HIGH_LAST );
    }

    inline BOOL IsLowSurrogateChar(char16 ch)
    {
        return InRange( ch, WCH_UTF16_LOW_FIRST, WCH_UTF16_LOW_LAST );
    }

    _At_(ptr, _In_reads_(end - ptr) _Post_satisfies_(ptr >= _Old_(ptr) - 1 && ptr <= end))
    inline char16 DecodeTail(char16 c1, LPCUTF8& ptr, LPCUTF8 end, DecodeOptions& options, bool *chunkEndsAtTruncatedSequence)
    {
        char16 ch = 0;
        BYTE c2, c3, c4;

        switch (EncodedBytes(c1))
        {
        case 1:
            if (c1 < 0x80) return c1;

            if ((options & doSecondSurrogatePair) != 0)
            {
                // We're in the middle of decoding a surrogate pair from a four-byte utf8 sequence.
                // The high word has already been returned, but without advancing ptr, which was on byte 1.
                // ptr was then advanced externally when reading c1, which is byte 1, so ptr is now on byte 2.
                // byte 1 must have been a continuation byte, hence will be in case 1.
                ptr--; // back to byte 1
                c1 = ptr[-1]; // the original first byte

                // ptr is now on c2. We must also have c3 and c4, otherwise doSecondSurrogatePair won't set.
                _Analysis_assume_(ptr + 2 < end);
                goto LFourByte;
            }

            // 10xxxxxx (trail byte appearing in a lead byte position
            return GetUnknownCharacter(options);

        case 2:
            // Look for an overlong utf-8 sequence.
            if (ptr >= end)
            {
                if ((options & doChunkedEncoding) != 0)
                {
                    // The is a sequence that spans a chunk, push ptr back to the beginning of the sequence.
                    ptr--;

                    if (chunkEndsAtTruncatedSequence)
                    {
                        *chunkEndsAtTruncatedSequence = true;
                    }
                }
                return GetUnknownCharacter(options);
            }
            c2 = *ptr++;
            // 110XXXXx 10xxxxxx
            //      UTF16       |   UTF8 1st byte  2nd byte
            // U+0080..U+07FF   |    C2..DF         80..BF
            if (
                    InRange(c1, 0xC2, 0xDF)
                    && InRange(c2, 0x80, 0xBF)
                )
            {
                ch |= WCHAR(c1 & 0x1f) << 6;     // 0x0080 - 0x07ff
                ch |= WCHAR(c2 & 0x3f);
                if (!IsValidWideChar(ch) && ((options & doAllowInvalidWCHARs) == 0))
                {
                    ch = GetUnknownCharacter(options);
                }
            }
            else
            {
                ptr--;
                ch = GetUnknownCharacter(options);
            }
            break;

        case 3:
            // 1110XXXX 10Xxxxxx 10xxxxxx
            // Look for overlong utf-8 sequence.
            if (ptr + 1 >= end)
            {
                if ((options & doChunkedEncoding) != 0)
                {
                    // The is a sequence that spans a chunk, push ptr back to the beginning of the sequence.
                    ptr--;

                    if (chunkEndsAtTruncatedSequence)
                    {
                        *chunkEndsAtTruncatedSequence = true;
                    }
                }

                return GetUnknownCharacter(options);
            }

            //      UTF16       |   UTF8 1st byte  2nd byte 3rd byte
            // U+0800..U+0FFF   |    E0             A0..BF   80..BF
            // U+1000..U+CFFF   |    E1..EC         80..BF   80..BF
            // U+D000..U+D7FF   |    ED             80..9F   80..BF
            // U+E000..U+FFFF   |    EE..EF         80..BF   80..BF
            c2 = ptr[0];
            c3 = ptr[1];
            if (
                // any following be true
                    (c1 == 0xE0
                    && InRange(c2, 0xA0, 0xBF)
                    && InRange(c3, 0x80, 0xBF))
                    ||
                    (InRange(c1, 0xE1, 0xEC)
                    && InRange(c2, 0x80, 0xBF)
                    && InRange(c3, 0x80, 0xBF))
                    ||
                    (c1 == 0xED
                    && InRange(c2, 0x80, 0x9F)
                    && InRange(c3, 0x80, 0xBF))
                    ||
                    (InRange(c1, 0xEE, 0xEF)
                    && InRange(c2, 0x80, 0xBF)
                    && InRange(c3, 0x80, 0xBF))
                    ||
                    (((options & doAllowThreeByteSurrogates) != 0)
                    &&
                    c1 == 0xED
                    && InRange(c2, 0x80, 0xBF)
                    && InRange(c3, 0x80, 0xBF)
                    )
                )
            {
                ch  = WCHAR(c1 & 0x0f) << 12;    // 0x0800 - 0xffff
                ch |= WCHAR(c2 & 0x3f) << 6;     // 0x0080 - 0x07ff
                ch |= WCHAR(c3 & 0x3f);
                if (!IsValidWideChar(ch) && ((options & (doAllowThreeByteSurrogates | doAllowInvalidWCHARs)) == 0))
                {
                    ch = GetUnknownCharacter(options);
                }
                ptr += 2;
            }
            else
            {
                ch = GetUnknownCharacter(options);
                // Windows OS 1713952. Only drop the illegal leading byte
                // Retry next byte.
                // ptr is already advanced.
            }
            break;

        case 4:
LFourByte:
            // 11110XXX 10XXxxxx 10xxxxxx 10xxxxxx or 11111xxx ....
            // NOTE: 11111xxx is not supported
            if (ptr + 2 >= end)
            {
                if ((options & doChunkedEncoding) != 0)
                {
                    // The is a sequence that spans a chunk, push ptr back to the beginning of the sequence.
                    ptr--;

                    if (chunkEndsAtTruncatedSequence)
                    {
                        *chunkEndsAtTruncatedSequence = true;
                    }
                }

                ch = GetUnknownCharacter(options);
                break;
            }

            c2 = ptr[0];
            c3 = ptr[1];
            c4 = ptr[2];

            //      UTF16         |   UTF8 1st byte  2nd byte 3rd byte 4th byte
            // U+10000..U+3FFFF   |    F0             90..BF   80..BF   80..BF
            // U+40000..U+FFFFF   |    F1..F3         80..BF   80..BF   80..BF
            // U+100000..U+10FFFF |    F4             80..8F   80..BF   80..BF
            if (! // NOT Unicode well-formed byte sequences
                    (
                    // any following be true
                        (c1 == 0xF0
                        && InRange(c2, 0x90,0xBF)
                        && InRange(c3, 0x80,0xBF)
                        && InRange(c4, 0x80,0xBF))
                    ||
                        (InRange(c1, 0xF1, 0xF3)
                        && InRange(c2, 0x80,0xBF)
                        && InRange(c3, 0x80,0xBF)
                        && InRange(c4, 0x80,0xBF))
                    ||
                        (c1 == 0xF4
                        && InRange(c2, 0x80,0x8F)
                        && InRange(c3, 0x80,0xBF)
                        && InRange(c4, 0x80,0xBF))
                    )
                )
            {
                // Windows OS 1713952. Only drop the illegal leading byte.
                // Retry next byte.
                // ptr is already advanced 1.
                ch = GetUnknownCharacter(options);
                break;
            }

            if ((options & doSecondSurrogatePair) == 0)
            {
                // Decode high 10 bits of utf-8 20 bit char
                ch  = WCHAR(c1 & 0x07) << 2;
                ch |= WCHAR(c2 & 0x30) >> 4;
                ch  = (ch - 1) << 6;             // ch == 0000 00ww ww00 0000
                ch |= WCHAR(c2 & 0x0f) << 2;     // ch == 0000 00ww wwzz zz00
                ch |= WCHAR(c3 & 0x30) >> 4;     // ch == 0000 00ww wwzz zzyy
                // Encode first word of utf-16 surrogate pair
                ch += 0xD800;
                // Remember next call must return second word
                options = (DecodeOptions)(options | doSecondSurrogatePair);
                // Leave ptr on byte 1, this way:
                //  - callers who test that ptr has been advanced by utf8::Decode will see progress for
                //    both words of the surrogate pair.
                //  - callers who calculate the number of multi-unit chars by subtracting after from before ptr
                //    will accumulate 0 for first word and 2 for second, thus utf8 chars equals 2 utf16 chars + 2
                //    multi-unit chars, as it should be.
            }
            else
            {
                // Decode low 10 bits of utf-8 20 bit char
                ch = WCHAR(c3 & 0x0f) << 6;     // ch == 0000 00yy yy00 0000
                ch |= WCHAR(c4 & 0x3f);          // ch == 0000 00yy yyxx xxxx
                // Encode second word of utf-16 surrogate pair
                ch += 0xDC00;
                // We're done with this char
                options = (DecodeOptions)(options & ~doSecondSurrogatePair);
                ptr += 3; // remember, got here by subtracting one from ptr in case 1, so effective increment is 2
            }
            break;
        }

        return ch;
    }

    LPUTF8 EncodeFull(char16 ch, __out_ecount(3) LPUTF8 ptr)
    {
        if( ch < 0x0080 )
        {
            // One byte
            *ptr++ = static_cast< utf8char_t >(ch);
        }
        else if( ch < 0x0800 )
        {
            // Two bytes   : 110yyyxx 10xxxxxx
            *ptr++ = static_cast<utf8char_t>(ch >> 6) | 0xc0;
            *ptr++ = static_cast<utf8char_t>(ch & 0x3F) | 0x80;
        }
        else
        {
            // Three bytes : 1110yyyy 10yyyyxx 10xxxxxx
            *ptr++ = static_cast<utf8char_t>(ch >> 12) | 0xE0;
            *ptr++ = static_cast<utf8char_t>((ch >> 6) & 0x3F) | 0x80;
            *ptr++ = static_cast<utf8char_t>(ch & 0x3F) | 0x80;
        }

        return ptr;
    }

    _Use_decl_annotations_
    LPUTF8 EncodeSurrogatePair(char16 surrogateHigh, char16 surrogateLow, LPUTF8 ptr)
    {
        // A unicode codepoint is encoded into a surrogate pair by doing the following:
        //  subtract 0x10000 from the codepoint
        //  Split the resulting value into the high-ten bits and low-ten bits
        //  Add 0xD800 to the high ten bits, and 0xDC00 to the low ten bits
        // Below, we want to decode the surrogate pair to its original codepoint
        // So we do the above process in reverse
        uint32 highTen = (surrogateHigh - 0xD800);
        uint32 lowTen  = (surrogateLow - 0xDC00);
        uint32 codepoint = 0x10000 + ((highTen << 10) | lowTen);

        // This is the maximum valid unicode codepoint
        // This should be ensured anyway since you can't encode a value higher
        // than this as a surrogate pair, so we assert this here
        CodexAssert(codepoint <= 0x10FFFF);

        // Now we need to encode the code point into utf-8
        // Codepoints in the range that gets encoded into a surrogate pair
        // gets encoded into 4 bytes under utf8
        // Since the codepoint can be represented by 21 bits, the encoding
        // does the following: first 3 bits in the first byte, the next 6 in the
        // second, the next six in the third, and the last six in the 4th byte
        *ptr++ = static_cast<utf8char_t>(codepoint >> 18) | 0xF0;
        *ptr++ = static_cast<utf8char_t>((codepoint >> 12) & 0x3F) | 0x80;
        *ptr++ = static_cast<utf8char_t>((codepoint >> 6) & 0x3F) | 0x80;
        *ptr++ = static_cast<utf8char_t>(codepoint & 0x3F) | 0x80;

        return ptr;
    }

    LPCUTF8 NextCharFull(LPCUTF8 ptr)
    {
        return ptr + EncodedBytes(*ptr);
    }

    LPCUTF8 PrevCharFull(LPCUTF8 ptr, LPCUTF8 start)
    {
        if (ptr > start)
        {
            LPCUTF8 current = ptr - 1;
            while (current > start && (*current & 0xC0) == 0x80)
                current--;
            if (NextChar(current) == ptr)
                return current;

            // It is not a valid encoding, just go back one character.
            return ptr - 1;
        }
        else
            return ptr;
    }
    
    _Use_decl_annotations_
    size_t DecodeUnitsInto(char16 *buffer, LPCUTF8& pbUtf8, LPCUTF8 pbEnd, DecodeOptions options, bool *chunkEndsAtTruncatedSequence)
    {
        DecodeOptions localOptions = options;

        if (chunkEndsAtTruncatedSequence)
        {
            *chunkEndsAtTruncatedSequence = false;
        }

        LPCUTF8 p = pbUtf8;
        char16 *dest = buffer;

        if (!ShouldFastPath(p, dest)) goto LSlowPath;

LFastPath:
        while (p + 3 < pbEnd)
        {
            unsigned bytes = *(unsigned *)p;
            if ((bytes & 0x80808080) != 0) goto LSlowPath;
            ((uint32 *)dest)[0] = (char16(bytes) & 0x00FF) | ((char16(bytes) & 0xFF00) << 8);
            ((uint32 *)dest)[1] = (char16(bytes >> 16) & 0x00FF) | ((char16(bytes >> 16) & 0xFF00) << 8);
            p += 4;
            dest += 4;
        }

LSlowPath:
        while (p < pbEnd)
        {
            LPCUTF8 s = p;
            char16 chDest = Decode(p, pbEnd, localOptions, chunkEndsAtTruncatedSequence);

            if (s < p)
            {
                // We decoded the character, store it
                *dest++ = chDest;
            }
            else
            {
                // Nothing was converted. This might happen at the end of a buffer with doChunkedEncoding.
                break;
            }

            if (ShouldFastPath(p, dest)) goto LFastPath;
        }

        pbUtf8 = p;

        return dest - buffer;
    }

    _Use_decl_annotations_
    size_t DecodeUnitsIntoAndNullTerminate(char16 *buffer, LPCUTF8& pbUtf8, LPCUTF8 pbEnd, DecodeOptions options, bool *chunkEndsAtTruncatedSequence)
    {
        size_t result = DecodeUnitsInto(buffer, pbUtf8, pbEnd, options, chunkEndsAtTruncatedSequence);
        buffer[result] = 0;
        return result;
    }

    _Use_decl_annotations_
    size_t DecodeUnitsIntoAndNullTerminateNoAdvance(char16 *buffer, LPCUTF8 pbUtf8, LPCUTF8 pbEnd, DecodeOptions options, bool *chunkEndsAtTruncatedSequence)
    {
        return DecodeUnitsIntoAndNullTerminate(buffer, pbUtf8, pbEnd, options, chunkEndsAtTruncatedSequence);
    }

    bool CharsAreEqual(LPCOLESTR pch, LPCUTF8 bch, LPCUTF8 end, DecodeOptions options)
    {
        DecodeOptions localOptions = options;
        while (bch < end)
        {
            if (*pch++ != utf8::Decode(bch, end, localOptions))
            {
                return false;
            }
        }
        return true;
    }

    template <bool cesu8Encoding>
    __range(0, cchIn * 3)
    size_t EncodeIntoImpl(__out_ecount(cchIn * 3) LPUTF8 buffer, __in_ecount(cchIn) const char16 *source, charcount_t cchIn)
    {
        charcount_t cch = cchIn; // SAL analysis gets confused by EncodeTrueUtf8's dest buffer requirement unless we alias cchIn with a local
        LPUTF8 dest = buffer;

        if (!ShouldFastPath(dest, source)) goto LSlowPath;

LFastPath:
        while (cch >= 4)
        {
            uint32 first = ((const uint32 *)source)[0];
            if ( (first & 0xFF80FF80) != 0) goto LSlowPath;
            uint32 second = ((const uint32 *)source)[1];
            if ( (second & 0xFF80FF80) != 0) goto LSlowPath;
            *(uint32 *)dest = (first & 0x0000007F) | ((first & 0x007F0000) >> 8) | ((second & 0x0000007f) << 16) | ((second & 0x007F0000) << 8);
            dest += 4;
            source += 4;
            cch -= 4;
        }

LSlowPath:
        if (cesu8Encoding)
        {
            while (cch-- > 0)
            {
                dest = Encode(*source++, dest);
                if (ShouldFastPath(dest, source)) goto LFastPath;
            }
        }
        else
        {
            while (cch-- > 0)
            {
                // We increment the source pointer here since at least one utf16 code unit is read here
                // If the code unit turns out to be the high surrogate in a surrogate pair, then
                // EncodeTrueUtf8 will consume the low surrogate code unit too by decrementing cch
                // and incrementing source
                dest = EncodeTrueUtf8(*source++, &source, &cch, dest);
                if (ShouldFastPath(dest, source)) goto LFastPath;
            }
        }

        return dest - buffer;
    }

    __range(0, cch * 3)
        size_t EncodeInto(__out_ecount(cch * 3) LPUTF8 buffer, __in_ecount(cch) const char16 *source, charcount_t cch)
    {
        return EncodeIntoImpl<true>(buffer, source, cch);
    }

    __range(0, cch * 3)
    size_t EncodeIntoAndNullTerminate(__out_ecount(cch * 3 + 1) utf8char_t *buffer, __in_ecount(cch) const char16 *source, charcount_t cch)
    {
        size_t result = EncodeInto(buffer, source, cch);
        buffer[result] = 0;
        return result;
    }

    __range(0, cch * 3)
        size_t EncodeTrueUtf8IntoAndNullTerminate(__out_ecount(cch * 3 + 1) utf8char_t *buffer, __in_ecount(cch) const char16 *source, charcount_t cch)
    {
        size_t result = EncodeIntoImpl<false>(buffer, source, cch);
        buffer[result] = 0;
        return result;
    }

    // Convert the character index into a byte index.
    size_t CharacterIndexToByteIndex(__in_ecount(cbLength) LPCUTF8 pch, size_t cbLength, charcount_t cchIndex, DecodeOptions options)
    {
        return CharacterIndexToByteIndex(pch, cbLength, cchIndex, 0, 0, options);
    }

    size_t CharacterIndexToByteIndex(__in_ecount(cbLength) LPCUTF8 pch, size_t cbLength, const charcount_t cchIndex, size_t cbStartIndex, charcount_t cchStartIndex, DecodeOptions options)
    {
        DecodeOptions localOptions = options;
        LPCUTF8 pchCurrent = pch + cbStartIndex;
        LPCUTF8 pchEnd = pch + cbLength;
        LPCUTF8 pchEndMinus4 = pch + (cbLength - 4);
        charcount_t i = cchIndex - cchStartIndex;

        // Avoid using a reinterpret_cast to start a misaligned read.
        if (!IsAligned(pchCurrent)) goto LSlowPath;
LFastPath:
        // Skip 4 bytes at a time.
        while (pchCurrent < pchEndMinus4 && i > 4)
        {
            uint32 ch4 = *reinterpret_cast<const uint32 *>(pchCurrent);
            if ((ch4 & 0x80808080) == 0)
            {
                pchCurrent += 4;
                i -= 4;
            }
            else break;
        }

LSlowPath:
        while (pchCurrent < pchEnd && i > 0)
        {
            Decode(pchCurrent, pchEnd, localOptions);
            i--;

            // Try to return to the fast path avoiding misaligned reads.
            if (i > 4 && IsAligned(pchCurrent)) goto LFastPath;
        }
        return i > 0 ? cbLength : pchCurrent - pch;
    }

    // Convert byte index into character index
    charcount_t ByteIndexIntoCharacterIndex(__in_ecount(cbIndex) LPCUTF8 pch, size_t cbIndex, DecodeOptions options)
    {
        DecodeOptions localOptions = options;
        LPCUTF8 pchCurrent = pch;
        LPCUTF8 pchEnd = pch + cbIndex;
        LPCUTF8 pchEndMinus4 = pch + (cbIndex - 4);
        charcount_t i = 0;

        // Avoid using a reinterpret_cast to start a misaligned read.
        if (!IsAligned(pchCurrent)) goto LSlowPath;

LFastPath:
        // Skip 4 bytes at a time.
        while (pchCurrent < pchEndMinus4)
        {
            uint32 ch4 = *reinterpret_cast<const uint32 *>(pchCurrent);
            if ((ch4 & 0x80808080) == 0)
            {
                pchCurrent += 4;
                i += 4;
            }
            else break;
        }

LSlowPath:
        while (pchCurrent < pchEnd)
        {
            LPCUTF8 s = pchCurrent;
            Decode(pchCurrent, pchEnd, localOptions);
            if (s == pchCurrent) break;
            i++;

            // Try to return to the fast path avoiding misaligned reads.
            if (IsAligned(pchCurrent)) goto LFastPath;
        }

        return i;
    }

} // namespace utf8

#ifdef _MSC_VER
#pragma warning(pop)
#endif
