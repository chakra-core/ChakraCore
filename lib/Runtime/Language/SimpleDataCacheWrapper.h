//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if ENABLE_PROFILE_INFO
namespace Js
{
    // A small wrapper class to help manage the lifetime of and access to IActiveScriptDataCache.
    // AddRef's the IActiveScriptDataCache passed to constructor and Release's it in the destructor.
    // We primarily need this class to support writing multiple blocks of data to the underlying
    // cache as it doesn't support opening multiple write streams.
    // The cache underlying this wrapper is segmented into blocks where each block has a small
    // header. The overall cache itself has a versioned header tied to the running version of
    // Chakra. A cache built with one version of Chakra may not be loaded by other versions.
    //
    // Cache layout:
    //      /----------------\
    //      | majorVersion   |  <- Stream header
    //      | minorVersion   |
    //      ------------------
    //      | BlockType      |  <- Block header
    //      | blockByteCount |
    //      ------------------
    //      | <bytes>        |  <- Block contents
    //      \----------------/
    //
    // To write a block of data, call StartBlock (to create a header) and then Write or WriteArray
    // as many times as necessary to write the data into the cache.
    // To read a block of data, call SeekReadStreamToBlock (optionally fetching the size of this
    // block) and then call Read or ReadArray as necessary to read the data from the cache.
    class SimpleDataCacheWrapper : public FinalizableObject
    {
    public:
        enum BlockType : byte
        {
            BlockType_ProfileData = 0x1,
            BlockType_ParserState = 0x2,
            BlockType_Invalid = 0xf
        };

        // Does an AddRef on dataCache
        SimpleDataCacheWrapper(IActiveScriptDataCache* dataCache);

        // Begin to write a block into the cache.
        // Note: Must be called before writing bytes into the block as the block header is
        //       used to lookup the block when reading.
        HRESULT StartBlock(_In_ BlockType blockType, _In_ ULONG byteCount);

        // A counter of bytes written to the cache as part of the current block.
        // A new block begins (resets this counter to 0) when StartBlock is called.
        ULONG BytesWrittenInBlock() { return this->bytesWrittenInBlock; }

        // Counter for the number of blocks written to the cache.
        // Counts only when a new block is created via StartBlock.
        uint BlocksWritten() { return this->blocksWritten; }

        // Seek the read stream to a block.
        // After this call, calls to Read or ReadArray will read bytes from the block itself.
        HRESULT SeekReadStreamToBlock(_In_ BlockType blockType, _Out_opt_ ULONG* bytesInBlock = nullptr);

        HRESULT SaveWriteStream();

        template <typename T>
        HRESULT Write(_In_ T const& data)
        {
            HRESULT hr = E_FAIL;
            IFFAILRET(EnsureWriteStream());
#ifdef ENABLE_WININET_PROFILE_DATA_CACHE
            ULONG bytesWritten = 0;
            hr = this->outStream->Write(&data, sizeof(T), &bytesWritten);
            Assert(bytesWritten == sizeof(T) || FAILED(hr) || hr == S_FALSE);
            bytesWrittenInBlock += bytesWritten;
            // hr is S_FALSE if bytesWritten < sizeOf(T)
            if (hr == S_FALSE)
            {
                hr = E_FAIL;
            }
#endif
            return hr;
        }

        template <typename T>
        HRESULT WriteArray(_In_reads_(len) T * data, _In_ ULONG len)
        {
            HRESULT hr = E_FAIL;
            IFFAILRET(EnsureWriteStream());
#ifdef ENABLE_WININET_PROFILE_DATA_CACHE
            ULONG bytesSize = sizeof(T) * len;
            ULONG bytesWritten = 0;
            hr = this->outStream->Write(data, bytesSize, &bytesWritten);
            Assert(bytesWritten == bytesSize || FAILED(hr) || hr == S_FALSE);
            bytesWrittenInBlock += bytesWritten;
            // hr is S_FALSE if bytesWritten < bytesSize
            if (hr == S_FALSE)
            {
                hr = E_FAIL;
            }
#endif
            return hr;
        }

        template <typename T>
        HRESULT Read(T * data)
        {
            HRESULT hr = E_FAIL;
            IFFAILRET(EnsureReadStream());
#ifdef ENABLE_WININET_PROFILE_DATA_CACHE
            ULONG bytesRead = 0;
            hr = this->inStream->Read(data, sizeof(T), &bytesRead);
            // hr should be S_FALSE if bytesRead < sizeof(T) but this is not always the case
            // Just assert we didn't overflow data and convert S_FALSE into a failing hr.
            Assert(bytesRead <= sizeof(T));
            if (hr == S_FALSE)
            {
                hr = E_FAIL;
            }
#endif
            return hr;
        }

        template <typename T>
        HRESULT ReadArray(_Out_writes_(len) T * data, ULONG len)
        {
            HRESULT hr = E_FAIL;
            IFFAILRET(EnsureReadStream());
#ifdef ENABLE_WININET_PROFILE_DATA_CACHE
            ULONG bytesSize = sizeof(T) * len;
            ULONG bytesRead = 0;
            hr = this->inStream->Read(data, bytesSize, &bytesRead);
            // hr should be S_FALSE if bytesRead < bytesSize but this is not always the case
            // Just assert we didn't overflow data and convert S_FALSE into a failing hr.
            Assert(bytesRead <= bytesSize);
            if (hr == S_FALSE)
            {
                hr = E_FAIL;
            }
#endif
            return hr;
        }

        virtual void Dispose(bool isShutdown) override { Close(); }
        virtual void Finalize(bool isShutdown) override { }
        virtual void Mark(Recycler * recycler) override { }

    private:
        const static uint MAX_BLOCKS_ALLOWED = 0xff;

        bool IsWriteStreamOpen() { return this->outStream != nullptr; }
        bool IsReadStreamOpen() { return this->inStream != nullptr; }

        HRESULT EnsureWriteStream();
        HRESULT EnsureReadStream();

        HRESULT OpenWriteStream();
        HRESULT OpenReadStream();

        HRESULT WriteHeader();
        HRESULT ReadHeader();
        HRESULT ResetReadStream();

        HRESULT SeekReadStreamToBlockHelper(_In_ BlockType blockType, _Out_opt_ ULONG* bytesInBlock);
        HRESULT Close();

        Field(IActiveScriptDataCache*) dataCache;
        Field(IStream*) outStream;
        Field(IStream*) inStream;
        Field(ULONG) bytesWrittenInBlock;
        Field(uint) blocksWritten;
    };
}
#endif
