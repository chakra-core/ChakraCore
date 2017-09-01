//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class LineOffsetCache
    {
    private:
        typedef JsUtil::List<charcount_t, Recycler, true /*isLeaf*/> LineOffsetCacheList;
        typedef JsUtil::ReadOnlyList<charcount_t> LineOffsetCacheReadOnlyList;

    public:

        static int FindLineForCharacterOffset(
            _In_z_ LPCUTF8 sourceStartCharacter,
            _In_z_ LPCUTF8 sourceEndCharacter,
            charcount_t &inOutLineCharOffset,
            charcount_t &inOutByteOffset,
            charcount_t characterOffset);

        LineOffsetCache(Recycler* allocator,
            _In_z_ LPCUTF8 sourceStartCharacter,
            _In_z_ LPCUTF8 sourceEndCharacter,
            charcount_t startingCharacterOffset = 0,
            charcount_t startingByteOffset = 0);

        LineOffsetCache(Recycler *allocator,
            _In_reads_(numberOfLines) const charcount_t *lineCharacterOffsets,
            _In_reads_opt_(numberOfLines) const charcount_t *lineByteOffsets,
            __in int numberOfLines);

        // outLineCharOffset - The character offset of the start of the line returned
        int GetLineForCharacterOffset(charcount_t characterOffset, charcount_t *outLineCharOffset, charcount_t *outByteOffset);

        charcount_t GetCharacterOffsetForLine(charcount_t line, charcount_t *outByteOffset) const;

        uint32 GetLineCount() const;

        const charcount_t * GetLineCharacterOffsetBuffer() const;
        const charcount_t * GetLineByteOffsetBuffer() const;

    private:

        static bool FindNextLine(_In_z_ LPCUTF8 &currentSourcePosition, _In_z_ LPCUTF8 sourceEndCharacter, charcount_t &inOutCharacterOffset, charcount_t &inOutByteOffset, charcount_t maxCharacterOffset = UINT32_MAX);

        // Builds the cache of line offsets from the passed in source.
        void BuildCache(Recycler * allocator, _In_z_ LPCUTF8 sourceStartCharacter, _In_z_ LPCUTF8 sourceEndCharacter, charcount_t startingCharacterOffset, charcount_t startingByteOffset);

        // Tracks a new line offset in the cache.
        void AddLine(Recycler * allocator, charcount_t characterOffset, charcount_t byteOffset);
        
    private:
        // Line offset cache list used for quickly finding line/column offsets.
        Field(LineOffsetCacheReadOnlyList*) lineCharacterOffsetCacheList;
        Field(LineOffsetCacheReadOnlyList*) lineByteOffsetCacheList;
    };
}
