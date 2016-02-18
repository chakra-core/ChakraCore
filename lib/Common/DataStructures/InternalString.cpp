//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonDataStructuresPch.h"
#include "DataStructures/CharacterBuffer.h"
#include "DataStructures/InternalString.h"

namespace Js
{
    InternalString::InternalString(const wchar16* content, charcount_t length, unsigned char offset):
        m_content(content),
        m_charLength(length),
        m_offset(offset)
    {
        AssertMsg(length < INT_MAX, "Length should be a valid string length");
    }

    // This will make a copy of the entire buffer
    InternalString *InternalString::New(ArenaAllocator* alloc, const wchar16* content, charcount_t length)
    {
        size_t bytelength = sizeof(wchar16) * length;
        DWORD* allocbuffer = (DWORD*)alloc->Alloc(sizeof(DWORD) + bytelength + sizeof(wchar16));
        allocbuffer[0] = (DWORD) bytelength;

        wchar16* buffer = (wchar16*)(allocbuffer+1);
        js_memcpy_s(buffer, bytelength, content, bytelength);
        buffer[length] = L'\0';
        InternalString* newInstance = Anew(alloc, InternalString, buffer, length);
        return newInstance;
    }

    // This will make a copy of the entire buffer
    // Allocated using recycler memory
    InternalString *InternalString::New(Recycler* recycler, const wchar16* content, charcount_t length)
    {
        size_t bytelength = sizeof(wchar16) * length;

        // Allocate 3 extra bytes, two for the first DWORD with the size, the third for the null character
        // This is so that we can pretend that internal strings are BSTRs for purposes of clients who want to use
        // it as thus
        const unsigned char offset = sizeof(DWORD)/sizeof(wchar16);
        InternalString* newInstance = RecyclerNewPlusLeaf(recycler, bytelength + (sizeof(DWORD) + sizeof(wchar16)), InternalString, nullptr, length, offset);
        DWORD* allocbuffer = (DWORD*) (newInstance + 1);
        allocbuffer[0] = (DWORD) bytelength;
        wchar16* buffer = (wchar16*)(allocbuffer + 1);
        js_memcpy_s(buffer, bytelength, content, bytelength);
        buffer[length] = L'\0';
        newInstance->m_content = (const wchar16*) allocbuffer;

        return newInstance;
    }


    // This will only store the pointer and length, not making a copy of the buffer
    InternalString *InternalString::NewNoCopy(ArenaAllocator* alloc, const wchar16* content, charcount_t length)
    {
        InternalString* newInstance = Anew(alloc, InternalString, const_cast<wchar16*> (content), length);
        return newInstance;
    }
}
