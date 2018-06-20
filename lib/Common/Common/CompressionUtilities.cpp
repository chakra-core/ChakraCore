//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "CommonCommonPch.h"
#include "CompressionUtilities.h"

#ifdef ENABLE_COMPRESSION_UTILITIES
#include <compressapi.h>
#endif

#define IFFALSEGO(expr,label) do { if(!(expr)) { goto label; } } while(0);
#define IFFALSEGOANDGETLASTERROR(expr,label) do { if(!(expr)) { hr = HRESULT_FROM_WIN32(GetLastError()); goto label; } } while(0);

using namespace Js;

DWORD ConvertCompressionAlgorithm(CompressionUtilities::CompressionAlgorithm algorithm)
{
    // Note: The algorithms listed in CompressionUtilities.h should be kept in-sync with those
    // defined in compressapi.h or else we will need to do more than a simple cast here.
    return static_cast<DWORD>(algorithm);
}

HRESULT CompressionUtilities::CompressBuffer(
    _In_ ArenaAllocator* alloc,
    _In_ const byte* inputBuffer,
    _In_ size_t inputBufferByteCount,
    _Out_ byte** compressedBuffer,
    _Out_ size_t* compressedBufferByteCount,
    _In_opt_ CompressionAlgorithm algorithm)
{
    Assert(compressedBuffer != nullptr);
    Assert(compressedBufferByteCount != nullptr);

    *compressedBuffer = nullptr;
    *compressedBufferByteCount = 0;

    HRESULT hr = E_FAIL;

#ifdef ENABLE_COMPRESSION_UTILITIES
    COMPRESSOR_HANDLE compressor = nullptr;
    IFFALSEGOANDGETLASTERROR(CreateCompressor(ConvertCompressionAlgorithm(algorithm), nullptr, &compressor), Error);

    if (algorithm == CompressionAlgorithm_Xpress || algorithm == CompressionAlgorithm_Xpress_Huff)
    {
        DWORD level = 0;
        IFFALSEGOANDGETLASTERROR(SetCompressorInformation(compressor, COMPRESS_INFORMATION_CLASS_LEVEL, &level, sizeof(DWORD)), Error);
    }

    SIZE_T compressedByteCount = 0;
    bool result = Compress(compressor, inputBuffer, inputBufferByteCount, nullptr, 0, &compressedByteCount);

    if (!result)
    {
        DWORD errorCode = GetLastError();
        if (errorCode != ERROR_INSUFFICIENT_BUFFER)
        {
            hr = HRESULT_FROM_WIN32(errorCode);
            goto Error;
        }
    }

    *compressedBuffer = AnewNoThrowArray(alloc, byte, compressedByteCount);
    IFFALSEGO(*compressedBuffer != nullptr, Error);

    SIZE_T compressedDataSize;
    IFFALSEGOANDGETLASTERROR(Compress(compressor, inputBuffer, inputBufferByteCount, *compressedBuffer, compressedByteCount, &compressedDataSize), Error);
    *compressedBufferByteCount = compressedDataSize;

    hr = S_OK;

Error:
    if (compressor != nullptr)
    {
        CloseCompressor(compressor);
    }
#else
    hr = E_NOTIMPL;
#endif

    return hr;
}

HRESULT CompressionUtilities::DecompressBuffer(
    _In_ ArenaAllocator* alloc,
    _In_ const byte* compressedBuffer,
    _In_ size_t compressedBufferByteCount,
    _Out_ byte** decompressedBuffer,
    _Out_ size_t* decompressedBufferByteCount,
    _In_opt_ CompressionAlgorithm algorithm)
{
    Assert(decompressedBuffer != nullptr);
    Assert(decompressedBufferByteCount != nullptr);

    *decompressedBuffer = nullptr;
    *decompressedBufferByteCount = 0;

    HRESULT hr = E_FAIL;

#ifdef ENABLE_COMPRESSION_UTILITIES
    DECOMPRESSOR_HANDLE decompressor = nullptr;
    IFFALSEGOANDGETLASTERROR(CreateDecompressor(ConvertCompressionAlgorithm(algorithm), nullptr, &decompressor), Error);

    SIZE_T decompressedByteCount = 0;
    bool result = Decompress(decompressor, compressedBuffer, compressedBufferByteCount, nullptr, 0, &decompressedByteCount);

    if (!result)
    {
        DWORD errorCode = GetLastError();
        if (errorCode != ERROR_INSUFFICIENT_BUFFER)
        {
            hr = HRESULT_FROM_WIN32(errorCode);
            goto Error;
        }
    }

    *decompressedBuffer = AnewNoThrowArray(alloc, byte, decompressedByteCount);
    IFFALSEGO(*decompressedBuffer != nullptr, Error);

    SIZE_T uncompressedDataSize = 0;
    IFFALSEGOANDGETLASTERROR(Decompress(decompressor, compressedBuffer, compressedBufferByteCount, *decompressedBuffer, decompressedByteCount, &uncompressedDataSize), Error);
    *decompressedBufferByteCount = uncompressedDataSize;

    hr = S_OK;

Error:
    if (decompressor != nullptr)
    {
        CloseDecompressor(decompressor);
    }
#else
    hr = E_NOTIMPL;
#endif

    return hr;
}
