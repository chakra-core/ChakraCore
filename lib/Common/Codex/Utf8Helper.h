//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
#include "Utf8Codex.h"

namespace utf8
{
    ///
    /// Use the codex library to encode a UTF16 string to UTF8.
    /// The caller is responsible for freeing the memory, which is allocated
    /// using Allocator.
    /// The returned string is null terminated.
    ///
    template <class Allocator>
    HRESULT WideStringToNarrow(LPCWSTR sourceString, LPSTR* destStringPtr)
    {
        size_t cchSourceString = wcslen(sourceString);

        if (cchSourceString >= MAXUINT32)
        {
            return E_OUTOFMEMORY;
        }

        size_t cbDestString = (cchSourceString + 1) * 3;

        // Check for overflow- cbDestString should be >= cchSourceString
        if (cbDestString < cchSourceString)
        {
            return E_OUTOFMEMORY;
        }

        utf8char_t* destString = (utf8char_t*)Allocator::allocate(cbDestString);
        if (destString == nullptr)
        {
            return E_OUTOFMEMORY;
        }

        utf8::EncodeIntoAndNullTerminate(destString, sourceString, (charcount_t) cchSourceString);
        static_assert(sizeof(utf8char_t) == sizeof(char), "Needs to be valid for cast");
        *destStringPtr = (char*)destString;
        return S_OK;
    }

    ///
    /// Use the codex library to encode a UTF8 string to UTF16.
    /// The caller is responsible for freeing the memory, which is allocated
    /// using Allocator.
    /// The returned string is null terminated.
    ///
    template <class Allocator>
    HRESULT NarrowStringToWide(LPCSTR sourceString, LPWSTR* destStringPtr)
    {
        size_t cbSourceString = strlen(sourceString);
        charcount_t cchDestString = utf8::ByteIndexIntoCharacterIndex((LPCUTF8) sourceString, cbSourceString);
        size_t cbDestString = (cchDestString + 1) * sizeof(WCHAR);

        // Check for overflow- cbDestString should be >= cchSourceString
        if (cbDestString < cchDestString)
        {
            return E_OUTOFMEMORY;
        }

        WCHAR* destString = (WCHAR*)Allocator::allocate(cbDestString);
        if (destString == nullptr)
        {
            return E_OUTOFMEMORY;
        }

        utf8::DecodeIntoAndNullTerminate(destString, (LPCUTF8) sourceString, cchDestString);
        static_assert(sizeof(utf8char_t) == sizeof(char), "Needs to be valid for cast");
        *destStringPtr = destString;
        return S_OK;
    }

    class malloc_allocator
    {
    public:
        static void* allocate(size_t size) { return ::malloc(size); }
        static void free(void* ptr) { ::free(ptr); }
    };

    inline HRESULT WideStringToNarrowDynamic(LPCWSTR sourceString, LPSTR* destStringPtr)
    {
        return WideStringToNarrow<malloc_allocator>(sourceString, destStringPtr);
    }

    inline HRESULT NarrowStringToWideDynamic(LPCSTR sourceString, LPWSTR* destStringPtr)
    {
        return NarrowStringToWide<malloc_allocator>(sourceString, destStringPtr);
    }


    template <class Allocator, class SrcType, class DstType>
    class NarrowWideStringConverter
    {
    public:
        static HRESULT Convert(SrcType src, DstType* dst);
    };

    template <class Allocator>
    class NarrowWideStringConverter<Allocator, LPCSTR, LPWSTR>
    {
    public:
        static HRESULT Convert(LPCSTR sourceString, LPWSTR* destStringPtr)
        {
            return NarrowStringToWide<Allocator>(sourceString, destStringPtr);
        }
    };

    template <class Allocator>
    class NarrowWideStringConverter<Allocator, LPCWSTR, LPSTR>
    {
    public:
        static HRESULT Convert(LPCWSTR sourceString, LPSTR* destStringPtr)
        {
            return WideStringToNarrow<Allocator>(sourceString, destStringPtr);
        }
    };

    template <class Allocator, class SrcType, class DstType>
    class NarrowWideConverter
    {
    private:
        DstType dst;

    public:
        NarrowWideConverter(const SrcType& src): dst()
        {
            NarrowWideStringConverter<Allocator, SrcType, DstType>::Convert(src, &dst);
        }

        ~NarrowWideConverter()
        {
            if (dst)
            {
                Allocator::free(dst);
            }
        }

        operator DstType()
        {
            return dst;
        }
    };

    typedef NarrowWideConverter<malloc_allocator, LPCSTR, LPWSTR> NarrowToWide;
    typedef NarrowWideConverter<malloc_allocator, LPCWSTR, LPSTR> WideToNarrow;
}
