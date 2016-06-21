//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace JSON
{
    class JSONParser;

    // Small scanner for exclusive JSON purpose. The general
    // JScript scanner is not appropriate here because of the JSON restricted lexical grammar
    // token enums and structures are shared although the token semantics is slightly different.
    class JSONScanner
    {
    public:
        JSONScanner();
        tokens Scan();
        void Init(const char16* input, uint len, Token* pOutToken,
            ::Js::ScriptContext* sc, const char16* current, ArenaAllocator* allocator);

        void Finalizer();
        char16* GetCurrentString() { return currentString; } 
        uint GetCurrentStringLen() { return currentIndex; }
        uint GetScanPosition() { return uint(currentChar - inputText); }

        void __declspec(noreturn) ThrowSyntaxError(int wErr)
        {
            char16 scanPos[16];
            ::_itow_s(GetScanPosition(), scanPos, _countof(scanPos) / sizeof(char16), 10);
            Js::JavascriptError::ThrowSyntaxError(scriptContext, wErr, scanPos);
        }

    private:

        // Data structure for unescaping strings
        struct RangeCharacterPair {
        public:
            uint m_rangeStart;
            uint m_rangeLength;
            char16 m_char;
            RangeCharacterPair() {}
            RangeCharacterPair(uint rangeStart, uint rangeLength, char16 ch) : m_rangeStart(rangeStart), m_rangeLength(rangeLength), m_char(ch) {}
        };

        typedef JsUtil::List<RangeCharacterPair, ArenaAllocator> RangeCharacterPairList;

        RangeCharacterPairList* currentRangeCharacterPairList;

        Js::TempGuestArenaAllocatorObject* allocatorObject;
        ArenaAllocator* allocator;
        void BuildUnescapedString(bool shouldSkipLastCharacter);

        RangeCharacterPairList* GetCurrentRangeCharacterPairList(void);

        inline char16 ReadNextChar(void)
        {
            return *currentChar++;
        }

        inline char16 PeekNextChar(void)
        {
            return *currentChar;
        }

        tokens ScanString();
        bool IsJSONNumber();

        const char16* inputText;
        uint    inputLen;
        const char16* currentChar;
        const char16* pTokenString;

        Token*   pToken;
        ::Js::ScriptContext* scriptContext;

        uint     currentIndex;
        char16* currentString;
        __field_ecount(stringBufferLength) char16* stringBuffer;
        int      stringBufferLength;

        friend class JSONParser;
    };
} // namespace JSON
