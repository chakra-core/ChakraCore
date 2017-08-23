//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeBasePch.h"

namespace Js
{
    int LineOffsetCache::FindLineForCharacterOffset(
        _In_z_ LPCUTF8 sourceStartCharacter,
        _In_z_ LPCUTF8 sourceEndCharacter,
        charcount_t &inOutLineCharOffset,
        charcount_t &inOutByteOffset,
        charcount_t characterOffset)
    {
        int lastLine = 0;

        while (FindNextLine(sourceStartCharacter, sourceEndCharacter, inOutLineCharOffset, inOutByteOffset, characterOffset))
        {
            lastLine++;
        }

        return lastLine;
    }

    LineOffsetCache::LineOffsetCache(Recycler* allocator,
        _In_z_ LPCUTF8 sourceStartCharacter,
        _In_z_ LPCUTF8 sourceEndCharacter,
        charcount_t startingCharacterOffset,
        charcount_t startingByteOffset) :
        lineCharacterOffsetCacheList(nullptr),
        lineByteOffsetCacheList(nullptr)
    {
        AssertMsg(allocator, "An allocator must be supplied to the cache for allocation of items.");
        this->BuildCache(allocator, sourceStartCharacter, sourceEndCharacter, startingCharacterOffset, startingByteOffset);
    }

    LineOffsetCache::LineOffsetCache(Recycler *allocator,
        _In_reads_(numberOfLines) const charcount_t *lineCharacterOffsets,
        _In_reads_opt_(numberOfLines) const charcount_t *lineByteOffsets,
        __in int numberOfLines)
    {
        this->lineCharacterOffsetCacheList = LineOffsetCacheReadOnlyList::New(allocator, (charcount_t *)lineCharacterOffsets, numberOfLines);
        if (lineByteOffsets)
        {
            this->lineByteOffsetCacheList = LineOffsetCacheReadOnlyList::New(allocator, (charcount_t *)lineByteOffsets, numberOfLines);
        }
        else
        {
            this->lineByteOffsetCacheList = nullptr;
        }
    }

    // outLineCharOffset - The character offset of the start of the line returned
    int LineOffsetCache::GetLineForCharacterOffset(charcount_t characterOffset, charcount_t *outLineCharOffset, charcount_t *outByteOffset)
    {
        Assert(this->lineCharacterOffsetCacheList->Count() > 0);

        // The list is sorted, so binary search to find the line info.
        int closestIndex = -1;
        int minRange = INT_MAX;

        this->lineCharacterOffsetCacheList->BinarySearch([&](const charcount_t item, int index)
        {
            int offsetRange = characterOffset - item;
            if (offsetRange >= 0)
            {
                if (offsetRange < minRange)
                {
                    // There are potentially many lines with starting offsets greater than the one we're searching
                    // for.  As a result, we should track which index we've encountered so far that is the closest
                    // to the offset we're looking for without going under.  This will find the line that contains
                    // the offset.
                    closestIndex = index;
                    minRange = offsetRange;
                }

                // Search lower to see if we can find a closer index.
                return -1;
            }
            else
            {
                // Search higher to get into a range that is greater than the offset.
                return 1;
            }

            // Note that we purposely don't return 0 (==) here.  We want the search to end in failure (-1) because
            // we're searching for the closest element, not necessarily an exact element offset.  Exact offsets
            // are possible when the offset we're searching for is the first character of the line, but that will
            // be handled by the if statement above.
        });

        if (closestIndex >= 0)
        {
            charcount_t lastItemCharacterOffset = GetCharacterOffsetForLine(closestIndex, outByteOffset);

            if (outLineCharOffset != nullptr)
            {
                *outLineCharOffset = lastItemCharacterOffset;
            }
        }

        return closestIndex;
    }

    charcount_t LineOffsetCache::GetCharacterOffsetForLine(charcount_t line, charcount_t *outByteOffset) const
    {
        AssertMsg(line < this->GetLineCount(), "Invalid line value passed in.");

        charcount_t characterOffset = this->lineCharacterOffsetCacheList->Item(line);

        if (outByteOffset != nullptr)
        {            
            *outByteOffset = this->lineByteOffsetCacheList? this->lineByteOffsetCacheList->Item(line) : characterOffset;            
        }

        return characterOffset;
    }

    uint32 LineOffsetCache::GetLineCount() const
    {
        AssertMsg(this->lineCharacterOffsetCacheList != nullptr, "The list was either not set from the ByteCode or not created.");
        return this->lineCharacterOffsetCacheList->Count();
    }

    const charcount_t * LineOffsetCache::GetLineCharacterOffsetBuffer() const
    {
        return this->lineCharacterOffsetCacheList->GetBuffer();
    }

    const charcount_t * LineOffsetCache::GetLineByteOffsetBuffer() const
    {
        return this->lineByteOffsetCacheList ? this->lineByteOffsetCacheList->GetBuffer() : nullptr;
    }

    bool LineOffsetCache::FindNextLine(_In_z_ LPCUTF8 &currentSourcePosition, _In_z_ LPCUTF8 sourceEndCharacter, charcount_t &inOutCharacterOffset, charcount_t &inOutByteOffset, charcount_t maxCharacterOffset)
    {
        charcount_t currentCharacterOffset = inOutCharacterOffset;
        charcount_t currentByteOffset = inOutByteOffset;
        utf8::DecodeOptions options = utf8::doAllowThreeByteSurrogates;

        while (currentSourcePosition < sourceEndCharacter)
        {
            LPCUTF8 previousCharacter = currentSourcePosition;

            // Decode from UTF8 to wide char.  Note that Decode will advance the current character by 1 at least.
            char16 decodedCharacter = utf8::Decode(currentSourcePosition, sourceEndCharacter, options);

            bool wasLineEncountered = false;
            switch (decodedCharacter)
            {
            case _u('\r'):
                // Check if the next character is a '\n'.  If so, consume that character as well
                // (consider as one line).
                if (*currentSourcePosition == '\n')
                {
                    ++currentSourcePosition;
                    ++currentCharacterOffset;
                }

                // Intentional fall-through.
            case _u('\n'):
            case 0x2028:
            case 0x2029:
                // Found a new line.
                wasLineEncountered = true;
                break;
            }

            // Move to the next character offset.
            ++currentCharacterOffset;

            // Count the current byte offset we're at in the UTF-8 buffer.
            // The character size can be > 1 for unicode characters.
            currentByteOffset += static_cast<int>(currentSourcePosition - previousCharacter);

            if (wasLineEncountered)
            {
                inOutCharacterOffset = currentCharacterOffset;
                inOutByteOffset = currentByteOffset;
                return true;
            }
            else if (currentCharacterOffset >= maxCharacterOffset)
            {
                return false;
            }
        }

        return false;
    }

    // Builds the cache of line offsets from the passed in source.
    void LineOffsetCache::BuildCache(Recycler * allocator, _In_z_ LPCUTF8 sourceStartCharacter,
        _In_z_ LPCUTF8 sourceEndCharacter,
        charcount_t startingCharacterOffset,
        charcount_t startingByteOffset)
    {
        AssertMsg(sourceStartCharacter, "The source start character passed in is null.");
        AssertMsg(sourceEndCharacter, "The source end character passed in is null.");
        AssertMsg(sourceStartCharacter <= sourceEndCharacter, "The source start character should not be beyond the source end character.");
        AssertMsg(!this->lineCharacterOffsetCacheList, "The cache is already built.");

        this->lineCharacterOffsetCacheList = RecyclerNew(allocator, LineOffsetCacheList, allocator);

        // Add the first line in the cache list.
        this->AddLine(allocator, startingCharacterOffset, startingByteOffset);

        while (FindNextLine(sourceStartCharacter, sourceEndCharacter, startingCharacterOffset, startingByteOffset))
        {
            this->AddLine(allocator, startingCharacterOffset, startingByteOffset);
        }

    }

    // Tracks a new line offset in the cache.
    void LineOffsetCache::AddLine(Recycler * allocator, charcount_t characterOffset, charcount_t byteOffset)
    {
        LineOffsetCacheList * characterOffsetList = (LineOffsetCacheList *)(LineOffsetCacheReadOnlyList*)this->lineCharacterOffsetCacheList;
        characterOffsetList->Add(characterOffset);
        LineOffsetCacheList * byteOffsetList = (LineOffsetCacheList *)(LineOffsetCacheReadOnlyList*)this->lineByteOffsetCacheList;
        if (byteOffsetList == nullptr && characterOffset != byteOffset)
        {
            byteOffsetList = RecyclerNew(allocator, LineOffsetCacheList, allocator);
            byteOffsetList->Copy(characterOffsetList);
            this->lineByteOffsetCacheList = byteOffsetList;
        }
        else if (byteOffsetList != nullptr)
        {
            byteOffsetList->Add(byteOffset);
        }        

#if DBG
        Assert(this->lineByteOffsetCacheList == nullptr || this->lineByteOffsetCacheList->Count() == this->lineCharacterOffsetCacheList->Count());
        if (this->lineCharacterOffsetCacheList->Count() > 1)
        {
            // Ensure that the list remains sorted during insertion.
            charcount_t previousCharacterOffset = this->lineCharacterOffsetCacheList->Item(this->lineCharacterOffsetCacheList->Count() - 2);
            AssertMsg(characterOffset > previousCharacterOffset, "The character offsets must be inserted in increasing order per line.");
            if (this->lineByteOffsetCacheList != nullptr)
            {
                charcount_t previousByteOffset = this->lineByteOffsetCacheList->Item(this->lineByteOffsetCacheList->Count() - 2);
                AssertMsg(byteOffset > previousByteOffset, "The byte offsets must be inserted in increasing order per line.");
            }
        }
#endif // DBG
    }
}
