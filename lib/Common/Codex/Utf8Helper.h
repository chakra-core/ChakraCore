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
    HRESULT WideStringToNarrow(
        LPCWSTR sourceString, size_t sourceCount,
        LPSTR* destStringPtr, size_t* destCount)
    {
        size_t cchSourceString = sourceCount;

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

        size_t cbEncoded = utf8::EncodeTrueUtf8IntoAndNullTerminate(destString, sourceString, (charcount_t) cchSourceString);
        Assert(cbEncoded <= cbDestString);
        static_assert(sizeof(utf8char_t) == sizeof(char), "Needs to be valid for cast");
        *destStringPtr = (char*)destString;
        *destCount = cbEncoded;
        return S_OK;
    }

    ///
    /// Use the codex library to encode a UTF8 string to UTF16.
    /// The caller is responsible for freeing the memory, which is allocated
    /// using Allocator.
    /// The returned string is null terminated.
    ///
    template <class Allocator>
    HRESULT NarrowStringToWide(
        LPCSTR sourceString, size_t sourceCount,
        LPWSTR* destStringPtr, size_t* destCount)
    {
        size_t cbSourceString = sourceCount;
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

        // Some node tests depend on the utf8 decoder not swallowing invalid unicode characters
        // instead of replacing them with the "replacement" chracter. Pass a flag to our 
        // decoder to require such behavior
        utf8::DecodeIntoAndNullTerminate(destString, (LPCUTF8) sourceString, cchDestString,
            DecodeOptions::doAllowInvalidWCHARs);
        Assert(destString[cchDestString] == 0);
        static_assert(sizeof(utf8char_t) == sizeof(char), "Needs to be valid for cast");
        *destStringPtr = destString;
        *destCount = cchDestString;
        return S_OK;
    }

    class malloc_allocator
    {
    public:
        static void* allocate(size_t size) { return ::malloc(size); }
        static void free(void* ptr, size_t count) { ::free(ptr); }
    };

    inline HRESULT WideStringToNarrowDynamic(LPCWSTR sourceString, LPSTR* destStringPtr)
    {
        size_t unused;
        return WideStringToNarrow<malloc_allocator>(
            sourceString, wcslen(sourceString), destStringPtr, &unused);
    }

    inline HRESULT NarrowStringToWideDynamic(LPCSTR sourceString, LPWSTR* destStringPtr)
    {
        size_t unused;
        return NarrowStringToWide<malloc_allocator>(
            sourceString, strlen(sourceString), destStringPtr, &unused);
    }

    inline HRESULT NarrowStringToWideDynamicGetLength(LPCSTR sourceString, LPWSTR* destStringPtr, size_t* destLength)
    {
        return NarrowStringToWide<malloc_allocator>(
            sourceString, strlen(sourceString), destStringPtr, destLength);
    }

    template <class Allocator, class SrcType, class DstType>
    class NarrowWideStringConverter
    {
    public:
        static size_t Length(const SrcType& src);
        static HRESULT Convert(
            SrcType src, size_t srcCount, DstType* dst, size_t* dstCount);
    };

    template <class Allocator>
    class NarrowWideStringConverter<Allocator, LPCSTR, LPWSTR>
    {
    public:
        // Note: Typically caller should pass in Utf8 string length. Following
        // is used as fallback.
        static size_t Length(LPCSTR src)
        {
            return strnlen(src, INT_MAX);
        }

        static HRESULT Convert(
            LPCSTR sourceString, size_t sourceCount,
            LPWSTR* destStringPtr, size_t* destCount)
        {
            return NarrowStringToWide<Allocator>(
                sourceString, sourceCount, destStringPtr, destCount);
        }
    };

    template <class Allocator>
    class NarrowWideStringConverter<Allocator, LPCWSTR, LPSTR>
    {
    public:
        // Note: Typically caller should pass in WCHAR string length. Following
        // is used as fallback.
        static size_t Length(LPCWSTR src)
        {
            return wcslen(src);
        }

        static HRESULT Convert(
            LPCWSTR sourceString, size_t sourceCount,
            LPSTR* destStringPtr, size_t* destCount)
        {
            return WideStringToNarrow<Allocator>(
                sourceString, sourceCount, destStringPtr, destCount);
        }
    };

    template <class Allocator, class SrcType, class DstType>
    class NarrowWideConverter
    {
        typedef NarrowWideStringConverter<Allocator, SrcType, DstType>
            StringConverter;
    private:
        DstType dst;
        size_t dstCount;

    public:
        NarrowWideConverter(const SrcType& src, size_t srcCount = -1): dst()
        {
            if (srcCount == -1)
            {
                srcCount = StringConverter::Length(src);
            }

            StringConverter::Convert(src, srcCount, &dst, &dstCount);
        }

        ~NarrowWideConverter()
        {
            if (dst)
            {
                Allocator::free(dst, dstCount);
            }
        }

        DstType Detach()
        {
            DstType result = dst;
            dst = DstType();
            return result;
        }

        operator DstType()
        {
            return dst;
        }

        size_t Length() const
        {
            return dstCount;
        }
    };

    typedef NarrowWideConverter<malloc_allocator, LPCSTR, LPWSTR> NarrowToWide;
    typedef NarrowWideConverter<malloc_allocator, LPCWSTR, LPSTR> WideToNarrow;
}
