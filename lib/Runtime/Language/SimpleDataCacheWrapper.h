//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if ENABLE_PROFILE_INFO
namespace Js
{
    class SimpleDataCacheWrapper
    {
    public:
        enum BlockType : byte
        {
            BlockType_ProfileData = 0x1,
            BlockType_ParserState = 0x2,
            BlockType_Invalid = 0xf
        };

        SimpleDataCacheWrapper(IActiveScriptDataCache* dataCache);
        ~SimpleDataCacheWrapper() { this->Close(); }

        bool StartBlock(_In_ BlockType blockType, _In_ ULONG byteCount);
        ULONG BytesWrittenInBlock() { return this->bytesWrittenInBlock; }

        bool Close();

        HRESULT GetReadStream(_Out_ IStream** readStream);
        bool SeekReadStreamToBlock(_In_ IStream* readStream, _In_ BlockType blockType, _Out_opt_ ULONG* bytesInBlock = nullptr);

        template <typename T>
        bool Write(_In_ T const& data)
        {
            HRESULT hr = E_FAIL;
            if (!EnsureWriteStream()) {
                return false;
            }
#ifdef ENABLE_WININET_PROFILE_DATA_CACHE
            ULONG bytesWritten = 0;
            hr = outStream->Write(&data, sizeof(T), &bytesWritten);
            bytesWrittenInBlock += bytesWritten;
            // hr is S_FALSE if bytesRead < sizeOf(T)
#endif
            return hr == S_OK;
        }

        template <typename T>
        bool WriteArray(_In_reads_(len) T * data, _In_ ULONG len)
        {
            HRESULT hr = E_FAIL;
            if (!EnsureWriteStream()) {
                return false;
            }
#ifdef ENABLE_WININET_PROFILE_DATA_CACHE
            ULONG bytesSize = sizeof(T) * len;
            ULONG bytesWritten = 0;
            hr = outStream->Write(data, bytesSize, &bytesWritten);
            bytesWrittenInBlock += bytesWritten;
            // hr is S_FALSE if bytesRead < bytesSize
#endif
            return hr == S_OK;
        }

        template <typename T>
        bool Read(_In_ IStream* readStream, T * data)
        {
            HRESULT hr = E_FAIL;
#ifdef ENABLE_WININET_PROFILE_DATA_CACHE
            ULONG bytesRead = 0;
            hr = readStream->Read(data, sizeof(T), &bytesRead);
            // hr is S_FALSE if bytesRead < sizeOf(T)
#endif
            return hr == S_OK;
        }

        template <typename T>
        bool ReadArray(_In_ IStream* readStream, _Out_writes_(len) T * data, ULONG len)
        {
            HRESULT hr = E_FAIL;
#ifdef ENABLE_WININET_PROFILE_DATA_CACHE
            ULONG bytesSize = sizeof(T) * len;
            ULONG bytesRead = 0;
            hr = readStream->Read(data, bytesSize, &bytesRead);
            // hr is S_FALSE if bytesRead < bytesSize
#endif
            return hr == S_OK;
        }

    private:
        bool IsWriteStreamOpen() { return this->outStream != nullptr; }

        bool EnsureWriteStream();
        bool OpenWriteStream();
        bool WriteHeader();
        bool ReadHeader(_In_ IStream* readStream);

        Field(IActiveScriptDataCache*) dataCache;
        Field(IStream*) outStream;
        Field(ULONG) bytesWrittenInBlock;
    };
}
#endif
