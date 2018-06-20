//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class CompressionUtilities
    {
    public:
        enum CompressionAlgorithm : byte
        {
            CompressionAlgorithm_MSZip = 0x2,
            CompressionAlgorithm_Xpress = 0x3,
            CompressionAlgorithm_Xpress_Huff = 0x4,
            CompressionAlgorithm_LZMS = 0x5,
            CompressionAlgorithm_Invalid = 0xf
        };

        static HRESULT CompressBuffer(
            _In_ ArenaAllocator* alloc,
            _In_ const byte* inputBuffer,
            _In_ size_t inputBufferByteCount,
            _Out_ byte** compressedBuffer,
            _Out_ size_t* compressedBufferByteCount,
            _In_opt_ CompressionAlgorithm algorithm = CompressionAlgorithm_Xpress);

        static HRESULT DecompressBuffer(
            _In_ ArenaAllocator* alloc,
            _In_ const byte* compressedBuffer,
            _In_ size_t compressedBufferByteCount,
            _Out_ byte** decompressedBuffer,
            _Out_ size_t* decompressedBufferByteCount,
            _In_opt_ CompressionAlgorithm algorithm = CompressionAlgorithm_Xpress);
    };
}
