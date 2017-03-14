//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonDataStructuresPch.h"
#include "Option.h"
#include "ImmutableList.h"

// Include it just for VC- non-VC++ platforms get the required definitions
// from CommonPal.h
#ifdef _MSC_VER
#include <strsafe.h>
#endif

template<int chunkSize>
void regex::ImmutableStringBuilder<chunkSize>::AppendInt32(int32 value)
{
    WCHAR buffer[11]; // -2,147,483,648 w.o ',' + \0
    HRESULT hr = S_OK;
    hr = StringCchPrintfW(buffer, _countof(buffer), _u("%d"), value);
    AssertMsg(SUCCEEDED(hr), "StringCchPrintfW");
    if (FAILED(hr) )
    {
        Js::Throw::OutOfMemory();
    }

    // append to the stringBuilder string
    this->AppendWithCopy(buffer);
}

template<int chunkSize>
void regex::ImmutableStringBuilder<chunkSize>::AppendUInt64(uint64 value)
{
    WCHAR buffer[21]; // 18,446,744,073,709,551,615 w.o ',' + \0
    HRESULT hr = S_OK;
    hr = StringCchPrintfW(buffer, _countof(buffer), _u("%llu"), value);
    AssertMsg(SUCCEEDED(hr), "StringCchPrintfW");
    if (FAILED(hr) )
    {
        Js::Throw::OutOfMemory();
    }

    // append to the stringBuilder string
    this->AppendWithCopy(buffer);
}

template<int chunkSize>
void regex::ImmutableStringBuilder<chunkSize>::AppendWithCopy(_In_z_ LPCWSTR str)
{
    AssertMsg(str != nullptr, "str != nullptr");
    size_t strLength = wcslen(str) + 1; // include null-terminated

    WCHAR* buffer = HeapNewNoThrowArray(WCHAR, strLength);
    IfNullThrowOutOfMemory(buffer);
    wcsncpy_s(buffer, strLength, str, strLength);

    // append in front of the tracked allocated strings
    AllocatedStringChunk* newAllocatedString = HeapNewNoThrow(AllocatedStringChunk);
    if (newAllocatedString == nullptr)
    {
        // cleanup
        HeapDeleteArray(strLength, buffer);
        Js::Throw::OutOfMemory();
    }

    newAllocatedString->dataPtr = buffer;
    newAllocatedString->next = this->allocatedStringChunksHead;
    this->allocatedStringChunksHead = newAllocatedString;

    // append to the stringBuilder string
    this->Append(buffer);
}

// template instantiation
template void regex::ImmutableStringBuilder<8>::AppendInt32(int32 value);
template void regex::ImmutableStringBuilder<8>::AppendUInt64(uint64 value);
