//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"
#include "catch.hpp"
#include <process.h>
#include "Codex\Utf8Codex.h"

#pragma warning(disable:4100) // unreferenced formal parameter
#pragma warning(disable:6387) // suppressing preFAST which raises warning for passing null to the JsRT APIs
#pragma warning(disable:6262) // CATCH is using stack variables to report errors, suppressing the preFAST warning.

namespace CodexTest
{
    ///
    /// The following test verifies that for invalid characters, we replace them
    /// with the unicode replacement character
    ///

    // Verify single utf8-encoded codepoint
    void CheckIsUnicodeReplacementChar(const utf8char_t* encodedBuffer)
    {
        CHECK(encodedBuffer[0] == 0xEF);
        CHECK(encodedBuffer[1] == 0xBF);
        CHECK(encodedBuffer[2] == 0xBD);
    }

    //
    // Following test cases are based on the Utf-8 decoder tests 
    // suggested by Markus Kuhn at https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt
    //
    TEST_CASE("CodexTest_EncodeTrueUtf8_SingleSurrogates", "[CodexTest]")
    {
        const charcount_t charCount = 1;
        utf8char_t encodedBuffer[(charCount + 1) * 3]; // +1 since the buffer will be null-terminated

        char16 testValues[] = { 0xD800, 0xDB7F, 0xDB80, 0xDBFF, 0xDC00, 0xDF80, 0xDFFF };
        const int numTestCases = _countof(testValues);

        for (int i = 0; i < numTestCases; i++)
        {
            size_t numEncodedBytes = utf8::EncodeTrueUtf8IntoAndNullTerminate(encodedBuffer, &testValues[i], charCount);
            CHECK(numEncodedBytes == 3);
            CheckIsUnicodeReplacementChar(encodedBuffer);
        }
    }

    // 
    // Test encoding of given utf16-encoded strings into another encoding
    //
    // In the expected encoded string, extra bytes are represented as 0
    //

    template <typename TTestCase, typename TEncodingFunc>
    void RunUtf8EncodingTestCase(const TTestCase &testCases, const TEncodingFunc func)
    {
        const int numTestCases = _countof(testCases);
        const charcount_t charCount = _countof(testCases[0].surrogatePair);
        const charcount_t maxEncodedByteCount = _countof(testCases[0].utf8Encoding);
        utf8char_t encodedBuffer[maxEncodedByteCount + 1]; // +1 in case a null-terminating func is passed in

        for (int i = 0; i < numTestCases; i++)
        {
            size_t numEncodedBytes = func(encodedBuffer, testCases[i].surrogatePair, charCount);
            CHECK(numEncodedBytes <= maxEncodedByteCount);
            for (size_t j = 0; j < numEncodedBytes; j++)
            {
                CHECK(encodedBuffer[j] == testCases[i].utf8Encoding[j]);
            }

            // Check and make sure there were no other bytes expected in the encoded string
            if (numEncodedBytes < maxEncodedByteCount)
            {
                for (size_t j = numEncodedBytes; j < maxEncodedByteCount; j++)
                {
                    CHECK(testCases[i].utf8Encoding[j] == 0);
                }
            }
        }
    }

    TEST_CASE("CodexTest_EncodeCesu_PairedSurrogates", "[CodexTest]")
    {
        // Each of these test cases verifies the encoding 
        // of a single surrogate pair into a 6 byte CESU string
        // Each surrogate-pair unit is encoded seperately into utf8
        struct TestCase
        {
            char16     surrogatePair[2];
            utf8char_t utf8Encoding[6];
        };

        TestCase testCases[] = {
            { { 0xD800, 0xDC00 }, { 0xED, 0xA0, 0x80, 0xED, 0xB0, 0x80 } }, //  U+010000 LINEAR B SYLLABLE B008 A character
            { { 0xD800, 0xDFFF }, { 0xED, 0xA0, 0x80, 0xED, 0xBF, 0xBF } }, //  U+0103FF
            { { 0xDB7F, 0xDC00 }, { 0xED, 0xAD, 0xBF, 0xED, 0xB0, 0x80 } }, //  U+0EFC00
            { { 0xDB7F, 0xDFFF }, { 0xED, 0xAD, 0xBF, 0xED, 0xBF, 0xBF } }, //  U+0EFFFF
            { { 0xDB80, 0xDC00 }, { 0xED, 0xAE, 0x80, 0xED, 0xB0, 0x80 } }, //  U+0F0000 Plane 15 Private Use First
            { { 0xDB80, 0xDFFF }, { 0xED, 0xAE, 0x80, 0xED, 0xBF, 0xBF } }, //  U+0F03FF
            { { 0xDBFF, 0xDC00 }, { 0xED, 0xAF, 0xBF, 0xED, 0xB0, 0x80 } }, //  U+10FC00
            { { 0xDBFF, 0xDFFF }, { 0xED, 0xAF, 0xBF, 0xED, 0xBF, 0xBF } }  //  U+10FFFF
        };

        RunUtf8EncodingTestCase(testCases, static_cast<size_t (*)(utf8char_t*, const char16*, charcount_t)>(utf8::EncodeInto));
    }

    TEST_CASE("CodexTest_EncodeUtf8_PairedSurrogates", "[CodexTest]")
    {
        // Each of these test cases verifies the encoding 
        // of a single surrogate pair into a 4 byte utf8 string
        // Each surrogate-pair unit is decoded to its original codepoint
        // and then encoded into utf8
        struct TestCase
        {
            char16     surrogatePair[2];
            utf8char_t utf8Encoding[4];
        };

        TestCase testCases[] = {
            { { 0xD800, 0xDC00 }, { 0xF0, 0x90, 0x80, 0x80 } }, //  U+010000 LINEAR B SYLLABLE B008 A character
            { { 0xD800, 0xDFFF }, { 0xF0, 0x90, 0x8F, 0xBF } }, //  U+0103FF
            { { 0xDB7F, 0xDC00 }, { 0xF3, 0xAF, 0xB0, 0x80 } }, //  U+0EFC00
            { { 0xDB7F, 0xDFFF }, { 0xF3, 0xAF, 0xBF, 0xBF } }, //  U+0EFFFF
            { { 0xDB80, 0xDC00 }, { 0xF3, 0xB0, 0x80, 0x80 } }, //  U+0F0000 Plane 15 Private Use First
            { { 0xDB80, 0xDFFF }, { 0xF3, 0xB0, 0x8F, 0xBF } }, //  U+0F03FF
            { { 0xDBFF, 0xDC00 }, { 0xF4, 0x8F, 0xB0, 0x80 } }, //  U+10FC00
            { { 0xDBFF, 0xDFFF }, { 0xF4, 0x8F, 0xBF, 0xBF } }  //  U+10FFFF
        };

        RunUtf8EncodingTestCase(testCases, utf8::EncodeTrueUtf8IntoAndNullTerminate);
    }

    TEST_CASE("CodexTest_EncodeUtf8_NonCharacters", "[CodexTest]")
    {
        // Each of these test cases verifies the encoding 
        // of certain problematic codepoints that do not represent
        // characters
        struct TestCase
        {
            char16     surrogatePair[1];
            utf8char_t utf8Encoding[3];
        };

        TestCase testCases[] = {
            { { 0xFFFE }, { 0xEF, 0xBF, 0xBE } }, //  U+FFFE
            { { 0xFFFF }, { 0xEF, 0xBF, 0xBF } }  //  U+FFFF
        };

        RunUtf8EncodingTestCase(testCases, utf8::EncodeTrueUtf8IntoAndNullTerminate);
    }

    TEST_CASE("CodexTest_EncodeUtf8_BoundaryChars", "[CodexTest]")
    {
        // Each of these test cases verifies the encoding 
        // of boundary conditions
        struct SingleChar16TestCase
        {
            char16     surrogatePair[1];
            utf8char_t utf8Encoding[3];
        };

        SingleChar16TestCase testCases[] = {
            { { 0xD7FF }, { 0xED, 0x9F, 0xBF } }, //  U+D7FF
            { { 0xE000 }, { 0xEE, 0x80, 0x80 } }, //  U+E000
            { { 0xFFFD }, { 0xEF, 0xBF, 0xBD } }  //  U+FFFD
        };

        struct TwoChar16TestCase
        {
            char16     surrogatePair[2];
            utf8char_t utf8Encoding[4];
        };

        TwoChar16TestCase testCases2[] = {
            { { 0xDBFF, 0xDFFF }, { 0xF4, 0x8F, 0xBF, 0xBF } } //  U+10FFFF
        };

        RunUtf8EncodingTestCase(testCases, utf8::EncodeTrueUtf8IntoAndNullTerminate);
        RunUtf8EncodingTestCase(testCases2, utf8::EncodeTrueUtf8IntoAndNullTerminate);
    }

    TEST_CASE("CodexTest_EncodeUtf8_SimpleCharacters", "[CodexTest]")
    {
        // Each of these test cases verifies the encoding 
        // of certain problematic codepoints that do not represent
        // characters
        struct TestCase
        {
            char16     surrogatePair[1];
            utf8char_t utf8Encoding[3];
        };

        TestCase testCases[] = {
            { { 0x0024 }, { 0x24 } },              //  U+0024 - Dollar Symbol
            { { 0x00A2 }, { 0xC2, 0xA2 } },        //  U+00A2 - Cent symbol
            { { 0x20AC }, { 0xE2, 0x82, 0xAC } }   //  U+20AC - Euro symbol
        };

        RunUtf8EncodingTestCase(testCases, utf8::EncodeTrueUtf8IntoAndNullTerminate);
    }

    TEST_CASE("CodexTest_EncodeTrueUtf8_SimpleString", "[CodexTest]")
    {
        const charcount_t charCount = 3;
        utf8char_t encodedBuffer[(charCount + 1) * 3]; // +1 since the buffer will be null terminated
        const char16* sourceBuffer = L"abc";
        size_t numEncodedBytes = utf8::EncodeTrueUtf8IntoAndNullTerminate(encodedBuffer, sourceBuffer, charCount);
        CHECK(numEncodedBytes == charCount);
        for (int i = 0; i < charCount; i++)
        {
            CHECK(sourceBuffer[i] == (const char16)encodedBuffer[i]);
        }
    }

    template <typename TTestCase, typename TDecodeFunc>
    void RunUtf8DecodeTestCase(const TTestCase &testCases, const TDecodeFunc func)
    {
        const int numTestCases = _countof(testCases);
        const charcount_t charCount = _countof(testCases[0].result);
        char16 decodedBuffer[charCount + 1]; // +1 in case a null-terminating func is passed in

        for (int i = 0; i < numTestCases; i++)
        {
            bool chunkEndsInTruncatedSequence = false;
            size_t decodedCount = func(decodedBuffer, testCases[i].utf8Encoding, testCases[i].utf8Encoding + testCases[i].bytesToDecode, utf8::DecodeOptions::doChunkedEncoding, &chunkEndsInTruncatedSequence);
            CHECK(decodedCount == testCases[i].expectedDecodedChars);

            for (size_t j = 0; j < decodedCount; j++)
            {
                CHECK(decodedBuffer[j] == testCases[i].result[j]);
            }

            CHECK(testCases[i].shouldEndInTruncation == chunkEndsInTruncatedSequence);
        }
    }

    TEST_CASE("CodexTest_DecodeUnitsInto_ChunkEndsInTruncatedSequence", "[CodexTest]")
    {
        struct TestCase
        {
            int bytesToDecode;
            size_t expectedDecodedChars;
            bool shouldEndInTruncation;
            char16 result[8];
            utf8char_t utf8Encoding[8];
        };

        TestCase testCases[] = {
            { 2, 1, false, { 0xc1 }, { 0xc3, 0x81 } }, // Valid 2-byte sequence
            { 1, 0, true, { 0x0 }, { 0xc3, 0x81 } }, // Valid 2-byte sequence truncated at the end of the chunk
            { 2, 2, false, { 0xfffd, 0x79 },{ 0xc3, 0x79 } }, // Invalid 2-byte sequence
            { 1, 0, true, { 0x0 }, { 0xc3, 0x79 } }, // Invalid 2-byte sequence truncated at the end of the chunk
            { 3, 1, false, { 0x3042 },{ 0xe3, 0x81, 0x82 } }, // Valid 3-byte sequence
            { 1, 0, true, { 0x0 }, { 0xe3, 0x81, 0x82 } }, // Valid 3-byte sequence truncated at the end of the chunk
            { 2, 0, true, { 0x0 }, { 0xe3, 0x81, 0x82 } }, // Valid 3-byte sequence truncated at the end of the chunk
            { 3, 3, false, { 0xfffd, 0x79, 0xfffd }, { 0xe3, 0x79, 0x82 } }, // Invalid 3-byte sequence
            { 1, 0, true, { 0x0 }, { 0xe3, 0x79, 0x82 } }, // Invalid 3-byte sequence truncated at the end of the chunk
            { 2, 0, true, { 0x0 }, { 0xe3, 0x79, 0x82 } }, // Invalid 3-byte sequence truncated at the end of the chunk
            { 3, 3, false, { 0xfffd, 0xfffd, 0x79 }, { 0xe3, 0x81, 0x79 } }, // Invalid 3-byte sequence
            { 1, 0, true, { 0x0 }, { 0xe3, 0x81, 0x79 } }, // Invalid 3-byte sequence truncated at the end of the chunk
            { 2, 0, true, { 0x0 }, { 0xe3, 0x81, 0x79 } }, // Invalid 3-byte sequence truncated at the end of the chunk
            { 4, 2, false, { 0xd9c4, 0xdc83 }, { 0xf2, 0x81, 0x82, 0x83 } }, // Valid 4-byte sequence
            { 1, 0, true, { 0x0 }, { 0xf2, 0x81, 0x82, 0x83 } }, // Valid 4-byte sequence truncated at the end of the chunk
            { 2, 0, true, { 0x0 }, { 0xf2, 0x81, 0x82, 0x83 } }, // Valid 4-byte sequence truncated at the end of the chunk
            { 3, 0, true, { 0x0 }, { 0xf2, 0x81, 0x82, 0x83 } }, // Valid 4-byte sequence truncated at the end of the chunk
            { 4, 4, false, { 0xfffd, 0x79, 0xfffd, 0xfffd }, { 0xf2, 0x79, 0x82, 0x83 } }, // Invalid 4-byte sequence
            { 1, 0, true, { 0x0 }, { 0xf2, 0x79, 0x82, 0x83 } }, // Invalid 4-byte sequence truncated at the end of the chunk
            { 2, 0, true, { 0x0 }, { 0xf2, 0x79, 0x82, 0x83 } }, // Invalid 4-byte sequence truncated at the end of the chunk
            { 3, 0, true, { 0x0 }, { 0xf2, 0x79, 0x82, 0x83 } }, // Invalid 4-byte sequence truncated at the end of the chunk
            { 4, 4, false, { 0xfffd, 0xfffd, 0x79, 0xfffd }, { 0xf2, 0x81, 0x79, 0x83 } }, // Invalid 4-byte sequence
            { 1, 0, true, { 0x0 }, { 0xf2, 0x81, 0x79, 0x83 } }, // Invalid 4-byte sequence truncated at the end of the chunk
            { 2, 0, true, { 0x0 }, { 0xf2, 0x81, 0x79, 0x83 } }, // Invalid 4-byte sequence truncated at the end of the chunk
            { 3, 0, true, { 0x0 }, { 0xf2, 0x81, 0x79, 0x83 } }, // Invalid 4-byte sequence truncated at the end of the chunk
            { 4, 4, false, { 0xfffd, 0xfffd, 0xfffd, 0x79 }, { 0xf2, 0x81, 0x82, 0x79 } }, // Invalid 4-byte sequence
            { 1, 0, true, { 0x0 }, { 0xf2, 0x81, 0x82, 0x79 } }, // Invalid 4-byte sequence truncated at the end of the chunk
            { 2, 0, true, { 0x0 }, { 0xf2, 0x81, 0x82, 0x79 } }, // Invalid 4-byte sequence truncated at the end of the chunk
            { 3, 0, true, { 0x0 }, { 0xf2, 0x81, 0x82, 0x79 } }, // Invalid 4-byte sequence truncated at the end of the chunk
        };
        
        RunUtf8DecodeTestCase(testCases, utf8::DecodeUnitsIntoAndNullTerminateNoAdvance);
    }
};