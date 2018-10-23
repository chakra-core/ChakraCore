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
    /// TODO(jahorto): This file's dependencies mean that it cannot be included in PlatformAgnostic
    ///     Thus, this function is currently ~duplicated in PlatformAgnostic::Intl::Utf16ToUtf8 (Intl.cpp)
    ///     As long as that function exists, it _must_ be updated alongside any updates here
    ///
    template <typename AllocatorFunction>
    HRESULT WideStringToNarrow(
        _In_ AllocatorFunction allocator,
        _In_ LPCWSTR sourceString,
        size_t sourceCount,
        _Out_ LPSTR* destStringPtr,
        _Out_ size_t* destCount,
        size_t* allocateCount = nullptr)
    {
        size_t cchSourceString = sourceCount;

        if (cchSourceString >= MAXUINT32)
        {
            return E_OUTOFMEMORY;
        }

        // Multiply by 3 for max size of encoded character, plus 1 for the null terminator (don't need 3 bytes for the null terminator)
        size_t cbDestString = (cchSourceString * 3) + 1;

        // Check for overflow- cbDestString should be >= cchSourceString
        if (cbDestString < cchSourceString)
        {
            return E_OUTOFMEMORY;
        }

        utf8char_t* destString = (utf8char_t*)allocator(cbDestString);
        if (destString == nullptr)
        {
            return E_OUTOFMEMORY;
        }

        size_t cbEncoded = utf8::EncodeIntoAndNullTerminate<utf8::Utf8EncodingKind::TrueUtf8>(destString, cbDestString, sourceString, static_cast<charcount_t>(cchSourceString));
        Assert(cbEncoded <= cbDestString);
        static_assert(sizeof(utf8char_t) == sizeof(char), "Needs to be valid for cast");
        *destStringPtr = (char*)destString;
        *destCount = cbEncoded;
        if (allocateCount != nullptr)
        {
            *allocateCount = cbEncoded;
        }

        return S_OK;
    }

    ///
    /// Use the codex library to encode a UTF16 string to UTF8.
    /// The caller is responsible for providing the buffer
    /// The returned string is null terminated.
    ///
    inline HRESULT WideStringToNarrowNoAlloc(
        _In_ LPCWSTR sourceString,
        size_t sourceCount,
        __out_ecount(destCount) LPSTR destString,
        size_t destCount,
        size_t* writtenCount = nullptr)
    {
        size_t cchSourceString = sourceCount;

        if (cchSourceString >= MAXUINT32)
        {
            return E_OUTOFMEMORY;
        }

        static_assert(sizeof(utf8char_t) == sizeof(char), "Needs to be valid for cast");

        size_t cbEncoded = 0;
        if (destString == nullptr)
        {
            cbEncoded = utf8::CountTrueUtf8(sourceString, (charcount_t)cchSourceString);
        }
        else
        {
            cbEncoded = utf8::EncodeInto<utf8::Utf8EncodingKind::TrueUtf8>((utf8char_t*)destString, destCount, sourceString, static_cast<charcount_t>(cchSourceString));
            Assert(cbEncoded <= destCount);
        }

        if (writtenCount != nullptr)
        {
            *writtenCount = cbEncoded;
        }

        return S_OK;
    }

    template <class Allocator>
    HRESULT WideStringToNarrow(_In_ LPCWSTR sourceString, size_t sourceCount, _Out_ LPSTR* destStringPtr, _Out_ size_t* destCount, size_t* allocateCount = nullptr)
    {
        return WideStringToNarrow(Allocator::allocate, sourceString, sourceCount, destStringPtr, destCount, allocateCount);
    }

    inline HRESULT NarrowStringToWideNoAlloc(_In_ LPCSTR sourceString, size_t sourceCount,
        __out_ecount(destBufferCount) LPWSTR destString, size_t destBufferCount, _Out_ charcount_t* destCount)
    {
        size_t sourceStart = 0;
        size_t cbSourceString = sourceCount;

        if (sourceCount >= MAXUINT32)
        {
            destString[0] = WCHAR(0);
            return E_OUTOFMEMORY;
        }

        if (destString == nullptr)
        {
            return E_INVALIDARG;
        }

        if (sourceCount >= destBufferCount)
        {
            destString[0] = WCHAR(0);
            return E_INVALIDARG;
        }

        for (; sourceStart < sourceCount; sourceStart++)
        {
            const char ch = sourceString[sourceStart];
            if ( ! (ch > 0 && ch < 0x0080) )
            {
                size_t fallback = sourceStart > 3 ? 3 : sourceStart; // 3 + 1 -> fallback at least 1 unicode char
                sourceStart -= fallback;
                break;
            }
            destString[sourceStart] = (WCHAR) ch;
        }

        if (sourceStart == sourceCount)
        {
            *destCount = static_cast<charcount_t>(sourceCount);
            destString[sourceCount] = WCHAR(0);
        }
        else
        {
            LPCUTF8 remSourceString = (LPCUTF8)sourceString + sourceStart;
            WCHAR *remDestString = destString + sourceStart;

            charcount_t cchDestString = utf8::ByteIndexIntoCharacterIndex(remSourceString, cbSourceString - sourceStart);
            cchDestString += (charcount_t)sourceStart;
            if (cchDestString > sourceCount)
            {
                return E_OUTOFMEMORY;
            }

            // Some node tests depend on the utf8 decoder not swallowing invalid unicode characters
            // instead of replacing them with the "replacement" chracter. Pass a flag to our
            // decoder to require such behavior
            utf8::DecodeUnitsIntoAndNullTerminateNoAdvance(remDestString, remSourceString, (LPCUTF8) sourceString + cbSourceString, DecodeOptions::doAllowInvalidWCHARs);

            static_assert(sizeof(utf8char_t) == sizeof(char), "Needs to be valid for cast");
            *destCount = cchDestString;
        }

        Assert(destString[*destCount] == 0);

        return S_OK;
    }

    ///
    /// Use the codex library to encode a UTF8 string to UTF16.
    /// The caller is responsible for freeing the memory, which is allocated
    /// using Allocator.
    /// The returned string is null terminated.
    ///
    template <typename AllocatorFunction>
    HRESULT NarrowStringToWide(_In_ AllocatorFunction allocator,_In_ LPCSTR sourceString,
        size_t sourceCount, _Out_ LPWSTR* destStringPtr, _Out_ charcount_t* destCount, size_t* allocateCount = nullptr)
    {
        size_t cbDestString = (sourceCount + 1) * sizeof(WCHAR);
        if (cbDestString < sourceCount) // overflow ?
        {
            return E_OUTOFMEMORY;
        }

        WCHAR* destString = (WCHAR*)allocator(cbDestString);
        if (destString == nullptr)
        {
            return E_OUTOFMEMORY;
        }

        if (allocateCount != nullptr)
        {
            *allocateCount = cbDestString;
        }

        *destStringPtr = destString;
        return NarrowStringToWideNoAlloc(sourceString, sourceCount, destString, sourceCount + 1, destCount);
    }

    template <class Allocator>
    HRESULT NarrowStringToWide(_In_ LPCSTR sourceString, size_t sourceCount, _Out_ LPWSTR* destStringPtr, _Out_ charcount_t* destCount, size_t* allocateCount = nullptr)
    {
        return NarrowStringToWide(Allocator::allocate, sourceString, sourceCount, destStringPtr, destCount, allocateCount);
    }

    class malloc_allocator
    {
    public:
        static void* allocate(size_t size) { return ::malloc(size); }
        static void free(void* ptr, size_t count) { ::free(ptr); }
    };

    inline HRESULT WideStringToNarrowDynamic(_In_ LPCWSTR sourceString, _Out_ LPSTR* destStringPtr)
    {
        size_t unused;
        return WideStringToNarrow<malloc_allocator>(
            sourceString, wcslen(sourceString), destStringPtr, &unused);
    }

    inline HRESULT NarrowStringToWideDynamic(_In_ LPCSTR sourceString, _Out_ LPWSTR* destStringPtr)
    {
        charcount_t unused;
        return NarrowStringToWide<malloc_allocator>(
            sourceString, strlen(sourceString), destStringPtr, &unused);
    }

    inline HRESULT NarrowStringToWideDynamicGetLength(_In_ LPCSTR sourceString, _Out_ LPWSTR* destStringPtr, _Out_ charcount_t* destLength)
    {
        return NarrowStringToWide<malloc_allocator>(
            sourceString, strlen(sourceString), destStringPtr, destLength);
    }

    template <class Allocator, class SrcType, class DstType, class CountType>
    class NarrowWideStringConverter
    {
    public:
        static size_t Length(const SrcType& src);
        static HRESULT Convert(
            SrcType src, size_t srcCount, DstType* dst, CountType* dstCount, size_t* allocateCount = nullptr);
        static HRESULT ConvertNoAlloc(
            SrcType src, size_t srcCount, DstType dst, CountType dstCount, CountType* written);
    };

    template <class Allocator>
    class NarrowWideStringConverter<Allocator, LPCSTR, LPWSTR, charcount_t>
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
            LPWSTR* destStringPtr, charcount_t * destCount, size_t* allocateCount = nullptr)
        {
            return NarrowStringToWide<Allocator>(
                sourceString, sourceCount, destStringPtr, destCount, allocateCount);
        }

        static HRESULT ConvertNoAlloc(
            LPCSTR sourceString, size_t sourceCount,
            LPWSTR destStringPtr, charcount_t destCount, charcount_t* written)
        {
            return NarrowStringToWideNoAlloc(
                sourceString, sourceCount, destStringPtr, destCount, written);
        }
    };

    template <class Allocator>
    class NarrowWideStringConverter<Allocator, LPCWSTR, LPSTR, size_t>
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
            LPSTR* destStringPtr, size_t* destCount, size_t* allocateCount = nullptr)
        {
            return WideStringToNarrow<Allocator>(
                sourceString, sourceCount, destStringPtr, destCount, allocateCount);
        }

        static HRESULT ConvertNoAlloc(
            LPCWSTR sourceString, size_t sourceCount,
            LPSTR destStringPtr, size_t destCount, size_t* written)
        {
            return WideStringToNarrowNoAlloc(
                sourceString, sourceCount, destStringPtr, destCount, written);
        }
    };

    template <class Allocator, class SrcType, class DstType, class CountType>
    class NarrowWideConverter
    {
        typedef NarrowWideStringConverter<Allocator, SrcType, DstType, CountType>
            StringConverter;
    private:
        DstType dst;
        CountType dstCount;
        size_t allocateCount;
        bool freeDst;

    public:
        NarrowWideConverter() : dst()
        {
            // do nothing
        }

        NarrowWideConverter(const SrcType& src, size_t srcCount = -1): dst()
        {
            Initialize(src, srcCount);
        }

        NarrowWideConverter(const SrcType& src, size_t srcCount, DstType dst, size_t dstSize) : dst(dst), freeDst(false)
        {
            StringConverter::ConvertNoAlloc(src, srcCount, dst, dstSize, &dstCount);
        }

        void Initialize(const SrcType& src, size_t srcCount = -1)
        {
            if (srcCount == -1)
            {
                srcCount = StringConverter::Length(src);
            }

            StringConverter::Convert(src, srcCount, &dst, &dstCount, &allocateCount);
            freeDst = true;
        }

        ~NarrowWideConverter()
        {
            if (dst && freeDst)
            {
                Allocator::free(dst, allocateCount);
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

    typedef NarrowWideConverter<malloc_allocator, LPCSTR, LPWSTR, charcount_t> NarrowToWide;
    typedef NarrowWideConverter<malloc_allocator, LPCWSTR, LPSTR, size_t> WideToNarrow;
}
